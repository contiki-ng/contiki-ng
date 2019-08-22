/*
 * Copyright (c) 2016, University of Bristol - http://www.bristol.ac.uk
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
 */
/**
 * \addtogroup cc26xx-aes
 * @{
 *
 * \file
 *         Implementation of the AES driver for the CC26x0/CC13x0 SoC
 * \author
 *         Atis Elsts <atis.elsts@gmail.com>
 */
#include "contiki.h"
#include "dev/cc26xx-aes.h"
#include "ti-lib.h"
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "cc26xx-aes"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
static uint32_t skey[AES_128_KEY_LENGTH / sizeof(uint32_t)];

/*---------------------------------------------------------------------------*/
void
cc26xx_aes_set_key(const uint8_t *key)
{
  memcpy(skey, key, AES_128_KEY_LENGTH);
}
/*---------------------------------------------------------------------------*/
static void
encrypt_decrypt(uint8_t *plaintext_and_result, bool do_encrypt)
{
  uint32_t result[AES_128_BLOCK_SIZE / sizeof(uint32_t)];
  unsigned status;
  int i;

  /* First, make sure the PERIPH PD is on */
  ti_lib_prcm_power_domain_on(PRCM_DOMAIN_PERIPH);
  while((ti_lib_prcm_power_domain_status(PRCM_DOMAIN_PERIPH)
          != PRCM_DOMAIN_POWER_ON));

  /* Enable CRYPTO peripheral */
  ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_CRYPTO);
  ti_lib_prcm_load_set();
  while(!ti_lib_prcm_load_get());

  status = ti_lib_crypto_aes_load_key(skey, CRYPTO_KEY_AREA_0);
  if(status != AES_SUCCESS) {
    LOG_WARN("load key failed: %u\n", status);
  } else {

    status = ti_lib_crypto_aes_ecb((uint32_t *)plaintext_and_result, result, CRYPTO_KEY_AREA_0, do_encrypt, false);
    if(status != AES_SUCCESS) {
      LOG_WARN("ecb failed: %u\n", status);
    } else {

      for(i = 0; i < 100; ++i) {
        ti_lib_cpu_delay(10);
        status = ti_lib_crypto_aes_ecb_status();
        if(status != AES_DMA_BSY) {
          break;
        }
      }

      ti_lib_crypto_aes_ecb_finish();

      if(status != AES_SUCCESS) {
        LOG_WARN("ecb get result failed: %u\n", status);
      }
    }
  }

  ti_lib_prcm_peripheral_run_disable(PRCM_PERIPH_CRYPTO);
  ti_lib_prcm_load_set();
  while(!ti_lib_prcm_load_get());

  if(status == AES_SUCCESS) {
    memcpy(plaintext_and_result, result, AES_128_BLOCK_SIZE);
  } else {
    /* corrupt the result */
    plaintext_and_result[0] ^= 1;
  }
}
/*---------------------------------------------------------------------------*/
void
cc26xx_aes_encrypt(uint8_t *plaintext_and_result)
{
  encrypt_decrypt(plaintext_and_result, true);
}
/*---------------------------------------------------------------------------*/
void
cc26xx_aes_decrypt(uint8_t *cyphertext_and_result)
{
  encrypt_decrypt(cyphertext_and_result, false);
}
/*---------------------------------------------------------------------------*/
const struct aes_128_driver cc26xx_aes_128_driver = {
  cc26xx_aes_set_key,
  cc26xx_aes_encrypt
};

/** @} */
