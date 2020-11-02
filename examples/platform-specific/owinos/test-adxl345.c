/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * Copyright (c) 2020, Oppila Microsystems - http://wwww.oppila.in
 *
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         A simple program for testing the adxl345 on-board accelerometer of the
 *         Owinos Omote.
 * \author
 *         Marcus Lundén, SICS <mlunden@sics.se>
 */

#include <stdio.h>
#include "contiki.h"
#include "dev/leds.h"
#include "dev/adxl345.h"
/*---------------------------------------------------------------------------*/
#define ACCM_READ_INTERVAL    CLOCK_SECOND
/*---------------------------------------------------------------------------*/
static struct etimer et;
/*---------------------------------------------------------------------------*/
PROCESS(accel_process, "Test Accel process");
AUTOSTART_PROCESSES(&accel_process);
/*---------------------------------------------------------------------------*/
/* Main process, setups  */
PROCESS_THREAD(accel_process, ev, data)
{
  PROCESS_BEGIN();

  /* Start and setup the accelerometer with default values, eg no interrupts
   * enabled.
   */
  SENSORS_ACTIVATE(adxl345);

  while(1) {
    printf("x: %d y: %d z: %d\n", adxl345.value(X_AXIS), 
                                  adxl345.value(Y_AXIS), 
                                  adxl345.value(Z_AXIS));

    etimer_set(&et, ACCM_READ_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

