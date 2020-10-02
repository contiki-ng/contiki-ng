/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2020, Oppila Microsystems <http://www.oppila.in>
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
/*---------------------------------------------------------------------------*/
/* 
 * \addtogroup oppila-platforms
 * @{
 *
 * \defgroup oppila-orouter-ethernet-router Oppila IoT Ethernet ORouter
 *
 * The Oppila ORouter includes an Ethernet ENC28J60 controller, operating over IPv4/IP64.
 * It features a RF interface (2.4GHz) with on board PCB antenna
 *
 * This file provides connectivity information on LED, Buttons, UART and
 * other peripherals
 * @{
 *
 * \file
 * Header file with definitions related to the I/O connections on the Oppila's
 * Ethernet ORouter
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 */         
/*---------------------------------------------------------------------------*/
#ifndef BOARD_H_
#define BOARD_H_

#include "dev/gpio.h"
#include "dev/nvic.h"
/*---------------------------------------------------------------------------*/
/** \name Orouter Ethernet Router LED configuration
 *
 * LEDs on the eth-gw are connected as follows:
 * - LED1 (Green)    -> PD2
 * @{
 */
/*---------------------------------------------------------------------------*/
#define LEDS_ARCH_L1_PORT GPIO_D_NUM
#define LEDS_ARCH_L1_PIN  2

#define LEDS_CONF_RED     1

#define LEDS_CONF_COUNT   1
/** @} */
/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup for D+ is not included in this platform
 */
#ifdef USB_PULLUP_PORT
#undef USB_PULLUP_PORT
#endif
#ifdef USB_PULLUP_PIN
#undef USB_PULLUP_PIN
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/** \name UART configuration
 *
 * On the eth-gw, the UARTs are connected to the following ports/pins:
 *
 * - UART0:
 *   - RX:  PA0
 *   - TX:  PA1
 * @{
 */
#define UART0_RX_PORT            GPIO_A_NUM
#define UART0_RX_PIN             0
#define UART0_TX_PORT            GPIO_A_NUM
#define UART0_TX_PIN             1

/** @} */
/*---------------------------------------------------------------------------*/
/** \name ORouter button configuration
 *
 * Buttons on the eth-gw are connected as follows:
 * - BUTTON_USER  -> PA3,Sw2 user button, shared with bootloader
 * - BUTTON_RESET -> RESET_N line
 * @{
 */
/** BUTTON_USER -> PA3 */
#define BUTTON_USER_PORT       GPIO_A_NUM
#define BUTTON_USER_PIN        3
#define BUTTON_USER_VECTOR     GPIO_A_IRQn

#define PLATFORM_HAS_BUTTON    1
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name SPI (SSI0) configuration
 *
 * These values configure which CC2538 pins to use for the SPI (SSI0) lines,
 * TX -> MOSI, RX -> MISO
 * @{
 */
#define SPI0_CLK_PORT             GPIO_B_NUM
#define SPI0_CLK_PIN              2
#define SPI0_TX_PORT              GPIO_B_NUM
#define SPI0_TX_PIN               1
#define SPI0_RX_PORT              GPIO_B_NUM
#define SPI0_RX_PIN               3
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Ethernet ENC28J60 configuration
 *
 * These values configure the required pins to drive an external Ethernet
 * module.
 * @{
 */
#define ETH_SPI_INSTANCE           0
#define ETH_SPI_CLK_PORT           SPI0_CLK_PORT
#define ETH_SPI_CLK_PIN            SPI0_CLK_PIN
#define ETH_SPI_MOSI_PORT          SPI0_TX_PORT
#define ETH_SPI_MOSI_PIN           SPI0_TX_PIN
#define ETH_SPI_MISO_PORT          SPI0_RX_PORT
#define ETH_SPI_MISO_PIN           SPI0_RX_PIN
#define ETH_SPI_CSN_PORT           GPIO_B_NUM
#define ETH_SPI_CSN_PIN            5
#define ETH_INT_PORT               GPIO_D_NUM
#define ETH_INT_PIN                0
#define ETH_RESET_PORT             GPIO_D_NUM
#define ETH_RESET_PIN              1
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "Opilla Microsystem Ethernet ORouter"
/** @} */

#endif /* BOARD_H_ */

/**
 * @}
 * @}
 */

