/*
 * Copyright (c) 2020, Industrial Systems Institute (ISI), Patras, Greece
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \file
 *         ecdh, an interface between the ECC and Secure Hash Algorithms with the edhoc implementation.
 *         Interface the ECC key used library with the edhoc implementation. New ECC libraries can be include it here.
 *         (UECC macro must be definded at config file) and with the CC2538 HW module
 *         Interface the Secure Hash Algorithms SH256 with the edhoc implementation.
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */

#include "ecdh.h"
#include "contiki-lib.h"
#include <dev/watchdog.h>
#include "sys/rtimer.h"
#include "sys/process.h"
/*static rtimer_clock_t time; */

#ifndef HKDF_INFO_MAXLEN
#define HKDF_INFO_MAXLEN 255
#endif

#ifndef HKDF_OUTPUT_MAXLEN
#define HKDF_OUTPUT_MAXLEN 255
#endif

static uint8_t aggregate_buffer[HAS_LENGHT + HKDF_INFO_MAXLEN + 1];
static uint8_t out_buffer[HKDF_OUTPUT_MAXLEN + HAS_LENGHT];

uint8_t
generate_IKM(uint8_t *gx, uint8_t *gy, uint8_t *private_key, uint8_t *ikm, ecc_curve_t curve)
{
  int er = 0;
#if ECC == UECC_ECC     /*use GX and Gy */
  er = uecc_generate_IKM(gx, gy, private_key, ikm, curve);
#endif
#if ECC == CC2538_ECC
  er = cc2538_generate_IKM(gx, gy, private_key, ikm, curve);
#endif
  return er;
}
static void
hmac_sha256_init(hmac_context_t **ctx, const uint8_t *key, uint8_t key_sz)
{
  hmac_storage_init();
  (*ctx) = hmac_new(key, key_sz);
}
static void
hmac_sha256_create(hmac_context_t **ctx, const uint8_t *key, uint8_t key_sz, const uint8_t *data, uint8_t data_sz, uint8_t *hmac)
{
  hmac_update(*ctx, data, data_sz);
  hmac_finalize(*ctx, hmac);
}
static void
hmac_sha256_reset(hmac_context_t **ctx, const unsigned char *key, size_t k_sz)
{
  hmac_init(*ctx, key, k_sz);
}
static void
hmac_sha256_free(hmac_context_t *ctx)
{
  hmac_free(ctx);
}
uint8_t
compute_TH(uint8_t *in, uint8_t in_sz, uint8_t *hash, uint8_t hash_sz)
{
  int er = sha256(in, in_sz, hash);
  return er;
}
uint8_t
hkdf_extrac(uint8_t *salt, uint8_t salt_sz, uint8_t *ikm, uint8_t ikm_sz, uint8_t *hmac)
{
  hmac_context_t *ctx = NULL;
  hmac_sha256_init(&ctx, salt, salt_sz);
  hmac_sha256_create(&ctx, salt, salt_sz, ikm, ikm_sz, hmac);
  hmac_sha256_free(ctx);
  return 1;
}
int8_t
hkdf_expand(uint8_t *prk, uint16_t prk_sz, uint8_t *info, uint16_t info_sz, uint8_t *okm, uint16_t okm_sz)
{
  if(info_sz > HKDF_INFO_MAXLEN) {
    LOG_ERR("error code (%d)\n ", ERR_INFO_SIZE);
    return ERR_INFO_SIZE;
  }
  if(okm_sz > HKDF_OUTPUT_MAXLEN) {
    LOG_ERR("error code (%d)\n ", ERR_OKM_SIZE);
    return ERR_OKM_SIZE;
  }
  int hash_sz = HAS_LENGHT;

  /*ceil */
  int N = (okm_sz + hash_sz - 1) / hash_sz;

  /* Compose T(1) */
  memcpy(aggregate_buffer, info, info_sz);
  aggregate_buffer[info_sz] = 0x01;

  hmac_context_t *ctx = NULL;
  hmac_sha256_init(&ctx, prk, prk_sz);
  hmac_sha256_create(&ctx, prk, prk_sz, aggregate_buffer, info_sz + 1, &(out_buffer[0]));

  /*Compose T(2) ... T(N) */
  memcpy(aggregate_buffer, &(out_buffer[0]), 32);
  for(int i = 1; i < N; i++) {
    hmac_sha256_reset(&ctx, prk, prk_sz);
    memcpy(&(aggregate_buffer[32]), info, info_sz);
    aggregate_buffer[32 + info_sz] = i + 1;
    hmac_sha256_create(&ctx, prk, prk_sz, aggregate_buffer, 32 + info_sz + 1, &(out_buffer[i * 32]));
    memcpy(aggregate_buffer, &(out_buffer[i * 32]), 32);
  }

  hmac_sha256_free(ctx);
  memcpy(okm, out_buffer, okm_sz);
  return 1;
}
void
generate_cose_key(ecc_key *key, cose_key *cose, char *identity, uint8_t id_sz)
{
  cose->kid = (bstr_cose){ key->kid, key->kid_sz };
  cose->identity = (sstr_cose){ identity, id_sz };
  cose->crv = KEY_CRV; /* P-256 */
  cose->kty = KEY_TYPE; /* EC2 */
  cose->x = (bstr_cose){ key->public.x, ECC_KEY_BYTE_LENGHT };
  cose->y = (bstr_cose){ key->public.y, ECC_KEY_BYTE_LENGHT };
}
void
set_cose_key(ecc_key *key, cose_key *cose, cose_key_t *auth_key, ecc_curve_t curve)
{
  if(auth_key->kid_sz == 0) {
    key->kid_sz = 0;
    memcpy(key->public.x, auth_key->x, ECC_KEY_BYTE_LENGHT);
    memcpy(key->public.y, auth_key->y, ECC_KEY_BYTE_LENGHT);
  } else {
    key->kid_sz = auth_key->kid_sz;
    memcpy(key->kid, auth_key->kid, auth_key->kid_sz);
    memcpy(key->public.x, auth_key->x, ECC_KEY_BYTE_LENGHT);
    memcpy(key->public.y, auth_key->y, ECC_KEY_BYTE_LENGHT);
  }
  generate_cose_key(key, cose, auth_key->identity, auth_key->identity_sz);
}
