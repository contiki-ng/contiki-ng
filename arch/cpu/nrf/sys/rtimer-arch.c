/*
 * Copyright (c) 2020, Toshiba BRIL
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-sys System drivers
 * @{
 *
 * \addtogroup nrf-rtimer Rtimer driver
 * @{
 *
 * \file
 *         Implementation of the architecture dependent rtimer functions for the nRF
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "nrf.h"
#include "hal/nrf_timer.h"

#ifdef NRF_RTIMER_CONF_TIMER_INSTANCE
#define TIMER_INSTANCE NRF_RTIMER_CONF_TIMER_INSTANCE
#else
#define TIMER_INSTANCE 0
#endif

#if TIMER_INSTANCE == 0
#define NRF_RTIMER_TIMER      NRF_TIMER0
#define NRF_RTIMER_IRQn       TIMER0_IRQn
#define NRF_RTIMER_IRQHandler TIMER0_IRQHandler
#elif TIMER_INSTANCE == 1
#define NRF_RTIMER_TIMER      NRF_TIMER1
#define NRF_RTIMER_IRQn       TIMER1_IRQn
#define NRF_RTIMER_IRQHandler TIMER1_IRQHandler
#else
#error Unsupported timer for rtimer
#endif

/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
  nrf_timer_event_clear(NRF_RTIMER_TIMER, NRF_TIMER_EVENT_COMPARE0);

  nrf_timer_frequency_set(NRF_RTIMER_TIMER, NRF_TIMER_FREQ_62500Hz);
  nrf_timer_bit_width_set(NRF_RTIMER_TIMER, NRF_TIMER_BIT_WIDTH_32);
  nrf_timer_mode_set(NRF_RTIMER_TIMER, NRF_TIMER_MODE_TIMER);
  nrf_timer_int_enable(NRF_RTIMER_TIMER, NRF_TIMER_INT_COMPARE0_MASK);
  NVIC_ClearPendingIRQ(NRF_RTIMER_IRQn);
  NVIC_EnableIRQ(NRF_RTIMER_IRQn);
  nrf_timer_task_trigger(NRF_RTIMER_TIMER, NRF_TIMER_TASK_START);
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_schedule(rtimer_clock_t t)
{
  /* 
   * This function schedules a one-shot event with the nRF RTC.
   */
  nrf_timer_cc_set(NRF_RTIMER_TIMER, NRF_TIMER_CC_CHANNEL0, t);
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
rtimer_arch_now()
{
  nrf_timer_task_trigger(NRF_RTIMER_TIMER, NRF_TIMER_TASK_CAPTURE1);
  return nrf_timer_cc_get(NRF_RTIMER_TIMER, NRF_TIMER_CC_CHANNEL1);
}
/*---------------------------------------------------------------------------*/
void
NRF_RTIMER_IRQHandler(void)
{
  if(nrf_timer_event_check(NRF_RTIMER_TIMER, NRF_TIMER_EVENT_COMPARE0)) {
    nrf_timer_event_clear(NRF_RTIMER_TIMER, NRF_TIMER_EVENT_COMPARE0);
    rtimer_run_next();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
