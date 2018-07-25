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
 * \addtogroup cc13xx-cc26xx-rtimer
 * @{
 *
 * \file
 *        Implementation of the rtimer driver for CC13xx/CC26xx.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/aon_event.h)
#include DeviceFamily_constructPath(driverlib/aon_rtc.h)
#include DeviceFamily_constructPath(driverlib/interrupt.h)

#include <ti/drivers/dpl/ClockP.h>
#include <ti/drivers/dpl/HwiP.h>
/*---------------------------------------------------------------------------*/
#include <stddef.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define HWIP_RTC_CH     AON_RTC_CH0
#define RTIMER_RTC_CH   AON_RTC_CH1
/*---------------------------------------------------------------------------*/
typedef void (*isr_fxn_t)(void);
typedef void (*hwi_dispatch_fxn_t)(void);

static hwi_dispatch_fxn_t hwi_dispatch_fxn;
/*---------------------------------------------------------------------------*/
/**
 * \brief  Stub function used when creating the dummy clock object.
 */
static void
rtimer_clock_stub(uintptr_t unused)
{
  (void)unused;
  /* do nothing */
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  The Man-in-the-Middle ISR hook for the HWI dispatch ISR. This
 *         will be the ISR dispatched when INT_AON_RTC_COMB is triggered,
 *         and will either dispatch the interrupt to the rtimer driver or
 *         the HWI driver, depening on which event triggered the interrupt.
 */
static void
rtimer_isr_hook(void)
{
  if(AONRTCEventGet(RTIMER_RTC_CH)) {
    AONRTCEventClear(RTIMER_RTC_CH);
    AONRTCChannelDisable(RTIMER_RTC_CH);

    rtimer_run_next();
  }
  /*
   * HWI Dispatch clears the interrupt. If HWI wasn't triggered, clear
   * the interrupt manually.
   */
  if(AONRTCEventGet(HWIP_RTC_CH)) {
    hwi_dispatch_fxn();
  } else {
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
  uintptr_t key;
  ClockP_Struct clk_object;
  ClockP_Params clk_params;
  volatile isr_fxn_t *ramvec_table;

  key = HwiP_disable();

  /*
   * Create a dummy clock to guarantee the RAM vector table is initialized.
   *
   * Creating a dummy clock will trigger initialization of TimerP, which
   * subsequently initializes the driverlib/interrupt.h module. It is the
   * interrupt module that initializes the RAM vector table.
   *
   * It is safe to destruct the Clock object immediately afterwards.
   */
  ClockP_Params_init(&clk_params);
  ClockP_construct(&clk_object, rtimer_clock_stub, 0, &clk_params);
  ClockP_destruct(&clk_object);

  /* Try to access the RAM vector table. */
  ramvec_table = (isr_fxn_t *)HWREG(NVIC_VTABLE);
  if(!ramvec_table) {
    /*
     * Unable to find the RAM vector table is a serious fault.
     * Spin-lock forever.
     */
    for(;;) { /* hang */ }
  }

  /*
   * The HWI Dispatch ISR is located at interrupt number INT_AON_RTC_COMB
   * in the RAM vector table. Fetch and store it.
   */
  hwi_dispatch_fxn = (hwi_dispatch_fxn_t)ramvec_table[INT_AON_RTC_COMB];
  if(!hwi_dispatch_fxn) {
    /*
     * Unable to find the HWI dispatch ISR in the RAM vector table is
     * a serious fault. Spin-lock forever.
     */
    for(;;) { /* hang */ }
  }

  /*
   * Override the INT_AON_RTC_COMB interrupt number with our own ISR hook,
   * which will act as a man-in-the-middle ISR for the HWI dispatch.
   */
  IntRegister(INT_AON_RTC_COMB, rtimer_isr_hook);

  AONEventMcuWakeUpSet(AON_EVENT_MCU_WU1, AON_EVENT_RTC_CH1);
  AONRTCCombinedEventConfig(HWIP_RTC_CH | RTIMER_RTC_CH);

  HwiP_restore(key);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief    Schedules an rtimer task to be triggered at time \p t.
 * \param t  The time when the task will need executed.
 *
 *           \p t is an absolute time, in other words the task will be
 *           executed AT time \p t, not IN \p t rtimer ticks.
 *
 *           This function schedules a one-shot event with the AON RTC.
 *
 *           This functions converts \p t to a value suitable for the AON RTC.
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
 * \brief   Returns the current real-time clock time.
 * \return  The current rtimer time in ticks.
 *
 *          The value is read from the AON RTC counter and converted to a
 *          number of rtimer ticks.
 */
rtimer_clock_t
rtimer_arch_now()
{
  return (rtimer_clock_t)AONRTCCurrentCompareValueGet();
}
/*---------------------------------------------------------------------------*/
/** @} */
