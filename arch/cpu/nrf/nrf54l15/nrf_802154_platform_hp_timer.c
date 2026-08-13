/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * High Precision Timer platform for nrf_802154 on nRF54L15.
 * Uses TIMER10 and matches Nordic's upstream HP timer contract:
 * CC[1] = current-time capture, CC[2] = sync capture, CC[3] = event timestamp.
 */

#include "platform/nrf_802154_hp_timer.h"
#include "hal/nrf_timer.h"
#include "nrf.h"

#define HP_TIMER NRF_TIMER10

/* Upstream nrf_802154 HP timer channel layout. */
#define CAPTURE_CC   1
#define SYNC_CC      2
#define TIMESTAMP_CC 3

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
  nrf_timer_prescaler_set(HP_TIMER, NRF_TIMER_FREQ_1MHz);
  nrf_timer_mode_set(HP_TIMER, NRF_TIMER_MODE_TIMER);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_hp_timer_deinit(void)
{
#if NRF_TIMER_HAS_SHUTDOWN
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_SHUTDOWN);
#else
  HP_TIMER->TASKS_STOP = 1;
  HP_TIMER->TASKS_CLEAR = 1;
#endif
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
#if NRF_TIMER_HAS_SHUTDOWN
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_SHUTDOWN);
#else
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_STOP);
  nrf_timer_task_trigger(HP_TIMER, NRF_TIMER_TASK_CLEAR);
#endif
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
  return nrf_timer_task_address_get(HP_TIMER, (nrf_timer_task_t)NRF_TIMER_TASK_CAPTURE2);
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
  return nrf_timer_task_address_get(HP_TIMER, (nrf_timer_task_t)NRF_TIMER_TASK_CAPTURE3);
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_hp_timer_timestamp_get(void)
{
  return nrf_timer_cc_get(HP_TIMER, (nrf_timer_cc_channel_t)TIMESTAMP_CC);
}
/*---------------------------------------------------------------------------*/
