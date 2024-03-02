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
#include "buzzer.h"

PROCESS(process_etimer, "ETimer");
AUTOSTART_PROCESSES(&process_etimer);

static int counter_etimer;
int buzzerFrequency[8]={2093,2349,2637,2794,3156,3520,3951,4186}; // hgh notes on a piano

void
do_etimer_timeout()
{
  clock_time_t t;
  int f,s, ms1,ms2,ms3;
  t = clock_time();
  s = t / CLOCK_SECOND;
  ms1 = (t% CLOCK_SECOND)*10/CLOCK_SECOND;
  ms2 = ((t% CLOCK_SECOND)*100/CLOCK_SECOND)%10;
  ms3 = ((t% CLOCK_SECOND)*1000/CLOCK_SECOND)%10;
  f = s % 9;

  counter_etimer++;
  printf("Time(E): %d (cnt) %d (ticks) %d.%d%d%d (sec) \n",counter_etimer,t, s, ms1,ms2,ms3); 
  //toggle the buzzer
  if (f == 8) 
	buzzer_stop(); 
  else 
	buzzer_start(buzzerFrequency[f]);

}

PROCESS_THREAD(process_etimer, ev, data)
{
  static struct etimer timer_etimer;

  PROCESS_BEGIN();
  buzzer_init();
  printf(" The value of CLOCK_SECOND is %d \n",CLOCK_SECOND);

  while(1) {
    etimer_set(&timer_etimer, CLOCK_SECOND);  //1s timer
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);
    do_etimer_timeout();
  }

  PROCESS_END();
}

