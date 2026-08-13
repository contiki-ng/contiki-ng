/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Board definition header for the Seeed Studio XIAO nRF54L15.
 * Mirrors the structure used by other nRF boards.
 */
#ifndef XIAO_NRF54L15_DEF_H_
#define XIAO_NRF54L15_DEF_H_

#define XIAO_NRF54L15_LED_PORT       2
#define XIAO_NRF54L15_LED_PIN        0

#define XIAO_NRF54L15_RF_SW_PWR_PORT 2
#define XIAO_NRF54L15_RF_SW_PWR_PIN  3
#define XIAO_NRF54L15_RF_SW_SEL_PORT 2
#define XIAO_NRF54L15_RF_SW_SEL_PIN  5

#define NRF_LED1_PORT XIAO_NRF54L15_LED_PORT
#define NRF_LED1_PIN  XIAO_NRF54L15_LED_PIN
#define LEDS_CONF_COUNT 1
#define LEDS_CONF_RED   1

#define XIAO_NRF54L15_BUTTON_PORT    0
#define XIAO_NRF54L15_BUTTON_PIN     0

/*
 * The onboard user button is exposed through button-hal (board-buttons.c).
 * PLATFORM_SUPPORTS_BUTTON_HAL matters as well: code that consults it
 * otherwise selects the legacy button_sensor API, which this board does not
 * implement.
 */
#define PLATFORM_HAS_BUTTON          1
#define PLATFORM_SUPPORTS_BUTTON_HAL 1

#define XIAO_NRF54L15_UART_INSTANCE  20 /* UART20 */
#define XIAO_NRF54L15_UART_TX_PORT   1
#define XIAO_NRF54L15_UART_TX_PIN    9
#define XIAO_NRF54L15_UART_RX_PORT   1
#define XIAO_NRF54L15_UART_RX_PIN    8

#define NRF_UARTE_INSTANCE_ID XIAO_NRF54L15_UART_INSTANCE
#define NRF_UARTE_TX_PORT     XIAO_NRF54L15_UART_TX_PORT
#define NRF_UARTE_TX_PIN      XIAO_NRF54L15_UART_TX_PIN
#define NRF_UARTE_RX_PORT     XIAO_NRF54L15_UART_RX_PORT
#define NRF_UARTE_RX_PIN      XIAO_NRF54L15_UART_RX_PIN

#endif /* XIAO_NRF54L15_DEF_H_ */
