/*
 * Original file:
 * Copyright (C) 2012 Texas Instruments Incorporated - http://www.ti.com/
 * All rights reserved.
 *
 * Port to Contiki:
 * Copyright (c) 2013, ADVANSEE - http://www.advansee.com/
 * All rights reserved.
 *
 * Adaptation to platform-independent API:
 * Copyright (c) 2021, Uppsala universitet
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
 *       Implementation of the SHA-256 driver for CCXXXX MCUs.
 */

#include "dev/crypto/cc/cc-sha-256.h"
#include "dev/crypto/cc/cc-crypto.h"
#include "lib/aes-128.h"
#include "lib/assert.h"
#include "sys/cc.h"
#include <stdbool.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "cc-sha-256"
#define LOG_LEVEL LOG_LEVEL_NONE

static const uint8_t empty_digest[SHA_256_DIGEST_LENGTH] = {
  0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14,
  0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
  0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c,
  0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55
};
static bool was_crypto_enabled;

/*---------------------------------------------------------------------------*/
static bool
is_valid_source_address(uintptr_t address)
{
  static const uintptr_t sram_base = 0x20000000;
  return address >= sram_base;
}
/*---------------------------------------------------------------------------*/
static void
enable_crypto(void)
{
  was_crypto_enabled = cc_crypto_is_enabled();
  if(!was_crypto_enabled) {
    cc_crypto_enable();
  }
  /* enable DMA path to the SHA-256 engine + Digest readout */
  cc_crypto->ctrl.alg_sel = CC_CRYPTO_CTRL_ALG_SEL_TAG
                            | CC_CRYPTO_CTRL_ALG_SEL_HASH_SHA_256;
}
/*---------------------------------------------------------------------------*/
static void
disable_crypto(void)
{
  /* disable master control/DMA clock */
  cc_crypto->ctrl.alg_sel = 0;
  if(!was_crypto_enabled) {
    cc_crypto_disable();
  }
}
/*---------------------------------------------------------------------------*/
static void
do_hash(const uint8_t *data, size_t len,
        void *digest, uint64_t final_bit_count)
{
  /* DMA fails if data does not reside in RAM  */
  assert(is_valid_source_address((uintptr_t)data));
  /* all previous interrupts should have been acknowledged */
  assert(!cc_crypto->ctrl.int_stat);

  /* set up AES interrupts */
  cc_crypto->ctrl.int_cfg = CC_CRYPTO_CTRL_INT_CFG_LEVEL;
  cc_crypto->ctrl.int_en = CC_CRYPTO_CTRL_INT_EN_RESULT_AV;

  if(sha_256_checkpoint.bit_count) {
    /* configure resumed hash session */
    cc_crypto->hash.mode = CC_CRYPTO_HASH_MODE_SHA256_MODE;
    for(size_t i = 0; i < CC_ARRAY_LENGTH(sha_256_checkpoint.state); i++) {
      cc_crypto->hash.digest[i] = sha_256_checkpoint.state[i];
    }
  } else {
    /* configure new hash session */
    cc_crypto->hash.mode = CC_CRYPTO_HASH_MODE_SHA256_MODE
                           | CC_CRYPTO_HASH_MODE_NEW_HASH;
  }

  if(final_bit_count) {
    /* configure for generating the final hash */
    cc_crypto->hash.length_in[0] = final_bit_count;
    cc_crypto->hash.length_in[1] = final_bit_count >> 32;
    cc_crypto->hash.io_buf_ctrl = CC_CRYPTO_HASH_IO_BUF_CTRL_PAD_DMA_MESSAGE;
  }

  /* enable DMA channel 0 for message data */
  cc_crypto->dmac.ch0.ctrl = CC_CRYPTO_DMAC_CH_CTRL_EN;
  /* set base address of the data in ext. memory */
  cc_crypto->dmac.ch0.extaddr = (uintptr_t)data;
  /* set input data length in bytes */
  cc_crypto->dmac.ch0.dmalength = len;
  /* enable DMA channel 1 for result digest */
  cc_crypto->dmac.ch1.ctrl = CC_CRYPTO_DMAC_CH_CTRL_EN;
  /* set base address of the digest buffer */
  cc_crypto->dmac.ch1.extaddr = (uintptr_t)digest;
  /* set length of the result digest */
  cc_crypto->dmac.ch1.dmalength = SHA_256_DIGEST_LENGTH;

  /* wait for operation done (hash and DMAC are ready) */
  while(!(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_RESULT_AV));

  /* clear the interrupt */
  cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_CLR_RESULT_AV;

  /* check for the absence of errors */
  if(cc_crypto->ctrl.int_stat & CC_CRYPTO_CTRL_INT_STAT_DMA_BUS_ERR) {
    LOG_ERR("error at line %d\n", __LINE__);
    cc_crypto->ctrl.int_clr = CC_CRYPTO_CTRL_INT_STAT_DMA_BUS_ERR;
    sha_256_checkpoint.is_error_free = false;
  }

  /* all interrupts should have been acknowledged */
  assert(!cc_crypto->ctrl.int_stat);
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  sha_256_checkpoint.buf_len = 0;
  sha_256_checkpoint.bit_count = 0;
  sha_256_checkpoint.is_error_free = true;
  enable_crypto();
}
/*---------------------------------------------------------------------------*/
static void
update(const uint8_t *data, size_t len)
{
  while(len) {
    size_t n;
    if(!sha_256_checkpoint.buf_len && (len > SHA_256_BLOCK_SIZE)) {
      if(is_valid_source_address((uintptr_t)data)) {
        n = (len - 1) & ~(SHA_256_BLOCK_SIZE - 1);
        do_hash(data, n, sha_256_checkpoint.state, 0);
      } else {
        n = SHA_256_BLOCK_SIZE;
        memcpy(sha_256_checkpoint.buf, data, n);
        do_hash(sha_256_checkpoint.buf, n, sha_256_checkpoint.state, 0);
      }
      sha_256_checkpoint.bit_count += n << 3;
      data += n;
      len -= n;
    } else {
      n = MIN(len, SHA_256_BLOCK_SIZE - sha_256_checkpoint.buf_len);
      memcpy(sha_256_checkpoint.buf + sha_256_checkpoint.buf_len, data, n);
      sha_256_checkpoint.buf_len += n;
      data += n;
      len -= n;
      if((sha_256_checkpoint.buf_len == SHA_256_BLOCK_SIZE) && len) {
        do_hash(sha_256_checkpoint.buf, SHA_256_BLOCK_SIZE,
                sha_256_checkpoint.state, 0);
        sha_256_checkpoint.bit_count += SHA_256_BLOCK_SIZE << 3;
        sha_256_checkpoint.buf_len = 0;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
cancel(void)
{
  if(cc_crypto_is_enabled()
     && (cc_crypto->ctrl.alg_sel & CC_CRYPTO_CTRL_ALG_SEL_HASH_SHA_256)) {
    disable_crypto();
  }
}
/*---------------------------------------------------------------------------*/
static bool
finalize(uint8_t digest[static SHA_256_DIGEST_LENGTH])
{
  uint64_t final_bit_count = sha_256_checkpoint.bit_count
                             + (sha_256_checkpoint.buf_len << 3);
  if(!final_bit_count) {
    /* the CC2538 would freeze otherwise */
    memcpy(digest, empty_digest, sizeof(empty_digest));
  } else {
    do_hash(sha_256_checkpoint.buf, sha_256_checkpoint.buf_len,
            digest, final_bit_count);
  }
  disable_crypto();
  return sha_256_checkpoint.is_error_free;
}
/*---------------------------------------------------------------------------*/
static void
create_checkpoint(sha_256_checkpoint_t *cp)
{
  disable_crypto();
  memcpy(cp, &sha_256_checkpoint, sizeof(*cp));
}
/*---------------------------------------------------------------------------*/
static void
restore_checkpoint(const sha_256_checkpoint_t *cp)
{
  memcpy(&sha_256_checkpoint, cp, sizeof(sha_256_checkpoint));
  enable_crypto();
}
/*---------------------------------------------------------------------------*/
static bool
hash(const uint8_t *data, size_t len,
     uint8_t digest[static SHA_256_DIGEST_LENGTH])
{
  if(!len) {
    /* the CC2538 would freeze otherwise */
    memcpy(digest, empty_digest, sizeof(empty_digest));
    return true;
  } else if(is_valid_source_address((uintptr_t)data)) {
    init();
    do_hash(data, len, digest, len << 3);
    disable_crypto();
    return sha_256_checkpoint.is_error_free;
  } else {
    return sha_256_hash(data, len, digest);
  }
}
/*---------------------------------------------------------------------------*/
const struct sha_256_driver cc_sha_256_driver = {
  init,
  update,
  cancel,
  finalize,
  create_checkpoint,
  restore_checkpoint,
  hash,
};
/*---------------------------------------------------------------------------*/

/** @} */
