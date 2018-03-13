/*
 * Copyright (c) 2015, Zolertia - http://www.zolertia.com
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
 * \addtogroup zoul-core
 * @{
 *
 * \defgroup zoul-platforms Zolertia Zoul platforms
 *
 * The Zoul allows a fast reuse and easy integration to most applications and
 * products.  Its small size and module format eases to place in different PCB
 * designs and to integrate in existing products.  The Zoul-based platforms
 * share most of the Zoul core implementation.
 *
 * \file
 *  Configuration for the Zoul-based platforms
 */
#ifndef CONTIKI_CONF_H_
#define CONTIKI_CONF_H_

#include <stdint.h>
#include <string.h>
#include <inttypes.h>
/*---------------------------------------------------------------------------*/
/* Include Project Specific conf */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
/*---------------------------------------------------------------------------*/
#include "cc2538-def.h"
/*---------------------------------------------------------------------------*/
/**
 * \name Serial Boot Loader Backdoor configuration
 *
 * @{
 */
#ifndef FLASH_CCA_CONF_BOOTLDR_BACKDOOR
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR 1 /**<Enable the boot loader backdoor */
#endif

#ifndef FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN 3 /**< Pin PA_3 (user button), activates the boot loader */
#endif

#ifndef FLASH_CCA_CONF_BOOTLDR_BACKDOOR_ACTIVE_HIGH
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR_ACTIVE_HIGH 0 /**< A logic low level activates the boot loader */
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name CC2538 System Control configuration
 *
 * @{
 */
#ifndef SYS_CTRL_CONF_OSC32K_USE_XTAL
#define SYS_CTRL_CONF_OSC32K_USE_XTAL   1 /**< Use the on-board 32.768-kHz crystal */
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/* board.h assumes that basic configuration is done */
#include "board.h"

#define PLATFORM_SUPPORTS_BUTTON_HAL PLATFORM_HAS_BUTTON
/*---------------------------------------------------------------------------*/
/**
 * \name Radio Configuration
 *
 * @{
 */
/* Configure CSMA for when it's selected */

#if CC1200_CONF_SUBGHZ_50KBPS_MODE
#define NETSTACK_CONF_RADIO                                 cc1200_driver
#define CC1200_CONF_RF_CFG                                  cc1200_802154g_863_870_fsk_50kbps
#define ANTENNA_SW_SELECT_DEF_CONF                          ANTENNA_SW_SELECT_SUBGHZ
#define CC1200_CONF_USE_GPIO2                               0
#define CC1200_CONF_USE_RX_WATCHDOG                         0

#define CSMA_CONF_ACK_WAIT_TIME                          (RTIMER_SECOND / 200)
#define CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME           (RTIMER_SECOND / 1500)

#endif

/* This can be overriden to use the cc1200_driver instead */
#ifndef NETSTACK_CONF_RADIO
#define NETSTACK_CONF_RADIO         cc2538_rf_driver
#endif

/*
 * RE-Mote specific:
 * If dual RF enabled, we set the RF switch to enable the CC1200 and use 2.4GHz
 * on the available uFl/chip antenna (not mounted as default).  In contiki main
 * platform routine we set the right antenna depending on NETSTACK_CONF_RADIO,
 * but as changing the RF antenna also implies enabling/disabling the CC1200,
 * is better to start off with the right configuration
 */
#if REMOTE_DUAL_RF_ENABLED
#define ANTENNA_SW_SELECT_DEFAULT ANTENNA_SW_SELECT_SUBGHZ
#else /* REMOTE_DUAL_RF_ENABLED */
#ifndef ANTENNA_SW_SELECT_DEF_CONF
#define ANTENNA_SW_SELECT_DEFAULT ANTENNA_SW_SELECT_2_4GHZ
#else /* ANTENNA_SW_SELECT_DEF_CONF */
#define ANTENNA_SW_SELECT_DEFAULT ANTENNA_SW_SELECT_DEF_CONF
#endif /* ANTENNA_SW_SELECT_DEF_CONF */
#endif /* REMOTE_DUAL_RF_ENABLED */

/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name LPM configuration
 * @{
 */
#ifndef LPM_CONF_ENABLE
#define LPM_CONF_ENABLE       1 /**< Set to 0 to disable LPM entirely */
#endif

/**
 * \brief Maximum PM
 *
 * The SoC will never drop to a Power Mode deeper than the one specified here.
 * 0 for PM0, 1 for PM1 and 2 for PM2
 */
#ifndef LPM_CONF_MAX_PM
#define LPM_CONF_MAX_PM       2
#endif

#ifndef LPM_CONF_STATS
#define LPM_CONF_STATS        0 /**< Set to 1 to enable LPM-related stats */
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name RTC
 *
 * @{
 */
#ifdef PLATFORM_HAS_RTC

#ifndef RTC_CONF_INIT
#define RTC_CONF_INIT   0 /**< Whether to initialize the RTC */
#endif

#ifndef RTC_CONF_SET_FROM_SYS
#define RTC_CONF_SET_FROM_SYS    0 /**< Whether to set the RTC from the build system */
#endif

#else
#undef RTC_CONF_INIT
#define RTC_CONF_INIT   0
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/* Include CPU-related configuration */
#include "cc2538-conf.h"
/*---------------------------------------------------------------------------*/
#endif /* CONTIKI_CONF_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
