/*
 * Copyright (c) 2019, Hasso-Plattner-Institut.
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
 *
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
 * \addtogroup cc-crypto
 * @{
 *
 * \file
 *         Implementation of the CCM* driver for CCXXXX MCUs.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "dev/crypto/cc/cc-ccm-star.h"
#include "dev/crypto/cc/cc-aes-128.h"
#include "dev/crypto/cc/cc-crypto.h"
#include "lib/assert.h"
#include <stdbool.h>
#include <string.h>

#define CCM_L 2
#define CCM_FLAGS_LEN 1

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "cc-ccm-star"
#define LOG_LEVEL LOG_LEVEL_NONE

typedef union {
  uint8_t u8[AES_128_BLOCK_SIZE];
  uint32_t u32[AES_128_BLOCK_SIZE / sizeof(uint32_t)];
} block_t;

/*---------------------------------------------------------------------------*/
static void
set_key(const uint8_t *key)
{
  cc_aes_128_driver.set_key(key);
}
/*---------------------------------------------------------------------------*/
static void
aead(const uint8_t *nonce, uint8_t *m, uint16_t m_len, const uint8_t *a,
     uint16_t a_len, uint8_t *result, uint8_t mic_len, int forward)
{
  if(!a_len && !m_len) {
    /* fall back on software implementation as the hardware implementation
     * would freeze */
    ccm_star_driver.aead(nonce, m, m_len, a, a_len, result, mic_len, forward);
    return;
  }

  bool was_crypto_enabled = cc_crypto_is_enabled();
  if(!was_crypto_enabled) {
    cc_crypto_enable();
  }

  /* all previous interrupts should have been acknowledged */
  assert(!cc_crypto->ctrl.int_stat);

  /* set up AES interrupts */
  cc_crypto->ctrl.int_cfg = CC_CRYPTO_CTRL_INT_CFG_LEVEL;
  cc_crypto->ctrl.int_en = CC_CRYPTO_CTRL_INT_EN_DMA_IN_DONE
                           | CC_CRYPTO_CTRL_INT_EN_RESULT_AV;

  /* enable the DMA path to the AES engine */
  cc_crypto->ctrl.alg_sel = CC_CRYPTO_CTRL_ALG_SEL_AES;

  /* configure the key store to provide pre-loaded AES key */
  cc_crypto->key_store.read_area = CC_AES_128_KEY_AREA;

  /* prepare IV while the AES key loads */
  {
    block_t iv;
    iv.u8[0] = CCM_L - 1;
    memcpy(iv.u8 + CCM_FLAGS_LEN, nonce, CCM_STAR_NONCE_LENGTH);
    memset(iv.u8 + CCM_FLAGS_LEN + CCM_STAR_NONCE_LENGTH,
           0,
           AES_128_BLOCK_SIZE - CCM_FLAGS_LEN - CCM_STAR_NONCE_LENGTH);

    /* wait until the AES key is loaded */
    while(cc_crypto->key_store.read_area & CC_CRYPTO_KEY_STORE_READ_AREA_BUSY);

    /* check that the key was loaded without errors */
    if(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_KEY_ST_RD_ERR) {
      LOG_ERR("error at line %d\n", __LINE__);
      /* clear error */
      cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_STAT_KEY_ST_RD_ERR;
      goto exit;
    }

    /* write the initialization vector */
    cc_crypto->aes.iv[0] = iv.u32[0];
    cc_crypto->aes.iv[1] = iv.u32[1];
    cc_crypto->aes.iv[2] = iv.u32[2];
    cc_crypto->aes.iv[3] = iv.u32[3];
  }

  /* configure AES engine */
  cc_crypto->aes.ctrl =
      CC_CRYPTO_AES_CTRL_SAVE_CONTEXT /* Save context */
      | (((MAX(mic_len, 2) - 2) >> 1) << CC_CRYPTO_AES_CTRL_CCM_M_S) /* M */
      | ((CCM_L - 1) << CC_CRYPTO_AES_CTRL_CCM_L_S) /* L */
      | CC_CRYPTO_AES_CTRL_CCM /* CCM */
      | CC_CRYPTO_AES_CTRL_CTR_WIDTH_128 /* CTR width 128 */
      | CC_CRYPTO_AES_CTRL_CTR /* CTR */
      | (forward ? CC_CRYPTO_AES_CTRL_DIRECTION_ENCRYPT : 0); /* En/de-crypt */
  /* write m_len (lo) */
  cc_crypto->aes.data_length[0] = m_len;
  /* write m_len (hi) */
  cc_crypto->aes.data_length[1] = 0;
  /* write a_len */
  cc_crypto->aes.auth_length = a_len;

  /* configure DMAC to fetch "a" */
  if(a_len) {
    /* enable DMA channel 0 */
    cc_crypto->dmac.ch0.ctrl = CC_CRYPTO_DMAC_CH_CTRL_EN;
    /* base address of "a" in external memory */
    cc_crypto->dmac.ch0.extaddr = (uintptr_t)a;
    /* length of the input data to be transferred */
    cc_crypto->dmac.ch0.dmalength = a_len;

    /* wait for completion of the DMA transfer */
    while(!(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_DMA_IN_DONE));

    /* acknowledge the interrupt */
    cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_CLR_DMA_IN_DONE;

    /* check for errors */
    if(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_DMA_BUS_ERR) {
      LOG_ERR("error at line %d\n", __LINE__);
      /* clear error */
      cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_STAT_DMA_BUS_ERR;
      goto exit;
    }
  }

  /* configure DMAC to fetch "m" */
  if(m_len) {
    /* disable DMA_IN interrupt for this transfer */
    cc_crypto->ctrl.int_en = CC_CRYPTO_CTRL_INT_EN_RESULT_AV;
    /* enable DMA channel 0 */
    cc_crypto->dmac.ch0.ctrl = CC_CRYPTO_DMAC_CH_CTRL_EN;
    /* base address of "m" in external memory */
    cc_crypto->dmac.ch0.extaddr = (uintptr_t)m;
    /* length of the input data to be transferred */
    cc_crypto->dmac.ch0.dmalength = m_len;
    /* enable DMA channel 1 */
    cc_crypto->dmac.ch1.ctrl = CC_CRYPTO_DMAC_CH_CTRL_EN;
    /* base address of the output in external memory */
    cc_crypto->dmac.ch1.extaddr = (uintptr_t)m;
    /* length of the output data to be transferred */
    cc_crypto->dmac.ch1.dmalength = m_len;
  }

  /* wait for completion */
  while(!(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_RESULT_AV));

  /* acknowledge interrupt */
  cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_CLR_RESULT_AV;

  /* check for errors */
  uint32_t errors = cc_crypto->ctrl.int_stat
                    & (CC_CRYPTO_CTRL_INT_STAT_DMA_BUS_ERR
                       | CC_CRYPTO_CTRL_INT_STAT_KEY_ST_RD_ERR);
  if(errors) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear errors */
    cc_crypto->ctrl.int_clr = errors;
    goto exit;
  }

  /* wait for the context ready bit */
  while(!(cc_crypto->aes.ctrl & CC_CRYPTO_AES_CTRL_SAVED_CONTEXT_READY)) {
  }

  /* read tag */
  {
    block_t tag;
    tag.u32[0] = cc_crypto->aes.tag_out[0];
    tag.u32[1] = cc_crypto->aes.tag_out[1];
    tag.u32[2] = cc_crypto->aes.tag_out[2];

    /* this read clears the ‘saved_context_ready’ flag */
    tag.u32[3] = cc_crypto->aes.tag_out[3];

    memcpy(result, tag.u8, mic_len);
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
const struct ccm_star_driver cc_ccm_star_driver = {
  set_key,
  aead
};
/*---------------------------------------------------------------------------*/

/** @} */
