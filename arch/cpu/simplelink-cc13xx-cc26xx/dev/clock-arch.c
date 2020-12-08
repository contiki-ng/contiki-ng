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
 * \addtogroup cc13xx-cc26xx-clock
 * @{
 *
 * \file
 *        Implementation of the clock libary for CC13xx/CC26xx.
 *
 *        The periodic polling of etimer is implemented using SysTick. Since
 *        SysTick is paused when the system clock stopped, that is when the
 *        device enters some low-power mode, a wakeup clock is armed with the
 *        next etimer expiration time OR watchdog timeout everytime the device
 *        enters some low-power mode.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/cc.h"
#include "sys/clock.h"
#include "sys/etimer.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/aon_rtc.h)
#include DeviceFamily_constructPath(driverlib/systick.h)

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/power/PowerCC26XX.h>
/*---------------------------------------------------------------------------*/
#include "watchdog-arch.h"
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
static ClockP_Struct wakeup_clk;
/*---------------------------------------------------------------------------*/
#define H_WAKEUP_CLK    (ClockP_handle(&wakeup_clk))

#define NO_TIMEOUT      (~(uint32_t)0)

#define CLOCK_TO_SYSTEM(t) \
    (uint32_t)(((uint64_t)(t) * 1000 * 1000) / (CLOCK_SECOND * ClockP_getSystemTickPeriod()))
#define SYSTEM_TO_CLOCK(t) \
    (clock_time_t)(((uint64_t)(t) * CLOCK_SECOND * ClockP_getSystemTickPeriod()) / (1000 * 1000))

#define RTC_SUBSEC_FRAC  ((uint64_t)1 << 32)  /* Important to cast to 64-bit */
/*---------------------------------------------------------------------------*/
static void
check_etimer(void)
{
  clock_time_t now;
  clock_time_t next_etimer;

  if(etimer_pending()) {
    now = clock_time();
    next_etimer = etimer_next_expiration_time();

    if(!CLOCK_LT(now, next_etimer)) {
      etimer_request_poll();
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
systick_fxn(void)
{
  check_etimer();
}
/*---------------------------------------------------------------------------*/
static void
wakeup_fxn(uintptr_t arg)
{
  (void)arg;
  check_etimer();
}
/*---------------------------------------------------------------------------*/
static uint32_t
get_etimer_timeout(void)
{
  clock_time_t now;
  clock_time_t next_etimer;

  if(etimer_pending()) {
    now = clock_time();
    next_etimer = etimer_next_expiration_time();

    if(!CLOCK_LT(now, next_etimer)) {
      etimer_request_poll();
      /* etimer already expired, return 0 */
      return 0;

    } else {
      /* Convert from clock ticks to system ticks */
      return CLOCK_TO_SYSTEM(next_etimer - now);
    }
  } else {
    /* No expiration */
    return NO_TIMEOUT;
  }
}
/*---------------------------------------------------------------------------*/
static uint32_t
get_watchdog_timeout(void)
{
#if (WATCHDOG_DISABLE == 0)
  /* Convert from watchdog ticks to system ticks */
  return watchdog_arch_next_timeout() / ClockP_getSystemTickPeriod();

#else
  /* No expiration */
  return NO_TIMEOUT;
#endif
}
/*---------------------------------------------------------------------------*/
bool
clock_arch_enter_idle(void)
{
  /*
   * If the Watchdog is enabled, we must take extra care to wakeup before the
   * Watchdog times out if the next watchdog timeout is before the next etimer
   * timeout. This is because the Watchdog only pauses if the device enters
   * standby. If the device enters idle, the Watchdog still runs and hence the
   * watchdog will timeout if the next etimer timeout is longer than the
   * watchdog timeout.
   */

  uint32_t etimer_timeout;
  uint32_t watchdog_timeout;
  uint32_t timeout;

  etimer_timeout = get_etimer_timeout();
  watchdog_timeout = get_watchdog_timeout();

  timeout = MIN(etimer_timeout, watchdog_timeout);
  if(timeout == 0) {
    /* We are not going to sleep, and hence we don't arm the wakeup clock. */
    return false;
  }

  /* We are dropping to some low-power mode */

  /* Arm the wakeup clock with the calculated timeout if there is any */
  if(timeout != NO_TIMEOUT) {
    ClockP_setTimeout(H_WAKEUP_CLK, timeout);
    ClockP_start(H_WAKEUP_CLK);
  }

  return true;
}
/*---------------------------------------------------------------------------*/
void
clock_arch_exit_idle(void)
{
  ClockP_stop(H_WAKEUP_CLK);
}
/*---------------------------------------------------------------------------*/
void
clock_arch_standby_policy(void)
{
  /*
   * XXX: Workaround for an observed issue where if SysTick interrupt is not
   * disabled when entering/leaving some low-power mode may in very rare
   * occasions clobber the CPU and cause a crash.
   */
  SysTickIntDisable();

  /* Drop to some low-power mode */
  PowerCC26XX_standbyPolicy();

  SysTickIntEnable();
}
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  ClockP_Params clk_params;
  ClockP_FreqHz freq;

  ClockP_Params_init(&clk_params);
  clk_params.startFlag = false;
  clk_params.period = 0;
  ClockP_construct(&wakeup_clk, wakeup_fxn, 0, &clk_params);

  ClockP_getCpuFreq(&freq);

  SysTickPeriodSet(freq.lo / CLOCK_SECOND);
  SysTickIntRegister(systick_fxn);
  SysTickIntEnable();
  SysTickEnable();

  /* enable sync with radio timer */
  HWREGBITW(AON_RTC_BASE + AON_RTC_O_CTL, AON_RTC_CTL_RTC_UPD_EN_BITN) = 1;
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  /*
   * RTC counter is in a 64-bits format (SEC[31:0].SUBSEC[31:0]), where SUBSEC
   * is represented in fractions of a second (VALUE/2^32).
   */
  uint64_t now = AONRTCCurrent64BitValueGet();
  clock_time_t ticks = (clock_time_t)(now / (RTC_SUBSEC_FRAC / CLOCK_SECOND));
  return ticks;
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  unsigned long sec = (unsigned long)AONRTCSecGet();
  return sec;
}
/*---------------------------------------------------------------------------*/
void
clock_wait(clock_time_t i)
{
  uint32_t usec;

  usec = (uint32_t)((1000 * 1000 * i) / CLOCK_SECOND);
  clock_delay_usec(usec);
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
 */
