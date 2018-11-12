/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \defgroup cc13xx-cc26xx-ccfg Customer Configuration (CCFG)
 *
 * @{
 *
 * \file
 *        Configuration of CCFG.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki-conf.h"
/*---------------------------------------------------------------------------*/
/**
 * \name JTAG interface configuration
 *
 * Enable/Disable the JTAG DAP and TAP interfaces on the chip.
 * Setting this to 0 will disable access to the debug interface
 * to secure deployed images.
 * @{
 */
#if CCFG_CONF_JTAG_INTERFACE_DISABLE
#define SET_CCFG_CCFG_TI_OPTIONS_TI_FA_ENABLE        0x00
#define SET_CCFG_CCFG_TAP_DAP_0_CPU_DAP_ENABLE       0x00
#define SET_CCFG_CCFG_TAP_DAP_0_PRCM_TAP_ENABLE      0x00
#define SET_CCFG_CCFG_TAP_DAP_0_TEST_TAP_ENABLE      0x00
#define SET_CCFG_CCFG_TAP_DAP_1_PBIST2_TAP_ENABLE    0x00
#define SET_CCFG_CCFG_TAP_DAP_1_PBIST1_TAP_ENABLE    0x00
#define SET_CCFG_CCFG_TAP_DAP_1_WUC_TAP_ENABLE       0x00
#endif /* CCFG_CONF_JTAG_INTERFACE_DISABLE */
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name TX Power Boost Mode
 *
 * CC13xx only: Enable/Disable boost mode, which enables maximum +14 dBm
 * output power with the default PA front-end configuration.
 * @{
 */
#if defined(DEVICE_LINE_CC13XX) && (RF_CONF_TXPOWER_BOOST_MODE)
#define CCFG_FORCE_VDDR_HH                           1
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name ROM Bootloader configuration
 *
 * Enable/Disable the ROM bootloader in your image, if the board supports it.
 * Look in Board.h to choose the DIO and corresponding level that will cause
 * the chip to enter bootloader mode.
 * @{
 */
#ifndef CCFG_CONF_ROM_BOOTLOADER_ENABLE
#define CCFG_CONF_ROM_BOOTLOADER_ENABLE               0
#endif

#if CCFG_CONF_ROM_BOOTLOADER_ENABLE
#define SET_CCFG_BL_CONFIG_BOOTLOADER_ENABLE         0xC5
#define SET_CCFG_BL_CONFIG_BL_LEVEL                  0x00
#if defined(CCFG_CONF_BL_PIN_NUMBER)
#define SET_CCFG_BL_CONFIG_BL_PIN_NUMBER            CCFG_CONF_BL_PIN_NUMBER
#endif
#define SET_CCFG_BL_CONFIG_BL_ENABLE                 0xC5
#endif /* CCFG_CONF_ROM_BOOTLOADER_ENABLE */
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Include the device-specific CCFG file from the SDK.
 *
 * @{
 */
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(startup_files/ccfg.c)
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
