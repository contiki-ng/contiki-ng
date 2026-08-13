/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "contiki.h"
#include "sys/clock.h"
#include "nrf.h"
#include <stdint.h>

/* GRTC runs at 1 MHz on nRF54L15 (matches the M33 clock-arch
 * GRTC_TICK_FREQUENCY_HZ). 1 GRTC tick = 1 us. */
#define GRTC_TICK_HZ  1000000UL

/* The HAL exposes GRTC_SYSCOUNTER = SYSCOUNTER[NRF_GRTC_DOMAIN_INDEX], where
 * NRF_GRTC_DOMAIN_INDEX = GRTC_IRQ_GROUP. With NRF_FLPR defined,
 * GRTC_IRQ_GROUP = 0, so we read SYSCOUNTER[0] - the FLPR's dedicated read
 * port. The underlying 52-bit counter is shared with the M33. */

static inline uint64_t
grtc_syscounter_now(void)
{
  uint32_t lo, hi;
  do {
    lo = NRF_GRTC_S->SYSCOUNTER[0].SYSCOUNTERL;
    hi = NRF_GRTC_S->SYSCOUNTER[0].SYSCOUNTERH;
  } while(hi & GRTC_SYSCOUNTER_SYSCOUNTERH_BUSY_Msk);
  return ((uint64_t)(hi & GRTC_SYSCOUNTER_SYSCOUNTERH_VALUE_Msk) << 32) | lo;
}

void
clock_init(void)
{
  /* GRTC is already running - the M33 application brings it up before
   * releasing the VPR. Nothing to do here. */
}

clock_time_t
clock_time(void)
{
  return (clock_time_t)(grtc_syscounter_now() / (GRTC_TICK_HZ / CLOCK_SECOND));
}

unsigned long
clock_seconds(void)
{
  return (unsigned long)(grtc_syscounter_now() / GRTC_TICK_HZ);
}

void
clock_wait(clock_time_t t)
{
  clock_time_t end = clock_time() + t;
  while(clock_time() < end) { }
}

void
clock_delay_usec(uint16_t us)
{
  uint64_t end = grtc_syscounter_now() + us;
  while(grtc_syscounter_now() < end) { }
}

void
clock_delay(unsigned int us)
{
  clock_delay_usec((uint16_t)us);
}
