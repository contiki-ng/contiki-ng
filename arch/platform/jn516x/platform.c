/*
 * Copyright (c) 2014, SICS Swedish ICT.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * This file is part of the Contiki OS
 *
 */

/**
 * \file
 *         Contiki main for NXP JN516X platform
 *
 * \author
 *         Beshr Al Nahas <beshr@sics.se>
 *         Atis Elsts <atis.elsts@sics.se>
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "dev/watchdog.h"
#include <AppHardwareApi.h>
#include <BbcAndPhyRegs.h>
#include <recal.h>
#include "dev/uart0.h"
#include "dev/uart-driver.h"

#include "contiki.h"
#include "sys/energest.h"
#include "net/netstack.h"

#include "dev/serial-line.h"

#include "dev/leds.h"

#include "lib/random.h"
#include "sys/node-id.h"
#include "sys/platform.h"
#include "rtimer-arch.h"

#if NETSTACK_CONF_WITH_IPV6
#include "net/ipv6/uip-ds6.h"
#endif /* NETSTACK_CONF_WITH_IPV6 */

#include "dev/micromac-radio.h"
#include "MMAC.h"
/* Includes depending on connected sensor boards */
#if SENSOR_BOARD_DR1175
#include "light-sensor.h"
#include "ht-sensor.h"
SENSORS(&light_sensor, &ht_sensor);
#elif SENSOR_BOARD_DR1199
#include "button-sensor.h"
#include "pot-sensor.h"
SENSORS(&pot_sensor, &button_sensor);
#else
#include "dev/button-sensor.h"
/* #include "dev/pir-sensor.h" */
/* #include "dev/vib-sensor.h" */
/* &pir_sensor, &vib_sensor */
SENSORS(&button_sensor);
#endif
unsigned char node_mac[8];

/* Symbol defined by the linker script
 * marks the end of the stack taking into account the used heap  */
extern uint32_t heap_location;

#ifdef EXPERIMENT_SETUP
#include "experiment-setup.h"
#endif

/* _EXTRA_LPM is the sleep mode, _LPM is the doze mode */
#define ENERGEST_TYPE_EXTRA_LPM ENERGEST_TYPE_LPM

extern int main(void);

#if DCOSYNCH_CONF_ENABLED
static unsigned long last_dco_calibration_time;
#endif
static uint64_t sleep_start;
static uint32_t sleep_start_ticks;
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "JN516x"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
/* Reads MAC from SoC
 * Must be called before network addresses initialization */
static void
init_node_mac(void)
{
  tuAddr psExtAddress;
  vMMAC_GetMacAddress(&psExtAddress.sExt);
  node_mac[7] = psExtAddress.sExt.u32L;
  node_mac[6] = psExtAddress.sExt.u32L >> (uint32_t)8;
  node_mac[5] = psExtAddress.sExt.u32L >> (uint32_t)16;
  node_mac[4] = psExtAddress.sExt.u32L >> (uint32_t)24;
  node_mac[3] = psExtAddress.sExt.u32H;
  node_mac[2] = psExtAddress.sExt.u32H >> (uint32_t)8;
  node_mac[1] = psExtAddress.sExt.u32H >> (uint32_t)16;
  node_mac[0] = psExtAddress.sExt.u32H >> (uint32_t)24;
}
/*---------------------------------------------------------------------------*/
static void
set_linkaddr(void)
{
  linkaddr_t addr;
  memset(&addr, 0, LINKADDR_SIZE);

#if NETSTACK_CONF_WITH_IPV6
  memcpy(addr.u8, node_mac, sizeof(addr.u8));
#else
  int i;
  for(i = 0; i < LINKADDR_SIZE; ++i) {
    addr.u8[i] = node_mac[LINKADDR_SIZE - 1 - i];
  }
#endif
  linkaddr_set_node_addr(&addr);
}
/*---------------------------------------------------------------------------*/
bool_t
xosc_init(void)
{
  /* The internal 32kHz RC oscillator is used by default;
   * Initialize and enable the external 32.768kHz crystal.
   */
  vAHI_Init32KhzXtal();
  /* Switch to the 32.768kHz crystal.
   * This will block and wait up to 1 sec for it to stabilize. */
  return bAHI_Set32KhzClockMode(E_AHI_XTAL);
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_one(void)
{
  /* Set stack overflow address for detecting overflow in runtime */
  vAHI_SetStackOverflow(TRUE, ((uint32_t *)&heap_location)[0]);

  /* Initialize random with a seed from the SoC random generator.
   * This must be done before selecting the high-precision external oscillator.
   */
  vAHI_StartRandomNumberGenerator(E_AHI_RND_SINGLE_SHOT, E_AHI_INTS_DISABLED);
  random_init(u16AHI_ReadRandomNumber());

  clock_init();
  rtimer_init();

#if JN516X_EXTERNAL_CRYSTAL_OSCILLATOR
  /* initialize the 32kHz crystal and wait for ready */
  xosc_init();
  /* need to reinitialize because the wait-for-ready process uses system timers */
  clock_init();
  rtimer_init();
#endif

  leds_init();
  leds_on(LEDS_ALL);
  init_node_mac();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_two(void)
{
  uart0_init(UART_BAUD_RATE); /* Must come before first PRINTF */

  /* check for reset source */
  if(bAHI_WatchdogResetEvent()) {
    LOG_INFO("Init: Watchdog timer has reset device!\r\n");
  }
  set_linkaddr();
}
/*---------------------------------------------------------------------------*/
void
platform_init_stage_three(void)
{
#ifndef UIP_FALLBACK_INTERFACE
  uart0_set_input(serial_line_input_byte);
  serial_line_init();
#endif /* UIP_FALLBACK_INTERFACE */

#if TIMESYNCH_CONF_ENABLED
  timesynch_init();
  timesynch_set_authority_level((linkaddr_node_addr.u8[0] << 4) + 16);
#endif /* TIMESYNCH_CONF_ENABLED */

  /* need this to reliably generate the first rtimer callback and callbacks in other
     auto-start processes */
  (void)u32AHI_Init();

  leds_off(LEDS_ALL);
}
/*---------------------------------------------------------------------------*/
void
platform_idle()
{
  clock_time_t time_to_etimer;
  rtimer_clock_t ticks_to_rtimer;

#if DCOSYNCH_CONF_ENABLED
  /* Calibrate the DCO every DCOSYNCH_PERIOD
   * if we have more than 500uSec until next rtimer
   * PS: Calibration disables interrupts and blocks for 200uSec.
   *  */
  if(clock_seconds() - last_dco_calibration_time > DCOSYNCH_PERIOD) {
    if(rtimer_arch_time_to_rtimer() > RTIMER_SECOND / 2000) {
      /* PRINTF("ContikiMain: Calibrating the DCO\n"); */
      eAHI_AttemptCalibration();
      /* Patch to allow CpuDoze after calibration */
      vREG_PhyWrite(REG_PHY_IS, REG_PHY_INT_VCO_CAL_MASK);
      last_dco_calibration_time = clock_seconds();
    }
  }
#endif /* DCOSYNCH_CONF_ENABLED */

  /* flush standard output before sleeping */
  uart_driver_flush(E_AHI_UART_0, TRUE, FALSE);

  /* calculate the time to the next etimer and rtimer */
  time_to_etimer = clock_arch_time_to_etimer();
  ticks_to_rtimer = rtimer_arch_time_to_rtimer();

#if JN516X_SLEEP_ENABLED
  /* we can sleep only up to the next rtimer/etimer */
  rtimer_clock_t max_sleep_time = ticks_to_rtimer;
  if(max_sleep_time >= JN516X_MIN_SLEEP_TIME) {
    /* also take into account etimers */
    uint64_t ticks_to_etimer = ((uint64_t)time_to_etimer * RTIMER_SECOND) / CLOCK_SECOND;
    max_sleep_time = MIN(ticks_to_etimer, ticks_to_rtimer);
  }

  if(max_sleep_time >= JN516X_MIN_SLEEP_TIME) {
    max_sleep_time -= JN516X_SLEEP_GUARD_TIME;
    /* bound the sleep time to 1 second */
    max_sleep_time = MIN(max_sleep_time, JN516X_MAX_SLEEP_TIME);

#if !RTIMER_USE_32KHZ
    /* convert to 32.768 kHz oscillator ticks */
    max_sleep_time = (uint64_t)max_sleep_time * JN516X_XOSC_SECOND / RTIMER_SECOND;
#endif
    vAHI_WakeTimerEnable(WAKEUP_TIMER, TRUE);
    /* sync with the tick timer */
    WAIT_FOR_EDGE(sleep_start);
    sleep_start_ticks = u32AHI_TickTimerRead();

    vAHI_WakeTimerStartLarge(WAKEUP_TIMER, max_sleep_time);
    ENERGEST_SWITCH(ENERGEST_TYPE_CPU, ENERGEST_TYPE_EXTRA_LPM);
    vAHI_Sleep(E_AHI_SLEEP_OSCON_RAMON);
  } else {
#else
    {
#endif /* JN516X_SLEEP_ENABLED */
      clock_arch_schedule_interrupt(time_to_etimer, ticks_to_rtimer);
      ENERGEST_SWITCH(ENERGEST_TYPE_CPU, ENERGEST_TYPE_LPM);
      vAHI_CpuDoze();
      watchdog_start();
      ENERGEST_SWITCH(ENERGEST_TYPE_LPM, ENERGEST_TYPE_CPU);
    }
}
/*---------------------------------------------------------------------------*/
void
platform_main_loop(void)
{
  int r;

  while(1) {
    do {
      /* Reset watchdog. */
      watchdog_periodic();
      r = process_run();
    } while(r > 0);
    /*
     * Idle processing.
     */
    platform_idle();
  }
}
/*---------------------------------------------------------------------------*/
void
AppColdStart(void)
{
  /* After reset or sleep with memory off */
  main();
}
/*---------------------------------------------------------------------------*/
void
AppWarmStart(void)
{
  /* Wakeup after sleep with memory on.
   * Need to initialize devices but not the application state.
   * Note: the actual time this function is called is
   * ~8 ticks (32kHz timer) later than the scheduled sleep end time.
   */
  uint32_t sleep_ticks;
  uint64_t sleep_end;
  rtimer_clock_t sleep_ticks_rtimer;

  clock_arch_calibrate();
  leds_init();
  uart0_init(UART_BAUD_RATE); /* Must come before first PRINTF */
  NETSTACK_RADIO.init();
  watchdog_init();
  watchdog_stop();

  WAIT_FOR_EDGE(sleep_end);
  sleep_ticks = (uint32_t)(sleep_start - sleep_end) + 1;

#if RTIMER_USE_32KHZ
  sleep_ticks_rtimer = sleep_ticks;
#else
  {
    static uint32_t remainder;
    uint64_t t = (uint64_t)sleep_ticks * RTIMER_SECOND + remainder;
    sleep_ticks_rtimer = (uint32_t)(t / JN516X_XOSC_SECOND);
    remainder = t - sleep_ticks_rtimer * JN516X_XOSC_SECOND;
  }
#endif

  /* reinitialize rtimers */
  rtimer_arch_reinit(sleep_start_ticks, sleep_ticks_rtimer);

  ENERGEST_SWITCH(ENERGEST_TYPE_EXTRA_LPM, ENERGEST_TYPE_CPU);

  watchdog_start();

  /* reinitialize clock */
  clock_arch_init(1);
  /* schedule etimer interrupt */
  clock_arch_schedule_interrupt(clock_arch_time_to_etimer(), rtimer_arch_time_to_rtimer());

#if DCOSYNCH_CONF_ENABLED
  /* The radio is recalibrated on wakeup */
  last_dco_calibration_time = clock_seconds();
#endif

  platform_main_loop();
}
/*---------------------------------------------------------------------------*/
