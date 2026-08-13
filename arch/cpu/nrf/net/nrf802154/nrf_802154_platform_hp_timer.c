/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 * \file
 *      High-precision timer for nrf_802154 on the nRF5340 network core.
 *
 *      Uses TIMER2 at 1 MHz (Contiki-NG rtimer uses TIMER0, the radio uses
 *      TIMER1). Only exercised when frame timestamping is enabled; it is
 *      currently disabled in the project config, but the timer is provided
 *      so the feature can be turned on without further platform work.
 *      Channel layout matches Nordic's upstream HP timer contract.
 */
/*---------------------------------------------------------------------------*/
#include "platform/nrf_802154_hp_timer.h"
#include "hal/nrf_timer.h"
#include "nrf.h"

#define HP_TIMER     NRF_TIMER2

#define CAPTURE_CC   1
#define SYNC_CC      2
#define TIMESTAMP_CC 3

/* 1 MHz tick (1 us resolution). Derive the prescaler from the instance base
 * clock (16 MHz on the nRF5340) so the value stays correct by construction:
 * log2(16 MHz / 1 MHz) = 4. */
#define HP_TIMER_FREQ_HZ   1000000UL
#define HP_TIMER_PRESCALER NRF_TIMER_PRESCALER_CALCULATE(            \
                             NRF_TIMER_BASE_FREQUENCY_GET(HP_TIMER), \
                             HP_TIMER_FREQ_HZ)

static uint32_t unexpected_sync;
/*---------------------------------------------------------------------------*/
static inline uint32_t
timer_time_get(void)
{
  nrf_timer_task_trigger(HP_TIMER, (nrf_timer_task_t)NRF_TIMER_TASK_CAPTURE1);
  return nrf_timer_cc_get(HP_TIMER, (nrf_timer_cc_channel_t)CAPTURE_CC);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_hp_timer_init(void)
{
  nrf_timer_bit_width_set(HP_TIMER, NRF_TIMER_BIT_WIDTH_32);
  nrf_timer_prescaler_set(HP_TIMER, HP_TIMER_PRESCALER);
  nrf_timer_mode_set(HP_TIMER, NRF_TIMER_MODE_TIMER);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_hp_timer_deinit(void)
{
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_STOP);
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_CLEAR);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_hp_timer_start(void)
{
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_START);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_hp_timer_stop(void)
{
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_STOP);
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_CLEAR);
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_hp_timer_current_time_get(void)
{
  return timer_time_get();
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_hp_timer_sync_task_get(void)
{
  return nrf_timer_task_address_get(HP_TIMER,
                                    (nrf_timer_task_t)NRF_TIMER_TASK_CAPTURE2);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_hp_timer_sync_prepare(void)
{
  uint32_t past_time = timer_time_get() - 1;

  unexpected_sync = past_time;
  nrf_timer_cc_set(HP_TIMER, (nrf_timer_cc_channel_t)SYNC_CC, past_time);
}
/*---------------------------------------------------------------------------*/
bool
nrf_802154_hp_timer_sync_time_get(uint32_t *p_timestamp)
{
  uint32_t sync_time = nrf_timer_cc_get(HP_TIMER, (nrf_timer_cc_channel_t)SYNC_CC);

  if(sync_time != unexpected_sync) {
    *p_timestamp = sync_time;
    return true;
  }

  return false;
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_hp_timer_timestamp_task_get(void)
{
  return nrf_timer_task_address_get(HP_TIMER,
                                    (nrf_timer_task_t)NRF_TIMER_TASK_CAPTURE3);
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_hp_timer_timestamp_get(void)
{
  return nrf_timer_cc_get(HP_TIMER, (nrf_timer_cc_channel_t)TIMESTAMP_CC);
}
/*---------------------------------------------------------------------------*/
