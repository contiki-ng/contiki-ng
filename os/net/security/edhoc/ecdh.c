/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
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
 *         Synchronous wrapper around lib/ecc.h for the EDHOC implementation.
 *
 *         The Contiki-NG ECC driver exposes long-running operations as
 *         protothreads so that other protothreads may make progress
 *         while the cryptographic operation is in flight. The current
 *         EDHOC protocol implementation is synchronous, so we busy-wait
 *         the underlying protothread to completion. This still
 *         deduplicates the per-backend wrappers EDHOC used to ship.
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund,
 *         Marco Tiloca, Nicolas Tsiftes
 */

#include "contiki.h"
#include "dev/watchdog.h"
#include "ecdh.h"
#include "edhoc-config.h"
#include "lib/ecc.h"
#include "lib/ecc-curve.h"
#include "sys/process-mutex.h"
#include "sys/pt.h"
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "ECDH"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*---------------------------------------------------------------------------*/
static const ecc_curve_t *
get_curve(uint8_t curve_id)
{
  switch(curve_id) {
  case EDHOC_CURVE_P256:
    return &ecc_curve_p_256;
  default:
    LOG_ERR("Unsupported ECDH curve id %u\n", curve_id);
    return NULL;
  }
}
/*---------------------------------------------------------------------------*/
static bool
acquire_ecc(const ecc_curve_t *curve)
{
  process_mutex_t *mutex = ecc_get_mutex();
  while(!process_mutex_try_lock(mutex)) {
    watchdog_periodic();
  }
  if(ecc_enable(curve) != 0) {
    /* ecc_enable() releases the mutex on failure. */
    LOG_ERR("ecc_enable() failed\n");
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
bool
ecdh_generate_keypair(uint8_t curve_id,
                      uint8_t *pub_x, uint8_t *pub_y, uint8_t *priv)
{
  const ecc_curve_t *curve = get_curve(curve_id);
  if(!curve) {
    return false;
  }
  if(!acquire_ecc(curve)) {
    return false;
  }

  uint8_t public_key[2 * ECC_KEY_LEN];
  int result = -1;

  PT_INIT(ecc_get_protothread());
  while(PT_SCHEDULE(ecc_generate_key_pair(public_key, priv, &result))) {
    watchdog_periodic();
  }

  ecc_disable();

  if(result != 0) {
    LOG_ERR("ecc_generate_key_pair() failed (%d)\n", result);
    return false;
  }

  memcpy(pub_x, public_key, ECC_KEY_LEN);
  memcpy(pub_y, public_key + ECC_KEY_LEN, ECC_KEY_LEN);
  return true;
}
/*---------------------------------------------------------------------------*/
bool
ecdh_generate_ikm(uint8_t curve_id,
                  const uint8_t *peer_x,
                  const uint8_t *private_key, uint8_t *ikm)
{
  const ecc_curve_t *curve = get_curve(curve_id);
  if(!curve) {
    return false;
  }
  if(!acquire_ecc(curve)) {
    return false;
  }

  /*
   * EDHOC encodes peer ephemeral keys as the x-coordinate only. Recover
   * the full point by decompressing the (0x03 || x) compressed encoding.
   * The 0x03 sign-byte (odd y) matches the convention used by the
   * reference implementations and the EDHOC test vectors.
   */
  uint8_t compressed[1 + ECC_KEY_LEN];
  uint8_t public_key[2 * ECC_KEY_LEN];
  compressed[0] = 0x03;
  memcpy(compressed + 1, peer_x, ECC_KEY_LEN);

  int result = -1;
  PT_INIT(ecc_get_protothread());
  while(PT_SCHEDULE(ecc_decompress_public_key(compressed, public_key,
                                              &result))) {
    watchdog_periodic();
  }
  if(result != 0) {
    LOG_ERR("ecc_decompress_public_key() failed (%d)\n", result);
    ecc_disable();
    return false;
  }

  PT_INIT(ecc_get_protothread());
  while(PT_SCHEDULE(ecc_generate_shared_secret(public_key, private_key,
                                               ikm, &result))) {
    watchdog_periodic();
  }

  ecc_disable();

  if(result != 0) {
    LOG_ERR("ecc_generate_shared_secret() failed (%d)\n", result);
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
bool
ecc_sign_hash(uint8_t curve_id,
              const uint8_t *hash,
              const uint8_t *private_key,
              uint8_t *signature)
{
  const ecc_curve_t *curve = get_curve(curve_id);
  if(!curve) {
    return false;
  }
  if(!acquire_ecc(curve)) {
    return false;
  }

  int result = -1;
  PT_INIT(ecc_get_protothread());
  while(PT_SCHEDULE(ecc_sign(hash, private_key, signature, &result))) {
    watchdog_periodic();
  }

  ecc_disable();

  if(result != 0) {
    LOG_ERR("ecc_sign() failed (%d)\n", result);
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
bool
ecc_verify_hash(uint8_t curve_id,
                const uint8_t *hash,
                const uint8_t *public_key,
                const uint8_t *signature)
{
  const ecc_curve_t *curve = get_curve(curve_id);
  if(!curve) {
    return false;
  }
  if(!acquire_ecc(curve)) {
    return false;
  }

  int result = -1;
  PT_INIT(ecc_get_protothread());
  while(PT_SCHEDULE(ecc_verify(signature, hash, public_key, &result))) {
    watchdog_periodic();
  }

  ecc_disable();

  if(result != 0) {
    LOG_ERR("ecc_verify() failed (%d)\n", result);
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
