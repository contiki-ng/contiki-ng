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

/* ---- Interrupt-driven system tick (GRTC compare on the FLPR's group) ----
 * Mirrors Zephyr's VPR system timer (drivers/timer/nrf_grtc_timer.c): a GRTC
 * compare channel in the FLPR's IRQ group (GRTC_0 / INTEN0) fires a periodic
 * interrupt so platform_idle() can WFI instead of busy-polling the syscounter.
 * Raw register access to the secure GRTC (0x5xxx alias); offsets per the SVD. */
#include "sys/etimer.h"

#define GRTC_S_BASE            0x500E2000UL
#define GRTC_REG(o)            (*(volatile uint32_t *)(GRTC_S_BASE + (o)))
#define GRTC_EVENTS_COMPARE(n) GRTC_REG(0x100u + 4u * (n))
#define GRTC_INTENSET0         GRTC_REG(0x304u)          /* FLPR IRQ group */
#define GRTC_CC_CCADD(n)       GRTC_REG(0x528u + 0x10u * (n))
#define GRTC_CC_CCEN(n)        GRTC_REG(0x52Cu + 0x10u * (n))

#define FLPR_TICK_CH     4                        /* in the FLPR's CC mask (3,4) */
#define FLPR_TICK_TICKS  (GRTC_TICK_HZ / 100)     /* 10 ms tick (100 Hz)         */

void
clock_grtc_start_tick(void)
{
  /* Arm the first compare relative to the live syscounter (CCADD bit31=1),
   * then enable the channel + its interrupt in the FLPR's group. */
  GRTC_CC_CCADD(FLPR_TICK_CH) = FLPR_TICK_TICKS | (1u << 31);
  GRTC_CC_CCEN(FLPR_TICK_CH)  = 1;
  GRTC_INTENSET0              = (1u << FLPR_TICK_CH);
  /* VPR: enable the machine external interrupt + global interrupt enable. */
  __asm__ volatile("csrs mie, %0" :: "r"(1u << 11));   /* MEIE */
  __asm__ volatile("csrs mstatus, %0" :: "r"(1u << 3)); /* MIE  */
}

void
clock_grtc_tick_isr(void)
{
  GRTC_EVENTS_COMPARE(FLPR_TICK_CH) = 0;                /* ack the compare event */
  GRTC_CC_CCADD(FLPR_TICK_CH) = FLPR_TICK_TICKS;        /* re-arm drift-free (bit31=0) */
  etimer_request_poll();                               /* let the etimer process run */
}
