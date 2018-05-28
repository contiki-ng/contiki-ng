/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup cc26xx-clocks
 * @{
 *
 * \defgroup cc26xx-software-clock Software Clock
 *
 * Implementation of the clock module for the CC26xx and CC13xx.
 *
 * The software clock uses the facilities provided by the AON RTC driver
 * @{
 *
 * \file
 * Software clock implementation for the TI CC13xx/CC26xx
 */
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/aon_rtc.h)
#include DeviceFamily_constructPath(driverlib/cpu.h)
#include DeviceFamily_constructPath(driverlib/interrupt.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)
#include DeviceFamily_constructPath(driverlib/timer.h)
#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/power/PowerCC26XX.h>

#include "contiki.h"

#define DPL_CLOCK_TICK_PERIOD_US ClockP_getSystemTickPeriod()
#define CLOCK_TICKS_SECOND ((uint32_t)1000000 / (CLOCK_SECOND) / (DPL_CLOCK_TICK_PERIOD_US))

/*---------------------------------------------------------------------------*/
static volatile uint64_t count;
static ClockP_Struct etimerClock;
static void clock_update(void);
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  count = 0;
  ClockP_Params params;
  ClockP_Params_init(&params);
  params.period = CLOCK_TICKS_SECOND;
  params.startFlag = true;
  ClockP_construct(&etimerClock, (ClockP_Fxn)&clock_update, CLOCK_TICKS_SECOND, &params);
}
/*---------------------------------------------------------------------------*/
CCIF clock_time_t
clock_time(void)
{
  uint64_t count_read;
  {
    const uintptr_t key = HwiP_disable();
    count_read = count;
    HwiP_restore(key);
  }

  return (clock_time_t)(count_read & 0xFFFFFFFF);
}
/*---------------------------------------------------------------------------*/
static void
clock_update(void)
{
  {
    const uintptr_t key = HwiP_disable();
    count += 1;
    HwiP_restore(key);
  }

  if (etimer_pending()) {
    etimer_request_poll();
  }
}
/*---------------------------------------------------------------------------*/
CCIF unsigned long
clock_seconds(void)
{
  uint64_t count_read;
  {
    const uintptr_t key = HwiP_disable();
    count_read = count;
    HwiP_restore(key);
  }

  return (unsigned long)count_read / CLOCK_SECOND;
}
/*---------------------------------------------------------------------------*/
void
clock_wait(clock_time_t i)
{
  const clock_time_t start = clock_time();
  while(clock_time() - start < (clock_time_t)i);
}
/*---------------------------------------------------------------------------*/
void
clock_delay_usec(uint16_t len)
{
  // See driverlib/cpu.h
  const uint32_t cpu_clock_mhz = 48;
  // Code in flash, cache disabled: 7 cycles per loop
  const uint32_t cycles_per_loop = 7;
  // ui32Count = [delay in us] * [CPU clock in MHz] / [cycles per loop]
  const uint32_t count = (uint32_t)len * cpu_clock_mhz / cycles_per_loop;
  CPUdelay(count);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Obsolete delay function but we implement it here since some code
 * still uses it
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
