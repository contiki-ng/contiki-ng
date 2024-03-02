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
#include "sys/rtimer.h"

#include "board-peripherals.h"

#include <stdint.h>

PROCESS(process_rtimer, "RTimer");
AUTOSTART_PROCESSES(&process_rtimer);

static int counter_rtimer;
static struct rtimer timer_rtimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND / 4;  
/*---------------------------------------------------------------------------*/
static void init_opt_reading(void);
static void get_light_reading(void);

/*---------------------------------------------------------------------------*/
void
do_rtimer_timeout(struct rtimer *timer, void *ptr)
{

  rtimer_clock_t now=RTIMER_NOW();

  rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);

  int s, ms1,ms2,ms3;
  s = now /RTIMER_SECOND;
  ms1 = (now% RTIMER_SECOND)*10/RTIMER_SECOND;
  ms2 = ((now% RTIMER_SECOND)*100/RTIMER_SECOND)%10;
  ms3 = ((now% RTIMER_SECOND)*1000/RTIMER_SECOND)%10;
  
  counter_rtimer++;
  printf(": %d (cnt) %d (ticks) %d.%d%d%d (sec) \n",counter_rtimer,now, s, ms1,ms2,ms3); 
  get_light_reading();

}

static void
get_light_reading()
{
  int value;

  value = opt_3001_sensor.value(0);
  if(value != CC26XX_SENSOR_READING_ERROR) {
    printf("OPT: Light=%d.%02d lux\n", value / 100, value % 100);
  } else {
    printf("OPT: Light Sensor's Warming Up\n\n");
  }
  init_opt_reading();
}

static void
init_opt_reading(void)
{
  SENSORS_ACTIVATE(opt_3001_sensor);
}

PROCESS_THREAD(process_rtimer, ev, data)
{
  PROCESS_BEGIN();
  init_opt_reading();

  printf(" The value of RTIMER_SECOND is %d \n",RTIMER_SECOND);
  printf(" The value of timeout_rtimer is %d \n",timeout_rtimer);

  while(1) {
    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0,  do_rtimer_timeout, NULL);
    PROCESS_YIELD();
  }

  PROCESS_END();
}
