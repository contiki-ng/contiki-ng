/*
 * Copyright (C) 2015, Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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

#include <stdio.h>

#include "contiki.h"
#include "sys/etimer.h"
#include "sys/stimer.h"
#include "sys/timer.h"
#include "sys/rtimer.h"

PROCESS(timer_process, "ETimer x Timer x STimer Process");
AUTOSTART_PROCESSES(&timer_process);

static int counter_etimer;
static int counter_timer;
static int counter_stimer;
static struct timer timer_timer;
static struct stimer timer_stimer;
static struct ctimer timer_ctimer;
static struct rtimer timer_rtimer;

/*---------------------------------------------------------------------------*/
void
ctimer_callback(void *ptr)
{
  /* rearm the ctimer */
  ctimer_reset(&timer_ctimer);
  printf("CTimer callback called\n");
}
/*---------------------------------------------------------------------------*/
void
rtimer_callback(struct rtimer *timer, void *ptr)
{
  /* Normally avoid printing from rtimer - rather do a process poll */
  printf("RTimer callback called\n");

  /* Re-arm rtimer */
  rtimer_set(&timer_rtimer, RTIMER_NOW() + RTIMER_SECOND / 2, 0,
             rtimer_callback, NULL);
}
/*---------------------------------------------------------------------------*/
/*
 * This Process will count timer expires on one e-timer (that drives the
 * events to the process), one timer, and one stimer.
 *
 */
PROCESS_THREAD(timer_process, ev, data)
{
  static struct etimer timer_etimer;

  PROCESS_BEGIN();

  ctimer_set(&timer_ctimer, CLOCK_SECOND, ctimer_callback, NULL);
  rtimer_set(&timer_rtimer, RTIMER_NOW() + RTIMER_SECOND / 2, 0,
             rtimer_callback, NULL);

  while(1) {
    /* set all the timers */
    timer_set(&timer_timer, 3 * CLOCK_SECOND);
    stimer_set(&timer_stimer, 3);
    etimer_set(&timer_etimer, 3 * CLOCK_SECOND);

    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

    counter_etimer++;
    if(timer_expired(&timer_timer)) {
      counter_timer++;
    }

    if(stimer_expired(&timer_stimer)) {
      counter_stimer++;
    }

    printf("Timer process: %s\n", counter_timer == counter_etimer
           && counter_timer == counter_stimer ? "SUCCESS" : "FAIL");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
