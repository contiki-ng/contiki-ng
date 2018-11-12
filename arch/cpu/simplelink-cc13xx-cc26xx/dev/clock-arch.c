/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \defgroup cc13xx-cc26xx-clock CC13xx/CC26xx clock library
 *
 * @{
 * \file
 *        Implementation of the clock libary for CC13xx/CC26xx.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/etimer.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/aon_rtc.h)
#include DeviceFamily_constructPath(driverlib/cpu.h)
#include DeviceFamily_constructPath(driverlib/interrupt.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/timer.h)

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/power/PowerCC26XX.h>
/*---------------------------------------------------------------------------*/
static volatile clock_time_t count;
static ClockP_Struct etimer_clock;
/*---------------------------------------------------------------------------*/
static void
clock_update_cb(void)
{
  const uintptr_t key = HwiP_disable();
  count += 1;
  HwiP_restore(key);

  /* Notify the etimer system. */
  if(etimer_pending()) {
    etimer_request_poll();
  }
}
/*---------------------------------------------------------------------------*/
static inline clock_time_t
get_count(void)
{
  clock_time_t count_read;

  const uintptr_t key = HwiP_disable();
  count_read = count;
  HwiP_restore(key);

  return count_read;
}
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  /* ClockP_getSystemTickPeriod() returns ticks per us. */
  const uint32_t clockp_ticks_second =
    (uint32_t)(1000 * 1000) / (CLOCK_SECOND) / ClockP_getSystemTickPeriod();

  count = 0;

  ClockP_Params params;
  ClockP_Params_init(&params);

  params.period = clockp_ticks_second;
  params.startFlag = true;

  ClockP_construct(&etimer_clock, (ClockP_Fxn)clock_update_cb,
                   clockp_ticks_second, &params);
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return get_count();
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  return (unsigned long)get_count() / CLOCK_SECOND;
}
/*---------------------------------------------------------------------------*/
void
clock_wait(clock_time_t i)
{
  clock_time_t start;

  start = clock_time();
  while(clock_time() - start < i);
}
/*---------------------------------------------------------------------------*/
void
clock_delay_usec(uint16_t usec)
{
  ClockP_usleep(usec);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Obsolete delay function but we implement it here since some code
 *         still uses it.
 */
void
clock_delay(unsigned int i)
{
  clock_delay_usec(i);
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
