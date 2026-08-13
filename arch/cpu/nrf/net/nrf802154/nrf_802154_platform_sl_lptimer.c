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
 *      Low Power Timer + SL Timer + Timer Coordinator backend for the
 *      nrf_802154 library on the nRF5340 network core.
 *
 *      The open-source nrf_802154 SL ships only a Zephyr implementation of
 *      nrf_802154_sl_timer.c, so on bare-metal Contiki-NG we must provide
 *      both the SL timer service (the software timer list) and the platform
 *      low-power-timer HW backend. The structure mirrors the nRF54L15 port
 *      (arch/cpu/nrf/nrf54l15/nrf_802154_platform_sl_lptimer.c) but the HW
 *      backend is RTC-based instead of TIMER-based.
 *
 *      Hardware notes for the nRF5340 network core:
 *        - The network core has only RTC0 and RTC1 (no RTC2). RTC0 is the
 *          Contiki-NG system clock (clock-arch.c), so this backend uses
 *          RTC1. nrf_802154_project_config.h sets NRF_802154_RTC_INSTANCE_NO=1
 *          to match (the nrf53 library default is RTC2, which does not exist
 *          on this core).
 *        - One lptick is one 32.768 kHz RTC tick (~30.5 us). The contract
 *          allows lpticks > 1 us; fine radio timing uses the separate
 *          high-frequency TIMER, not this timer.
 *        - The RTC COUNTER is 24-bit; overflow to a 64-bit lptick count is
 *          handled here. LFCLK must already be running (started by
 *          clock-arch.c for RTC0).
 */
/*---------------------------------------------------------------------------*/
#include "nrf_802154_sl_timer.h"
#include "platform/nrf_802154_platform_sl_lptimer.h"
#include "timer/nrf_802154_timer_coord.h"
#include "helpers/nrfx_gppi.h"
#include "hal/nrf_rtc.h"
#include "nrf.h"

#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/* RTC1 backs the low-power timer (RTC0 is the Contiki-NG system clock). */
#define LP_RTC                 NRF_RTC1
#define LP_RTC_IRQn            RTC1_IRQn
#define LP_RTC_IRQHandler      RTC1_IRQHandler
#define LP_RTC_IRQ_PRIORITY    1

/* Compare-channel assignment. The RTC counter is read directly, so unlike
 * the TIMER backend no capture channel is needed. */
#define ALARM_CC               0
#define SYNC_CC                1
#define HW_TASK_CC             2

/* 24-bit RTC counter. */
#define COUNTER_WRAP           (UINT64_C(1) << 24)
#define COUNTER_HALF_SPAN      (UINT64_C(1) << 23)

/* The RTC compare hardware needs the compare value to be at least two ticks
 * ahead of COUNTER to guarantee a match; use three for margin. */
#define MIN_TICKS_FROM_NOW     3U
#define SYNC_MARGIN_TICKS      3U

/* lptick <-> microsecond conversion. 1000000 / 32768 == 15625 / 512. */
#define US_NUM                 UINT64_C(15625)
#define US_DEN                 UINT64_C(512)

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
static inline nrf_rtc_event_t
compare_event(uint8_t cc_channel)
{
  return NRF_RTC_CHANNEL_EVENT_ADDR(cc_channel);
}
/*---------------------------------------------------------------------------*/
static inline uint32_t
rtc_event_address_get(uint8_t cc_channel)
{
  return nrf_rtc_event_address_get(LP_RTC, compare_event(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline bool
rtc_event_check_cc(uint8_t cc_channel)
{
  return nrf_rtc_event_check(LP_RTC, compare_event(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline void
rtc_event_clear_cc(uint8_t cc_channel)
{
  nrf_rtc_event_clear(LP_RTC, compare_event(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline uint32_t
rtc_int_mask_get(uint8_t cc_channel)
{
  return NRF_RTC_CHANNEL_INT_MASK(cc_channel);
}
/*---------------------------------------------------------------------------*/
static inline void
rtc_compare_int_enable(uint8_t cc_channel)
{
  nrf_rtc_int_enable(LP_RTC, rtc_int_mask_get(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline void
rtc_compare_int_disable(uint8_t cc_channel)
{
  nrf_rtc_int_disable(LP_RTC, rtc_int_mask_get(cc_channel));
}
/*---------------------------------------------------------------------------*/
static inline uint64_t
timer_time_get_locked(void)
{
  uint32_t low = nrf_rtc_counter_get(LP_RTC);

  if(low < timer_last_low) {
    timer_time_upper += COUNTER_WRAP;
  }

  timer_last_low = low;

  /* timer_time_upper is always a multiple of COUNTER_WRAP. */
  return timer_time_upper + low;
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
/*
 * Program compare channel cc_channel to fire at absolute lptick time target.
 * Arithmetic is done in the full 64-bit lptick space; only the value written
 * to the 24-bit CC register is masked. When exact is false, a target at or
 * just past "now" is handled by forcing the lptimer ISR instead.
 */
static int
timer_compare_set_locked(uint8_t cc_channel, uint64_t target, bool exact)
{
  uint64_t now = timer_time_get_locked();
  uint64_t min_target;

  if(target_is_too_distant(now, target)) {
    return -1;
  }

  if(target <= now) {
    if(exact) {
      return -1;
    }

    force_isr_mask |= (1UL << cc_channel);
    rtc_event_clear_cc(cc_channel);
    return 0;
  }

  min_target = now + MIN_TICKS_FROM_NOW;
  if(target < min_target) {
    if(exact) {
      return -1;
    }
    target = min_target;
  }

  force_isr_mask &= ~(1UL << cc_channel);
  rtc_event_clear_cc(cc_channel);
  nrf_rtc_cc_set(LP_RTC, cc_channel, NRF_RTC_WRAP((uint32_t)target));

  /*
   * Guard against the RTC counter having already reached the freshly written
   * compare value (which would otherwise be missed until the next wrap). The
   * caller of the exact path detects "too late" itself, so only force here
   * for the non-exact (alarm/sync) path.
   */
  if(!exact && timer_time_get_locked() >= target) {
    force_isr_mask |= (1UL << cc_channel);
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static inline bool
hw_task_triggered_check_locked(uint64_t now)
{
  return rtc_event_check_cc(HW_TASK_CC) || now >= hw_task_fire_lpticks;
}
/*---------------------------------------------------------------------------*/
static inline void
hw_task_ppi_bind_locked(uint32_t ppi_channel)
{
  if(ppi_channel == NRF_802154_SL_HW_TASK_PPI_INVALID) {
    return;
  }

  nrfx_gppi_event_endpoint_setup((uint8_t)ppi_channel,
                                 rtc_event_address_get(HW_TASK_CC));
}
/*---------------------------------------------------------------------------*/
static inline void
hw_task_ppi_unbind_locked(uint32_t ppi_channel)
{
  if(ppi_channel == NRF_802154_SL_HW_TASK_PPI_INVALID) {
    return;
  }

  nrfx_gppi_event_endpoint_clear((uint8_t)ppi_channel,
                                 rtc_event_address_get(HW_TASK_CC));
}
/*---------------------------------------------------------------------------*/
/*
 * Enable a compare channel's interrupt, unless a critical section is active
 * (in which case critical_section_exit() re-enables it on exit). If the
 * deadline was already in the past, timer_compare_set_locked() recorded that
 * in force_isr_mask, so make the IRQ pending to run the handler promptly. The
 * caller must hold the local IRQ lock.
 */
static inline void
cc_enable_locked(uint8_t cc_channel, uint32_t force_mask)
{
  if(critical_section_depth != 0U) {
    return;
  }

  rtc_compare_int_enable(cc_channel);
  if((force_isr_mask & force_mask) != 0U) {
    NVIC_SetPendingIRQ(LP_RTC_IRQn);
  }
}
/*---------------------------------------------------------------------------*/
static void
alarm_reschedule_locked(void)
{
  if(alarm_head == NULL) {
    alarm_pending = false;
    alarm_target_lpticks = 0;
    force_isr_mask &= ~FORCE_MASK_ALARM;
    rtc_compare_int_disable(ALARM_CC);
    rtc_event_clear_cc(ALARM_CC);
    return;
  }

  alarm_pending = true;
  alarm_target_lpticks = alarm_head->trigger_time;

  (void)timer_compare_set_locked(ALARM_CC, alarm_target_lpticks, false);

  cc_enable_locked(ALARM_CC, FORCE_MASK_ALARM);
}
/*---------------------------------------------------------------------------*/
static void
sync_schedule_locked(uint64_t fire_lpticks)
{
  sync_pending = true;
  sync_fire_lpticks = fire_lpticks;

  (void)timer_compare_set_locked(SYNC_CC, sync_fire_lpticks, false);

  cc_enable_locked(SYNC_CC, FORCE_MASK_SYNC);
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
LP_RTC_IRQHandler(void)
{
  uint32_t primask = irq_lock_local();

  /*
   * Bookkeep the 24-bit counter wrap. The OVRFLW interrupt guarantees that
   * timer_time_get_locked() runs at least once per wrap period (~512 s),
   * which its single-wrap detection requires; without it, an idle radio
   * would silently lose whole wrap periods of absolute time.
   */
  if(nrf_rtc_event_check(LP_RTC, NRF_RTC_EVENT_OVERFLOW)) {
    nrf_rtc_event_clear(LP_RTC, NRF_RTC_EVENT_OVERFLOW);
    (void)timer_time_get_locked();
  }

  bool alarm_forced = (force_isr_mask & FORCE_MASK_ALARM) != 0U;
  bool sync_forced = (force_isr_mask & FORCE_MASK_SYNC) != 0U;
  bool alarm_event = rtc_event_check_cc(ALARM_CC);
  bool sync_event = rtc_event_check_cc(SYNC_CC);

  if(alarm_event) {
    rtc_event_clear_cc(ALARM_CC);
  }
  if(sync_event) {
    rtc_event_clear_cc(SYNC_CC);
  }

  /*
   * Clear only the bits captured above, and do it under the IRQ lock: a
   * higher-priority interrupt (e.g. RADIO, priority 0) can preempt this
   * handler and schedule a new past-deadline alarm, and a blanket unlocked
   * clear would silently discard that force request while this invocation
   * has already decided not to process it.
   */
  force_isr_mask &= ~((alarm_forced ? FORCE_MASK_ALARM : 0U) |
                      (sync_forced ? FORCE_MASK_SYNC : 0U));

  irq_unlock_local(primask);

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
    /* LFCLK is assumed to be running already (started by clock-arch.c). */
    nrf_rtc_task_trigger(LP_RTC, NRF_RTC_TASK_STOP);
    nrf_rtc_task_trigger(LP_RTC, NRF_RTC_TASK_CLEAR);
    nrf_rtc_prescaler_set(LP_RTC, 0); /* 32.768 kHz, 1 lptick per RTC tick. */

    rtc_event_clear_cc(ALARM_CC);
    rtc_event_clear_cc(SYNC_CC);
    rtc_event_clear_cc(HW_TASK_CC);
    rtc_compare_int_disable(ALARM_CC);
    rtc_compare_int_disable(SYNC_CC);
    rtc_compare_int_disable(HW_TASK_CC);
    /* The OVRFLW interrupt keeps the 64-bit lptick time base correct while
     * the timer is otherwise unused; see LP_RTC_IRQHandler(). */
    nrf_rtc_event_clear(LP_RTC, NRF_RTC_EVENT_OVERFLOW);
    nrf_rtc_int_enable(LP_RTC, NRF_RTC_INT_OVERFLOW_MASK);
    nrf_rtc_task_trigger(LP_RTC, NRF_RTC_TASK_START);

    NVIC_SetPriority(LP_RTC_IRQn, LP_RTC_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(LP_RTC_IRQn);
    NVIC_EnableIRQ(LP_RTC_IRQn);

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
    rtc_compare_int_disable(ALARM_CC);
    rtc_compare_int_disable(SYNC_CC);
    nrf_rtc_int_disable(LP_RTC, NRF_RTC_INT_OVERFLOW_MASK);
    nrf_rtc_event_clear(LP_RTC, NRF_RTC_EVENT_OVERFLOW);
    rtc_event_clear_cc(ALARM_CC);
    rtc_event_clear_cc(SYNC_CC);
    rtc_event_clear_cc(HW_TASK_CC);
    nrf_rtc_task_trigger(LP_RTC, NRF_RTC_TASK_STOP);
    nrf_rtc_task_trigger(LP_RTC, NRF_RTC_TASK_CLEAR);
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
  uint64_t numerator = us * US_DEN;

  if(round_up) {
    numerator += US_NUM - 1U;
  }

  return numerator / US_NUM;
}
/*---------------------------------------------------------------------------*/
uint64_t
nrf_802154_platform_sl_lptimer_lpticks_to_us_convert(uint64_t lpticks)
{
  return (lpticks * US_NUM) / US_DEN;
}
/*---------------------------------------------------------------------------*/
/*
 * NOTE: nrf_802154_platform_sl_lptimer_schedule_at() and
 * nrf_802154_platform_sl_lptimer_disable() are unused in this port: their
 * only caller in the open-source SL, nrf_802154_sl_timer.c, is excluded
 * from the build (nrf802154.mk) because this file provides the SL timer
 * service directly on top of the same RTC compare channel. They are kept
 * for platform API completeness.
 */
void
nrf_802154_platform_sl_lptimer_schedule_at(uint64_t fire_lpticks)
{
  uint32_t primask = irq_lock_local();

  alarm_pending = true;
  alarm_target_lpticks = fire_lpticks;
  (void)timer_compare_set_locked(ALARM_CC, fire_lpticks, false);

  cc_enable_locked(ALARM_CC, FORCE_MASK_ALARM);

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
  rtc_compare_int_disable(ALARM_CC);
  rtc_event_clear_cc(ALARM_CC);

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_platform_sl_lptimer_critical_section_enter(void)
{
  uint32_t primask = irq_lock_local();

  critical_section_depth++;

  if(critical_section_depth == 1U) {
    rtc_compare_int_disable(ALARM_CC);
    rtc_compare_int_disable(SYNC_CC);
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
      rtc_compare_int_enable(SYNC_CC);
      if(rtc_event_check_cc(SYNC_CC) || ((force_isr_mask & FORCE_MASK_SYNC) != 0U)) {
        NVIC_SetPendingIRQ(LP_RTC_IRQn);
      }
    }

    if(alarm_pending) {
      rtc_compare_int_enable(ALARM_CC);
      if(rtc_event_check_cc(ALARM_CC) || ((force_isr_mask & FORCE_MASK_ALARM) != 0U)) {
        NVIC_SetPendingIRQ(LP_RTC_IRQn);
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
  rtc_event_clear_cc(HW_TASK_CC);

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

  rtc_event_clear_cc(HW_TASK_CC);
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
  rtc_compare_int_disable(SYNC_CC);
  rtc_event_clear_cc(SYNC_CC);

  irq_unlock_local(primask);
}
/*---------------------------------------------------------------------------*/
uint32_t
nrf_802154_platform_sl_lptimer_sync_event_get(void)
{
  return rtc_event_address_get(SYNC_CC);
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
  /* One lptick rounded up to whole microseconds: ceil(15625 / 512) == 31. */
  return (uint32_t)((US_NUM + US_DEN - 1U) / US_DEN);
}
/*---------------------------------------------------------------------------*/
/* SL timer service (replaces the library's Zephyr-only implementation). */
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
  nrf_802154_sl_timer_t *curr;
  uint64_t now = timer_time_get_locked();

  if(target_is_too_distant(now, p_timer->trigger_time)) {
    irq_unlock_local(primask);
    return NRF_802154_SL_TIMER_RET_TOO_DISTANT;
  }

  if(timer_is_active(p_timer)) {
    (void)timer_remove_locked(p_timer);
  }

  /* Read the head only after the removal above; removing an active timer can
   * change alarm_head (and unlinks p_timer), so a head captured earlier would
   * be stale and could splice p_timer into a self-referencing loop. */
  curr = alarm_head;
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
