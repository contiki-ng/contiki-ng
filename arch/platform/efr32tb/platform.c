/*
 * Copyright (c) 2018, RISE SICS AB.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "contiki.h"
#include "dev/button-hal.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include "dev/debug-uart.h"
#include "lib/sensors.h"
#include "net/netstack.h"
#include "net/mac/framer/frame802154.h"
#include <em_device.h>
#include <em_chip.h>
#include <em_cmu.h>
#include <em_emu.h>
#include <em_usart.h>
#include <em_system.h>

#include <stdint.h>
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "EFR32TB"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
/**
 * \brief Board specific initialisation
 */
void board_init(void);
/*---------------------------------------------------------------------------*/
static inline uint64_t
swap_long(uint64_t val)
{
  val = (val & 0x00000000FFFFFFFF) << 32 | (val & 0xFFFFFFFF00000000) >> 32;
  val = (val & 0x0000FFFF0000FFFF) << 16 | (val & 0xFFFF0000FFFF0000) >> 16;
  val = (val & 0x00FF00FF00FF00FF) << 8  | (val & 0xFF00FF00FF00FF00) >> 8;
  return val;
}
/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint16_t short_addr;

  short_addr = (linkaddr_node_addr.u8[6] << 8) +
    linkaddr_node_addr.u8[7];

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, IEEE802154_DEFAULT_CHANNEL);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, linkaddr_node_addr.u8, 8);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one(void)
{
  CHIP_Init();

  CMU_HFXOInit_TypeDef hfxoInit = CMU_HFXOINIT_DEFAULT;
  EMU_DCDCInit_TypeDef dcdcInit = EMU_DCDCINIT_DEFAULT;

  CMU_HFXOInit(&hfxoInit);
  EMU_DCDCInit(&dcdcInit);

  CMU_ClockSelectSet(cmuClock_HF, cmuSelect_HFXO);
  CMU_OscillatorEnable(cmuOsc_HFRCO, false, false);


  /* enable low energy domain clock (required for watchdog */
  CMU_ClockEnable(cmuClock_HFLE, true);

  /* enable rtcc timer clock */
  CMU_ClockSelectSet(cmuClock_RTCC, cmuSelect_ULFRCO);
  CMU_ClockSelectSet(cmuClock_LFE, cmuSelect_ULFRCO);
  CMU_ClockEnable(cmuClock_LFE, true);
  CMU_ClockEnable(cmuClock_RTCC, true);

  /* enable CRYPTO clock */
  CMU_ClockEnable(cmuClock_CRYPTO, true);

  /* enable LETIMER0 clock */
#ifdef cmuSelect_PLFRCO
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_PLFRCO);
  CMU_ClockSelectSet(cmuClock_LETIMER0, cmuSelect_PLFRCO);
  CMU_OscillatorEnable(cmuOsc_PLFRCO, true, true);
#else
  CMU_ClockSelectSet(cmuClock_LFA, cmuSelect_ULFRCO);
  CMU_ClockSelectSet(cmuClock_LETIMER0, cmuSelect_ULFRCO);
  CMU_OscillatorEnable(cmuOsc_ULFRCO, true, true);
#endif

  CMU_ClockDivSet(cmuClock_LFA, cmuClkDiv_32);
  CMU_ClockDivSet(cmuClock_LETIMER0, cmuClkDiv_32);
  CMU_ClockEnable(cmuClock_LETIMER0, true);

}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two()
{
  uint64_t uid = SYSTEM_GetUnique();
  uint64_t uid_net_order = swap_long(uid);

  debug_uart_init();

  gpio_hal_init();

  leds_init();
  leds_on(LEDS_RED);

  /* linkaddr configuration */
  memcpy(linkaddr_node_addr.u8, &uid_net_order, LINKADDR_SIZE);

  debug_uart_set_input_handler(serial_line_input_byte);
  serial_line_init();

#if PLATFORM_HAS_BUTTON
  button_hal_init();
#endif

  leds_off(LEDS_RED);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three()
{
  LOG_INFO("%s\n", BOARD_STRING);

  set_rf_params();

#if BOARD_HAS_SENSORS
  process_start(&sensors_process, NULL);
#endif

  board_init();
}
/*---------------------------------------------------------------------------*/
void
platform_idle()
{
  /* We have serviced all pending events. Enter a Low-Power mode. */
  EMU_EnterEM1();
}
/*---------------------------------------------------------------------------*/
