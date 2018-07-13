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
/*---------------------------------------------------------------------------*/
/* Simplelink SDK includes */
#include <Board.h>
#include <NoRTOS.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)
#include DeviceFamily_constructPath(inc/hw_cpu_scs.h)

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/UART.h>
/*---------------------------------------------------------------------------*/
/* Contiki API */
#include "contiki.h"
#include "contiki-net.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "sys/node-id.h"
#include "sys/platform.h"
#include "dev/button-hal.h"
#include "dev/gpio-hal.h"
#include "dev/serial-line.h"
#include "dev/leds.h"
#include "net/mac/framer/frame802154.h"
#include "lib/sensors.h"
/*---------------------------------------------------------------------------*/
/* Arch driver implementations */
#include "board-peripherals.h"
#include "uart0-arch.h"
/*---------------------------------------------------------------------------*/
#include "ieee-addr.h"
#include "rf-core.h"
#include "rf-ble-beacond.h"
#include "lib/random.h"
#include "button-sensor.h"
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CC26xx/CC13xx"
#define LOG_LEVEL LOG_LEVEL_DBG
/*---------------------------------------------------------------------------*/
unsigned short g_nodeId = 0;
/*---------------------------------------------------------------------------*/
/*
 *  Board-specific initialization function.
 *  This function is defined in the file <BOARD>_fxns.c
 */
extern void Board_initHook(void);
/*---------------------------------------------------------------------------*/
#ifdef BOARD_CONF_HAS_SENSORS
#define BOARD_HAS_SENSORS   BOARD_CONF_HAS_SENSORS
#else
#define BOARD_HAS_SENSORS   1
#endif
/*---------------------------------------------------------------------------*/
static void
fade(unsigned char l)
{
  volatile int i;
  for(int k = 0; k < 800; ++k) {
    int j = k > 400 ? 800 - k : k;

    leds_on(l);
    for(i = 0; i < j; ++i) { __asm("nop"); }
    leds_off(l);
    for(i = 0; i < 400 - j; ++i) { __asm("nop"); }
  }
}
/*---------------------------------------------------------------------------*/
static void
set_rf_params(void)
{
  uint8_t ext_addr[8];
  memset(ext_addr, 0x0, sizeof(ext_addr));

  ieee_addr_cpy_to(ext_addr, sizeof(ext_addr));

  uint16_t short_addr = (ext_addr[7] << 0)
                      | (ext_addr[6] << 8);

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, sizeof(ext_addr));

  /* also set the global node id */
  g_nodeId = short_addr;
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one(void)
{
  DRIVERLIB_ASSERT_CURR_RELEASE();

  /* Enable flash cache */
  VIMSModeSet(VIMS_BASE, VIMS_MODE_ENABLED);
  /* Configure round robin arbitration and prefetching */
  VIMSConfigure(VIMS_BASE, true, true);

  Power_init();

  if (PIN_init(BoardGpioInitTable) != PIN_SUCCESS) {
    /* Error with PIN_init */
    while (1);
  }

  /* Perform board-specific initialization */
  Board_initHook();

  /* Contiki drivers init */
  gpio_hal_init();
  leds_init();

  fade(LEDS_RED);

  /* TI Drivers init */
#if TI_UART_CONF_ENABLE
  UART_init();
  uart0_init();
#endif
#if TI_I2C_CONF_ENABLE
  I2C_init();
#endif
#if TI_SPI_CONF_ENABLE
  SPI_init();
#endif
#if TI_NVS_CONF_ENABLE
  NVS_init();
#endif

  fade(LEDS_GREEN);

  /* NoRTOS should be called last */
  NoRTOS_start();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two(void)
{
  serial_line_init();

#if BUILD_WITH_SHELL
  uart0_set_callback(serial_line_input_byte);
#endif

  random_init(0x1234);

  /* Populate linkaddr_node_addr */
  ieee_addr_cpy_to(linkaddr_node_addr.u8, LINKADDR_SIZE);

  button_hal_init();

  fade(LEDS_RED);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three(void)
{
#if RF_BLE_BEACON_ENABLE
  rf_ble_beacond_init();
#endif

  radio_value_t chan = 0, pan = 0;

  set_rf_params();

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &pan);

  LOG_DBG("With DriverLib v%u.%u\n", DRIVERLIB_RELEASE_GROUP,
          DRIVERLIB_RELEASE_BUILD);
  LOG_DBG("IEEE 802.15.4: %s, Sub-GHz: %s, BLE: %s\n",
          ChipInfo_SupportsIEEE_802_15_4() ? "Yes" : "No",
          ChipInfo_SupportsPROPRIETARY() ? "Yes" : "No",
          ChipInfo_SupportsBLE() ? "Yes" : "No");

#if (RF_MODE == RF_MODE_SUB_1_GHZ)
  LOG_INFO("Operating frequency on Sub-1 GHz\n");
#else
  LOG_INFO("Operating frequency on 2.4 GHz\n");
#endif
  LOG_INFO("RF: Channel %d, PANID 0x%04X\n", chan, pan);
  LOG_INFO("Node ID: %d\n", g_nodeId);

#if BOARD_HAS_SENSORS
  process_start(&sensors_process, NULL);
#endif

  fade(LEDS_GREEN);
}
/*---------------------------------------------------------------------------*/
void
platform_idle(void)
{
  /* Drop to some low power mode */
  Power_idleFunc();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
