/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
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
 * \defgroup zoul Zolertia Zoul core module
 *
 * The Zoul comprises the CC2538SF53 and CC1200 in a single module
 * format, which allows a fast reuse of its core components in different
 * formats and form-factors.
 * @{
 *
 * \file
 *   Main module for the Zolertia Zoul core and based platforms
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/leds.h"
#include "dev/uart.h"
#include "dev/button-sensor.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/cc2538-rf.h"
#include "dev/udma.h"
#include "dev/crypto.h"
#include "dev/rtcc.h"
#include "dev/button-hal.h"
#include "usb/usb-serial.h"
#include "lib/random.h"
#include "lib/sensors.h"
#include "net/netstack.h"
#include "net/mac/framer/frame802154.h"
#include "net/linkaddr.h"
#include "sys/platform.h"
#include "soc.h"
#include "cpu.h"
#include "reg.h"
#include "ieee-addr.h"
#include "lpm.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Zoul"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
/**
 * \brief Board specific iniatialisation
 */
void board_init(void);
/*---------------------------------------------------------------------------*/
static void
fade(leds_mask_t l)
{
  volatile int i;
  int k, j;
  for(k = 0; k < 800; ++k) {
    j = k > 400 ? 800 - k : k;

    leds_on(l);
    for(i = 0; i < j; ++i) {
      __asm("nop");
    }
    leds_off(l);
    for(i = 0; i < 400 - j; ++i) {
      __asm("nop");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
rtc_init(void)
{
#if RTC_CONF_INIT
#if RTC_CONF_SET_FROM_SYS
  char *next;
  simple_td_map td;
#endif

  /* Configure RTC and return structure with all parameters */
  rtcc_init();

#if RTC_CONF_SET_FROM_SYS
#ifndef DATE
#error Could not retrieve date from system
#endif

  /* Alternatively, for test only, undefine DATE and define it on your own as:
   * #define DATE "07 06 12 15 16 00 00"
   * Also note that if you restart the node at a given time, it will use the
   * already defined DATE, so if you want to update the device date/time you
   * need to reflash the node.
   */

  /* Get the system date in the following format: wd dd mm yy hh mm ss */
  LOG_INFO("Setting RTC from system date: %s\n", DATE);

  /* Configure the RTC with the current values */
  td.weekdays = (uint8_t)strtol(DATE, &next, 10);
  td.day      = (uint8_t)strtol(next, &next, 10);
  td.months   = (uint8_t)strtol(next, &next, 10);
  td.years    = (uint8_t)strtol(next, &next, 10);
  td.hours    = (uint8_t)strtol(next, &next, 10);
  td.minutes  = (uint8_t)strtol(next, &next, 10);
  td.seconds  = (uint8_t)strtol(next, NULL, 10);

  /* Don't care about the milliseconds... */
  td.miliseconds = 0;

  /* This example relies on 24h mode */
  td.mode = RTCC_24H_MODE;

  /*
   * And to simplify the configuration, it relies on the fact that it will be
   * executed in the present century
   */
  td.century = RTCC_CENTURY_20XX;

  /* Set the time and date */
  if(rtcc_set_time_date(&td) == AB08_ERROR) {
    LOG_ERR("Failed to set time and date\n");
  }
#endif
#endif
}
/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint16_t short_addr;
  uint8_t ext_addr[8];

  ieee_addr_cpy_to(ext_addr, 8);

  short_addr = ext_addr[7];
  short_addr |= ext_addr[6] << 8;

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, IEEE802154_DEFAULT_CHANNEL);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, 8);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one(void)
{
  soc_init();

  leds_init();
  fade(LEDS_RED);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two()
{
  /*
   * Character I/O Initialisation.
   * When the UART receives a character it will call serial_line_input_byte to
   * notify the core. The same applies for the USB driver.
   *
   * If slip-arch is also linked in afterwards (e.g. if we are a border router)
   * it will overwrite one of the two peripheral input callbacks. Characters
   * received over the relevant peripheral will be handled by
   * slip_input_byte instead
   */
#if UART_CONF_ENABLE
  uart_init(0);
  uart_init(1);
  uart_set_input(SERIAL_LINE_CONF_UART, serial_line_input_byte);
#endif

#if USB_SERIAL_CONF_ENABLE
  usb_serial_init();
  usb_serial_set_input(serial_line_input_byte);
#endif

  serial_line_init();

  /* Initialise the H/W RNG engine. */
  random_init(0);

  udma_init();

#if CRYPTO_CONF_INIT
  crypto_init();
  crypto_disable();
#endif

  /* Populate linkaddr_node_addr */
  ieee_addr_cpy_to(linkaddr_node_addr.u8, LINKADDR_SIZE);

#if PLATFORM_HAS_BUTTON
  button_hal_init();
#endif

  INTERRUPTS_ENABLE();

  fade(LEDS_BLUE);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three()
{
  LOG_INFO("%s\n", BOARD_STRING);

  set_rf_params();

  board_init();

  rtc_init();

  soc_print_info();

  process_start(&sensors_process, NULL);

  fade(LEDS_GREEN);
}
/*---------------------------------------------------------------------------*/
void
platform_idle()
{
  /* We have serviced all pending events. Enter a Low-Power mode. */
  lpm_enter();
}
/*---------------------------------------------------------------------------*/
unsigned
radio_phy_overhead(void) {
  radio_value_t ret;
  NETSTACK_RADIO.get_value(RADIO_CONST_PHY_OVERHEAD, &ret);
  return (unsigned)ret;
}
/*---------------------------------------------------------------------------*/
unsigned
radio_byte_air_time(void) {
  radio_value_t ret;
  NETSTACK_RADIO.get_value(RADIO_CONST_BYTE_AIR_TIME, &ret);
  return (unsigned)ret;
}
/*---------------------------------------------------------------------------*/
unsigned
radio_delay_before_tx(void) {
  radio_value_t ret;
  NETSTACK_RADIO.get_value(RADIO_CONST_DELAY_BEFORE_TX, &ret);
  return (unsigned)ret;
}
/*---------------------------------------------------------------------------*/
unsigned
radio_delay_before_rx(void) {
  radio_value_t ret;
  NETSTACK_RADIO.get_value(RADIO_CONST_DELAY_BEFORE_RX, &ret);
  return (unsigned)ret;
}
/*---------------------------------------------------------------------------*/
unsigned
radio_delay_before_detect(void) {
  radio_value_t ret;
  NETSTACK_RADIO.get_value(RADIO_CONST_DELAY_BEFORE_DETECT, &ret);
  return (unsigned)ret;
}
/*---------------------------------------------------------------------------*/
uint16_t *
radio_tsch_timeslot_timing(void) {
  uint16_t *ret;
  /* Get and return pointer to TSCH timings in usec */
  NETSTACK_RADIO.get_object(RADIO_CONST_TSCH_TIMING, &ret, sizeof(ret));
  return ret;
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
