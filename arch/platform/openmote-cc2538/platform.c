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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup cc2538-platforms
 * @{
 *
 * \defgroup openmote-cc2538 OpenMote-CC2538 platform
 *
 * The OpenMote-CC2538 is based on the CC2538, the new platform by Texas Instruments
 * based on an ARM Cortex-M3 core and a IEEE 802.15.4 radio.
 * @{
 *
 * \file
 * Main module for the OpenMote-CC2538 platform
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/leds.h"
#include "dev/uart.h"
#include "dev/i2c.h"
#include "dev/button-sensor.h"
#include "dev/serial-line.h"
#include "dev/slip.h"
#include "dev/cc2538-rf.h"
#include "dev/udma.h"
#include "dev/crypto.h"
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
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "OpenMote CC2538"
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
#if UART_CONF_ENABLE
  uart_init(0);
  uart_init(1);
  uart_set_input(SERIAL_LINE_CONF_UART, serial_line_input_byte);
#endif

#if USB_SERIAL_CONF_ENABLE
  usb_serial_init();
  usb_serial_set_input(serial_line_input_byte);
#endif

  i2c_init(I2C_SDA_PORT, I2C_SDA_PIN, I2C_SCL_PORT, I2C_SCL_PIN, I2C_SCL_NORMAL_BUS_SPEED);

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

  button_hal_init();

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
/**
 * @}
 * @}
 */
