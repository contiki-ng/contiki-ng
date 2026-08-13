/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "contiki.h"
#include "sys/etimer.h"
#include "nrf.h"
#include "flpr-shared.h"
#include <stdint.h>

/* User LED, on GPIO port 2. Pin and polarity are board-dependent; flpr-host
 * forwards the right values per BOARD:
 *   XIAO nRF54L15 user LED = P2.00, active low  (default)
 *   nRF54L15-DK    LED0    = P2.09, active high  (LED0_PIN=9 LED0_ACTIVE_LOW=0)
 * Override LED0_PIN / LED0_ACTIVE_LOW from the make command line for another board. */
#ifndef LED0_PIN
#define LED0_PIN        0
#endif
#ifndef LED0_ACTIVE_LOW
#define LED0_ACTIVE_LOW 1
#endif
#define LED0_BIT        (1u << LED0_PIN)

#if LED0_ACTIVE_LOW
#define LED0_ON()       (NRF_P2_S->OUTCLR = LED0_BIT)   /* drive low to light */
#define LED0_OFF()      (NRF_P2_S->OUTSET = LED0_BIT)
#else
#define LED0_ON()       (NRF_P2_S->OUTSET = LED0_BIT)
#define LED0_OFF()      (NRF_P2_S->OUTCLR = LED0_BIT)
#endif

PROCESS(hello_vpr_process, "hello-vpr");
AUTOSTART_PROCESSES(&hello_vpr_process);

PROCESS_THREAD(hello_vpr_process, ev, data)
{
  static struct etimer et;
  static uint32_t tick;

  PROCESS_BEGIN();

  /* Configure LED0 as output. M33 may have already done this, but it's
   * idempotent — DIRSET is a write-1-to-set register. */
  NRF_P2_S->DIRSET = LED0_BIT;

  while(1) {
    FLPR_SHARED_COUNTER = tick;
    if(tick & 1) {
      LED0_ON();
    } else {
      LED0_OFF();
    }
    etimer_set(&et, CLOCK_SECOND / 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    tick++;
  }

  PROCESS_END();
}
