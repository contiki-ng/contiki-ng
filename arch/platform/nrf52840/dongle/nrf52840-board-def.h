/*
 * Copyright (c) 2015, Nordic Semiconductor
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
 *
 */

/**
 * \addtogroup platform
 * @{
 *
 * \addtogroup nrf52840dongle nRF52840 Dongle
 * @{
 *
 * \addtogroup nrf52840dongle-platform-conf Platform configuration
 * @{
 * \file
 *         Platform features configuration.
 * \author
 *         Wojciech Bober <wojciech.bober@nordicsemi.no>
 *
 */
#ifndef NRF52840DK_DEF_H_
#define NRF52840DK_DEF_H_

#include "boards.h"

#define PLATFORM_HAS_BATTERY                    0
#define PLATFORM_HAS_RADIO                      0
#define PLATFORM_HAS_TEMPERATURE                0

/**
 * \name Leds configurations
 *
 * On nRF52dk all leds are green.
 *
 * @{
 */
#define PLATFORM_HAS_LEDS                       1
#define LEDS_CONF_LEGACY_API                    1

#define LEDS_1         1
#define LEDS_2         2
#define LEDS_3         4
#define LEDS_4         8

#define LEDS_CONF_YELLOW    LEDS_1
#define LEDS_CONF_RED       LEDS_2
#define LEDS_CONF_GREEN     LEDS_3
#define LEDS_CONF_BLUE      LEDS_4

#define LEDS_CONF_ALL  (LEDS_1 | LEDS_2 | LEDS_3 | LEDS_4)

/**
 * \name Button configurations
 *
 * @{
 */
/* Notify various examples that we have Buttons */
#define PLATFORM_HAS_BUTTON      1

/**
 * \brief nRF52 RTC instance to be used for Contiki clock driver.
 */
#define PLATFORM_RTC_INSTANCE_ID     0

/**
 * \brief nRF52 timer instance to be used for Contiki rtimer driver.
 */
#define PLATFORM_TIMER_INSTANCE_ID   0

/** @} */

/**
 * \name UART0 Pin configurations
 *
 * @{
 */

#define NRF_UART0_TX_PIN 29
#define NRF_UART0_RX_PIN 31

/** @} */
/*---------------------------------------------------------------------------*/
/** @}
 *  @}
 *  @}
 */
#endif /* NRF52840DK_DEF_H_ */
