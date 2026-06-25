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
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * \file
 *   nRF52840 CryptoCell-310 (CC310) hardware backend for the Contiki-NG
 *   ECC service (\c lib/ecc.h).
 *
 *   This is the nRF52840 analogue of arch/cpu/cc2538/dev/cc2538-ecc.c
 *   (which backs the same lib/ecc.h API onto the CC2538 PKA engine).
 *   It routes ECDSA / ECDH / key-gen onto the ARM CryptoCell-310 via
 *   Nordic's nrf_crypto abstraction (NRF_CRYPTO_BACKEND_CC310_ENABLED).
 *
 *   Once selected (see the build wiring at the bottom of this file), the
 *   EDHOC / COSE / C509 stack accelerates transparently: cose.c calls
 *   ecc_sign_hash() -> edhoc/ecdh.c -> ecc_sign() (this file) -> CC310.
 *   No changes to cose.c / ecdh.c / edhoc are required.
 *
 *   DESIGN NOTE — blocking vs. protothread.
 *   The lib/ecc.h API is protothread-based (callers spin
 *   `while(PT_SCHEDULE(ecc_sign(...))) watchdog_periodic();`).
 *   nrf_crypto operations are *blocking* (they call synchronously into
 *   the closed-source CC310 runtime).  We therefore implement each
 *   PT_THREAD as a single-shot protothread: it performs the whole
 *   operation between PT_BEGIN/PT_END and returns PT_ENDED on the first
 *   schedule.  This blocks the calling protothread for the (short, ~ms)
 *   duration of the hardware op, which is acceptable for benchmarking and
 *   matches the semantics of a synchronous accelerator call.  A fully
 *   asynchronous variant would start the op, return PT_YIELDED, and resume
 *   from the CC310 completion IRQ — left as future work (requires
 *   nrf_crypto async mode + the CRYPTOCELL IRQ).
 *
 *   CURVE SUPPORT.  CC310 covers secp256r1 (and others), but this skeleton
 *   wires only secp256r1 — the curve used by scenarios C/D.  Ed25519 does
 *   NOT flow through lib/ecc.h (it uses os/services/c509/ed25519_verify.c),
 *   so it is intentionally out of scope here.  brainpoolP256r1 is not
 *   offered by CC310; ecc_enable() returns an error for unsupported curves
 *   so the caller can fall back to the software backend.
 * \author
 *   Joel Höglund <joel.hoglund@ri.se>
 */

#include "lib/ecc.h"
#include "lib/ecc-curve.h"
#include "lib/csprng.h"
#include "dev/watchdog.h"
#include "sys/int-master.h"
#include "sys/process-mutex.h"
#include "sys/pt.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

/* Nordic nRF5 SDK crypto abstraction (CC310 backend). */
#include "nrf_crypto.h"
#include "nrf_crypto_ecc.h"
#include "nrf_crypto_ecdsa.h"
#include "nrf_crypto_ecdh.h"
#include "nrf_crypto_error.h"
#include "nrf_crypto_rng.h"
#include "cc310_backend_shared.h"
#include "sns_silib.h"
#include "crys_rnd.h"

/*
 * uECC is kept linked solely for software point decompression (see
 * ecc_decompress_public_key below) — CC310/nrf_crypto exposes no public
 * decompression entry point.  The build wiring excludes only
 * os/services/ecc/ecc.c (the lib/ecc.h software backend, which would clash
 * with the ecc_* symbols defined here); micro-ecc's uECC_* symbols do not
 * clash and stay available.
 */
#include "uECC.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ECC-CC310"
#define LOG_LEVEL LOG_LEVEL_NONE

/*
 * Raw key/element sizes for secp256r1.  nrf_crypto's "raw" encodings are
 * big-endian and prefix-less, matching the lib/ecc.h convention exactly:
 *   - private key / shared secret / hash : |CURVE|      = 32 bytes
 *   - public key (X||Y, no 0x04)         : 2|CURVE|     = 64 bytes
 *   - ECDSA signature (R||S)             : 2|CURVE|     = 64 bytes
 */
#define P256_ELEMENT_BYTES   32
#define P256_POINT_BYTES     64
#define P256_SIGNATURE_BYTES 64

static struct pt main_protothread;
static const ecc_curve_t *curve;
static process_mutex_t mutex;

/*
 * nrf_crypto's default allocator on this toolchain is the ALLOCA one
 * (NRF_CRYPTO_ALLOC_ON_STACK == 1, see nrf_crypto_mem.h) -- and
 * nrf_crypto_rng_init() explicitly refuses a NULL context in that case
 * (returns NRF_ERROR_CRYPTO_ALLOC_FAILED) rather than handing back a
 * stack-allocated pointer that would dangle the instant the function
 * returns, since the RNG context must stay valid across every later
 * nrf_crypto_rng_vector_generate() call. A persistent (static) context is
 * required; passing NULL (as both the boot-time NRF_CRYPTO_RNG_AUTO_INIT
 * call and an earlier version of ecc_session_begin() did) always fails.
 *
 * The temp work buffer has the same stack-lifetime problem in spirit, plus
 * a size problem: it is 6112 bytes (CRYS_RND_WORK_BUFFER_SIZE_WORDS=1528,
 * crys_rnd.h) and the default nRF52840 stack is only 8192 bytes
 * (gcc_startup_nrf52840.S) -- alloca'ing it (passing NULL, the only other
 * option) at this call depth risks overflowing the stack outright. Static
 * for both, for the same reason.
 */
static nrf_crypto_rng_context_t rng_context;
static nrf_crypto_rng_temp_buffer_t rng_temp_buffer;

/* Map a lib/ecc.h curve to the nrf_crypto curve_info.  NULL => unsupported. */
static const nrf_crypto_ecc_curve_info_t *
map_curve(const ecc_curve_t *c)
{
  if(c == &ecc_curve_p_256) {
    return &g_nrf_crypto_ecc_secp256r1_curve_info;
  }
  /* brainpoolP256r1 / P-192 are not provided by CC310 in this skeleton. */
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
ecc_init(void)
{
  /*
   * nrf_crypto_init() is idempotent in the SDK (guarded by an internal
   * "initialized" flag), so it is safe to call here even if the mbedTLS
   * DTLS path (mbedtls-support.c) also initialises it.  It powers up the
   * CRYPTOCELL wrapper (NRF_CRYPTOCELL->ENABLE) and the CC310 runtime.
   */
  ret_code_t ret = nrf_crypto_init();
  if(ret != NRF_SUCCESS) {
    LOG_ERR("nrf_crypto_init() failed (0x%lx)\n", (unsigned long)ret);
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
  if(map_curve(c) == NULL) {
    LOG_WARN("curve '%s' not supported by CC310 backend\n",
             c ? c->name : "(null)");
    process_mutex_unlock(&mutex);
    return -1;     /* caller falls back to software backend */
  }
  curve = c;
  return 0;
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

  /*
   * nrf_crypto validates the point when importing it from raw, so a
   * successful public_key_from_raw() is an on-curve / range check.
   */
  nrf_crypto_ecc_public_key_t pub;
  ret_code_t ret = nrf_crypto_ecc_public_key_from_raw(map_curve(curve),
                                                      &pub, public_key,
                                                      P256_POINT_BYTES);
  *result = (ret == NRF_SUCCESS) ? 0 : (int)ret;
  if(ret == NRF_SUCCESS) {
    nrf_crypto_ecc_public_key_free(&pub);
  }

  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
void
ecc_compress_public_key(const uint8_t *uncompressed_public_key,
                        uint8_t *compressed_public_key)
{
  /* SECG SEC 1 point compression: 0x02|0x03 prefix + X. */
  compressed_public_key[0] =
    0x02 | (uncompressed_public_key[2 * P256_ELEMENT_BYTES - 1] & 1);
  memcpy(compressed_public_key + 1, uncompressed_public_key,
         P256_ELEMENT_BYTES);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(ecc_decompress_public_key(const uint8_t *compressed_public_key,
                                    uint8_t *uncompressed_public_key,
                                    int *result))
{
  PT_BEGIN(&main_protothread);

  watchdog_periodic();

  /*
   * nrf_crypto has no public point-decompression entry point, so we reuse
   * uECC's software decompressor (the same routine the lib/ecc.h software
   * backend uses).  This runs at most once per handshake, when a peer C509
   * credential carries a compressed EC point (c5t / c5c); all heavy
   * operations (sign / verify / ECDH / key-gen) still execute on CC310.
   * Input  : SEC1 compressed form (0x02|0x03 prefix || X), 1 + |CURVE| bytes.
   * Output : prefix-less X||Y, 2|CURVE| bytes — the raw form lib/ecc.h and
   *          nrf_crypto both expect.
   */
  if(curve == &ecc_curve_p_256) {
    uECC_decompress(compressed_public_key, uncompressed_public_key,
                    uECC_secp256r1());
    *result = 0;
  } else {
    *result = -1;     /* only secp256r1 is wired in this backend */
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

  nrf_crypto_ecc_private_key_t priv;
  ret_code_t ret = nrf_crypto_ecc_private_key_from_raw(map_curve(curve),
                                                       &priv, private_key,
                                                       P256_ELEMENT_BYTES);
  if(ret != NRF_SUCCESS) {
    *result = (int)ret;
    PT_EXIT(&main_protothread);
  }

  size_t sig_size = P256_SIGNATURE_BYTES;
  ret = nrf_crypto_ecdsa_sign(NULL, &priv,
                              message_hash, P256_ELEMENT_BYTES,
                              signature, &sig_size);
  nrf_crypto_ecc_private_key_free(&priv);

  *result = (ret == NRF_SUCCESS) ? 0 : (int)ret;

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

  nrf_crypto_ecc_public_key_t pub;
  ret_code_t ret = nrf_crypto_ecc_public_key_from_raw(map_curve(curve),
                                                      &pub, public_key,
                                                      P256_POINT_BYTES);
  if(ret != NRF_SUCCESS) {
    *result = (int)ret;
    PT_EXIT(&main_protothread);
  }

  ret = nrf_crypto_ecdsa_verify(NULL, &pub,
                                message_hash, P256_ELEMENT_BYTES,
                                signature, P256_SIGNATURE_BYTES);
  nrf_crypto_ecc_public_key_free(&pub);

  /* nrf_crypto returns NRF_ERROR_CRYPTO_ECDSA_INVALID_SIGNATURE on mismatch. */
  *result = (ret == NRF_SUCCESS) ? 0 : (int)ret;

  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
PT_THREAD(ecc_generate_key_pair(uint8_t *public_key,
                                uint8_t *private_key,
                                int *result))
{
  PT_BEGIN(&main_protothread);

  watchdog_periodic();

  nrf_crypto_ecc_private_key_t priv;
  nrf_crypto_ecc_public_key_t pub;
  ret_code_t ret = nrf_crypto_ecc_key_pair_generate(NULL, map_curve(curve),
                                                    &priv, &pub);
  if(ret != NRF_SUCCESS) {
    *result = (int)ret;
    PT_EXIT(&main_protothread);
  }

  size_t priv_size = P256_ELEMENT_BYTES;
  size_t pub_size = P256_POINT_BYTES;
  ret = nrf_crypto_ecc_private_key_to_raw(&priv, private_key, &priv_size);
  if(ret == NRF_SUCCESS) {
    ret = nrf_crypto_ecc_public_key_to_raw(&pub, public_key, &pub_size);
  }
  nrf_crypto_ecc_private_key_free(&priv);
  nrf_crypto_ecc_public_key_free(&pub);

  *result = (ret == NRF_SUCCESS) ? 0 : (int)ret;

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

  nrf_crypto_ecc_private_key_t priv;
  nrf_crypto_ecc_public_key_t pub;
  ret_code_t ret = nrf_crypto_ecc_private_key_from_raw(map_curve(curve),
                                                       &priv, private_key,
                                                       P256_ELEMENT_BYTES);
  if(ret != NRF_SUCCESS) {
    *result = (int)ret;
    PT_EXIT(&main_protothread);
  }
  ret = nrf_crypto_ecc_public_key_from_raw(map_curve(curve), &pub,
                                           public_key, P256_POINT_BYTES);
  if(ret != NRF_SUCCESS) {
    nrf_crypto_ecc_private_key_free(&priv);
    *result = (int)ret;
    PT_EXIT(&main_protothread);
  }

  /*
   * nrf_crypto_ecdh_compute yields the X coordinate of the shared point
   * (|CURVE| bytes) — the same raw ECDH output lib/ecc.h specifies.
   */
  size_t secret_size = P256_ELEMENT_BYTES;
  ret = nrf_crypto_ecdh_compute(NULL, &priv, &pub,
                                shared_secret, &secret_size);
  nrf_crypto_ecc_private_key_free(&priv);
  nrf_crypto_ecc_public_key_free(&pub);

  *result = (ret == NRF_SUCCESS) ? 0 : (int)ret;

  PT_END(&main_protothread);
}
/*---------------------------------------------------------------------------*/
void
ecc_disable(void)
{
  curve = NULL;
  process_mutex_unlock(&mutex);
}
/*---------------------------------------------------------------------------*/
void
ecc_session_begin(void)
{
  /*
   * Begin a CC310 crypto session: power the block on, ensure the CC310
   * runtime library is initialised for this power-up, and instantiate the
   * TRNG-seeded CTR-DRBG once for the whole session.
   *
   * ROOT CAUSE of the long CC310 bring-up failure (confirmed 2026-06-19,
   * PLAN section 9.11.4): the CC310 TRNG seeds the DRBG by sampling
   * ring-oscillator jitter over a bounded CPU-cycle window. Any unrelated
   * interrupt firing during that window -- notably Contiki's 128 Hz RTC0
   * system tick (clock.c) -- injects latency that blows the window and
   * fails the TRNG startup test (CRYS_RND_STARTUP_FAILED /
   * CRYS_RND_INSTANTIATION_ERROR). The bare-metal SDK reference has no such
   * interrupts and works. The instantiation must therefore run with all
   * interrupts masked EXCEPT CC310's own completion interrupt -- masking
   * that one too (e.g. via PRIMASK / __disable_irq) deadlocks, because the
   * entropy-wait needs it to make progress. BASEPRI gives exactly that:
   * raise CRYPTOCELL_IRQn to the top priority (0) and block every priority
   * >= 1. This only matters for the TRNG seeding; once the DRBG is
   * instantiated, per-op output (keygen / ECDSA nonce) is deterministic
   * AES-CTR and needs no isolation.
   *
   * SaSi_LibInit() is called here (not just at boot) because it sets up
   * CC310 HW state on the current power-up and clears NRF_CRYPTOCELL->ENABLE
   * on exit -- so ENABLE is explicitly re-asserted before instantiation.
   */
  cc310_backend_enable();                 /* NRF_CRYPTOCELL->ENABLE = 1 */

  NVIC_SetPriority(CRYPTOCELL_IRQn, 0);   /* highest priority: never BASEPRI-masked */
  NVIC_EnableIRQ(CRYPTOCELL_IRQn);
  uint32_t old_basepri = __get_BASEPRI();
  __set_BASEPRI(1u << (8u - __NVIC_PRIO_BITS)); /* mask all IRQs of priority >= 1 */

  SaSi_LibInit();
  *(volatile uint32_t *)0x5002A500UL = 1; /* SaSi_LibInit clears ENABLE; re-assert */
  ret_code_t ret = nrf_crypto_rng_init(&rng_context, &rng_temp_buffer);

  __set_BASEPRI(old_basepri);

  if(ret != NRF_SUCCESS) {
    LOG_ERR("ecc_session_begin: nrf_crypto_rng_init failed (0x%lx)\n",
            (unsigned long)ret);
  }
}
/*---------------------------------------------------------------------------*/
void
ecc_session_end(void)
{
  cc310_backend_disable();
}
/*---------------------------------------------------------------------------*/

/*
 * ============================ BUILD WIRING ============================
 *
 * 1. Add the blob + nrf_crypto sources.  Use the FULL library, NOT the
 *    bootloader subset:
 *      - WRONG: external/nrf_cc310_bl/.../libnrf_cc310_bl_0.9.13.a
 *               (verify + SHA-256 + AES only; no ECDSA sign, no ECDH,
 *                no keygen, and a different low-level nrf_cc310_bl_ / CRYS
 *                API — it cannot drive an EDHOC handshake).
 *      - RIGHT (verified, nRF5_SDK_17.1.0_ddde560, hard-float to match
 *               Contiki's -mfloat-abi=hard -mfpu=fpv4-sp-d16):
 *          external/nrf_cc310/lib/cortex-m4/hard-float/libnrf_cc310_0.9.13.a
 *        Exports CRYS_ECDSA_Sign / CRYS_ECDSA_Verify / CRYS_ECDH_SVDP_DH /
 *        CRYS_ECPKI_GenKeyPair — everything this backend needs.
 *
 *    High-level frontend + CC310 backend C sources: USE THE ALREADY-
 *    VENDORED Contiki copies under
 *      arch/cpu/nrf52840/lib/nrf52-sdk/components/libraries/crypto/
 *    They were diffed against SDK 17.1.0 and are identical except for
 *    (a) copyright year and (b) a deliberate Contiki power-gating patch
 *    that toggles NRF_CRYPTOCELL->ENABLE around each ECDSA/ECDH op (stock
 *    17.1.0 leaves CC310 enabled after init).  The blob-facing structs
 *    (nrf_crypto_ecc.h, cc310_backend_ecc.h) are byte-identical to 17.1.0,
 *    and the vendored backend calls the SAME CRYS_* symbols — so it is
 *    ABI-compatible with the 17.1.0 blob AND keeps the low-power gating.
 *    Only the CRYS/SaSi headers and the blob are NOT vendored; take those
 *    two from SDK 17.1.0:
 *      external/nrf_cc310/include/                       (CRYS/SaSi headers)
 *      external/nrf_cc310/lib/cortex-m4/hard-float/libnrf_cc310_0.9.13.a
 *
 *    In arch/cpu/nrf52840/Makefile.nrf52840 (or a dedicated
 *    Makefile.cc310-crypto it includes):
 *
 *      # CryptoCell-310 hardware crypto (full lib)
 *      CONTIKI_CPU_DIRS        += dev
 *      CONTIKI_CPU_SOURCEFILES += nrf52840-cc310-ecc.c
 *      # vendored nrf_crypto frontend + cc310 backend (already in-tree):
 *      VCRYPTO := lib/nrf52-sdk/components/libraries/crypto
 *      CONTIKI_CPU_DIRS        += $(VCRYPTO) $(VCRYPTO)/backend/cc310
 *      CONTIKI_CPU_SOURCEFILES += \
 *          nrf_crypto_ecc.c nrf_crypto_ecdsa.c nrf_crypto_ecdh.c \
 *          nrf_crypto_init.c \
 *          cc310_backend_ecc.c cc310_backend_ecdsa.c cc310_backend_ecdh.c \
 *          cc310_backend_init.c cc310_backend_mutex.c cc310_backend_shared.c
 *      # headers + blob from SDK 17.1.0 (NOT vendored):
 *      SDK := $(CONTIKI)/../nRF5_SDK_17.1.0_ddde560      # adjust to taste
 *      CFLAGS += -I$(SDK)/external/nrf_cc310/include
 *      CFLAGS += -DNRF_CRYPTO_BACKEND_CC310_ENABLED=1 \
 *                -DNRF_CRYPTO_ECC_SECP256R1_ENABLED=1
 *      TARGET_LIBFILES += \
 *          $(SDK)/external/nrf_cc310/lib/cortex-m4/hard-float/libnrf_cc310_0.9.13.a
 *
 *    (If the 2019-vintage vendored backend ever fails to compile against
 *    the 17.1.0 CRYS headers, fall back to copying 17.1.0's own frontend +
 *    backend .c over the vendored ones — they are a matched pair — at the
 *    cost of losing the power-gating patch.)
 *
 * 2. Suppress the software backend so symbols don't clash.  Only
 *    os/services/ecc/ecc.c defines the lib/ecc.h ecc_* entry points that
 *    this file also defines, so that ONE file must be excluded.  micro-ecc
 *    (uECC_*) does NOT clash and is kept linked for ecc_decompress_public_key
 *    above.  Gate Makefile.ecc on a flag, e.g.
 *      ifeq ($(ECC_HW_BACKEND),cc310)
 *        MODULES += os/net/security/micro-ecc      # for uECC_decompress only
 *        CONTIKI_SOURCES_EXCLUDES += ecc.c         # hw backend supplies ecc_*
 *      else
 *        MODULES += os/net/security/micro-ecc
 *      endif
 *    and set ECC_HW_BACKEND=cc310 in the nRF52840 app project Makefile.
 *
 * 3. SHA-256 (optional, separate swap).  os/lib/sha-256.c exposes a
 *    sha_256_driver struct; a CC310 SHA driver (cc310_backend_hash.c) can
 *    replace it the same way for additional acceleration of the COSE
 *    Sig_structure hashing.  Not required for the ECDSA/ECDH speedup.
 *
 * 4. Verify: build edhoc-oscore-c5t-* for TARGET=nrf52840, flash a DK, run
 *    the handshake, and compare wall-clock vs. the uECC software baseline
 *    (paper Table bp256-arm-timing, secp256r1 = 1172 ms) and Fedrecheski
 *    2024's ~135 ms CC310 EDHOC figure.
 * =====================================================================
 */
