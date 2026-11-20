/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES ARE DISCLAIMED.
 */

/**
 * \file
 *   nRF52840 CryptoCell-310 (CC310) hardware backend for the Contiki-NG
 *   ECC service (\c lib/ecc.h), on the unified \c arch/cpu/nrf tree and
 *   Nordic's maintained nrfxlib crypto.
 *
 *   The lib/ecc.h operations are routed onto the CryptoCell-310 by calling
 *   nrfxlib's cc3xx PSA transparent-driver entry points (cc3xx_generate_key,
 *   cc3xx_export_public_key, cc3xx_sign_hash, cc3xx_verify_hash,
 *   cc3xx_key_agreement) directly; cc3xx is the CC310 implementation, so every
 *   operation is hardware-executed. Currently secp256r1 only; ecc_enable()
 *   returns an error for any other curve. CC310 supports more curves -- see
 *   README.md for how to add one.
 *
 *   Raw key formats (match lib/ecc.h, big-endian): 32 B private key / hash /
 *   shared secret; 64 B public key X||Y (prefix-less); the PSA 65 B 0x04||X||Y
 *   form is prepended/stripped here; 64 B ECDSA signature R||S.
 *
 *   psa_key_attributes_t is the ABI contract with the prebuilt cc3xx blob
 *   (validated on the nRF52840-DK).
 */

#include "lib/ecc.h"
#include "lib/ecc-curve.h"
#include "dev/watchdog.h"
#include "sys/pt.h"
#include "sys/process-mutex.h"
#include <stdint.h>
#include <string.h>

/* Contiki's cc.h and Nordic/psa headers both define MIN/MAX; drop Contiki's so
 * the vendored headers can define their own without a -Werror clash. */
#undef MIN
#undef MAX

/* PSA types + attribute helpers (header-only; no PSA core linked). */
#include "psa/crypto.h"
/* nrfxlib cc3xx PSA transparent-driver entry points. */
#include "cc3xx_psa_asymmetric_signature.h"
#include "cc3xx_psa_key_generation.h"
#include "cc3xx_psa_key_agreement.h"
/* nrfxlib CC310 platform init (RNG/entropy, mutex, abort). */
#include "nrf_cc3xx_platform.h"

/* uECC kept linked solely for software point decompression (CC310 exposes no
 * decompress entry point); only os/services/ecc/ecc.c is excluded, not uECC. */
#include "uECC.h"

#include "sys/log.h"
#define LOG_MODULE "ECC-CC310"
#define LOG_LEVEL LOG_LEVEL_NONE

/*
 * Per-curve descriptor. The cc3xx PSA entry points are curve-agnostic -- they
 * read the family and bit size from the attributes -- so the backend is driven
 * by this table rather than hard-coded to one curve. Adding a curve is one row
 * here (plus a matching uECC curve for the software validate/decompress
 * helpers); see README.md. Only secp256r1 is registered.
 */
typedef struct {
  const ecc_curve_t *curve;        /* matching lib/ecc.h curve object */
  size_t element_bytes;            /* field element size (32 for P-256) */
  psa_ecc_family_t psa_family;     /* PSA ECC family */
  size_t psa_bits;                 /* key size in bits */
  psa_algorithm_t ecdsa_alg;       /* PSA ECDSA alg (carries the hash identity) */
  uECC_Curve (*uecc_curve)(void);  /* uECC curve for validate / decompress */
} cc310_curve_t;

static const cc310_curve_t cc310_curves[] = {
  { &ecc_curve_p_256, 32, PSA_ECC_FAMILY_SECP_R1, 256,
    PSA_ALG_ECDSA(PSA_ALG_SHA_256), uECC_secp256r1 },
};

/* Largest element size among the registered curves; sizes the stack buffers.
 * Raise when registering a larger curve (e.g. 48 for P-384). */
#define CC310_MAX_ELEMENT_BYTES 32
#define CC310_MAX_POINT_04      (1 + 2 * CC310_MAX_ELEMENT_BYTES)

static struct pt main_protothread;
static process_mutex_t mutex;
static const cc310_curve_t *active;   /* the curve enabled for this session */

/*---------------------------------------------------------------------------*/
/* Build a psa_key_attributes_t for the active curve. */
static psa_key_attributes_t
curve_attributes(int is_keypair, psa_key_usage_t usage, psa_algorithm_t alg)
{
  psa_key_attributes_t attr = PSA_KEY_ATTRIBUTES_INIT;
  psa_set_key_type(&attr, is_keypair
                   ? PSA_KEY_TYPE_ECC_KEY_PAIR(active->psa_family)
                   : PSA_KEY_TYPE_ECC_PUBLIC_KEY(active->psa_family));
  psa_set_key_bits(&attr, active->psa_bits);
  psa_set_key_usage_flags(&attr, usage);
  psa_set_key_algorithm(&attr, alg);
  return attr;
}
/*---------------------------------------------------------------------------*/
void
ecc_init(void)
{
  /* Sets up the CC310 (RNG/entropy, mutex, abort). Idempotent; safe to also
   * call from ecc_session_begin(). */
  int ret = nrf_cc3xx_platform_init();
  if(ret != NRF_CC3XX_PLATFORM_SUCCESS) {
    LOG_ERR("nrf_cc3xx_platform_init() failed (%d)\n", ret);
  }
  /* The cc3xx driver's keygen/sign draw randomness via the PSA core RNG, which
   * requires the PSA subsystem to be initialised first. Idempotent. */
  psa_status_t ps = psa_crypto_init();
  if(ps != PSA_SUCCESS) {
    LOG_ERR("psa_crypto_init() failed (%d)\n", (int)ps);
  }
}
/*---------------------------------------------------------------------------*/
process_mutex_t *
ecc_get_mutex(void)
{
  return &mutex;
}
/*---------------------------------------------------------------------------*/
int
ecc_enable(const ecc_curve_t *c)
{
  /* Select the descriptor for the requested curve. Only registered curves are
   * accepted (currently secp256r1); add a row to cc310_curves[] to support
   * more -- see README.md. */
  for(unsigned i = 0; i < sizeof(cc310_curves) / sizeof(cc310_curves[0]); i++) {
    if(cc310_curves[i].curve == c) {
      active = &cc310_curves[i];
      return 0;
    }
  }
  LOG_WARN("curve '%s' not supported by this CC310 backend\n",
           c ? c->name : "(null)");
  process_mutex_unlock(&mutex);   /* contract: unlock on error */
  return -1;
}
/*---------------------------------------------------------------------------*/
struct pt *
ecc_get_protothread(void)
{
  return &main_protothread;
}
/*---------------------------------------------------------------------------*/
PT_THREAD(ecc_validate_public_key(const uint8_t *public_key, int *result))
{
  PT_BEGIN(&main_protothread);
  watchdog_periodic();
  /* cc3xx_import_key does not perform on-curve validation, so use uECC's point
   * check (software, already linked here for decompress). It is cheap -- no
   * scalar multiply -- and rejects off-curve points. 0 = valid. */
  *result = !uECC_valid_public_key(public_key, active->uecc_curve());
  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
void
ecc_compress_public_key(const uint8_t *uncompressed_public_key,
                        uint8_t *compressed_public_key)
{
  /* SECG SEC1: 0x02|0x03 (parity of Y) || X. Pure byte math. */
  size_t el = active->element_bytes;
  compressed_public_key[0] =
    0x02 | (uncompressed_public_key[2 * el - 1] & 1);
  memcpy(compressed_public_key + 1, uncompressed_public_key, el);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(ecc_decompress_public_key(const uint8_t *compressed_public_key,
                                    uint8_t *uncompressed_public_key,
                                    int *result))
{
  PT_BEGIN(&main_protothread);
  watchdog_periodic();
  /* CC310 has no public-key decompression entry point; reuse uECC's software
   * decompression (the scalar-multiplication ops stay on CC310). */
  if(active) {
    uECC_decompress(compressed_public_key, uncompressed_public_key,
                    active->uecc_curve());
    *result = 0;
  } else {
    *result = -1;
  }
  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(ecc_sign(const uint8_t *message_hash,
                   const uint8_t *private_key,
                   uint8_t *signature,
                   int *result))
{
  PT_BEGIN(&main_protothread);
  watchdog_periodic();
  {
    size_t el = active->element_bytes;
    size_t sig_len = 0;
    psa_key_attributes_t attr =
      curve_attributes(1, PSA_KEY_USAGE_SIGN_HASH, active->ecdsa_alg);
    psa_status_t s = cc3xx_sign_hash(&attr, private_key, el,
                                     active->ecdsa_alg,
                                     message_hash, el,
                                     signature, 2 * el, &sig_len);
    *result = (s == PSA_SUCCESS) ? 0 : (int)s;
  }
  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(ecc_verify(const uint8_t *signature,
                     const uint8_t *message_hash,
                     const uint8_t *public_key,
                     int *result))
{
  PT_BEGIN(&main_protothread);
  watchdog_periodic();
  {
    size_t el = active->element_bytes;
    uint8_t pk04[CC310_MAX_POINT_04];
    psa_key_attributes_t attr =
      curve_attributes(0, PSA_KEY_USAGE_VERIFY_HASH, active->ecdsa_alg);
    pk04[0] = 0x04;
    memcpy(pk04 + 1, public_key, 2 * el);
    psa_status_t s = cc3xx_verify_hash(&attr, pk04, 1 + 2 * el,
                                       active->ecdsa_alg,
                                       message_hash, el,
                                       signature, 2 * el);
    /* PSA_ERROR_INVALID_SIGNATURE on mismatch. */
    *result = (s == PSA_SUCCESS) ? 0 : (int)s;
  }
  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(ecc_generate_key_pair(uint8_t *public_key,
                                uint8_t *private_key,
                                int *result))
{
  PT_BEGIN(&main_protothread);
  watchdog_periodic();
  {
    size_t el = active->element_bytes;
    uint8_t pk04[CC310_MAX_POINT_04];
    size_t priv_len = 0;
    size_t pub_len = 0;
    psa_key_attributes_t attr =
      curve_attributes(1, PSA_KEY_USAGE_SIGN_HASH | PSA_KEY_USAGE_DERIVE,
                       active->ecdsa_alg);
    psa_status_t s = cc3xx_generate_key(&attr, private_key, el, &priv_len);
    if(s == PSA_SUCCESS) {
      /* Derive the public point from the freshly generated private key. */
      s = cc3xx_export_public_key(&attr, private_key, el,
                                  pk04, sizeof(pk04), &pub_len);
    }
    if(s == PSA_SUCCESS) {
      memcpy(public_key, pk04 + 1, 2 * el);   /* strip 0x04 */
    }
    *result = (s == PSA_SUCCESS) ? 0 : (int)s;
  }
  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(ecc_generate_shared_secret(const uint8_t *public_key,
                                     const uint8_t *private_key,
                                     uint8_t *shared_secret,
                                     int *result))
{
  PT_BEGIN(&main_protothread);
  watchdog_periodic();
  {
    size_t el = active->element_bytes;
    uint8_t pk04[CC310_MAX_POINT_04];
    size_t out_len = 0;
    psa_key_attributes_t attr =
      curve_attributes(1, PSA_KEY_USAGE_DERIVE, PSA_ALG_ECDH);
    pk04[0] = 0x04;
    memcpy(pk04 + 1, public_key, 2 * el);
    /* ECDH yields the X coordinate (el bytes) -- the raw output lib/ecc.h wants. */
    psa_status_t s = cc3xx_key_agreement(&attr,
                                         private_key, el,
                                         pk04, 1 + 2 * el,
                                         shared_secret, el,
                                         &out_len, PSA_ALG_ECDH);
    *result = (s == PSA_SUCCESS) ? 0 : (int)s;
  }
  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
void
ecc_disable(void)
{
  active = NULL;
  process_mutex_unlock(&mutex);
}
/*---------------------------------------------------------------------------*/
void
ecc_session_begin(void)
{
  /* Bring up the CC310 (RNG/entropy) and the PSA core for this session; both
   * calls are idempotent. The no-interrupts platform RNG seeds reliably under
   * the running netstack, so no interrupt isolation is needed. The cc3xx driver
   * seeds its DRBG from the TRNG on the first key generation, so that first
   * keygen is slower than subsequent ones. */
  int r = nrf_cc3xx_platform_init();
  if(r != NRF_CC3XX_PLATFORM_SUCCESS) {
    LOG_ERR("nrf_cc3xx_platform_init() failed (%d)\n", r);
  }
  psa_status_t ps = psa_crypto_init();
  if(ps != PSA_SUCCESS) {
    LOG_ERR("psa_crypto_init() failed (%d)\n", (int)ps);
  }
}
/*---------------------------------------------------------------------------*/
void
ecc_session_end(void)
{
}
/*---------------------------------------------------------------------------*/
