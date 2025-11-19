/*
 * Copyright (c) 2025, Konrad-Felix Krentz
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
 * \addtogroup cc13xx-cc26xx-crypto
 * @{
 *
 * \file
 *         Implementation of the CCM* driver for SimpleLink MCUs.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "dev/simplelink-ccm-star.h"
#include "dev/crypto.h"
#include "dev/simplelink-aes-128.h"
#include "lib/assert.h"
#include <stdbool.h>
#include <string.h>
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_crypto.h)
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)

#define CCM_L 2
#define CCM_FLAGS_LEN 1

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "simplelink-ccm-star"
#define LOG_LEVEL LOG_LEVEL_NONE

typedef union {
  uint8_t u8[AES_128_BLOCK_SIZE];
  uint32_t u32[AES_128_BLOCK_SIZE / sizeof(uint32_t)];
} block_t;

/*---------------------------------------------------------------------------*/
static bool
set_key(const uint8_t key[static AES_128_KEY_LENGTH])
{
  return simplelink_aes_128_driver.set_key(key);
}
/*---------------------------------------------------------------------------*/
static bool
aead(const uint8_t nonce[static CCM_STAR_NONCE_LENGTH],
     uint8_t *m, uint16_t m_len,
     const uint8_t *a, uint16_t a_len,
     uint8_t *mic, uint8_t mic_len,
     bool forward)
{
  if(!a_len && !m_len) {
    /* fall back on software implementation as the hardware implementation
     * would freeze */
    return ccm_star_driver.aead(nonce,
                                m, m_len,
                                a, a_len,
                                mic, mic_len,
                                forward);
  }

  bool result = false;
  bool was_crypto_enabled = crypto_is_enabled();
  if(!was_crypto_enabled) {
    crypto_enable();
  }

  /* all previous interrupts should have been acknowledged */
  assert(!HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT));

  /* set up AES interrupts */
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_DMA_IN_DONE
                                        | CRYPTO_IRQEN_RESULT_AVAIL;

  /* enable the DMA path to the AES engine */
  HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_AES;

  /* configure the key store to provide pre-loaded AES key */
  HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) =
      simplelink_aes_128_active_key_area;

  /* prepare IV while the AES key loads */
  {
    block_t iv;
    iv.u8[0] = CCM_L - 1;
    memcpy(iv.u8 + CCM_FLAGS_LEN, nonce, CCM_STAR_NONCE_LENGTH);
    memset(iv.u8 + CCM_FLAGS_LEN + CCM_STAR_NONCE_LENGTH,
           0,
           AES_128_BLOCK_SIZE - CCM_FLAGS_LEN - CCM_STAR_NONCE_LENGTH);

    /* wait until the AES key is loaded */
    while(HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) & CRYPTO_KEYREADAREA_BUSY);

    /* check that the key was loaded without errors */
    if(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQSTAT_KEY_ST_RD_ERR) {
      LOG_ERR("error at line %d\n", __LINE__);
      /* clear error */
      HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQSTAT_KEY_ST_RD_ERR;
      goto exit;
    }

    /* write the initialization vector */
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV0) = iv.u32[0];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV1) = iv.u32[1];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV2) = iv.u32[2];
    HWREG(CRYPTO_BASE + CRYPTO_O_AESIV3) = iv.u32[3];
  }

  /* configure AES engine */
  HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) =
      CRYPTO_AESCTL_SAVE_CONTEXT /* Save context */
      | (((MAX(mic_len, 2) - 2) >> 1) << CRYPTO_AESCTL_CCM_M_S) /* M */
      | ((CCM_L - 1) << CRYPTO_AESCTL_CCM_L_S) /* L */
      | CRYPTO_AESCTL_CCM /* CCM */
      | CRYPTO_AESCTL_CTR_WIDTH_128_BIT /* CTR width 128 */
      | CRYPTO_AESCTL_CTR /* CTR */
      | (forward ? CRYPTO_AESCTL_DIR : 0); /* En/decryption */
  /* write m_len (lo) */
  HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN0) = m_len;
  /* write m_len (hi) */
  HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN1) = 0;
  /* write a_len */
  HWREG(CRYPTO_BASE + CRYPTO_O_AESAUTHLEN) = a_len;

  /* configure DMAC to fetch "a" */
  if(a_len) {
    /* enable DMA channel 0 */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0CTL) = CRYPTO_DMACH0CTL_EN;
    /* base address of "a" in external memory */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uintptr_t)a;
    /* length of the input data to be transferred */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = a_len;

    /* wait for completion of the DMA transfer */
    while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQSTAT_DMA_IN_DONE));

    /* acknowledge the interrupt */
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQSTAT_DMA_IN_DONE;

    /* check for errors */
    if(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQSTAT_DMA_BUS_ERR) {
      LOG_ERR("error at line %d\n", __LINE__);
      /* clear error */
      HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQSTAT_DMA_BUS_ERR;
      goto exit;
    }
  }

  /* configure DMAC to fetch "m" */
  if(m_len) {
    /* disable DMA_IN interrupt for this transfer */
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_RESULT_AVAIL;
    /* enable DMA channel 0 */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0CTL) = CRYPTO_DMACH0CTL_EN;
    /* base address of "m" in external memory */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uintptr_t)m;
    /* length of the input data to be transferred */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = m_len;
    /* enable DMA channel 1 */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1CTL) = CRYPTO_DMACH1CTL_EN;
    /* base address of the output in external memory */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1EXTADDR) = (uintptr_t)m;
    /* length of the output data to be transferred */
    HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1LEN) = m_len;
  }

  /* wait for completion */
  while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQCLR_RESULT_AVAIL));

  /* acknowledge interrupt */
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQCLR_RESULT_AVAIL;

  /* check for errors */
  uint32_t errors = HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT)
                    & (CRYPTO_IRQSTAT_DMA_BUS_ERR
                       | CRYPTO_IRQSTAT_KEY_ST_RD_ERR);
  if(errors) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear errors */
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = errors;
    goto exit;
  }

  /* wait for the context ready bit */
  while(!(HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) & CRYPTO_AESCTL_SAVED_CONTEXT_RDY_M)) {
  }

  /* read tag */
  {
    block_t tag;
    tag.u32[0] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT0);
    tag.u32[1] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT1);
    tag.u32[2] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT2);

    /* this read clears the ‘saved_context_ready’ flag */
    tag.u32[3] = HWREG(CRYPTO_BASE + CRYPTO_O_AESTAGOUT3);

    memcpy(mic, tag.u8, mic_len);
  }

  result = true;

exit:
  /* all interrupts should have been acknowledged */
  assert(!HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT));

  /* disable master control/DMA clock */
  HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = 0;
  if(!was_crypto_enabled) {
    crypto_disable();
  }
  return result;
}
/*---------------------------------------------------------------------------*/
const struct ccm_star_driver simplelink_ccm_star_driver = {
  set_key,
  aead
};
/*---------------------------------------------------------------------------*/

/** @} */
