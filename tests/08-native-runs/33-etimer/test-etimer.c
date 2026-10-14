/*
 * Copyright (c) 2026, Zhaoqi Xu.
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
 *         Tests that an event timer shorter than SELECT_TIMEOUT expires on
 *         time on the native platform.
 */
#include "contiki.h"
#include "unit-test/unit-test.h"
#include <stdio.h>

PROCESS(test_etimer_process, "test");
AUTOSTART_PROCESSES(&test_etimer_process);

/* A period well below the native platform's 1 s SELECT_TIMEOUT. */
#define PERIOD (CLOCK_SECOND / 10)
#define ROUNDS 10

/*
 * How late an expiration may be. Generous, so that a loaded host does not
 * make the test flaky, but far below SELECT_TIMEOUT.
 */
#define SLACK (CLOCK_SECOND / 4)

static clock_time_t expired_at[ROUNDS + 1];
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(periodic_on_time,
                   "A periodic etimer below SELECT_TIMEOUT expires on time");
UNIT_TEST(periodic_on_time)
{
  int i;

  UNIT_TEST_BEGIN();

  for(i = 1; i <= ROUNDS; i++) {
    clock_time_t gap = expired_at[i] - expired_at[i - 1];

    printf("expiration %d after %lu ticks\n", i, (unsigned long)gap);
    UNIT_TEST_ASSERT(gap <= PERIOD + SLACK);
  }

  /* The period is kept by etimer_reset(), so the total must not drift. */
  UNIT_TEST_ASSERT(expired_at[ROUNDS] - expired_at[0] <=
                   ROUNDS * PERIOD + SLACK);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_etimer_process, ev, data)
{
  static struct etimer et;
  static int round;

  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  etimer_set(&et, PERIOD);
  expired_at[0] = clock_time();

  for(round = 1; round <= ROUNDS; round++) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER && etimer_expired(&et));
    expired_at[round] = clock_time();
    etimer_reset(&et);
  }
  etimer_stop(&et);

  UNIT_TEST_RUN(periodic_on_time);

  if(!UNIT_TEST_PASSED(periodic_on_time)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
