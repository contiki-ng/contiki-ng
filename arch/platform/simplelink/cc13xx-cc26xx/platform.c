/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
/**
 * \addtogroup cc13xx-cc26xx-platform
 * @{
 *
 * \file
 *        Setup the SimpleLink CC13xx/CC26xx ecosystem with the
 *        Contiki environment.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "contiki-net.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "sys/node-id.h"
#include "sys/platform.h"
#include "sys/energest.h"
#include "dev/button-hal.h"
#include "dev/gpio-hal.h"
#include "dev/serial-line.h"
#include "dev/leds.h"
#include "dev/watchdog.h"
#include "net/mac/framer/frame802154.h"
#include "lib/random.h"
#include "lib/sensors.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>
#include <NoRTOS.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/driverlib_release.h)
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/vims.h)

#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/NVS.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/drivers/Power.h>
#include <ti/drivers/SPI.h>
#include <ti/drivers/TRNG.h>
#include <ti/drivers/UART.h>
/*---------------------------------------------------------------------------*/
#include "board-peripherals.h"
#include "clock-arch.h"
#include "uart0-arch.h"
#include "trng-arch.h"
/*---------------------------------------------------------------------------*/
#include "rf/rf.h"
#include "rf/ble-beacond.h"
#include "rf/ieee-addr.h"
/*---------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CC13xx/CC26xx"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
/*
 * Board-specific initialization function. This function is defined in
 * the <BOARD>_fxns.c file.
 */
extern void Board_initHook(void);
/*---------------------------------------------------------------------------*/
/*
 * \brief  Fade a specified LED.
 */
static void
fade(PIN_Id pin)
{
  volatile uint32_t i;
  uint32_t k;
  uint32_t j;
  uint32_t pivot = 800;
  uint32_t pivot_half = pivot / 2;

  for(k = 0; k < pivot; ++k) {
    j = (k > pivot_half) ? pivot - k : k;

    PINCC26XX_setOutputValue(pin, 1);
    for(i = 0; i < j; ++i) {
      __asm__ __volatile__ ("nop");
    }
    PINCC26XX_setOutputValue(pin, 0);
    for(i = 0; i < pivot_half - j; ++i) {
      __asm__ __volatile__ ("nop");
    }
  }
}
/*---------------------------------------------------------------------------*/
/*
 * \brief  Configure RF params for the radio driver.
 */
static void
set_rf_params(void)
{
  uint8_t ext_addr[8];
  uint16_t short_addr;

  memset(ext_addr, 0x0, sizeof(ext_addr));

  ieee_addr_cpy_to(ext_addr, sizeof(ext_addr));

  /* Short address is the last two bytes of the MAC address */
  short_addr = (((uint16_t)ext_addr[7] << 0) |
                ((uint16_t)ext_addr[6] << 8));

  NETSTACK_RADIO.set_value(RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  NETSTACK_RADIO.set_value(RADIO_PARAM_16BIT_ADDR, short_addr);
  NETSTACK_RADIO.set_object(RADIO_PARAM_64BIT_ADDR, ext_addr, sizeof(ext_addr));
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

  /* BoardGpioInitTable declared in Board.h */
  if(PIN_init(BoardGpioInitTable) != PIN_SUCCESS) {
    /*
     * Something is seriously wrong if PIN initialization of the Board GPIO
     * table fails.
     */
    for(;;) { /* hang */ }
  }

  /* Perform board-specific initialization */
  Board_initHook();

  /* Contiki drivers init */
  gpio_hal_init();
  leds_init();

  fade(Board_PIN_LED0);

  /* TI Drivers init */
#if TI_UART_CONF_ENABLE
  UART_init();
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

  TRNG_init();

  fade(Board_PIN_LED1);

  /* NoRTOS must be called last */
  NoRTOS_start();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two(void)
{
  serial_line_init();

#if TI_UART_CONF_UART0_ENABLE
  uart0_init();
#endif

#if BUILD_WITH_SHELL
  uart0_set_callback(serial_line_input_byte);
#endif

  /* Use TRNG to seed PRNG. If TRNG fails, use a hard-coded seed. */
  unsigned short seed = 0;
  if(!trng_rand((uint8_t *)&seed, sizeof(seed), TRNG_WAIT_FOREVER)) {
    /* Default to some hard-coded seed. */
    seed = 0x1234;
  }
  random_init(seed);

  /* Populate linkaddr_node_addr */
  ieee_addr_cpy_to(linkaddr_node_addr.u8, LINKADDR_SIZE);

  button_hal_init();

  fade(Board_PIN_LED0);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three(void)
{
#if RF_CONF_BLE_BEACON_ENABLE
  rf_ble_beacond_init();
#endif

  radio_value_t chan = 0;
  radio_value_t pan = 0;

  set_rf_params();

  LOG_DBG("With DriverLib v%u.%u\n", DRIVERLIB_RELEASE_GROUP,
          DRIVERLIB_RELEASE_BUILD);
  LOG_DBG("IEEE 802.15.4: %s, Sub-1 GHz: %s, BLE: %s\n",
          ChipInfo_SupportsIEEE_802_15_4() ? "Yes" : "No",
          ChipInfo_SupportsPROPRIETARY() ? "Yes" : "No",
          ChipInfo_SupportsBLE() ? "Yes" : "No");

#if (RF_MODE == RF_MODE_SUB_1_GHZ)
  LOG_INFO("Operating frequency on Sub-1 GHz\n");
#elif (RF_MODE == RF_MODE_2_4_GHZ)
  LOG_INFO("Operating frequency on 2.4 GHz\n");
#endif

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &chan);
  LOG_INFO("RF: Channel %d", chan);

  if(NETSTACK_RADIO.get_value(RADIO_PARAM_PAN_ID, &pan) == RADIO_RESULT_OK) {
    LOG_INFO_(", PANID 0x%04X", pan);
  }
  LOG_INFO_("\n");

  LOG_INFO("Node ID: %d\n", node_id);

#if BOARD_CONF_SENSORS_ENABLE
  process_start(&sensors_process, NULL);
#endif

  fade(Board_PIN_LED1);
}
/*---------------------------------------------------------------------------*/
void
platform_idle(void)
{
  /* Clear the Watchdog before we potentially go to some low power mode */
  watchdog_periodic();
  /*
   * Arm the wakeup clock. If it returns false, some timers already expired
   * and we shouldn't go to low-power yet.
   */
  if(clock_arch_enter_idle()) {
    /* Drop to some low power mode */
    ENERGEST_SWITCH(ENERGEST_TYPE_CPU, ENERGEST_TYPE_LPM);
    Power_idleFunc();
    /*
     * Clear the Watchdog immediately after wakeup, as the wakeup reason could
     * be to clear the watchdog. See the implementation of
     * clock_arch_set_wakeup() for why this might be the case.
     */
    watchdog_periodic();
    ENERGEST_SWITCH(ENERGEST_TYPE_LPM, ENERGEST_TYPE_CPU);
    clock_arch_exit_idle();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
