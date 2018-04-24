/*
 * Copyright (c) 2013, Hasso-Plattner-Institut.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         An OFB-AES-128-based CSPRNG.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/**
 * \addtogroup csprng
 * @{
 */

#include "lib/csprng.h"
#include "lib/aes-128.h"
#include "sys/cc.h"
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSPRNG"
#define LOG_LEVEL LOG_LEVEL_NONE

static struct csprng_seed seed;

/*---------------------------------------------------------------------------*/
void
csprng_feed(struct csprng_seed *new_seed)
{
  uint8_t i;

  /*
   * By XORing the current seed with the new seed, the seed of this CSPRNG
   * remains secret as long as any of the mixed seeds remains secret.
   */
  for(i = 0; i < CSPRNG_SEED_LEN; i++) {
    seed.u8[i] ^= new_seed->u8[i];
  }

  if(LOG_DBG_ENABLED) {
    LOG_DBG("key = ");
    for(i = 0; i < CSPRNG_KEY_LEN; i++) {
      LOG_DBG_("%02x", seed.key[i]);
    }
    LOG_DBG_("\n");
    LOG_DBG("state = ");
    for(i = 0; i < CSPRNG_STATE_LEN; i++) {
      LOG_DBG_("%02x", seed.state[i]);
    }
    LOG_DBG_("\n");
  }
}
/*---------------------------------------------------------------------------*/
void
csprng_rand(uint8_t *result, uint8_t len)
{
  uint16_t pos;

  AES_128_GET_LOCK();
  AES_128.set_key(seed.key);
  for(pos = 0; pos < len; pos += 16) {
    AES_128.encrypt(seed.state);
    memcpy(result + pos, seed.state, MIN(len - pos, 16));
  }
  AES_128_RELEASE_LOCK();
}
/*---------------------------------------------------------------------------*/

/** @} */
