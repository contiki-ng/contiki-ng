/*
 * Copyright (C) 2012, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2013, ADVANSEE - http://www.advansee.com/
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
 * \addtogroup crypto
 * @{
 *
 * \defgroup cc-crypto AES/SHA cryptoprocessor
 *
 * Drivers for the AES/SHA cryptoprocessor of CCXXXX MCUs.
 * @{
 *
 * \file
 *       General definitions for the AES/SHA cryptoprocessor.
 */

#ifndef CC_CRYPTO_H_
#define CC_CRYPTO_H_

#include "contiki.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef CC_CRYPTO_CONF_ENABLED
#define CC_CRYPTO_ENABLED CC_CRYPTO_CONF_ENABLED
#else /* CC_CRYPTO_CONF_ENABLED */
#define CC_CRYPTO_ENABLED 0
#endif /* CC_CRYPTO_CONF_ENABLED */

#ifdef CC_CRYPTO_CONF_HAS_SHA_512
#define CC_CRYPTO_HAS_SHA_512 CC_CRYPTO_CONF_HAS_SHA_512
#else /* CC_CRYPTO_CONF_HAS_SHA_512 */
#define CC_CRYPTO_HAS_SHA_512 0
#endif /* CC_CRYPTO_CONF_HAS_SHA_512 */

/**
 * \name DMAC_CHx_CTRL registers bit fields
 * @{
 */
#define CC_CRYPTO_DMAC_CH_CTRL_PRIO \
  0x00000002 /**< Channel priority 0: Low 1: High */
#define CC_CRYPTO_DMAC_CH_CTRL_EN \
  0x00000001 /**< Channel enable */
/** @} */

/**
 * \name DMAC_CHx_DMALENGTH registers bit fields
 * @{
 */
#define CC_CRYPTO_DMAC_CH_DMALENGTH_DMALEN_M \
  0x0000FFFF /**< Channel DMA length in bytes mask */
#define CC_CRYPTO_DMAC_CH_DMALENGTH_DMALEN_S \
  0 /**< Channel DMA length in bytes shift */
/** @} */

/**
 * \name DMAC_STATUS register bit fields
 * @{
 */
#define CC_CRYPTO_DMAC_STATUS_PORT_ERR \
  0x00020000 /**< AHB port transfer errors */
#define CC_CRYPTO_DMAC_STATUS_CH1_ACT \
  0x00000002 /**< Channel 1 active (DMA transfer on-going) */
#define CC_CRYPTO_DMAC_STATUS_CH0_ACT \
  0x00000001 /**< Channel 0 active (DMA transfer on-going) */
/** @} */

/**
 * \name DMAC_SW_RESET register bit fields
 * @{
 */
#define CC_CRYPTO_DMAC_SW_RESET_SW_RESET \
  0x00000001 /**< Software reset enable */
/** @} */

/**
 * \name DMAC_BUS_CFG register bit fields
 * @{
 */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_BURST_SIZE_4 \
  (2 << 12) /**< Maximum burst size: 4 bytes */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_BURST_SIZE_8 \
  (3 << 12) /**< Maximum burst size: 8 bytes */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_BURST_SIZE_16 \
  (4 << 12) /**< Maximum burst size: 16 bytes */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_BURST_SIZE_32 \
  (5 << 12) /**< Maximum burst size: 32 bytes */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_BURST_SIZE_64 \
  (6 << 12) /**< Maximum burst size: 64 bytes */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_BURST_SIZE_M \
  0x0000F000 /**< Maximum burst size mask */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_BURST_SIZE_S \
  12 /**< Maximum burst size shift */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_IDLE_EN \
  0x00000800 /**< Idle insertion between bursts */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_INCR_EN \
  0x00000400 /**< Fixed-length burst or single transfers */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_LOCK_EN \
  0x00000200 /**< Locked transfers */
#define CC_CRYPTO_DMAC_BUS_CFG_AHB_MST1_BIGEND \
  0x00000100 /**< Big endian AHB master */
/** @} */

/**
 * \name DMAC_PORT_ERR register bit fields
 * @{
 */
#define CC_CRYPTO_DMAC_PORT_ERR_PORT1_AHB_ERROR \
  0x00001000 /**< AHB bus error */
#define CC_CRYPTO_DMAC_PORT_ERR_PORT1_CHANNEL \
  0x00000200 /**< Last serviced channel (0 or 1) */
/** @} */

/**
 * \name DMAC_OPTIONS register bit fields
 * @{
 */
#define CC_CRYPTO_DMAC_OPTIONS_NR_OF_CHANNELS_M \
  0x00000F00 /**< Number of channels implemented mask */
#define CC_CRYPTO_DMAC_OPTIONS_NR_OF_CHANNELS_S \
  8 /**< Number of channels implemented shift */
#define CC_CRYPTO_DMAC_OPTIONS_NR_OF_PORTS_M \
  0x00000007 /**< Number of ports implemented mask */
#define CC_CRYPTO_DMAC_OPTIONS_NR_OF_PORTS_S \
  0 /**< Number of ports implemented shift */
/** @} */

/**
 * \name DMAC_VERSION register bit fields
 * @{
 */
#define CC_CRYPTO_DMAC_VERSION_HW_MAJOR_VERSION_M \
  0x0F000000 /**< Major version number mask */
#define CC_CRYPTO_DMAC_VERSION_HW_MAJOR_VERSION_S \
  24 /**< Major version number shift */
#define CC_CRYPTO_DMAC_VERSION_HW_MINOR_VERSION_M \
  0x00F00000 /**< Minor version number mask */
#define CC_CRYPTO_DMAC_VERSION_HW_MINOR_VERSION_S \
  20 /**< Minor version number shift */
#define CC_CRYPTO_DMAC_VERSION_HW_PATCH_LEVEL_M \
  0x000F0000 /**< Patch level mask */
#define CC_CRYPTO_DMAC_VERSION_HW_PATCH_LEVEL_S \
  16 /**< Patch level shift */
#define CC_CRYPTO_DMAC_VERSION_EIP_NUMBER_COMPL_M \
  0x0000FF00 /**< EIP_NUMBER 1's complement mask */
#define CC_CRYPTO_DMAC_VERSION_EIP_NUMBER_COMPL_S \
  8 /**< EIP_NUMBER 1's complement shift */
#define CC_CRYPTO_DMAC_VERSION_EIP_NUMBER_M \
  0x000000FF /**< DMAC EIP-number mask */
#define CC_CRYPTO_DMAC_VERSION_EIP_NUMBER_S \
  0 /**< DMAC EIP-number shift */
/** @} */

/**
 * \name KEY_STORE_SIZE register bit fields
 * @{
 */
#define CC_CRYPTO_KEY_STORE_SIZE_KEY_SIZE_128 \
  1 /**< Key size: 128 bits */
#define CC_CRYPTO_KEY_STORE_SIZE_KEY_SIZE_192 \
  2 /**< Key size: 192 bits */
#define CC_CRYPTO_KEY_STORE_SIZE_KEY_SIZE_256 \
  3 /**< Key size: 256 bits */
#define CC_CRYPTO_KEY_STORE_SIZE_KEY_SIZE_M \
  0x00000003 /**< Key size mask */
#define CC_CRYPTO_KEY_STORE_SIZE_KEY_SIZE_S \
  0 /**< Key size shift */
/** @} */

/**
 * \name KEY_STORE_READ_AREA register bit fields
 * @{
 */
#define CC_CRYPTO_KEY_STORE_READ_AREA_BUSY \
  0x80000000 /**< Key store operation busy */
#define CC_CRYPTO_KEY_STORE_READ_AREA_RAM_AREA_M \
  0x0000000F /**< Key store RAM area select mask */
#define CC_CRYPTO_KEY_STORE_READ_AREA_RAM_AREA_S \
  0 /**< Key store RAM area select shift */
/** @} */

/**
 * \name AES_CTRL register bit fields
 * @{
 */
#define CC_CRYPTO_AES_CTRL_CONTEXT_READY \
  0x80000000 /**< Context data registers can be overwritten */
#define CC_CRYPTO_AES_CTRL_SAVED_CONTEXT_READY \
  0x40000000 /**< AES auth. TAG and/or IV block(s) available */
#define CC_CRYPTO_AES_CTRL_SAVE_CONTEXT \
  0x20000000 /**< Auth. TAG or result IV needs to be stored */
#define CC_CRYPTO_AES_CTRL_GCM_CCM_CONTINUE \
  0x10000000 /**< Continue processing of GCM or CCM */
#define CC_CRYPTO_AES_CTRL_GET_DIGEST \
  0x08000000 /**< Interrupt processing of GCM or CCM */
#define CC_CRYPTO_AES_CTRL_GCM_CCM_CONTINUE_AAD \
  0x04000000 /**<Continue processing of GCM or CCM */
#define CC_CRYPTO_AES_CTRL_XCBC_MAC \
  0x02000000 /**< AES-XCBC MAC mode */
#define CC_CRYPTO_AES_CTRL_CCM_M_M \
  0x01C00000 /**< CCM auth. field length mask */
#define CC_CRYPTO_AES_CTRL_CCM_M_S \
  22 /**< CCM auth. field length shift */
#define CC_CRYPTO_AES_CTRL_CCM_L_M \
  0x00380000 /**< CCM length field width mask */
#define CC_CRYPTO_AES_CTRL_CCM_L_S \
  19 /**< CCM length field width shift */
#define CC_CRYPTO_AES_CTRL_CCM \
  0x00040000 /**< AES-CCM mode */
#define CC_CRYPTO_AES_CTRL_GCM \
  0x00030000 /**< AES-GCM mode */
#define CC_CRYPTO_AES_CTRL_CBC_MAC \
  0x00008000 /**< AES-CBC MAC mode */
#define CC_CRYPTO_AES_CTRL_CTR_WIDTH_32 \
  (0 << 7) /**< CTR counter width: 32 bits */
#define CC_CRYPTO_AES_CTRL_CTR_WIDTH_64 \
  (1 << 7) /**< CTR counter width: 64 bits */
#define CC_CRYPTO_AES_CTRL_CTR_WIDTH_96 \
  (2 << 7) /**< CTR counter width: 96 bits */
#define CC_CRYPTO_AES_CTRL_CTR_WIDTH_128 \
  (3 << 7) /**< CTR counter width: 128 bits */
#define CC_CRYPTO_AES_CTRL_CTR_WIDTH_M \
  0x00000180 /**< CTR counter width mask */
#define CC_CRYPTO_AES_CTRL_CTR_WIDTH_S \
  7 /**< CTR counter width shift */
#define CC_CRYPTO_AES_CTRL_CTR \
  0x00000040 /**< AES-CTR mode */
#define CC_CRYPTO_AES_CTRL_CBC \
  0x00000020 /**< AES-CBC mode */
#define CC_CRYPTO_AES_CTRL_KEY_SIZE_128 \
  (1 << 3) /**< Key size: 128 bits */
#define CC_CRYPTO_AES_CTRL_KEY_SIZE_192 \
  (2 << 3) /**< Key size: 192 bits */
#define CC_CRYPTO_AES_CTRL_KEY_SIZE_256 \
  (3 << 3) /**< Key size: 256 bits */
#define CC_CRYPTO_AES_CTRL_KEY_SIZE_M \
  0x00000018 /**< Key size mask */
#define CC_CRYPTO_AES_CTRL_KEY_SIZE_S \
  3 /**< Key size shift */
#define CC_CRYPTO_AES_CTRL_DIRECTION_ENCRYPT \
  0x00000004 /**< Encrypt */
#define CC_CRYPTO_AES_CTRL_INPUT_READY \
  0x00000002 /**< AES input buffer empty */
#define CC_CRYPTO_AES_CTRL_OUTPUT_READY \
  0x00000001 /**< AES output block available */
/** @} */

/**
 * \name AES_DATA_LENGTH_1 register bit fields
 * @{
 */
#define CC_CRYPTO_AES_DATA_LENGTH_1_C_LENGTH_M \
  0x1FFFFFFF /**< Crypto length bits [60:32] mask */
#define CC_CRYPTO_AES_DATA_LENGTH_1_C_LENGTH_S \
  0 /**< Crypto length bits [60:32] shift */
/** @} */

/**
 * \name HASH_IO_BUF_CTRL register bit fields
 * @{
 */
#define CC_CRYPTO_HASH_IO_BUF_CTRL_PAD_DMA_MESSAGE \
  0x00000080 /**< Hash engine message padding required */
#define CC_CRYPTO_HASH_IO_BUF_CTRL_GET_DIGEST \
  0x00000040 /**< Hash engine digest requested */
#define CC_CRYPTO_HASH_IO_BUF_CTRL_PAD_MESSAGE \
  0x00000020 /**< Last message data in HASH_DATA_IN, apply hash padding */
#define CC_CRYPTO_HASH_IO_BUF_CTRL_RFD_IN \
  0x00000004 /**< Hash engine input buffer can accept new data */
#define CC_CRYPTO_HASH_IO_BUF_CTRL_DATA_IN_AV \
  0x00000002 /**< Start processing HASH_DATA_IN data */
#define CC_CRYPTO_HASH_IO_BUF_CTRL_OUTPUT_FULL \
  0x00000001 /**< Output buffer registers available */
/** @} */

/**
 * \name HASH_MODE register bit fields
 * @{
 */
#define CC_CRYPTO_HASH_MODE_SHA384_MODE \
  0x00000040 /**< SHA-384 */
#define CC_CRYPTO_HASH_MODE_SHA512_MODE \
  0x00000020 /**< SHA-512 */
#define CC_CRYPTO_HASH_MODE_SHA224_MODE \
  0x00000010 /**< SHA-224 */
#define CC_CRYPTO_HASH_MODE_SHA256_MODE \
  0x00000008 /**< SHA-256 */
#define CC_CRYPTO_HASH_MODE_NEW_HASH \
  0x00000001 /**< New hash session */
/** @} */

/**
 * \name CTRL_ALG_SEL register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_ALG_SEL_TAG \
  0x80000000 /**< DMA operation includes TAG */
#define CC_CRYPTO_CTRL_ALG_SEL_HASH_SHA_512 \
  0x00000008 /**< SHA-512 */
#define CC_CRYPTO_CTRL_ALG_SEL_HASH_SHA_256 \
  0x00000004 /**< SHA-256 */
#define CC_CRYPTO_CTRL_ALG_SEL_AES \
  0x00000002 /**< Select AES engine as DMA source/destination */
#define CC_CRYPTO_CTRL_ALG_SEL_KEYSTORE \
  0x00000001 /**< Select Key Store as DMA destination */
/** @} */

/**
 * \name CTRL_PROT_EN register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_PROT_EN_PROT_EN \
  0x00000001 /**< m_h_prot[1] asserted for DMA reads towards key store */
/** @} */

/**
 * \name CTRL_SW_RESET register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_SW_RESET_SW_RESET \
  0x00000001 /**< Reset master control and key store */
/** @} */

/**
 * \name CTRL_INT_CFG register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_INT_CFG_LEVEL \
  0x00000001 /**< Level interrupt type */
/** @} */

/**
 * \name CTRL_INT_EN register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_INT_EN_DMA_IN_DONE \
  0x00000002 /**< DMA input done interrupt enabled */
#define CC_CRYPTO_CTRL_INT_EN_RESULT_AV \
  0x00000001 /**< Result available interrupt enabled */
/** @} */

/**
 * \name CTRL_INT_CLR register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_INT_CLR_DMA_BUS_ERR \
  0x80000000 /**< Clear DMA bus error status */
#define CC_CRYPTO_CTRL_INT_CLR_KEY_ST_WR_ERR \
  0x40000000 /**< Clear key store write error status */
#define CC_CRYPTO_CTRL_INT_CLR_KEY_ST_RD_ERR \
  0x20000000 /**< Clear key store read error status */
#define CC_CRYPTO_CTRL_INT_CLR_DMA_IN_DONE \
  0x00000002 /**< Clear DMA in done interrupt */
#define CC_CRYPTO_CTRL_INT_CLR_RESULT_AV \
  0x00000001 /**< Clear result available interrupt */
/** @} */

/**
 * \name CTRL_INT_SET register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_INT_SET_DMA_IN_DONE \
  0x00000002 /**< Set DMA data in done interrupt */
#define CC_CRYPTO_CTRL_INT_SET_RESULT_AV \
  0x00000001 /**< Set result available interrupt */
/** @} */

/**
 * \name CTRL_INT_STAT register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_INT_STAT_DMA_BUS_ERR \
  0x80000000 /**< DMA bus error detected */
#define CC_CRYPTO_CTRL_INT_STAT_KEY_ST_WR_ERR \
  0x40000000 /**< Write error detected */
#define CC_CRYPTO_CTRL_INT_STAT_KEY_ST_RD_ERR \
  0x20000000 /**< Read error detected */
#define CC_CRYPTO_CTRL_INT_STAT_DMA_IN_DONE \
  0x00000002 /**< DMA data in done interrupt status */
#define CC_CRYPTO_CTRL_INT_STAT_RESULT_AV \
  0x00000001 /**< Result available interrupt status */
/** @} */

/**
 * \name CTRL_OPTIONS register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_OPTIONS_TYPE_M \
  0xFF000000 /**< Device type mask */
#define CC_CRYPTO_CTRL_OPTIONS_TYPE_S \
  24 /**< Device type shift */
#define CC_CRYPTO_CTRL_OPTIONS_AHBINTERFACE \
  0x00010000 /**< AHB interface available */
#define CC_CRYPTO_CTRL_OPTIONS_SHA_256 \
  0x00000100 /**< The HASH core supports SHA-256 */
#define CC_CRYPTO_CTRL_OPTIONS_AES_CCM \
  0x00000080 /**< AES-CCM available as single operation */
#define CC_CRYPTO_CTRL_OPTIONS_AES_GCM \
  0x00000040 /**< AES-GCM available as single operation */
#define CC_CRYPTO_CTRL_OPTIONS_AES_256 \
  0x00000020 /**< AES core supports 256-bit keys */
#define CC_CRYPTO_CTRL_OPTIONS_AES_128 \
  0x00000010 /**< AES core supports 128-bit keys */
#define CC_CRYPTO_CTRL_OPTIONS_HASH \
  0x00000004 /**< HASH Core available */
#define CC_CRYPTO_CTRL_OPTIONS_AES \
  0x00000002 /**< AES core available */
#define CC_CRYPTO_CTRL_OPTIONS_KEYSTORE \
  0x00000001 /**< KEY STORE available */
/** @} */

/**
 * \name CTRL_VERSION register bit fields
 * @{
 */
#define CC_CRYPTO_CTRL_VERSION_MAJOR_VERSION_M \
  0x0F000000 /**< Major version number mask */
#define CC_CRYPTO_CTRL_VERSION_MAJOR_VERSION_S \
  24 /**< Major version number shift */
#define CC_CRYPTO_CTRL_VERSION_MINOR_VERSION_M \
  0x00F00000 /**< Minor version number mask */
#define CC_CRYPTO_CTRL_VERSION_MINOR_VERSION_S \
  20 /**< Minor version number shift */
#define CC_CRYPTO_CTRL_VERSION_PATCH_LEVEL_M \
  0x000F0000 /**< Patch level mask */
#define CC_CRYPTO_CTRL_VERSION_PATCH_LEVEL_S \
  16 /**< Patch level shift */
#define CC_CRYPTO_CTRL_VERSION_EIP_NUMBER_COMPL_M \
  0x0000FF00 /**< EIP_NUMBER 1's complement mask */
#define CC_CRYPTO_CTRL_VERSION_EIP_NUMBER_COMPL_S \
  8 /**< EIP_NUMBER 1's complement shift */
#define CC_CRYPTO_CTRL_VERSION_EIP_NUMBER_M \
  0x000000FF /**< EIP-120t EIP-number mask */
#define CC_CRYPTO_CTRL_VERSION_EIP_NUMBER_S \
  0 /**< EIP-120t EIP-number shift */
/** @} */

typedef volatile uint32_t cc_crypto_reg_t;

struct crypto_dma_channel {
  cc_crypto_reg_t ctrl; /**< Configures the DMA channel */
  cc_crypto_reg_t extaddr; /**< Sets the external address */
  cc_crypto_reg_t reserved1;
  cc_crypto_reg_t dmalength; /**< Sets transfer size and starts DMA transfer */
  cc_crypto_reg_t reserved2[2];
};

/** Registers of the AES/SHA cryptoprocessor. */
struct cc_crypto {

  /** DMA controller (DMAC) 0x000-0x3FF */
  struct {
    struct crypto_dma_channel ch0;
    cc_crypto_reg_t status; /**< Provides status and error information */
    cc_crypto_reg_t sw_reset; /**< Resets the DMAC */
    struct crypto_dma_channel ch1;
    cc_crypto_reg_t reserved1[16];
    cc_crypto_reg_t bus_cfg; /**< Configures the master interface port */
    cc_crypto_reg_t port_err; /** Provides details on errors */
    cc_crypto_reg_t reserved2[30];
    cc_crypto_reg_t options; /**< Provides info about supported features */
    cc_crypto_reg_t version; /**< Provides the hardware version */
    cc_crypto_reg_t reserved3[192];
  } dmac;

  /** Key store 0x400-0x4FF */
  struct {
    cc_crypto_reg_t write_area; /**< Mask of the 128-bit slots to write */
    cc_crypto_reg_t written_area; /**< Mask of written 128-bit slots */
    cc_crypto_reg_t size; /**< Size of the keys */
    cc_crypto_reg_t read_area; /**< Mask of the 128-bit slots to read */
    cc_crypto_reg_t reserved[60];
  } key_store;

  /** AES engine 0x500-0x5FF */
  struct {
    cc_crypto_reg_t key[8]; /**< Internally calculated keys */
    cc_crypto_reg_t reserved1[8];
    cc_crypto_reg_t iv[4]; /**< Initialization vector */
    cc_crypto_reg_t ctrl; /**< Configuration of the AES engine */
    cc_crypto_reg_t data_length[2]; /**< Length of the data to encrypt */
    cc_crypto_reg_t auth_length; /**< Length of the data to authenticate */
    cc_crypto_reg_t data_in_out[4]; /**< Input or output data */
    cc_crypto_reg_t tag_out[4]; /**< Authentication tag */
    cc_crypto_reg_t reserved2[21];
    cc_crypto_reg_t ccm_aln_wrd; /**< Context for later resumption */
    cc_crypto_reg_t blk_cnt[2]; /**< Block count for later resumption */
    cc_crypto_reg_t reserved3[8];
  } aes;

  /** Hash engine 0x600-0x6FF */
  struct {
    cc_crypto_reg_t data_in[CC_CRYPTO_HAS_SHA_512 ? 32 : 16]; /**< Input data */
    cc_crypto_reg_t io_buf_ctrl; /** Configuration of the I/O buffer */
    cc_crypto_reg_t mode; /**< Selection of the algorithm and the resumption */
    cc_crypto_reg_t length_in[2]; /**< Length of the input data */
    cc_crypto_reg_t reserved1[CC_CRYPTO_HAS_SHA_512 ? 12 : 0];
    cc_crypto_reg_t digest[CC_CRYPTO_HAS_SHA_512 ? 16 : 8]; /**< Hash digest */
    cc_crypto_reg_t reserved2[CC_CRYPTO_HAS_SHA_512 ? 0 : 36];
  } hash;

  /** Master control 0x700-0x7FF */
  struct {
    cc_crypto_reg_t alg_sel; /**< Configures the destination of the DMAC */
    cc_crypto_reg_t prot_en; /**< Protects DMA transfers to the key store */
    cc_crypto_reg_t reserved1[14];
    cc_crypto_reg_t sw_reset; /**< Resets master control and key store */
    cc_crypto_reg_t reserved2[15];
    cc_crypto_reg_t int_cfg; /**< Configures interrupts */
    cc_crypto_reg_t int_en; /**< Enables interrupts */
    cc_crypto_reg_t int_clr; /**< Acknowledges interrupts */
    cc_crypto_reg_t int_set; /**< Tests interrupts */
    cc_crypto_reg_t int_stat; /**< Checks for interrupts */
    cc_crypto_reg_t reserved3[25];
    cc_crypto_reg_t options; /**< Provides info about supported features */
    cc_crypto_reg_t version; /**< Provides the hardware version */
  } ctrl;
};

extern struct cc_crypto *const cc_crypto;

/**
 * \brief Enables and resets the AES/SHA cryptoprocessor.
 */
void cc_crypto_init(void);

/**
 * \brief Enables the AES/SHA cryptoprocessor.
 */
void cc_crypto_enable(void);

/**
 * \brief Disables the AES/SHA cryptoprocessor.
 * \note Call this function to save power when the cryptoprocessor is unused.
 */
void cc_crypto_disable(void);

/**
 * \brief  Checks if the AES/SHA cryptoprocessor is on.
 * \return \c true if the AES/SHA cryptoprocessor is on and \c false otherwise.
 */
bool cc_crypto_is_enabled(void);

#endif /* CC_CRYPTO_H_ */

/**
 * @}
 * @}
 */
