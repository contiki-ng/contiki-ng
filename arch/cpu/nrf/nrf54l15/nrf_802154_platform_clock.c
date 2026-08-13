/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Clock Abstraction Layer for nrf_802154 on nRF54L15.
 *
 * On nRF54L15 the HFXO is started via TASKS_XOSTART (not TASKS_HFCLKSTART
 * as on nRF52).  The PLL must also be started explicitly after the HFXO
 * (MLTPAN-20 workaround) for the radio to function correctly.
 * LFCLK is controlled through the CLOCK peripheral as usual.
 */

#include "platform/nrf_802154_clock.h"
#include "nrf.h"
#include <stdbool.h>

static volatile bool hfclk_running;
static volatile bool lfclk_running;
/*---------------------------------------------------------------------------*/
static void
set_constant_latency(bool enable)
{
#if defined(POWER_TASKS_CONSTLAT_TASKS_CONSTLAT_Msk) && defined(POWER_TASKS_LOWPWR_TASKS_LOWPWR_Msk)
  if(enable) {
    NRF_POWER->TASKS_CONSTLAT = POWER_TASKS_CONSTLAT_TASKS_CONSTLAT_Trigger;
  } else {
    NRF_POWER->TASKS_LOWPWR = POWER_TASKS_LOWPWR_TASKS_LOWPWR_Trigger;
  }
#else
  (void)enable;
#endif
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_clock_init(void)
{
  lfclk_running = true; /* LFCLK already started by clock-arch.c */
  set_constant_latency(false);

  /* Pre-start HFXO + PLL while interrupts are still enabled, so the
   * GRTC clock tick ISR keeps running during the busy-wait. */
  NRF_CLOCK->EVENTS_XOSTARTED = 0;
  NRF_CLOCK->TASKS_XOSTART = 1;
  while(NRF_CLOCK->EVENTS_XOSTARTED == 0) {
  }
  NRF_CLOCK->EVENTS_XOSTARTED = 0;

  NRF_CLOCK->EVENTS_PLLSTARTED = 0;
  NRF_CLOCK->TASKS_PLLSTART = 1;
  while(NRF_CLOCK->EVENTS_PLLSTARTED == 0) {
  }
  NRF_CLOCK->EVENTS_PLLSTARTED = 0;

  hfclk_running = true;
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_clock_deinit(void)
{
  /* Nothing to do. */
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_clock_hfclk_start(void)
{
  /*
   * HFXO+PLL are pre-started in nrf_802154_clock_init() so this is always
   * a fast path.  The radio library calls this from a critical section
   * (interrupts disabled), so we must not busy-wait here.
   */
  if(!hfclk_running) {
    /* Emergency start — should not happen if init was called. */
    NRF_CLOCK->EVENTS_XOSTARTED = 0;
    NRF_CLOCK->TASKS_XOSTART = 1;
    while(NRF_CLOCK->EVENTS_XOSTARTED == 0) {
    }
    NRF_CLOCK->EVENTS_XOSTARTED = 0;

    NRF_CLOCK->EVENTS_PLLSTARTED = 0;
    NRF_CLOCK->TASKS_PLLSTART = 1;
    while(NRF_CLOCK->EVENTS_PLLSTARTED == 0) {
    }
    NRF_CLOCK->EVENTS_PLLSTARTED = 0;

    hfclk_running = true;
  }
  set_constant_latency(true);
  nrf_802154_clock_hfclk_ready();
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_clock_hfclk_stop(void)
{
  /* Keep HFXO+PLL always running — stopping them on nRF54L15 can disrupt
   * the GRTC syscounter that Contiki-NG uses for clock ticks. */
  set_constant_latency(false);
}
/*---------------------------------------------------------------------------*/
bool
nrf_802154_clock_hfclk_is_running(void)
{
  return hfclk_running;
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_clock_lfclk_start(void)
{
  /* LFCLK is already started by clock-arch.c via nrfx_clock_lfclk_start().
   * Do NOT touch the CLOCK peripheral registers again — on nRF54L15 this
   * can interfere with the GRTC syscounter. */
  lfclk_running = true;
  nrf_802154_clock_lfclk_ready();
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_clock_lfclk_stop(void)
{
  /* Never stop LFCLK — GRTC depends on it. */
}
/*---------------------------------------------------------------------------*/
bool
nrf_802154_clock_lfclk_is_running(void)
{
  return lfclk_running;
}
/*---------------------------------------------------------------------------*/
