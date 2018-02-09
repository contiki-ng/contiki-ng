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
 */
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup cc13xx-cc26xx-platforms
 * @{
 *
 * \defgroup LaunchPads
 *
 * This platform supports a number of different boards:
 * - The TI CC1310 LaunchPad
 * - The TI CC1350 LaunchPad
 * - The TI CC2640 LaunchPad
 * - The TI CC2650 LaunchPad
 * - The TI CC1312R1 LaunchPad
 * - The TI CC1352R1 LaunchPad
 * - The TI CC1352P1 LaunchPad
 * - The TI CC26X2R1 LaunchPad
 * @{
 */
#include <Board.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Power.h>
#include <driverlib/driverlib_release.h>
#include <driverlib/chipinfo.h>
#include <NoRTOS.h>

#include "contiki.h"
#include "contiki-net.h"

#include "uart0-arch.h"

#include "leds.h"
//#include "gpio-interrupt.h"
#include "ieee-addr.h"
#include "uart0-arch.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "sys/node-id.h"
#include "sys/platform.h"
#include "lib/random.h"
#include "lib/sensors.h"
#include "button-sensor.h"
#include "dev/serial-line.h"
#include "net/mac/framer/frame802154.h"

//#include "driverlib/driverlib_release.h"

#include <stdio.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CC26xx/CC13xx"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
unsigned short node_id = 0;
/*---------------------------------------------------------------------------*/
/** \brief Board specific initialization */
void board_init(void);
/*---------------------------------------------------------------------------*/
static void
fade(unsigned char l)
{
  volatile int i;
  int k, j;
  for(k = 0; k < 800; ++k) {
    j = k > 400 ? 800 - k : k;

    GPIO_write(l, Board_GPIO_LED_ON);
    for(i = 0; i < j; ++i) {
      __asm("nop");
    }
    GPIO_write(l, Board_GPIO_LED_OFF);
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
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, RF_CORE_CHANNEL);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, 8);

  /* also set the global node id */
  node_id = short_addr;
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one()
{
    Board_initGeneral();
    GPIO_init();

    // Only enables interrupts
    NoRTOS_start();

//  /* Enable flash cache and prefetch. */
//  ti_lib_vims_mode_set(VIMS_BASE, VIMS_MODE_ENABLED);
//  ti_lib_vims_configure(VIMS_BASE, true, true);
//
//  ti_lib_int_master_disable();
//
//  /* Set the LF XOSC as the LF system clock source */
//  oscillators_select_lf_xosc();
//
//  lpm_init();
//
//  board_init();
//
//  gpio_interrupt_init();
//
//  leds_init();
  fade(Board_GPIO_LED0);
//
//  /*
//   * Disable I/O pad sleep mode and open I/O latches in the AON IOC interface
//   * This is only relevant when returning from shutdown (which is what froze
//   * latches in the first place. Before doing these things though, we should
//   * allow software to first regain control of pins
//   */
//  ti_lib_pwr_ctrl_io_freeze_disable();
//
//  ti_lib_int_master_enable();
//
//  soc_rtc_init();
  fade(Board_GPIO_LED1);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two()
{
    uart0_init();

//  random_init(0x1234);
//
//  serial_line_init();
//
  /* Populate linkaddr_node_addr */
  ieee_addr_cpy_to(linkaddr_node_addr.u8, LINKADDR_SIZE);
//
  fade(Board_GPIO_LED0);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three()
{
  radio_value_t chan, pan;

  set_rf_params();

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &pan);

  LOG_DBG("With DriverLib v%u.%u\n", DRIVERLIB_RELEASE_GROUP,
          DRIVERLIB_RELEASE_BUILD);
  //LOG_INFO(BOARD_STRING "\n");
  LOG_DBG("IEEE 802.15.4: %s, Sub-GHz: %s, BLE: %s, Prop: %s\n",
          ChipInfo_SupportsIEEE_802_15_4() ? "Yes" : "No",
          ChipInfo_ChipFamilyIs_CC13x0() ? "Yes" : "No",
          ChipInfo_SupportsBLE() ? "Yes" : "No",
          ChipInfo_SupportsPROPRIETARY() ? "Yes" : "No");
  LOG_INFO(" RF: Channel %d, PANID 0x%04X\n", chan, pan);
  LOG_INFO(" Node ID: %d\n", node_id);
//
//  process_start(&sensors_process, NULL);
  fade(Board_GPIO_LED1);
}
/*---------------------------------------------------------------------------*/
void
platform_idle()
{
  /* Drop to some low power mode */
//  lpm_drop();

    Power_idleFunc();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
