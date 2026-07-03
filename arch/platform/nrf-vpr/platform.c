/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "contiki.h"
#include "sys/process.h"
#include "sys/etimer.h"
#include "sys/clock.h"
#include "sys/int-master.h"

void
platform_init_stage_one(void)
{
}

void
platform_init_stage_two(void)
{
}

extern void clock_grtc_start_tick(void);

void
platform_init_stage_three(void)
{
  clock_grtc_start_tick();   /* interrupt-driven GRTC system tick (Zephyr-style) */
}

void
platform_main_loop(void)
{
  while(1) {
    while(process_run() > 0) { }
    platform_idle();
  }
}

void
platform_idle(void)
{
  __asm__ volatile("wfi");
}
