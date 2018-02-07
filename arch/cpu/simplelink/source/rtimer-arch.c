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
 * \addtogroup cc26xx-rtimer
 * @{
 *
 * \file
 * Implementation of the arch-specific rtimer functions for the CC13xx/CC26xx
 */
/*---------------------------------------------------------------------------*/
#include <ti/drivers/dpl/ClockP.h>

#include <driverlib/aon_event.h>
#include <driverlib/aon_rtc.h>
#include <driverlib/interrupt.h>

#include "contiki.h"

#include <stdint.h>


// FIXME NB TEMPORARY HACK START
#include "radio.h"
const struct radio_driver ieee_mode_driver = { 0 };
// FIXME NB TEMPORARY HACK END

#define RTIMER_RTC_CH AON_RTC_CH1

static ClockP_Struct gClk;
static ClockP_Handle hClk;

typedef void (*IsrFxn)(void);
typedef void (*HwiDispatchFxn)(void);

static volatile HwiDispatchFxn hwiDispatch = NULL;

/*---------------------------------------------------------------------------*/
/**
 * \brief TODO
 */
static void rtimer_clock_stub(uintptr_t arg) { /* do nothing */ }
/*---------------------------------------------------------------------------*/
/**
 * \brief TODO
 */
static void
rtimer_isr_hook(void)
{
  if (AONRTCEventGet(RTIMER_RTC_CH))
  {
    AONRTCEventClear(RTIMER_RTC_CH);
    AONRTCChannelDisable(RTIMER_RTC_CH);

    rtimer_run_next();
  }
  if (hwiDispatch && AONRTCEventGet(AON_RTC_CH0))
  {
    hwiDispatch(); 
  }
  else
  {
    IntPendClear(INT_AON_RTC_COMB);
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief TODO
 */
void
rtimer_arch_init(void)
{
  // Create dummy clock to trigger init of the RAM vector table
  ClockP_Params clkParams;
  ClockP_Params_init(&clkParams);
  hClk = ClockP_construct(&gClk, rtimer_clock_stub, 0, &clkParams);

  // Try to access the RAM vector table
  volatile IsrFxn * const pfnRAMVectors = (volatile IsrFxn *)(HWREG(NVIC_VTABLE));
  if (!pfnRAMVectors)
  {
    for (;;) { /* hang */ }
  }

  // The HWI Dispatch ISR should be located at int num INT_AON_RTC_COMB.
  // Fetch and store it.
  hwiDispatch = (HwiDispatchFxn)pfnRAMVectors[INT_AON_RTC_COMB];
  if (!hwiDispatch)
  {
    for (;;) { /* hang */ }
  }
  
  // Override the INT_AON_RTC_COMB int num with own ISR hook
  IntRegister(INT_AON_RTC_COMB, rtimer_isr_hook);

  AONEventMcuWakeUpSet(AON_EVENT_MCU_WU1, AON_EVENT_RTC_CH1);
  AONRTCCombinedEventConfig(AON_RTC_CH0 | RTIMER_RTC_CH);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Schedules an rtimer task to be triggered at time t
 * \param t The time when the task will need executed.
 *
 * \e t is an absolute time, in other words the task will be executed AT
 * time \e t, not IN \e t rtimer ticks.
 *
 * This function schedules a one-shot event with the AON RTC.
 *
 * This functions converts \e to a value suitable for the AON RTC.
 */
void
rtimer_arch_schedule(rtimer_clock_t t)
{
  /* Convert the rtimer tick value to a value suitable for the AON RTC */
  AONRTCCompareValueSet(RTIMER_RTC_CH, (uint32_t)t);
  AONRTCChannelEnable(RTIMER_RTC_CH);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns the current real-time clock time
 * \return The current rtimer time in ticks
 *
 * The value is read from the AON RTC counter and converted to a number of
 * rtimer ticks
 *
 */
rtimer_clock_t
rtimer_arch_now()
{
  return ((rtimer_clock_t)AONRTCCurrentCompareValueGet());
}
/*---------------------------------------------------------------------------*/
/** @} */
