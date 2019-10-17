/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
 *
 * This file is part of the Contiki operating system.
 *
 */
/* -------------------------------------------------------------------------- */
/**
 * \addtogroup openmote-boards
 * @{
 *
 * \defgroup openmote-b OpenMote-B
 * OpenMote-B is a new board by OpenMote Technologies, released in 2018.
 * @{
 *
 * \file
 * This file provides connectivity information on LEDs, Buttons, UART and
 * other OpenMote-B peripherals.
 *
 * This file can be used as the basis to configure other platforms using the
 * cc2538 SoC.
 *
 * \note   Do not include this file directly. It gets included by contiki-conf
 *         after all relevant directives have been set.
 */

#ifndef BOARD_H_
#define BOARD_H_
/*---------------------------------------------------------------------------*/
#include "dev/gpio.h"
#include "dev/nvic.h"
/*---------------------------------------------------------------------------*/
/**
 * \name Serial Boot Loader Backdoor configuration
 *
 * @{
 */
#ifndef FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR_PORT_A_PIN 7 /**< Pin PA7 activates the boot loader */
#endif

#ifndef FLASH_CCA_CONF_BOOTLDR_BACKDOOR_ACTIVE_HIGH
#define FLASH_CCA_CONF_BOOTLDR_BACKDOOR_ACTIVE_HIGH 0 /**< A logic low level activates the boot loader */
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/** \name OpenMote-B LED configuration
 *
 * LEDs on the OpenMote-B are connected as follows:
 * - LED1 (Red)    -> PC4
 * - LED2 (Yellow) -> PC6
 * - LED3 (Green)  -> PC7
 * - LED4 (Orange) -> PC5
 *
 * @{
 */
/*---------------------------------------------------------------------------*/
#define LEDS_ARCH_L1_PORT GPIO_C_NUM
#define LEDS_ARCH_L1_PIN  4
#define LEDS_ARCH_L2_PORT GPIO_C_NUM
#define LEDS_ARCH_L2_PIN  6
#define LEDS_ARCH_L3_PORT GPIO_C_NUM
#define LEDS_ARCH_L3_PIN  7
#define LEDS_ARCH_L4_PORT GPIO_C_NUM
#define LEDS_ARCH_L4_PIN  5

#define LEDS_CONF_RED     1
#define LEDS_CONF_YELLOW  2
#define LEDS_CONF_GREEN   4
#define LEDS_CONF_ORANGE  8

#define LEDS_CONF_COUNT   4
/** @} */
/*---------------------------------------------------------------------------*/
/** \name USB configuration
 *
 * The USB pullup is to be enabled by an external resistor, as it is not mapped
 * to a GPIO.
 * @{
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
 * On the OpenMote-B, the UART is connected to the
 * following ports/pins
 * - RX:  PA0
 * - TX:  PA1
 *
 * We configure the port to use UART0.
 * @{
 */
#define UART0_RX_PORT            GPIO_A_NUM
#define UART0_RX_PIN             0
#define UART0_TX_PORT            GPIO_A_NUM
#define UART0_TX_PIN             1
/** @} */
/*---------------------------------------------------------------------------*/
/** \name OpenMote-B Button configuration
 *
 * Buttons on the OpenMote-B are connected as follows:
 * - BUTTON_USER -> PD5
 * @{
 */
/** BUTTON_USER -> PD5 */
#define BUTTON_USER_PORT       GPIO_D_NUM
#define BUTTON_USER_PIN        5
#define BUTTON_USER_VECTOR     GPIO_D_IRQn
/* Notify various examples that we have Buttons */
#define PLATFORM_HAS_BUTTON    1
#define PLATFORM_SUPPORTS_BUTTON_HAL 1
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name SPI (SSI0) configuration
 *
 * These values configure which CC2538 pins to use for the SPI (SSI0) lines.
 * @{
 */
#define SPI_CLK_PORT             GPIO_A_NUM
#define SPI_CLK_PIN              2
#define SPI_MOSI_PORT            GPIO_A_NUM
#define SPI_MOSI_PIN             5
#define SPI_MISO_PORT            GPIO_A_NUM
#define SPI_MISO_PIN             4
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name SPI (SSI1) configuration
 *
 * These values configure which CC2538 pins to use for the SPI (SSI1) lines.
 * The SSI1 is currently not used.
 * @{
 */
#define SPI1_CLK_PORT            GPIO_C_NUM
#define SPI1_CLK_PIN             4
#define SPI1_TX_PORT             GPIO_C_NUM
#define SPI1_TX_PIN              5
#define SPI1_RX_PORT             GPIO_C_NUM
#define SPI1_RX_PIN              6
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name I2C configuration
 *
 * These values configure which CC2538 pins to use for the I2C lines.
 * @{
 */
#define I2C_SCL_PORT             GPIO_B_NUM
#define I2C_SCL_PIN              5
#define I2C_SDA_PORT             GPIO_B_NUM
#define I2C_SDA_PIN              4
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name OpenMote-B antenna switch configuration
 *
 * @{
 */
#define ANTENNA_BSP_RADIO_BASE   GPIO_PORT_TO_BASE(GPIO_D_NUM)
#define ANTENNA_BSP_RADIO_INT    GPIO_PIN_MASK(3)
#define ANTENNA_BSP_RADIO_EXT    GPIO_PIN_MASK(4)
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name CC2538 TSCH configuration
 *
 * @{
 */
#define RADIO_PHY_OVERHEAD        CC2538_PHY_OVERHEAD
#define RADIO_BYTE_AIR_TIME       CC2538_BYTE_AIR_TIME
#define RADIO_DELAY_BEFORE_TX     CC2538_DELAY_BEFORE_TX
#define RADIO_DELAY_BEFORE_RX     CC2538_DELAY_BEFORE_RX
#define RADIO_DELAY_BEFORE_DETECT CC2538_DELAY_BEFORE_DETECT
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Device string used on startup
 * @{
 */
#define BOARD_STRING "OpenMote-B"
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* BOARD_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
