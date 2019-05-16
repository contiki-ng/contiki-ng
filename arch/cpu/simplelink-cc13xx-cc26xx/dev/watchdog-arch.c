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
 * \addtogroup cc13xx-cc26xx-watchdog
 * @{
 *
 * \file
 *        Implementation of the CC13xx/CC26xx watchdog driver.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/watchdog.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>

#include <ti/drivers/Watchdog.h>
#include <ti/drivers/dpl/ClockP.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/watchdog.h)
/*---------------------------------------------------------------------------*/
#include "watchdog-arch.h"
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define WATCHDOG_DIV_RATIO       32    /* Watchdog division ratio */
#define WATCHDOG_TIMEOUT_MARGIN  1500  /* 1ms margin in Watchdog ticks */
/*---------------------------------------------------------------------------*/
#if (WATCHDOG_DISABLE == 0)
static Watchdog_Handle wdt_handle;
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief  Initialises the Watchdog module.
 *
 *         Simply sets the reload counter to a default value. The WDT is not
 *         started yet. To start it, watchdog_start() must be called.
 */
void
watchdog_init(void)
{
#if (WATCHDOG_DISABLE == 0)
  Watchdog_init();

  Watchdog_Params wdt_params;
  Watchdog_Params_init(&wdt_params);

  wdt_params.resetMode = Watchdog_RESET_ON;
  wdt_params.debugStallMode = Watchdog_DEBUG_STALL_ON;

  wdt_handle = Watchdog_open(Board_WATCHDOG0, &wdt_params);
#endif
}
/*---------------------------------------------------------------------------*/
uint32_t
watchdog_arch_next_timeout(void)
{
  if(!WatchdogRunning()) {
    return 0;
  }

  ClockP_FreqHz freq;
  ClockP_getCpuFreq(&freq);
  uint64_t value = (uint64_t)WatchdogValueGet();
  uint32_t timeout = (uint32_t)((value * 1000 * 1000 * WATCHDOG_DIV_RATIO) / freq.lo);

  /*
   * A margin should be applied to the timeout to ensure there is enough time
   * to enter low-power mode, wakeup, and clear the Watchdog timer before it
   * times out. If the timeout is equals to or less than the margin, simply
   * return the lowest possible timeout.
   */
  if(timeout <= WATCHDOG_TIMEOUT_MARGIN) {
    return 1;
  } else {
    return timeout - WATCHDOG_TIMEOUT_MARGIN;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Start the Watchdog.
 */
void
watchdog_start(void)
{
#if (WATCHDOG_DISABLE == 0)
  watchdog_periodic();
#endif
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Refresh (feed) the Watchdog.
 */
void
watchdog_periodic(void)
{
#if (WATCHDOG_DISABLE == 0)
  uint32_t timeout_ticks = Watchdog_convertMsToTicks(wdt_handle, WATCHDOG_TIMEOUT_MS);
  Watchdog_setReload(wdt_handle, timeout_ticks);
#endif
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Stop the Watchdog such that it won't timeout and cause a
 *         system reset.
 */
void
watchdog_stop(void)
{
#if (WATCHDOG_DISABLE == 0)
  Watchdog_clear(wdt_handle);
#endif
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Manually trigger a Watchdog timeout.
 */
void
watchdog_reboot(void)
{
#if (WATCHDOG_DISABLE == 0)
  watchdog_start();

  /* Busy loop until watchdog times out */
  for (;;) { /* hang */ }
#endif
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
