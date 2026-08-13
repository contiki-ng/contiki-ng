/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "contiki.h"
#include "sys/rtimer.h"
#include "rtimer-arch.h"
#include <stdint.h>

/*
 * rtimer is a stub on the FLPR: rtimer_arch_schedule() is a no-op, so
 * rtimer_set() callbacks never fire (anything relying on rtimers, e.g.
 * TSCH/CSL, will hang). rtimer_arch_now() and RTIMER_ARCH_SECOND are
 * placeholders too. A real implementation needs a GRTC compare channel
 * + interrupt.
 */

void
rtimer_arch_init(void)
{
}

void
rtimer_arch_schedule(rtimer_clock_t t)
{
  (void)t;   /* stub: see the WARNING above — does not actually schedule */
}

rtimer_clock_t
rtimer_arch_now(void)
{
  uint32_t lo;
  __asm__ volatile ("csrr %0, cycle" : "=r"(lo));
  return (rtimer_clock_t)lo;
}
