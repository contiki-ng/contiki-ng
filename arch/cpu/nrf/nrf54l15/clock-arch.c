/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Minimal GRTC-backed clock implementation for nRF54L15.
 *
 * NOTE: This ignores advanced low-power tuning and only provides the
 *       tick functionality required for basic Contiki timers.
 */

#include "contiki.h"

#include "nrfx_clock.h"
#include "soc/nrfx_coredep.h"  /* Delay functions in v3.x */

/*
 * nrfx HAL conditionally redefines GRTC_IRQn based on NRF_APPLICATION:
 * - For nRF54L15 (LUMOS) application core: GRTC_IRQn = GRTC_2_IRQn
 * - For nRF54L15 FLPR core: GRTC_IRQn = GRTC_0_IRQn
 * This is defined in hal/nrf_grtc.h, which is included by nrfx_grtc.h
 */
#include "nrfx_grtc.h"
#include "sys/etimer.h"

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"

#define GRTC_IRQ_PRIORITY      6
#define GRTC_TICK_FREQUENCY_HZ 1000000UL

#if CLOCK_SIZE != 4
#error CLOCK_CONF_SIZE must be 4 (32 bit)
#endif

static volatile clock_time_t ticks;
static uint32_t tick_interval_us;
static nrfx_grtc_channel_t tick_channel;
static bool is_initialized;
static volatile nrfx_err_t last_schedule_err;
static volatile uint32_t schedule_failure_count;
static nrfx_err_t init_error_code;
volatile uint8_t tick_channel_id;
static volatile uint32_t grtc_irq_count;
static volatile uint64_t last_tick_syscounter;
static volatile uint32_t recover_count;

static void schedule_next_tick(void);
/*---------------------------------------------------------------------------*/
static void
clock_update(void)
{
  ticks++;
  if(etimer_pending() && !CLOCK_LT(ticks, etimer_next_expiration_time())) {
    etimer_request_poll();
  }
}
/*---------------------------------------------------------------------------*/
static void
grtc_tick_handler(int32_t id, uint64_t cc_value, void *context)
{
  (void)id;
  (void)context;

  grtc_irq_count++;
  last_tick_syscounter = cc_value;
  clock_update();
  schedule_next_tick();
}
static void wait_for_lfclk_ready(void);
static void wait_for_syscounter_ready(void);
uint64_t clock_arch_get_syscounter(void);
/*---------------------------------------------------------------------------*/
static void
schedule_next_tick(void)
{
  /* Use absolute scheduling rather than relative. The relative form
   * (nrfx_grtc_syscounter_cc_relative_set) writes CCADD, which on
   * nRF54L15 LUMOS occasionally leaves CCEN Disabled when the deadline
   * is very close to "now" — the channel then never fires and the
   * kernel tick stops. Here we compute the deadline in code and write
   * an absolute CC value instead.
   *
   * NOTE: the unconditional CCEN write at the end of this function is
   * functionally required to start the channel — it only looks like a
   * redundant sanity check. nrfx_grtc_syscounter_cc_absolute_set() calls
   * cc_channel_prepare() (which disables CCEN) and then writes only
   * CCL/CCH via nrf_grtc_sys_counter_cc_set(); it never re-enables CCEN.
   * So on return from that call CCEN is Disabled, and without the write
   * below the channel stays disabled and no tick ever fires. Do not
   * remove it.
   */
  uint64_t deadline = clock_arch_get_syscounter() + tick_interval_us;
  nrfx_err_t err = nrfx_grtc_syscounter_cc_absolute_set(&tick_channel,
                                                       deadline,
                                                       true);
  last_schedule_err = err;
  if(err == NRFX_ERROR_INTERNAL) {
    wait_for_syscounter_ready();
    deadline = clock_arch_get_syscounter() + tick_interval_us;
    err = nrfx_grtc_syscounter_cc_absolute_set(&tick_channel,
                                               deadline,
                                               true);
    last_schedule_err = err;
  }

  if(err != NRFX_SUCCESS) {
    schedule_failure_count++;
  }

  /* Force CCEN active (see NOTE above — this is mandatory). CCEN ending
   * up Disabled was the root cause of the "GRTC won't wake from WFI after
   * soft-reset" hang that the arch/platform/nrf/lpm.h spin loop worked
   * around: the IRQ never fired, so WFI slept forever. With absolute
   * scheduling and this write that workaround is no longer needed, and
   * this PR removes it. */
  NRF_GRTC->CC[tick_channel_id].CCEN =
    (GRTC_CC_CCEN_ACTIVE_Enable << GRTC_CC_CCEN_ACTIVE_Pos);
}
/*---------------------------------------------------------------------------*/
static void
lfclk_init(void)
{
  nrfx_err_t err = nrfx_clock_init(NULL);
  if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
    return;
  }

  nrfx_clock_enable();
  nrfx_clock_lfclk_start();
  wait_for_lfclk_ready();
}
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  if(is_initialized) {
    return;
  }

  ticks = 0;
  grtc_irq_count = 0;
  schedule_failure_count = 0;
  tick_interval_us = (uint32_t)(((uint64_t)GRTC_TICK_FREQUENCY_HZ +
                                 (CLOCK_SECOND / 2)) / CLOCK_SECOND);
  if(tick_interval_us == 0) {
    tick_interval_us = 1;
  }

  lfclk_init();

  init_error_code = NRFX_ERROR_INTERNAL;

  nrfx_err_t err = nrfx_grtc_init(GRTC_IRQ_PRIORITY);
  if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
    init_error_code = err;
    return;
  }

  /* Ensure NVIC routes the interrupt to the Cortex-M33 core */
  NVIC_SetPriority(GRTC_IRQn, GRTC_IRQ_PRIORITY);
  NVIC_ClearPendingIRQ(GRTC_IRQn);
  NVIC_EnableIRQ(GRTC_IRQn);

  /* Start the GRTC syscounter and busy-wait until it is ready.
   * This call also allocates a main CC channel automatically. */
  uint8_t main_cc_channel = 0;
  err = nrfx_grtc_syscounter_start(true, &main_cc_channel);
  if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
    init_error_code = err;
    return;
  }
  wait_for_syscounter_ready();
  nrfx_grtc_active_request_set(true);

  /* Now allocate a separate GRTC channel for our ticking */
  uint8_t channel = 0;
  err = nrfx_grtc_channel_alloc(&channel);
  if(err != NRFX_SUCCESS) {
    init_error_code = err;
    return;
  }

  nrfx_grtc_syscounter_cc_int_enable(channel);

  /* Setup channel structure */
  tick_channel.channel = channel;
  tick_channel.handler = grtc_tick_handler;
  tick_channel.p_context = NULL;
  tick_channel_id = channel;

  schedule_next_tick();

  is_initialized = true;
  init_error_code = NRFX_SUCCESS;
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return ticks;
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  return (unsigned long)(ticks / CLOCK_SECOND);
}
/*---------------------------------------------------------------------------*/
void
clock_wait(clock_time_t i)
{
  clock_time_t start = clock_time();
  while(clock_time() - start < i) {
    __WFE();
  }
}
/*---------------------------------------------------------------------------*/
void
clock_delay_usec(uint16_t dt)
{
  nrfx_coredep_delay_us(dt);
}
/*---------------------------------------------------------------------------*/
void
clock_delay(unsigned int i)
{
  clock_delay_usec(i);
}
/*---------------------------------------------------------------------------*/
uint32_t
clock_arch_get_irq_count(void)
{
  return grtc_irq_count;
}
/*---------------------------------------------------------------------------*/
nrfx_err_t
clock_arch_get_last_schedule_err(void)
{
  return last_schedule_err;
}
/*---------------------------------------------------------------------------*/
uint32_t
clock_arch_get_schedule_failures(void)
{
  return schedule_failure_count;
}
/*---------------------------------------------------------------------------*/
uint8_t
clock_arch_get_tick_channel(void)
{
  return tick_channel_id;
}
/*---------------------------------------------------------------------------*/
uint32_t
clock_arch_get_tick_interval_us(void)
{
  return tick_interval_us;
}
/*---------------------------------------------------------------------------*/
uint64_t
clock_arch_get_syscounter(void)
{
  /* Read from the "active" domain (SYSCOUNTER[1]) to avoid the busy-wait
   * retry loop in nrfx_grtc_syscounter_get() which can deadlock. */
  uint32_t hi, lo;
  do {
    hi = NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERH;
    lo = NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERL;
  } while(hi != NRF_GRTC->SYSCOUNTER[1].SYSCOUNTERH);
  return ((uint64_t)(hi & 0x001FFFFFUL) << 32) | lo;
}
/*---------------------------------------------------------------------------*/
uint32_t
clock_arch_get_grtc_inten(void)
{
  return NRF_GRTC->INTENSET2;
}
/*---------------------------------------------------------------------------*/
uint32_t
clock_arch_get_grtc_ccen(void)
{
  return NRF_GRTC->CC[tick_channel_id].CCEN;
}
/*---------------------------------------------------------------------------*/
bool
clock_arch_is_initialized(void)
{
  return is_initialized;
}
/*---------------------------------------------------------------------------*/
nrfx_err_t
clock_arch_get_init_error(void)
{
  return init_error_code;
}
/*---------------------------------------------------------------------------*/
void
clock_arch_check_and_recover(void)
{
  static clock_time_t last_check_ticks;
  static uint32_t idle_count;

  if(!is_initialized) {
    return;
  }

  clock_time_t current = ticks;

  if(current != last_check_ticks) {
    last_check_ticks = current;
    idle_count = 0;
    return;
  }

  idle_count++;

  if(idle_count > 5000) {
    recover_count++;
    idle_count = 0;

    /* Try to force-reschedule */
    schedule_next_tick();
  }
}
/*---------------------------------------------------------------------------*/
static void
wait_for_lfclk_ready(void)
{
  while(!nrfx_clock_lfclk_is_running()) {
    __NOP();
  }
}
/*---------------------------------------------------------------------------*/
static void
wait_for_syscounter_ready(void)
{
  while(!nrfx_grtc_ready_check()) {
    __NOP();
  }
}
/*---------------------------------------------------------------------------*/
