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
 * \addtogroup cc2538-ccm-star
 * @{
 *
 * \file
 *         Implementation of the CCM* driver for the CC2538 SoC
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "dev/cc2538-ccm-star.h"
#include "dev/aes.h"
#include "dev/cc2538-aes-128.h"
#include "lib/assert.h"
#include <stdbool.h>
#include <string.h>

#define CCM_L 2
#define CCM_FLAGS_LEN 1

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "cc2538-ccm-star"
#define LOG_LEVEL LOG_LEVEL_NONE

typedef union {
  uint8_t u8[AES_128_BLOCK_SIZE];
  uint32_t u32[AES_128_BLOCK_SIZE / sizeof(uint32_t)];
} block_t;

/*---------------------------------------------------------------------------*/
static void
set_key(const uint8_t *key)
{
  cc2538_aes_128_driver.set_key(key);
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

  bool was_crypto_enabled = CRYPTO_IS_ENABLED();
  if(!was_crypto_enabled) {
    crypto_enable();
  }

  /* all previous interrupts should have been acknowledged */
  assert(!REG(AES_CTRL_INT_STAT));

  /* set up AES interrupts */
  REG(AES_CTRL_INT_CFG) = AES_CTRL_INT_CFG_LEVEL;
  REG(AES_CTRL_INT_EN) = AES_CTRL_INT_EN_DMA_IN_DONE
                         | AES_CTRL_INT_EN_RESULT_AV;

  /* enable the DMA path to the AES engine */
  REG(AES_CTRL_ALG_SEL) = AES_CTRL_ALG_SEL_AES;

  /* configure the key store to provide pre-loaded AES key */
  REG(AES_KEY_STORE_READ_AREA) = CC2538_AES_128_KEY_AREA;

  /* prepare IV while the AES key loads */
  {
    block_t iv;
    iv.u8[0] = CCM_L - 1;
    memcpy(iv.u8 + CCM_FLAGS_LEN, nonce, CCM_STAR_NONCE_LENGTH);
    memset(iv.u8 + CCM_FLAGS_LEN + CCM_STAR_NONCE_LENGTH,
           0,
           AES_128_BLOCK_SIZE - CCM_FLAGS_LEN - CCM_STAR_NONCE_LENGTH);

    /* wait until the AES key is loaded */
    while(REG(AES_KEY_STORE_READ_AREA) & AES_KEY_STORE_READ_AREA_BUSY);

    /* check that the key was loaded without errors */
    if(REG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_KEY_ST_RD_ERR) {
      LOG_ERR("error at line %d\n", __LINE__);
      /* clear error */
      REG(AES_CTRL_INT_CLR) = AES_CTRL_INT_STAT_KEY_ST_RD_ERR;
      goto exit;
    }

    /* write the initialization vector */
    REG(AES_AES_IV_0) = iv.u32[0];
    REG(AES_AES_IV_1) = iv.u32[1];
    REG(AES_AES_IV_2) = iv.u32[2];
    REG(AES_AES_IV_3) = iv.u32[3];
  }

  /* configure AES engine */
  REG(AES_AES_CTRL) =
      AES_AES_CTRL_SAVE_CONTEXT /* Save context */
      | (((MAX(mic_len, 2) - 2) >> 1) << AES_AES_CTRL_CCM_M_S) /* M */
      | ((CCM_L - 1) << AES_AES_CTRL_CCM_L_S) /* L */
      | AES_AES_CTRL_CCM /* CCM */
      | AES_AES_CTRL_CTR_WIDTH_128 /* CTR width 128 */
      | AES_AES_CTRL_CTR /* CTR */
      | (forward ? AES_AES_CTRL_DIRECTION_ENCRYPT : 0); /* En/decryption */
  /* write m_len (lo) */
  REG(AES_AES_C_LENGTH_0) = m_len;
  /* write m_len (hi) */
  REG(AES_AES_C_LENGTH_1) = 0;
  /* write a_len */
  REG(AES_AES_AUTH_LENGTH) = a_len;

  /* configure DMAC to fetch "a" */
  if(a_len) {
    /* enable DMA channel 0 */
    REG(AES_DMAC_CH0_CTRL) = AES_DMAC_CH_CTRL_EN;
    /* base address of "a" in external memory */
    REG(AES_DMAC_CH0_EXTADDR) = (uintptr_t)a;
    /* length of the input data to be transferred */
    REG(AES_DMAC_CH0_DMALENGTH) = a_len;

    /* wait for completion of the DMA transfer */
    while(!(REG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_DMA_IN_DONE));

    /* acknowledge the interrupt */
    REG(AES_CTRL_INT_CLR) = AES_CTRL_INT_CLR_DMA_IN_DONE;

    /* check for errors */
    if(REG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_DMA_BUS_ERR) {
      LOG_ERR("error at line %d\n", __LINE__);
      /* clear error */
      REG(AES_CTRL_INT_CLR) = AES_CTRL_INT_STAT_DMA_BUS_ERR;
      goto exit;
    }
  }

  /* configure DMAC to fetch "m" */
  if(m_len) {
    /* disable DMA_IN interrupt for this transfer */
    REG(AES_CTRL_INT_EN) = AES_CTRL_INT_EN_RESULT_AV;
    /* enable DMA channel 0 */
    REG(AES_DMAC_CH0_CTRL) = AES_DMAC_CH_CTRL_EN;
    /* base address of "m" in external memory */
    REG(AES_DMAC_CH0_EXTADDR) = (uintptr_t)m;
    /* length of the input data to be transferred */
    REG(AES_DMAC_CH0_DMALENGTH) = m_len;
    /* enable DMA channel 1 */
    REG(AES_DMAC_CH1_CTRL) = AES_DMAC_CH_CTRL_EN;
    /* base address of the output in external memory */
    REG(AES_DMAC_CH1_EXTADDR) = (uintptr_t)m;
    /* length of the output data to be transferred */
    REG(AES_DMAC_CH1_DMALENGTH) = m_len;
  }

  /* wait for completion */
  while(!(REG(AES_CTRL_INT_STAT) & AES_CTRL_INT_STAT_RESULT_AV));

  /* acknowledge interrupt */
  REG(AES_CTRL_INT_CLR) = AES_CTRL_INT_CLR_RESULT_AV;

  /* check for errors */
  uint32_t errors = REG(AES_CTRL_INT_STAT)
                    & (AES_CTRL_INT_STAT_DMA_BUS_ERR
                       | AES_CTRL_INT_STAT_KEY_ST_RD_ERR);
  if(errors) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear errors */
    REG(AES_CTRL_INT_CLR) = errors;
    goto exit;
  }

  /* wait for the context ready bit */
  while(!(REG(AES_AES_CTRL) & AES_AES_CTRL_SAVED_CONTEXT_READY)) {
  }

  /* read tag */
  {
    block_t tag;
    tag.u32[0] = REG(AES_AES_TAG_OUT_0);
    tag.u32[1] = REG(AES_AES_TAG_OUT_1);
    tag.u32[2] = REG(AES_AES_TAG_OUT_2);

    /* this read clears the ‘saved_context_ready’ flag */
    tag.u32[3] = REG(AES_AES_TAG_OUT_3);

    memcpy(result, tag.u8, mic_len);
  }

exit:
  /* all interrupts should have been acknowledged */
  assert(!REG(AES_CTRL_INT_STAT));

  /* disable master control/DMA clock */
  REG(AES_CTRL_ALG_SEL) = 0;
  if(!was_crypto_enabled) {
    crypto_disable();
  }
}
/*---------------------------------------------------------------------------*/
const struct ccm_star_driver cc2538_ccm_star_driver = {
  set_key,
  aead
};
/*---------------------------------------------------------------------------*/

/** @} */
