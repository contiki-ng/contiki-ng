/*
 * Copyright (c) 2020, Toshiba BRIL
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
 * \addtogroup nrf52840
 * @{
 *
 * \file
 *         Architecture dependent rtimer implementation header file.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "nrf.h"
#include "nrf_timer.h"

#include <stdint.h>
#include <stddef.h>
/*---------------------------------------------------------------------------*/
/*
 * Use Timer RTIMER_TIMER at 62500Hz. Generates 1 tick per exactly 16 usecs,
 * which is exactly 1 .15.4 symbol period.
 */
#define TIMER_INSTANCE NRF_TIMER0
/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
  nrf_timer_event_clear(TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE0);

  nrf_timer_frequency_set(TIMER_INSTANCE, NRF_TIMER_FREQ_62500Hz);
  nrf_timer_bit_width_set(TIMER_INSTANCE, NRF_TIMER_BIT_WIDTH_32);
  nrf_timer_mode_set(TIMER_INSTANCE, NRF_TIMER_MODE_TIMER);
  nrf_timer_int_enable(TIMER_INSTANCE, NRF_TIMER_INT_COMPARE0_MASK);
  NVIC_ClearPendingIRQ(TIMER0_IRQn);
  NVIC_EnableIRQ(TIMER0_IRQn);
  nrf_timer_task_trigger(TIMER_INSTANCE, NRF_TIMER_TASK_START);
}
/*---------------------------------------------------------------------------*/
/**
 *
 * This function schedules a one-shot event with the nRF RTC.
 */
void
rtimer_arch_schedule(rtimer_clock_t t)
{
  nrf_timer_cc_write(TIMER_INSTANCE, NRF_TIMER_CC_CHANNEL0, t);
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
rtimer_arch_now()
{
  nrf_timer_task_trigger(TIMER_INSTANCE, NRF_TIMER_TASK_CAPTURE1);
  return nrf_timer_cc_read(TIMER_INSTANCE, NRF_TIMER_CC_CHANNEL1);
}
/*---------------------------------------------------------------------------*/
void
TIMER0_IRQHandler(void)
{
  if(nrf_timer_event_check(TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE0)) {
    nrf_timer_event_clear(TIMER_INSTANCE, NRF_TIMER_EVENT_COMPARE0);
    rtimer_run_next();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
