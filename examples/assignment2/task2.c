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

PROCESS(task2, "task2");
AUTOSTART_PROCESSES(&task2);

static int counter_rtimer;
static struct rtimer timer_rtimer;
static rtimer_clock_t timeout_rtimer = RTIMER_SECOND /4;  

static int state;
static int num_buzzed;

#define IDLE 0
#define BUZZ 1
#define WAIT 2
/*---------------------------------------------------------------------------*/


/*---------------------------------------------------------------------------*/
void
do_rtimer_timeout(struct rtimer *timer, void *ptr)
{
    printf("Timer interrupt triggered\r\n");
    process_poll(&task2);
}



static void wait(int seconds)
{
    
    rtimer_clock_t timeout_rtimer = seconds * RTIMER_SECOND;
    rtimer_set(&timer_rtimer, RTIMER_NOW() + timeout_rtimer, 0, do_rtimer_timeout, NULL);
    PROCESS_YIELD();
    
}

PROCESS_THREAD(task2, ev, data)
{
    PROCESS_BEGIN();

    state = IDLE;
    while(1) {
        switch (state) {
            case IDLE :
                // if significant motion
                wait(2); // Replace with IMU stuff and Light Sensor stuff
                state = BUZZ;
                num_buzzed = 0;
                break;
                
            case BUZZ:
                if (num_buzzed == 4){
                    state = IDLE
                } else {
                    num_buzzed++;
                    buzzer_start(2794);
                    wait(2);
                    state = WAIT;
                }
                break;

            case WAIT:
                buzzer_stop()
                wait(2);
                state = BUZZ;
                break;

            default:
                // DIE
        }
    }

    PROCESS_END();
}
