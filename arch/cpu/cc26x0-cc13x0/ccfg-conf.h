/*
 * Copyright (c) 2017, Alex Stanoev
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
 * \addtogroup cc26xx
 * @{
 *
 * \defgroup cc13xx-cc26xx-ccfg Customer Configuration (CCFG)
 *
 * @{
 *
 * \file
 *  CCFG configuration for the cc26x0-cc13x0 CPU family
 */
#ifndef CCFG_CONF_H_
#define CCFG_CONF_H_

#include "contiki-conf.h"

/*---------------------------------------------------------------------------*/
#ifdef CCXXWARE_CONF_JTAG_INTERFACE_ENABLE
#error CCXXWARE_CONF_JTAG_INTERFACE_ENABLE is deprecated. Use \
  CCFG_CONF_JTAG_INTERFACE_DISABLE.
#endif
#ifdef CCXXWARE_CONF_ROM_BOOTLOADER_ENABLE
#error CCXXWARE_CONF_ROM_BOOTLOADER_ENABLE is deprecated. Use \
  CCFG_CONF_ROM_BOOTLOADER_ENABLE.
#endif
#ifdef CCXXWARE_CONF_BL_PIN_NUMBER
#error CCXXWARE_CONF_BL_PIN_NUMBER is deprecated. Use CCFG_CONF_BL_PIN_NUMBER.
#endif
#ifdef CCXXWARE_CONF_BL_LEVEL
#error CCXXWARE_CONF_BL_LEVEL is deprecated. Use CCFG_CONF_BL_LEVEL.
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief JTAG interface configuration
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#if CCFG_CONF_JTAG_INTERFACE_DISABLE
#define SET_CCFG_CCFG_TI_OPTIONS_TI_FA_ENABLE           0x00
#define SET_CCFG_CCFG_TAP_DAP_0_CPU_DAP_ENABLE          0x00
#define SET_CCFG_CCFG_TAP_DAP_0_PRCM_TAP_ENABLE         0x00
#define SET_CCFG_CCFG_TAP_DAP_0_TEST_TAP_ENABLE         0x00
#define SET_CCFG_CCFG_TAP_DAP_1_PBIST2_TAP_ENABLE       0x00
#define SET_CCFG_CCFG_TAP_DAP_1_PBIST1_TAP_ENABLE       0x00
#define SET_CCFG_CCFG_TAP_DAP_1_WUC_TAP_ENABLE          0x00
#else
#define SET_CCFG_CCFG_TI_OPTIONS_TI_FA_ENABLE           0xC5
#define SET_CCFG_CCFG_TAP_DAP_0_CPU_DAP_ENABLE          0xC5
#define SET_CCFG_CCFG_TAP_DAP_0_PRCM_TAP_ENABLE         0xC5
#define SET_CCFG_CCFG_TAP_DAP_0_TEST_TAP_ENABLE         0xC5
#define SET_CCFG_CCFG_TAP_DAP_1_PBIST2_TAP_ENABLE       0xC5
#define SET_CCFG_CCFG_TAP_DAP_1_PBIST1_TAP_ENABLE       0xC5
#define SET_CCFG_CCFG_TAP_DAP_1_WUC_TAP_ENABLE          0xC5
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \brief ROM bootloader configuration
 *
 * Those values are not meant to be modified by the user
 * @{
 */
#if CCFG_CONF_ROM_BOOTLOADER_ENABLE
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE      0xC5
#define SET_CCFG_BL_CONFIG_BL_LEVEL               CCFG_CONF_BL_LEVEL
#define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER          CCFG_CONF_BL_PIN_NUMBER
#define SET_CCFG_BL_CONFIG_BL_ENABLE              0xC5
#else
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE      0x00
#define SET_CCFG_BL_CONFIG_BL_LEVEL               0x01
#define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER          IOID_UNUSED
#define SET_CCFG_BL_CONFIG_BL_ENABLE              0xFF
#endif
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* CCFG_CONF_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
