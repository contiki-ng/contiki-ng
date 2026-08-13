/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Low Power Timer + SL Timer + Timer Coordinator for nrf_802154 on nRF54L15.
 *
 * The application core on nRF54L15 does not expose a classic RTC instance,
 * so this backend uses TIMER20 at 1 MHz instead. The scheduling semantics
 * preserved here -- safe compare programming, exact-vs-adjusted scheduling,
 * pending IRQ replay after critical sections, and a dedicated hardware-task
 * compare channel -- are inspired by Zephyr's nrf_rtc_timer.
 */

#include "nrf_802154_sl_timer.h"
#include "platform/nrf_802154_platform_sl_lptimer.h"
#include "timer/nrf_802154_timer_coord.h"
#include "helpers/nrfx_gppi.h"
#include "hal/nrf_timer.h"
#include "nrf.h"

#include <stdbool.h>
#include <stdint.h>

#define LP_TIMER               NRF_TIMER20
#define LP_TIMER_IRQn          TIMER20_IRQn
#define LP_TIMER_IRQ_PRIORITY  1

#define CAPTURE_CC             0
#define ALARM_CC               1
#define SYNC_CC                2
#define HW_TASK_CC             3

#define COUNTER_HALF_SPAN      (UINT64_C(1) << 31)
#define COUNTER_WRAP           (UINT64_C(1) << 32)
#define MIN_TICKS_FROM_NOW     8U
#define SYNC_MARGIN_TICKS      8U

#define FORCE_MASK_ALARM       (1UL << ALARM_CC)
#define FORCE_MASK_SYNC        (1UL << SYNC_CC)

static volatile bool alarm_pending;
static volatile bool sync_pending;
static uint32_t critical_section_depth;
static uint32_t force_isr_mask;

/* Active software timers, ordered by trigger_time. */
static nrf_802154_sl_timer_t *alarm_head;
static uint64_t alarm_target_lpticks;
static uint64_t sync_fire_lpticks;

enum hw_task_state {
  HW_TASK_STATE_IDLE,
  HW_TASK_STATE_SETTING_UP,
  HW_TASK_STATE_READY,
  HW_TASK_STATE_UPDATING,
  HW_TASK_STATE_CLEANING,
};

static enum hw_task_state hw_task_state;
static uint32_t hw_task_ppi_channel = NRF_802154_SL_HW_TASK_PPI_INVALID;
static uint64_t hw_task_fire_lpticks;

static bool timer_initialized;
static uint64_t timer_time_upper;
static uint32_t timer_last_low;
/*---------------------------------------------------------------------------*/
static inline uint32_t
irq_lock_local(void)
{
  uint32_t primask = __get_PRIMASK();

  __disable_irq();
  __DMB();

  return primask;
}
/*---------------------------------------------------------------------------*/
static inline void
irq_unlock_local(uint32_t primask)
{
  __DMB();
  __set_PRIMASK(primask);
}
/*---------------------------------------------------------------------------*/
static inline nrf_802154_sl_timer_t *
timer_next_get(nrf_802154_sl_timer_t *timer)
{
  return (nrf_802154_sl_timer_t *)(uintptr_t)timer->priv.placeholder[0];
}
/*---------------------------------------------------------------------------*/
static inline void
timer_next_set(nrf_802154_sl_timer_t *timer, nrf_802154_sl_timer_t *next)
{
  timer->priv.placeholder[0] = (uint64_t)(uintptr_t)next;
}
/*---------------------------------------------------------------------------*/
static inline bool
timer_is_active(nrf_802154_sl_timer_t *timer)
{
  return timer->priv.placeholder[1] != 0U;
}
/*---------------------------------------------------------------------------*/
static inline void
timer_active_set(nrf_802154_sl_timer_t *timer, bool active)
{
  timer->priv.placeholder[1] = active ? 1U : 0U;
}
/*---------------------------------------------------------------------------*/
__attribute__((weak)) void
nrf_802154_sl_timestamper_synchronized(void)
{
}
/*---------------------------------------------------------------------------*/
static inline bool
hw_task_state_set_locked(enum hw_task_state expected, enum hw_task_state new_state)
{
  if(hw_task_state != expected) {
    return false;
  }

  hw_task_state = new_state;
  return true;
}
/*---------------------------------------------------------------------------*/
static inline uint32_t
timer_event_address_get(uint8_t cc_channel)
{
  return nrf_timer_event_address_get(LP_TIMER, nrf_timer_compare_event_get(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline bool
timer_event_check_cc(uint8_t cc_channel)
{
  return nrf_timer_event_check(LP_TIMER, nrf_timer_compare_event_get(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline void
timer_event_clear_cc(uint8_t cc_channel)
{
  nrf_timer_event_clear(LP_TIMER, nrf_timer_compare_event_get(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline uint32_t
timer_int_mask_get(uint8_t cc_channel)
{
  return nrf_timer_compare_int_get(cc_channel);
}
/*---------------------------------------------------------------------------*/
static inline void
timer_compare_int_enable(uint8_t cc_channel)
{
  nrf_timer_int_enable(LP_TIMER, timer_int_mask_get(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline void
timer_compare_int_disable(uint8_t cc_channel)
{
  nrf_timer_int_disable(LP_TIMER, timer_int_mask_get(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline bool
timer_compare_int_lock(uint8_t cc_channel)
{
  bool enabled = (nrf_timer_int_enable_check(LP_TIMER, timer_int_mask_get(cc_channel)) != 0U);

  timer_compare_int_disable(cc_channel);
  __DMB();
  __ISB();

  return enabled;
}
/*---------------------------------------------------------------------------*/
static inline void
timer_compare_int_unlock(uint8_t cc_channel, bool key)
{
  if(key) {
    timer_compare_int_enable(cc_channel);
    if(force_isr_mask & (1UL << cc_channel)) {
      NVIC_SetPendingIRQ(LP_TIMER_IRQn);
    }
  }
}
/*---------------------------------------------------------------------------*/
static inline uint64_t
timer_time_get_locked(void)
{
  uint32_t low;

  nrf_timer_task_trigger(LP_TIMER, nrf_timer_capture_task_get(CAPTURE_CC));
  low = nrf_timer_cc_get(LP_TIMER, NRF_TIMER_CC_CHANNEL0);

  if(low < timer_last_low) {
    timer_time_upper += COUNTER_WRAP;
  }

  timer_last_low = low;

  return timer_time_upper | low;
}
/*---------------------------------------------------------------------------*/
static uint64_t
timer_time_get(void)
{
  uint32_t primask = irq_lock_local();
  uint64_t now = timer_time_get_locked();
  irq_unlock_local(primask);
  return now;
}
/*---------------------------------------------------------------------------*/
static inline bool
target_is_too_distant(uint64_t now, uint64_t target)
{
  return (target > now) && ((target - now) > COUNTER_HALF_SPAN);
}
/*---------------------------------------------------------------------------*/
static int
timer_compare_set_locked(uint8_t cc_channel, uint64_t target, bool exact)
{
  uint64_t now = timer_time_get_locked();

  if(target_is_too_distant(now, target)) {
    return -1;
  }

  if(target <= now) {
    if(exact) {
      return -1;
    }

    force_isr_mask |= (1UL << cc_channel);
    timer_event_clear_cc(cc_channel);
    return 0;
  }

  {
    uint32_t low_now = (uint32_t)now;
    uint32_t cc_value = (uint32_t)target;
    uint32_t min_cc = low_now + MIN_TICKS_FROM_NOW;

    if((int32_t)(cc_value - min_cc) <= 0) {
      if(exact) {
        return -1;
      }

      cc_value = min_cc;
    }

    force_isr_mask &= ~(1UL << cc_channel);
    timer_event_clear_cc(cc_channel);
    nrf_timer_cc_set(LP_TIMER, (nrf_timer_cc_channel_t)cc_channel, cc_value);
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static inline bool
hw_task_triggered_check_locked(uint64_t now)
{
  return timer_event_check_cc(HW_TASK_CC) || now >= hw_task_fire_lpticks;
}
/*---------------------------------------------------------------------------*/
static inline void
hw_task_ppi_bind_locked(uint32_t ppi_channel)
{
  if(ppi_channel == NRF_802154_SL_HW_TASK_PPI_INVALID) {
    return;
  }

  nrfx_gppi_event_endpoint_setup((uint8_t)ppi_channel, timer_event_address_get(HW_TASK_CC));
}
/*---------------------------------------------------------------------------*/
static inline void
hw_task_ppi_unbind_locked(uint32_t ppi_channel)
{
  if(ppi_channel == NRF_802154_SL_HW_TASK_PPI_INVALID) {
    return;
  }

  nrfx_gppi_event_endpoint_clear((uint8_t)ppi_channel, timer_event_address_get(HW_TASK_CC));
}
/*---------------------------------------------------------------------------*/
static void
alarm_reschedule_locked(void)
{
  if(alarm_head == NULL) {
    alarm_pending = false;
    alarm_target_lpticks = 0;
    force_isr_mask &= ~FORCE_MASK_ALARM;
    timer_compare_int_disable(ALARM_CC);
    timer_event_clear_cc(ALARM_CC);
    return;
  }

  alarm_pending = true;
  alarm_target_lpticks = alarm_head->trigger_time;

  (void)timer_compare_set_locked(ALARM_CC, alarm_target_lpticks, false);

  if(critical_section_depth == 0U) {
    timer_compare_int_enable(ALARM_CC);
    if((force_isr_mask & FORCE_MASK_ALARM) != 0U) {
      NVIC_SetPendingIRQ(LP_TIMER_IRQn);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
sync_schedule_locked(uint64_t fire_lpticks)
{
  sync_pending = true;
  sync_fire_lpticks = fire_lpticks;

  (void)timer_compare_set_locked(SYNC_CC, sync_fire_lpticks, false);

  if(critical_section_depth == 0U) {
    timer_compare_int_enable(SYNC_CC);
    if((force_isr_mask & FORCE_MASK_SYNC) != 0U) {
      NVIC_SetPendingIRQ(LP_TIMER_IRQn);
    }
  }
}
/*---------------------------------------------------------------------------*/
static nrf_802154_sl_timer_ret_t
timer_remove_locked(nrf_802154_sl_timer_t *timer)
{
  nrf_802154_sl_timer_t *prev = NULL;
  nrf_802154_sl_timer_t *curr = alarm_head;

  while(curr != NULL) {
    if(curr == timer) {
      nrf_802154_sl_timer_t *next = timer_next_get(curr);

      if(prev == NULL) {
        alarm_head = next;
      } else {
        timer_next_set(prev, next);
      }

      timer_next_set(curr, NULL);
      timer_active_set(curr, false);
      alarm_reschedule_locked();
      return NRF_802154_SL_TIMER_RET_SUCCESS;
    }

    prev = curr;
    curr = timer_next_get(curr);
  }

  return NRF_802154_SL_TIMER_RET_INACTIVE;
}
/*---------------------------------------------------------------------------*/
static void
alarm_process(void)
{
  while(true) {
    nrf_802154_sl_timer_t *timer;
    uint32_t primask = irq_lock_local();
    uint64_t now = timer_time_get_locked();

    timer = alarm_head;

    if(timer == NULL) {
      alarm_pending = false;
      alarm_target_lpticks = 0;
      irq_unlock_local(primask);
      break;
    }

    if(timer->trigger_time > now) {
      alarm_reschedule_locked();
      irq_unlock_local(primask);
      break;
    }

    alarm_head = timer_next_get(timer);
    timer_next_set(timer, NULL);
    timer_active_set(timer, false);
    alarm_reschedule_locked();

    irq_unlock_local(primask);

    if((timer->action_type & NRF_802154_SL_TIMER_ACTION_TYPE_CALLBACK) &&
       timer->action.callback.callback != NULL) {
      timer->action.callback.callback(timer);
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
sync_process(uint64_t now)
{
  if(sync_pending && now >= sync_fire_lpticks) {
    sync_pending = false;
    nrf_802154_sl_timestamper_synchronized();
  }
}
/*---------------------------------------------------------------------------*/
void
TIMER20_IRQHandler(void)
{
  bool alarm_forced = (force_isr_mask & FORCE_MASK_ALARM) != 0U;
  bool sync_forced = (force_isr_mask & FORCE_MASK_SYNC) != 0U;
  bool alarm_event = timer_event_check_cc(ALARM_CC);
  bool sync_event = timer_event_check_cc(SYNC_CC);

  if(alarm_event) {
    timer_event_clear_cc(ALARM_CC);
  }
  if(sync_event) {
    timer_event_clear_cc(SYNC_CC);
  }

  force_isr_mask &= ~(FORCE_MASK_ALARM | FORCE_MASK_SYNC);

  if(alarm_forced || alarm_event) {
    alarm_process();
  }

  if(sync_forced || sync_event) {
    sync_process(timer_time_get());
  }
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lp_timer_init(void)
{
  uint32_t primask;

  primask = irq_lock_local();

  alarm_pending = false;
  sync_pending = false;
  critical_section_depth = 0;
  force_isr_mask = 0;
  alarm_target_lpticks = 0;
  sync_fire_lpticks = 0;
  hw_task_state = HW_TASK_STATE_IDLE;
  hw_task_ppi_channel = NRF_802154_SL_HW_TASK_PPI_INVALID;
  hw_task_fire_lpticks = 0;
  timer_time_upper = 0;
  timer_last_low = 0;

  if(!timer_initialized) {
    nrf_timer_task_trigger(LP_TIMER, NRF_TIMER_TASK_STOP);
    nrf_timer_task_trigger(LP_TIMER, NRF_TIMER_TASK_CLEAR);
    nrf_timer_shorts_set(LP_TIMER, 0);
    nrf_timer_mode_set(LP_TIMER, NRF_TIMER_MODE_TIMER);
    nrf_timer_bit_width_set(LP_TIMER, NRF_TIMER_BIT_WIDTH_32);
    nrf_timer_prescaler_set(LP_TIMER, NRF_TIMER_FREQ_1MHz);

    timer_event_clear_cc(ALARM_CC);
    timer_event_clear_cc(SYNC_CC);
    timer_event_clear_cc(HW_TASK_CC);
    timer_compare_int_disable(ALARM_CC);
    timer_compare_int_disable(SYNC_CC);
    nrf_timer_task_trigger(LP_TIMER, NRF_TIMER_TASK_START);

    NVIC_SetPriority(LP_TIMER_IRQn, LP_TIMER_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(LP_TIMER_IRQn);
    NVIC_EnableIRQ(LP_TIMER_IRQn);

    timer_initialized = true;
  }

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lp_timer_deinit(void)
{
  uint32_t primask = irq_lock_local();

  alarm_pending = false;
  sync_pending = false;
  critical_section_depth = 0;
  force_isr_mask = 0;
  hw_task_state = HW_TASK_STATE_IDLE;
  hw_task_ppi_channel = NRF_802154_SL_HW_TASK_PPI_INVALID;
  hw_task_fire_lpticks = 0;

  if(timer_initialized) {
    timer_compare_int_disable(ALARM_CC);
    timer_compare_int_disable(SYNC_CC);
    timer_event_clear_cc(ALARM_CC);
    timer_event_clear_cc(SYNC_CC);
    timer_event_clear_cc(HW_TASK_CC);
    nrf_timer_task_trigger(LP_TIMER, NRF_TIMER_TASK_STOP);
    nrf_timer_task_trigger(LP_TIMER, NRF_TIMER_TASK_CLEAR);
  }

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
uint64_t
nrf_802154_platform_sl_lptimer_current_lpticks_get(void)
{
  return timer_time_get();
}
/*---------------------------------------------------------------------------*/
uint64_t
nrf_802154_platform_sl_lptimer_us_to_lpticks_convert(uint64_t us, bool round_up)
{
  (void)round_up;
  return us;
}
/*---------------------------------------------------------------------------*/
uint64_t
nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(uint64_t lpticks)
{
  return lpticks;
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lptimer_schedule_at(uint64_t fire_lpticks)
{
  uint32_t primask = irq_lock_local();

  alarm_pending = true;
  alarm_target_lpticks = fire_lpticks;
  (void)timer_compare_set_locked(ALARM_CC, fire_lpticks, false);

  if(critical_section_depth == 0U) {
    timer_compare_int_enable(ALARM_CC);
    if((force_isr_mask & FORCE_MASK_ALARM) != 0U) {
      NVIC_SetPendingIRQ(LP_TIMER_IRQn);
    }
  }

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lptimer_disable(void)
{
  uint32_t primask = irq_lock_local();

  alarm_pending = false;
  alarm_target_lpticks = 0;
  force_isr_mask &= ~FORCE_MASK_ALARM;
  timer_compare_int_disable(ALARM_CC);
  timer_event_clear_cc(ALARM_CC);

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lptimer_critical_section_enter(void)
{
  uint32_t primask = irq_lock_local();

  critical_section_depth++;

  if(critical_section_depth == 1U) {
    timer_compare_int_disable(ALARM_CC);
    timer_compare_int_disable(SYNC_CC);
  }

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lptimer_critical_section_exit(void)
{
  uint32_t primask = irq_lock_local();

  if(critical_section_depth != 0U) {
    critical_section_depth--;
  }

  if(critical_section_depth == 0U) {
    if(sync_pending) {
      timer_compare_int_enable(SYNC_CC);
      if(timer_event_check_cc(SYNC_CC) || ((force_isr_mask & FORCE_MASK_SYNC) != 0U)) {
        NVIC_SetPendingIRQ(LP_TIMER_IRQn);
      }
    }

    if(alarm_pending) {
      timer_compare_int_enable(ALARM_CC);
      if(timer_event_check_cc(ALARM_CC) || ((force_isr_mask & FORCE_MASK_ALARM) != 0U)) {
        NVIC_SetPendingIRQ(LP_TIMER_IRQn);
      }
    }
  }

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
nrf_802154_sl_lptimer_platform_result_t
nrf_802154_platform_sl_lptimer_hw_task_prepare(uint64_t fire_lpticks,
                                               uint32_t ppi_channel)
{
  uint32_t primask = irq_lock_local();
  uint64_t now = timer_time_get_locked();
  int rc;

  if(!hw_task_state_set_locked(HW_TASK_STATE_IDLE, HW_TASK_STATE_SETTING_UP)) {
    irq_unlock_local(primask);
    return NRF_802154_SL_LPTIMER_PLATFORM_NO_RESOURCES;
  }

  hw_task_ppi_unbind_locked(hw_task_ppi_channel);
  hw_task_ppi_channel = NRF_802154_SL_HW_TASK_PPI_INVALID;
  hw_task_fire_lpticks = fire_lpticks;
  timer_event_clear_cc(HW_TASK_CC);

  if(target_is_too_distant(now, fire_lpticks)) {
    hw_task_state = HW_TASK_STATE_IDLE;
    irq_unlock_local(primask);
    return NRF_802154_SL_LPTIMER_PLATFORM_TOO_DISTANT;
  }

  rc = timer_compare_set_locked(HW_TASK_CC, fire_lpticks, true);
  if(rc != 0) {
    hw_task_state = HW_TASK_STATE_IDLE;
    irq_unlock_local(primask);
    return NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE;
  }

  if(hw_task_triggered_check_locked(timer_time_get_locked())) {
    hw_task_state = HW_TASK_STATE_IDLE;
    irq_unlock_local(primask);
    return NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE;
  }

  hw_task_ppi_bind_locked(ppi_channel);
  hw_task_ppi_channel = ppi_channel;
  hw_task_state = HW_TASK_STATE_READY;

  irq_unlock_local(primask);

  return NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}
/*---------------------------------------------------------------------------*/
nrf_802154_sl_lptimer_platform_result_t
nrf_802154_platform_sl_lptimer_hw_task_cleanup(void)
{
  uint32_t primask = irq_lock_local();

  if(!hw_task_state_set_locked(HW_TASK_STATE_READY, HW_TASK_STATE_CLEANING)) {
    irq_unlock_local(primask);
    return NRF_802154_SL_LPTIMER_PLATFORM_WRONG_STATE;
  }

  timer_event_clear_cc(HW_TASK_CC);
  hw_task_ppi_unbind_locked(hw_task_ppi_channel);
  hw_task_ppi_channel = NRF_802154_SL_HW_TASK_PPI_INVALID;
  hw_task_state = HW_TASK_STATE_IDLE;

  irq_unlock_local(primask);

  return NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}
/*---------------------------------------------------------------------------*/
nrf_802154_sl_lptimer_platform_result_t
nrf_802154_platform_sl_lptimer_hw_task_update_ppi(uint32_t ppi_channel)
{
  bool too_late;
  uint32_t primask = irq_lock_local();

  if(!hw_task_state_set_locked(HW_TASK_STATE_READY, HW_TASK_STATE_UPDATING)) {
    irq_unlock_local(primask);
    return NRF_802154_SL_LPTIMER_PLATFORM_WRONG_STATE;
  }

  hw_task_ppi_unbind_locked(hw_task_ppi_channel);
  hw_task_ppi_bind_locked(ppi_channel);
  hw_task_ppi_channel = ppi_channel;
  too_late = hw_task_triggered_check_locked(timer_time_get_locked());
  hw_task_state = HW_TASK_STATE_READY;

  irq_unlock_local(primask);

  return too_late ? NRF_802154_SL_LPTIMER_PLATFORM_TOO_LATE :
         NRF_802154_SL_LPTIMER_PLATFORM_SUCCESS;
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lptimer_sync_schedule_now(void)
{
  uint32_t primask = irq_lock_local();
  uint64_t now = timer_time_get_locked();

  sync_schedule_locked(now + SYNC_MARGIN_TICKS);

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lptimer_sync_schedule_at(uint64_t fire_lpticks)
{
  uint32_t primask = irq_lock_local();

  sync_schedule_locked(fire_lpticks);

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lptimer_sync_abort(void)
{
  uint32_t primask = irq_lock_local();

  sync_pending = false;
  force_isr_mask &= ~FORCE_MASK_SYNC;
  timer_compare_int_disable(SYNC_CC);
  timer_event_clear_cc(SYNC_CC);

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_platform_sl_lptimer_sync_event_get(void)
{
  return timer_event_address_get(SYNC_CC);
}
/*---------------------------------------------------------------------------*/
uint64_t
nrf_802154_platform_sl_lptimer_sync_lpticks_get(void)
{
  return sync_fire_lpticks;
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_platform_sl_lptimer_granularity_get(void)
{
  return 1U;
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_sl_timer_module_init(void)
{
  alarm_head = NULL;
  nrf_802154_platform_sl_lp_timer_init();
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_sl_timer_module_uninit(void)
{
  alarm_head = NULL;
}
/*---------------------------------------------------------------------------*/
uint64_t
nrf_802154_sl_timer_current_time_get(void)
{
  return timer_time_get();
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_sl_timer_init(nrf_802154_sl_timer_t *p_timer)
{
  timer_next_set(p_timer, NULL);
  timer_active_set(p_timer, false);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_sl_timer_deinit(nrf_802154_sl_timer_t *p_timer)
{
  (void)nrf_802154_sl_timer_remove(p_timer);
}
/*---------------------------------------------------------------------------*/
nrf_802154_sl_timer_ret_t
nrf_802154_sl_timer_add(nrf_802154_sl_timer_t *p_timer)
{
  uint32_t primask = irq_lock_local();
  nrf_802154_sl_timer_t *prev = NULL;
  nrf_802154_sl_timer_t *curr = alarm_head;
  uint64_t now = timer_time_get_locked();

  if(target_is_too_distant(now, p_timer->trigger_time)) {
    irq_unlock_local(primask);
    return NRF_802154_SL_TIMER_RET_TOO_DISTANT;
  }

  if(timer_is_active(p_timer)) {
    (void)timer_remove_locked(p_timer);
  }

  while(curr != NULL && curr->trigger_time <= p_timer->trigger_time) {
    prev = curr;
    curr = timer_next_get(curr);
  }

  timer_next_set(p_timer, curr);
  timer_active_set(p_timer, true);

  if(prev == NULL) {
    alarm_head = p_timer;
    alarm_reschedule_locked();
  } else {
    timer_next_set(prev, p_timer);
  }

  irq_unlock_local(primask);

  return NRF_802154_SL_TIMER_RET_SUCCESS;
}
/*---------------------------------------------------------------------------*/
nrf_802154_sl_timer_ret_t
nrf_802154_sl_timer_remove(nrf_802154_sl_timer_t *p_timer)
{
  nrf_802154_sl_timer_ret_t ret;
  uint32_t primask = irq_lock_local();

  ret = timer_remove_locked(p_timer);

  irq_unlock_local(primask);

  return ret;
}
/*---------------------------------------------------------------------------*/
nrf_802154_sl_timer_ret_t
nrf_802154_sl_timer_update_ppi(nrf_802154_sl_timer_t *p_timer, uint32_t ppi_chn)
{
  (void)p_timer;
  (void)ppi_chn;
  return NRF_802154_SL_TIMER_RET_SUCCESS;
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_timer_coord_init(void)
{
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_timer_coord_uninit(void)
{
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_timer_coord_start(void)
{
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_timer_coord_stop(void)
{
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_timer_coord_timestamp_prepare(const nrf_802154_sl_event_handle_t *p_event)
{
  (void)p_event;
}
/*---------------------------------------------------------------------------*/
bool
nrf_802154_timer_coord_timestamp_get(uint64_t *p_timestamp)
{
  (void)p_timestamp;
  return false;
}
/*---------------------------------------------------------------------------*/
