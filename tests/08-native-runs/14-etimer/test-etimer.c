/*
 * Copyright (c) 2021, Uppsala universitet.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "contiki.h"
#include "unit-test.h"
#include <stdio.h>

PROCESS(test_process, "test");
AUTOSTART_PROCESSES(&test_process);

/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(expiration, "expiration");
UNIT_TEST(expiration)
{
  struct etimer et;

  UNIT_TEST_BEGIN();

  etimer_set(&et, CLOCK_SECOND);
  UNIT_TEST_ASSERT(etimer_pending());
  UNIT_TEST_ASSERT(
      etimer_start_time(&et) + CLOCK_SECOND == etimer_expiration_time(&et));
  etimer_stop(&et);
  UNIT_TEST_ASSERT(etimer_expired(&et));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(ordering, "ordering");
UNIT_TEST(ordering)
{
  struct etimer et[3];

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(etimer_check_ordering());
  etimer_set(&et[0], CLOCK_SECOND);
  etimer_set(&et[1], 3 * CLOCK_SECOND);
  etimer_set(&et[2], 2 * CLOCK_SECOND);
  UNIT_TEST_ASSERT(etimer_check_ordering());
  etimer_stop(&et[0]);
  etimer_stop(&et[2]);
  UNIT_TEST_ASSERT(etimer_check_ordering());
  etimer_reset(&et[0]);
  etimer_restart(&et[2]);
  UNIT_TEST_ASSERT(etimer_check_ordering());

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(expiration);
  UNIT_TEST_RUN(ordering);

  if(!UNIT_TEST_PASSED(expiration)
      || !UNIT_TEST_PASSED(ordering)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
