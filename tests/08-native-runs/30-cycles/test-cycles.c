/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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
 * \file
 *         Tests of the CPU cycle counter API.
 * \author
 *         Niclas Finne <niclas.finne@ri.se>
 */
#include "contiki.h"
#include "sys/cycles.h"
#include "unit-test/unit-test.h"
#include <stdio.h>

PROCESS(test_cycles_process, "test");
AUTOSTART_PROCESSES(&test_cycles_process);

/*
 * These tests assert only on the counter's semantics, never on how many
 * cycles a piece of code takes: a native reading is host CPU time, which
 * varies by tens of percent between runs, see arch/cpu/native/cycles-arch.h.
 * Tightening any of the bounds below makes the test flaky.
 */

/*
 * Long enough that the counter has to move, short enough to keep the
 * test quick and far away from the 4.3 s wrap on native.
 */
#define BURN_MS 20

/*
 * An unsigned difference of two readings stays below this unless the
 * count wrapped or a reading went backwards. It sits four orders of
 * magnitude above the CPU time BURN_MS spends, so it bounds nothing
 * else.
 */
#define WRAP_GUARD 4000000000u
/*---------------------------------------------------------------------------*/
/* Spin, so that the CPU time spent is the wall time spent. */
static void
burn_cpu(unsigned ms)
{
  clock_time_t end = clock_time() + ms * CLOCK_SECOND / 1000;
  volatile unsigned long sink = 0;

  while(clock_time() < end) {
    sink++;
  }
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(off_until_started,
                   "The counter is off until started (native)");
UNIT_TEST(off_until_started)
{
  UNIT_TEST_BEGIN();

  /*
   * Nothing has started the counter yet, so it reads a constant 0. That
   * is this port keeping the state in software; a hardware counter can
   * come up already running with a stale count, see os/sys/cycles.h.
   */
  UNIT_TEST_ASSERT(cycles_now() == 0);
  burn_cpu(BURN_MS);
  UNIT_TEST_ASSERT(cycles_now() == 0);

  /* Stopping a stopped counter is a no-op. */
  cycles_stop();
  UNIT_TEST_ASSERT(cycles_now() == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(us_conversion, "The cycles-to-microseconds helpers");
UNIT_TEST(us_conversion)
{
  UNIT_TEST_BEGIN();

  /* A cycle is a nanosecond of CPU time on this port. */
  UNIT_TEST_ASSERT(CYCLES_HZ == 1000000000);

  UNIT_TEST_ASSERT(CYCLES_TO_US(0) == 0);
  UNIT_TEST_ASSERT(CYCLES_TO_US_FRAC(0) == 0);
  UNIT_TEST_ASSERT(CYCLES_TO_US(7000) == 7);
  UNIT_TEST_ASSERT(CYCLES_TO_US_FRAC(7000) == 0);
  UNIT_TEST_ASSERT(CYCLES_TO_US(2500) == 2);
  UNIT_TEST_ASSERT(CYCLES_TO_US_FRAC(2500) == 50);
  UNIT_TEST_ASSERT(CYCLES_TO_US_FRAC(2509) == 50);
  UNIT_TEST_ASSERT(CYCLES_TO_US_FRAC(2510) == 51);

  /* The whole 32-bit range converts without overflow. */
  UNIT_TEST_ASSERT(CYCLES_TO_US(UINT32_MAX) == 4294967);
  UNIT_TEST_ASSERT(CYCLES_TO_US_FRAC(UINT32_MAX) == 29);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(counts_while_running, "A started counter counts CPU time");
UNIT_TEST(counts_while_running)
{
  uint32_t before, after, previous, current;

  UNIT_TEST_BEGIN();

  cycles_start();

  before = cycles_now();
  burn_cpu(BURN_MS);
  after = cycles_now();

  /*
   * Only that CPU time passed: how much depends on how busy the host is,
   * so a tighter bound than this would be flaky.
   */
  UNIT_TEST_ASSERT(after - before > 0);
  UNIT_TEST_ASSERT(after - before < WRAP_GUARD);

  /* Readings never go backwards. */
  previous = cycles_now();
  for(int i = 0; i < 1000; i++) {
    current = cycles_now();
    UNIT_TEST_ASSERT(current - previous < WRAP_GUARD);
    previous = current;
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(constant_while_stopped,
                   "A stopped counter reads a constant");
UNIT_TEST(constant_while_stopped)
{
  uint32_t stopped;

  UNIT_TEST_BEGIN();

  cycles_start();
  burn_cpu(BURN_MS);
  cycles_stop();

  stopped = cycles_now();
  UNIT_TEST_ASSERT(stopped > 0);

  burn_cpu(BURN_MS);
  UNIT_TEST_ASSERT(cycles_now() == stopped);
  burn_cpu(BURN_MS);
  UNIT_TEST_ASSERT(cycles_now() == stopped);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(restart_keeps_the_count, "Starting again keeps the count");
UNIT_TEST(restart_keeps_the_count)
{
  uint32_t stopped, restarted, running;

  UNIT_TEST_BEGIN();

  cycles_start();
  burn_cpu(BURN_MS);
  cycles_stop();
  stopped = cycles_now();

  /*
   * The CPU time spent while the counter was stopped is not counted, so
   * a restart resumes from the count it kept, give or take the handful
   * of cycles it takes to start it and read it back.
   */
  burn_cpu(BURN_MS);
  cycles_start();
  restarted = cycles_now();
  UNIT_TEST_ASSERT(restarted - stopped < CYCLES_HZ / 1000);

  burn_cpu(BURN_MS);
  running = cycles_now();
  UNIT_TEST_ASSERT(running - stopped > 0);

  /* Starting a running counter does not disturb it. */
  cycles_start();
  UNIT_TEST_ASSERT(cycles_now() - running < WRAP_GUARD);

  cycles_stop();

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_cycles_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  /*
   * off_until_started must run first: it is the only test that can see
   * the counter before anything has started it.
   */
  UNIT_TEST_RUN(off_until_started);
  UNIT_TEST_RUN(us_conversion);
  UNIT_TEST_RUN(counts_while_running);
  UNIT_TEST_RUN(constant_while_stopped);
  UNIT_TEST_RUN(restart_keeps_the_count);

  if(!UNIT_TEST_PASSED(off_until_started)
     || !UNIT_TEST_PASSED(us_conversion)
     || !UNIT_TEST_PASSED(counts_while_running)
     || !UNIT_TEST_PASSED(constant_while_stopped)
     || !UNIT_TEST_PASSED(restart_keeps_the_count)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
