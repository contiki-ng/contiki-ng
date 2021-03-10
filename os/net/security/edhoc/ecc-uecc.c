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
 *         ecc-uecc interface the uecc SW library
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */

#include "ecc-uecc.h"
#include "contiki-lib.h"
#include <dev/watchdog.h>
#include "sys/rtimer.h"
#include "sys/process.h"

#if ECC == UECC_ECC
static int
RNG(uint8_t *dest, unsigned size)
{
  while(size) {
    uint8_t val = (uint8_t)random_rand();
    *dest = val;
    ++dest;
    --size;
  }
  /* NOTE: it would be a good idea to hash the resulting random data using SHA-256 or similar. */
  return 1;
}
#endif

uint8_t
uecc_generate_key(ecc_key *key, ecc_curve_t curve)
{
  int er = 0;
  watchdog_periodic();
  uECC_set_rng(&RNG);
  uint8_t public_key[64];
  watchdog_periodic();
  er = uECC_make_key(public_key, key->private_key, curve.curve);
  watchdog_periodic();
  memcpy(key->public.x, public_key, 32);
  memcpy(key->public.y, public_key + 32, 32);
  watchdog_periodic();
  return er;
}
void
uecc_uncompress(uint8_t *compressed, uint8_t *gx, uint8_t *gy, ecc_curve_t *curve)
{
  uint8_t pub[2 * ECC_KEY_BYTE_LENGHT];
  uECC_decompress(compressed, pub, curve->curve);
  memcpy(gx, compressed + 1, ECC_KEY_BYTE_LENGHT);
  memcpy(gy, pub + ECC_KEY_BYTE_LENGHT, ECC_KEY_BYTE_LENGHT);
}
uint8_t
uecc_generate_IKM(uint8_t *gx, uint8_t *gy, uint8_t *private_key, uint8_t *ikm, ecc_curve_t curve)
{
  int er = 0;
  uint8_t compressed[ECC_KEY_BYTE_LENGHT + 1];
  compressed[0] = 0x03;
  memcpy(compressed + 1, gx, ECC_KEY_BYTE_LENGHT);
  uecc_uncompress(compressed, gx, gy, &curve);
  
  uint8_t public[2 * ECC_KEY_BYTE_LENGHT];
  memcpy(public, gx, ECC_KEY_BYTE_LENGHT);
  memcpy(public + ECC_KEY_BYTE_LENGHT, gy, ECC_KEY_BYTE_LENGHT);

  watchdog_periodic();
  er = uECC_shared_secret(public, private_key, ikm, curve.curve);
  watchdog_periodic();
  return er;
}
