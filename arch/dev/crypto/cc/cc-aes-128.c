/*
 * Copyright (c) 2015, Hasso-Plattner-Institut
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
 */

/**
 * \addtogroup cc-crypto
 * @{
 *
 * \file
 *         Implementation of the AES-128 driver for CCXXXX MCUs.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "dev/crypto/cc/cc-aes-128.h"
#include "dev/crypto/cc/cc-crypto.h"
#include "lib/assert.h"
#include <stdbool.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "cc-aes-128"
#define LOG_LEVEL LOG_LEVEL_NONE

/*---------------------------------------------------------------------------*/
static void
set_key(const uint8_t *key)
{
  bool was_crypto_enabled = cc_crypto_is_enabled();
  if(!was_crypto_enabled) {
    cc_crypto_enable();
  }

  /* all previous interrupts should have been acknowledged */
  assert(!cc_crypto->ctrl.int_stat);

  /* set up AES interrupts */
  cc_crypto->ctrl.int_cfg = CC_CRYPTO_CTRL_INT_CFG_LEVEL;
  cc_crypto->ctrl.int_en = CC_CRYPTO_CTRL_INT_EN_RESULT_AV;

  /* enable DMA path to the key store module */
  cc_crypto->ctrl.alg_sel = CC_CRYPTO_CTRL_ALG_SEL_KEYSTORE;

  /* configure key store module (area, size) - note that setting
   * cc_crypto->key_store.size is unnecessary because 128 bits is the reset
   * value because CC_CRYPTO_KEY_STORE_SIZE_KEY_SIZE_128 is the reset value.
   * Moreover, this would clear all other loaded keys. */
  /* clear key to write */
  cc_crypto->key_store.written_area = 1 << CC_AES_128_KEY_AREA;
  /* enable key to write */
  cc_crypto->key_store.write_area = 1 << CC_AES_128_KEY_AREA;

  /* configure DMAC */
  /* enable DMA channel 0 */
  cc_crypto->dmac.ch0.ctrl = CC_CRYPTO_DMAC_CH_CTRL_EN;
  /* set base address of the aligned key in external memory */
  uint32_t aligned_key[AES_128_KEY_LENGTH / sizeof(uint32_t)];
  memcpy(aligned_key, key, sizeof(aligned_key));
  cc_crypto->dmac.ch0.extaddr = (uintptr_t)aligned_key;
  /* total key length in bytes (e.g. 16 for 1 x 128-bit key) */
  cc_crypto->dmac.ch0.dmalength = AES_128_KEY_LENGTH;

  /* wait for completion */
  while(!(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_RESULT_AV));

  /* acknowledge the interrupt */
  cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_CLR_RESULT_AV;

  /* check for absence of errors in DMA and key store */
  uint32_t errors = cc_crypto->ctrl.int_stat
                    & (CC_CRYPTO_CTRL_INT_STAT_DMA_BUS_ERR
                       | CC_CRYPTO_CTRL_INT_STAT_KEY_ST_WR_ERR);
  if(errors) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear errors */
    cc_crypto->ctrl.int_clr = errors;
    goto exit;
  }

  /* check that key was written */
  if(!(cc_crypto->key_store.written_area & 1 << CC_AES_128_KEY_AREA)) {
    LOG_ERR("error at line %d\n", __LINE__);
    goto exit;
  }

exit:
  /* all interrupts should have been acknowledged */
  assert(!cc_crypto->ctrl.int_stat);

  /* disable master control/DMA clock */
  cc_crypto->ctrl.alg_sel = 0;

  if(!was_crypto_enabled) {
    cc_crypto_disable();
  }
}
/*---------------------------------------------------------------------------*/
static void
encrypt(uint8_t *plaintext_and_result)
{
  bool was_crypto_enabled = cc_crypto_is_enabled();
  if(!was_crypto_enabled) {
    cc_crypto_enable();
  }

  /* all previous interrupts should have been acknowledged */
  assert(!cc_crypto->ctrl.int_stat);

  /* set up AES interrupts */
  cc_crypto->ctrl.int_cfg = CC_CRYPTO_CTRL_INT_CFG_LEVEL;
  cc_crypto->ctrl.int_en = CC_CRYPTO_CTRL_INT_EN_RESULT_AV;

  /* enable the DMA path to the AES engine */
  cc_crypto->ctrl.alg_sel = CC_CRYPTO_CTRL_ALG_SEL_AES;

  /* configure the key store to provide pre-loaded AES key */
  cc_crypto->key_store.read_area = CC_AES_128_KEY_AREA;

  /* wait until the key is loaded to the AES module */
  while(cc_crypto->key_store.read_area & CC_CRYPTO_KEY_STORE_READ_AREA_BUSY);

  /* check if the key was loaded without errors */
  if(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_KEY_ST_RD_ERR) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear error */
    cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_STAT_KEY_ST_RD_ERR;
    goto exit;
  }

  /* configure AES engine */
  cc_crypto->aes.ctrl = CC_CRYPTO_AES_CTRL_DIRECTION_ENCRYPT;
  /* write length of the message (lo) */
  cc_crypto->aes.data_length[0] = AES_128_BLOCK_SIZE;
  /* write length of the message (hi) */
  cc_crypto->aes.data_length[1] = 0;

  /* configure DMAC */
  /* enable DMA channel 0 */
  cc_crypto->dmac.ch0.ctrl = CC_CRYPTO_DMAC_CH_CTRL_EN;
  /* base address of the input data in external memory */
  cc_crypto->dmac.ch0.extaddr = (uintptr_t)plaintext_and_result;
  /* length of the input data to be transferred */
  cc_crypto->dmac.ch0.dmalength = AES_128_BLOCK_SIZE;
  /* enable DMA channel 1 */
  cc_crypto->dmac.ch1.ctrl = CC_CRYPTO_DMAC_CH_CTRL_EN;
  /* base address of the output data in external memory */
  cc_crypto->dmac.ch1.extaddr = (uintptr_t)plaintext_and_result;
  /* length of the output data to be transferred */
  cc_crypto->dmac.ch1.dmalength = AES_128_BLOCK_SIZE;

  /* wait for completion */
  while(!(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_RESULT_AV));

  /* acknowledge the interrupt */
  cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_CLR_RESULT_AV;

  /* check for errors in DMA and key store */
  uint32_t errors = cc_crypto->ctrl.int_stat
                    & (CC_CRYPTO_CTRL_INT_STAT_DMA_BUS_ERR
                       | CC_CRYPTO_CTRL_INT_STAT_KEY_ST_RD_ERR);
  if(errors) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear errors */
    cc_crypto->ctrl.int_clr = errors;
    goto exit;
  }

exit:
  /* all interrupts should have been acknowledged */
  assert(!cc_crypto->ctrl.int_stat);

  /* disable master control/DMA clock */
  cc_crypto->ctrl.alg_sel = 0;

  if(!was_crypto_enabled) {
    cc_crypto_disable();
  }
}
/*---------------------------------------------------------------------------*/
const struct aes_128_driver cc_aes_128_driver = {
  set_key,
  encrypt
};
/*---------------------------------------------------------------------------*/

/** @} */
