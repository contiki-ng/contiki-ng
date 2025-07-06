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
 *         Implementation of the AES-128 driver for SimpleLink MCUs.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "dev/simplelink-aes-128.h"
#include "dev/crypto.h"
#include "lib/assert.h"
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_crypto.h)
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)

#ifdef SIMPLELINK_AES_128_CONF_KEY_AREA
#define KEY_AREA SIMPLELINK_AES_128_CONF_KEY_AREA
#else /* SIMPLELINK_AES_128_CONF_KEY_AREA */
#define KEY_AREA 0
#endif /* SIMPLELINK_AES_128_CONF_KEY_AREA */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "simplelink-aes-128"
#define LOG_LEVEL LOG_LEVEL_NONE

/*---------------------------------------------------------------------------*/
static bool
set_key(const uint8_t key[static AES_128_KEY_LENGTH])
{
  bool result = false;
  bool was_crypto_enabled = crypto_is_enabled();
  if(!was_crypto_enabled) {
    crypto_enable();
  }

  /* all previous interrupts should have been acknowledged */
  assert(!HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT));

  /* set up AES interrupts */
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_RESULT_AVAIL;

  /* enable DMA path to the key store module */
  HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_KEY_STORE;

  /* configure key store module (area, size) - note that setting
   * CRYPTO_O_KEYSIZE to CRYPTO_KEYSIZE_SIZE_128_BIT is unnecessary
   * because CRYPTO_KEYSIZE_SIZE_128_BIT is the reset value. Moreover,
   * writing CRYPTO_O_KEYSIZE would clear all other loaded keys. */
  /* clear key to write */
  HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA) = 1 << KEY_AREA;
  /* enable key to write */
  HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITEAREA) = 1 << KEY_AREA;

  /* configure DMAC */
  /* enable DMA channel 0 */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0CTL) = CRYPTO_DMACH0CTL_EN;
  /* set base address of the aligned key in external memory */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uintptr_t)key;
  /* total key length in bytes (e.g. 16 for 1 x 128-bit key) */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = AES_128_KEY_LENGTH;

  /* wait for completion */
  while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQCLR_RESULT_AVAIL));

  /* acknowledge the interrupt */
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQCLR_RESULT_AVAIL;

  /* check for absence of errors in DMA and key store */
  uint32_t errors = HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT)
                    & (CRYPTO_IRQSTAT_DMA_BUS_ERR
                       | CRYPTO_IRQSTAT_KEY_ST_WR_ERR);
  if(errors) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear errors */
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = errors;
    goto exit;
  }

  /* check that key was written */
  if(!(HWREG(CRYPTO_BASE + CRYPTO_O_KEYWRITTENAREA)
       & (1 << KEY_AREA))) {
    LOG_ERR("error at line %d\n", __LINE__);
    goto exit;
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
static bool
encrypt(uint8_t plaintext_and_result[static AES_128_BLOCK_SIZE])
{
  bool result = false;
  bool was_crypto_enabled = crypto_is_enabled();
  if(!was_crypto_enabled) {
    crypto_enable();
  }

  /* all previous interrupts should have been acknowledged */
  assert(!HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT));

  /* set up AES interrupts */
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQTYPE) = CRYPTO_IRQTYPE_LEVEL;
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQEN) = CRYPTO_IRQEN_RESULT_AVAIL;

  /* enable the DMA path to the AES engine */
  HWREG(CRYPTO_BASE + CRYPTO_O_ALGSEL) = CRYPTO_ALGSEL_AES;

  /* configure the key store to provide pre-loaded AES key */
  HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) = KEY_AREA;

  /* wait until the key is loaded to the AES module */
  while(HWREG(CRYPTO_BASE + CRYPTO_O_KEYREADAREA) & CRYPTO_KEYREADAREA_BUSY);

  /* check if the key was loaded without errors */
  if(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQSTAT_KEY_ST_RD_ERR) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear error */
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) = CRYPTO_IRQSTAT_KEY_ST_RD_ERR;
    goto exit;
  }

  /* configure AES engine */
  HWREG(CRYPTO_BASE + CRYPTO_O_AESCTL) = CRYPTO_AESCTL_DIR;
  HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN0) =
      AES_128_BLOCK_SIZE; /* write length of the message (lo) */
  HWREG(CRYPTO_BASE + CRYPTO_O_AESDATALEN1) =
      0; /* write length of the message (hi) */

  /* configure DMAC */
  /* enable DMA channel 0 */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0CTL) = CRYPTO_DMACH0CTL_EN;
  /* base address of the input data in external memory */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0EXTADDR) = (uintptr_t)plaintext_and_result;
  /* length of the input data to be transferred */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH0LEN) = AES_128_BLOCK_SIZE;
  /* enable DMA channel 1 */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1CTL) = CRYPTO_DMACH1CTL_EN;
  /* base address of the output data in external memory */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1EXTADDR) = (uintptr_t)plaintext_and_result;
  /* length of the output data to be transferred */
  HWREG(CRYPTO_BASE + CRYPTO_O_DMACH1LEN) = AES_128_BLOCK_SIZE;

  /* wait for completion */
  while(!(HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT) & CRYPTO_IRQCLR_RESULT_AVAIL));

  /* acknowledge the interrupt */
  HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = CRYPTO_IRQCLR_RESULT_AVAIL;

  /* check for errors in DMA and key store */
  uint32_t errors = HWREG(CRYPTO_BASE + CRYPTO_O_IRQSTAT)
                    & (CRYPTO_IRQSTAT_DMA_BUS_ERR
                       | CRYPTO_IRQSTAT_KEY_ST_RD_ERR);
  if(errors) {
    LOG_ERR("error at line %d\n", __LINE__);
    /* clear errors */
    HWREG(CRYPTO_BASE + CRYPTO_O_IRQCLR) = errors;
    goto exit;
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
const struct aes_128_driver simplelink_aes_128_driver = {
  set_key,
  encrypt
};
/*---------------------------------------------------------------------------*/

/** @} */
