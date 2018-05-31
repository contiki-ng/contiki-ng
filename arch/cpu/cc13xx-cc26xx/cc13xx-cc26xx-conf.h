/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup cc13xx-cc26xx
 * @{
 *
 * \file
 * Header with configuration defines common to all CC13xx/CC26xx platforms
 */
/*---------------------------------------------------------------------------*/
#ifndef SIMPLELINK_CONF_H_
#define SIMPLELINK_CONF_H_
/*---------------------------------------------------------------------------*/
#include "cc13xx-cc26xx-def.h"
/*---------------------------------------------------------------------------*/
/**
 * \name Network Stack Configuration
 *
 * @{
 */

/*
 * If set, the systems keeps the HF crystal oscillator on even when the radio is off.
 * You need to set this to 1 to use TSCH with its default 2.2ms or larger guard time.
 */
#ifndef CC2650_FAST_RADIO_STARTUP
#define CC2650_FAST_RADIO_STARTUP           (MAC_CONF_WITH_TSCH)
#endif

#ifdef RF_CHANNEL
#define RF_CORE_CONF_CHANNEL                RF_CHANNEL
#endif

#ifndef RF_CORE_CONF_CHANNEL
#define RF_CORE_CONF_CHANNEL                25
#endif

/* Number of Prop Mode RX buffers */
#ifndef PROP_MODE_CONF_RX_BUF_CNT
#define PROP_MODE_CONF_RX_BUF_CNT           4
#endif

/* Configure Radio mode, i.e. prop or ieee */
/* CC13xx supports both IEEE and Prop mode, depending on which device */
/* CC26xx only supports IEEE mode */
#if defined(DEVICE_LINE_CC13XX)

/* Default mode should be prop for prop-only devices (CC1310, CC1312);
 * Else, IEEE mode is default. */
#   ifndef CC13XX_CONF_PROP_MODE
#       if (SUPPORTS_IEEE_MODE == 0)
#           define CC13XX_CONF_PROP_MODE            1
#       else
#           define CC13XX_CONF_PROP_MODE            0
#       endif
#   endif


#   if (CC13XX_CONF_PROP_MODE == 1) && (SUPPORTS_PROP_MODE == 1)
/*----- CC13xx Prop Mode ----------------------------------------------------*/
#       define NETSTACK_CONF_RADIO          prop_mode_driver

#       define CSMA_CONF_ACK_WAIT_TIME      (RTIMER_SECOND / 400)
#       define CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME \
                                            (RTIMER_SECOND / 1000)
#       define CSMA_CONF_SEND_SOFT_ACK      1

#   elif (CC13XX_CONF_PROP_MODE == 0) && (SUPPORTS_IEEE_MODE == 1)
/*----- CC13xx IEEE Mode ----------------------------------------------------*/
#       define NETSTACK_CONF_RADIO          ieee_mode_driver

#       define CSMA_CONF_SEND_SOFT_ACK      0

#   else
#       error "Invalid radio mode configuration of CC13xx device"
#   endif /* (CC13XX_CONF_PROP_MODE == 1) && (SUPPORTS_PROP_MODE == 1) */

#elif defined(DEVICE_LINE_CC26XX)

#   if (SUPPORTS_IEEE_MODE == 1)
/*----- CC26xx IEEE Mode ----------------------------------------------------*/
#       define NETSTACK_CONF_RADIO          ieee_mode_driver

#       define CSMA_CONF_SEND_SOFT_ACK      0

#   else
#       error "IEEE mode only supported by CC26xx devices"
#   endif /* (SUPPORTS_IEEE_MODE == 1) */

#else
#   error "Unsupported Device Line defined"
#endif /* defined(DEVICE_LINE_CC13xx) */

#define NETSTACK_RADIO_MAX_PAYLOAD_LEN      125

/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name IEEE address configuration
 *
 * Used to generate our link-local & IPv6 address
 * @{
 */
/**
 * \brief Location of the IEEE address
 * 0 => Read from InfoPage,
 * 1 => Use a hardcoded address, configured by IEEE_ADDR_CONF_ADDRESS
 */
#ifndef IEEE_ADDR_CONF_HARDCODED
#define IEEE_ADDR_CONF_HARDCODED            0
#endif

/**
 * \brief The hardcoded IEEE address to be used when IEEE_ADDR_CONF_HARDCODED
 * is defined as 1
 */
#ifndef IEEE_ADDR_CONF_ADDRESS
#define IEEE_ADDR_CONF_ADDRESS              { 0x00, 0x12, 0x4B, 0x00, 0x89, 0xAB, 0xCD, 0xEF }
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name RF configuration
 *
 * @{
 */
/* RF Config */

#ifndef IEEE_MODE_CONF_AUTOACK
#define IEEE_MODE_CONF_AUTOACK              1 /**< RF H/W generates ACKs */
#endif

#ifndef IEEE_MODE_CONF_PROMISCOUS
#define IEEE_MODE_CONF_PROMISCOUS           0 /**< 1 to enable promiscous mode */
#endif

#ifndef RF_BLE_CONF_ENABLED
#define RF_BLE_CONF_ENABLED                 0 /**< 0 to disable BLE support */
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Character I/O Configuration
 *
 * @{
 */
#ifndef SIMPLELINK_UART_CONF_ENABLE
#define SIMPLELINK_UART_CONF_ENABLE         1 /**< Enable/Disable UART I/O */
#endif

#ifndef SIMPLELINK_UART_CONF_BAUD_RATE
#define SIMPLELINK_UART_CONF_BAUD_RATE      115200 /**< Default UART0 baud rate */
#endif

/* Enable I/O over the Debugger Devpack - Only relevant for the SensorTag */
#ifndef BOARD_CONF_DEBUGGER_DEVPACK
#define BOARD_CONF_DEBUGGER_DEVPACK         1
#endif

#ifndef SLIP_ARCH_CONF_ENABLED
/*
 * Determine whether we need SLIP
 * This will keep working while UIP_FALLBACK_INTERFACE and CMD_CONF_OUTPUT
 * keep using SLIP
 */
#if defined(UIP_FALLBACK_INTERFACE) || defined(CMD_CONF_OUTPUT)
#define SLIP_ARCH_CONF_ENABLED              1
#endif

#endif /* SLIP_ARCH_CONF_ENABLED */
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* SIMPLELINK_CONF_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
