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
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/clock.h"
#include "sys/etimer.h"
/*---------------------------------------------------------------------------*/
#include <ti/drivers/dpl/ClockP.h>
/*---------------------------------------------------------------------------*/
#include "watchdog-arch.h"
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
static ClockP_Struct wakeup_clk;
/*---------------------------------------------------------------------------*/
#define H_WAKEUP_CLK   (ClockP_handle(&wakeup_clk))
/*---------------------------------------------------------------------------*/
static void
wakeup_fxn(uintptr_t arg)
{
  (void)arg;

  if(etimer_pending()) {
    etimer_request_poll();
  }
}
/*---------------------------------------------------------------------------*/
static inline uint32_t
get_timeout(uint64_t time_to_etimer)
{
  uint32_t systemTickPeriod;
  uint32_t etimer_timeout;
  uint32_t watchdog_timeout;

  systemTickPeriod = ClockP_getSystemTickPeriod();

  /* Convert from clock ticks to system ticks */
  etimer_timeout = (uint32_t)((1000 * 1000 * time_to_etimer) / CLOCK_SECOND / systemTickPeriod);

  /*
   * If the Watchdog is enabled, we must take extra care to wakeup before the
   * Watchdog times out if the next timeout is before the next etimer timeout.
   * This is because the Watchdog pauses only if the device enters standy. If
   * the device enters idle, the Wathddog still runs, and hence the wathcdog
   * will timeout if the next etimer timeout is longer than the watchdog
   * timeout.
   */
#if (WATCHDOG_DISABLE == 0)
  /* Convert from watchdog ticks to system ticks */
  watchdog_timeout = watchdog_arch_next_timeout() / systemTickPeriod;
  if((watchdog_timeout != 0) && (watchdog_timeout < etimer_timeout)) {
    return watchdog_timeout;
  }
#endif

  return etimer_timeout;
}
/*---------------------------------------------------------------------------*/
bool
clock_arch_set_wakeup(void)
{
  clock_time_t now;
  clock_time_t next_etimer;
  uint64_t time_to_etimer;
  uint32_t timeout;

  if(ClockP_isActive(H_WAKEUP_CLK)) {
    ClockP_stop(H_WAKEUP_CLK);
  }

  if(etimer_pending()) {
    now = clock_time();
    next_etimer = etimer_next_expiration_time();

    if(!CLOCK_LT(now, next_etimer)) {
      etimer_request_poll();
      return false;

    } else {
      time_to_etimer = (uint64_t)(next_etimer - now);
      timeout = get_timeout(time_to_etimer);
      ClockP_setTimeout(H_WAKEUP_CLK, timeout);
      ClockP_start(H_WAKEUP_CLK);
    }
  }

  return true;
}
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  ClockP_Params clk_params;

  ClockP_Params_init(&clk_params);
  clk_params.startFlag = false;
  clk_params.period = 0;

  ClockP_construct(&wakeup_clk, wakeup_fxn, 0, &clk_params);
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  uint64_t usec;

  usec = (uint64_t)(ClockP_getSystemTicks() * ClockP_getSystemTickPeriod());
  return (clock_time_t)((usec * CLOCK_SECOND) / (1000 * 1000));
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  uint32_t usec;

  usec = ClockP_getSystemTicks() * ClockP_getSystemTickPeriod();
  return (unsigned long)(usec / (1000 * 1000));
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
