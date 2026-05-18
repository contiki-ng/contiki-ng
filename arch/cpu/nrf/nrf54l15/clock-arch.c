/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * GRTC-backed clock implementation for nRF54L15.
 *
 * TrustZone on nRF54L15 cannot safely let both worlds run nrfx GRTC init on
 * the same hardware block. The secure image owns shared GRTC bootstrap and the
 * non-secure image consumes fixed compare channels on GRTC interrupt group 1.
 */

#include "contiki.h"

#include "nrf.h"
#include "nrfx_clock.h"
#include "nrfx_grtc.h"
#include "soc/nrfx_coredep.h"
#include "sys/etimer.h"

#include <stdbool.h>
#include <stdint.h>

#define GRTC_IRQ_PRIORITY      6
#define GRTC_TICK_FREQUENCY_HZ 1000000UL
#define GRTC_WORKAROUND_WAKETIME 2U
#define GRTC_WORKAROUND_TIMEOUT  2U

#if CLOCK_SIZE != 4
#error CLOCK_CONF_SIZE must be 4 (32 bit)
#endif

static volatile clock_time_t ticks;
static uint32_t tick_interval_us;
static bool is_initialized;
static volatile nrfx_err_t last_schedule_err;
static volatile uint32_t schedule_failure_count;
static nrfx_err_t init_error_code;
volatile uint8_t tick_channel_id;
static volatile uint32_t grtc_irq_count;
static volatile uint32_t ccen_fix_count;
static volatile uint64_t last_tick_syscounter;
static volatile uint32_t recover_count;

static void schedule_next_tick(void);
/*---------------------------------------------------------------------------*/
static inline void
grtc_apply_sleep_workaround(void)
{
  /* Work around nRF54L15 GRTC missed compare events when TIMEOUT < WAKETIME.
   * Keep the SYSCOUNTER awake long enough that near-future wakeups are not
   * scheduled in the past or too close to the next LFCLK edge. */
  NRF_GRTC->WAKETIME = GRTC_WORKAROUND_WAKETIME;
  NRF_GRTC->TIMEOUT = GRTC_WORKAROUND_TIMEOUT;
}
/*---------------------------------------------------------------------------*/
static inline uint64_t
grtc_syscounter_read_active(uint8_t index)
{
  uint32_t hi;
  uint32_t lo;

  do {
    hi = NRF_GRTC->SYSCOUNTER[index].SYSCOUNTERH;
    lo = NRF_GRTC->SYSCOUNTER[index].SYSCOUNTERL;
  } while(hi != NRF_GRTC->SYSCOUNTER[index].SYSCOUNTERH);

  return ((uint64_t)(hi & GRTC_SYSCOUNTER_SYSCOUNTERH_VALUE_Msk) << 32) | lo;
}

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
wait_for_lfclk_ready(void)
{
  while(!nrfx_clock_lfclk_is_running()) {
    __NOP();
  }
}

static void
wait_for_syscounter_ready(void)
{
  while(!nrfx_grtc_ready_check()) {
    __NOP();
  }
}

#if defined(TRUSTZONE_SECURE)

void
clock_init(void)
{
  if(is_initialized) {
    return;
  }

  init_error_code = NRFX_ERROR_INTERNAL;

  nrfx_err_t err = nrfx_clock_init(NULL);
  if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
    init_error_code = err;
    return;
  }

  nrfx_clock_enable();
  nrfx_clock_lfclk_start();
  wait_for_lfclk_ready();

  err = nrfx_grtc_init(GRTC_IRQ_PRIORITY);
  if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
    init_error_code = err;
    return;
  }

  {
    uint8_t bootstrap_channel = 0;

    err = nrfx_grtc_syscounter_start(true, &bootstrap_channel);
    if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
      init_error_code = err;
      return;
    }
  }

  grtc_apply_sleep_workaround();
  nrfx_grtc_active_request_set(true);
  is_initialized = true;
  init_error_code = NRFX_SUCCESS;
}

clock_time_t
clock_time(void)
{
  return 0;
}

unsigned long
clock_seconds(void)
{
  return 0;
}

void
clock_wait(clock_time_t i)
{
  if(i > 0) {
    clock_delay_usec((uint32_t)(((uint64_t)i * 1000000UL) / CLOCK_SECOND));
  }
}

void
clock_delay_usec(uint16_t dt)
{
  nrfx_coredep_delay_us(dt);
}

void
clock_delay(unsigned int i)
{
  clock_delay_usec(i);
}

uint32_t
clock_arch_get_irq_count(void)
{
  return 0;
}

nrfx_err_t
clock_arch_get_last_schedule_err(void)
{
  return NRFX_SUCCESS;
}

uint32_t
clock_arch_get_schedule_failures(void)
{
  return 0;
}

uint8_t
clock_arch_get_tick_channel(void)
{
  return 0;
}

uint32_t
clock_arch_get_tick_interval_us(void)
{
  return 0;
}

uint64_t
clock_arch_get_syscounter(void)
{
  if(!is_initialized) {
    return 0;
  }

  return grtc_syscounter_read_active(GRTC_IRQ_GROUP);
}

uint32_t
clock_arch_get_ccen_fix_count(void)
{
  return 0;
}

uint32_t
clock_arch_get_grtc_inten(void)
{
  return 0;
}

uint32_t
clock_arch_get_grtc_ccen(void)
{
  return 0;
}

bool
clock_arch_is_initialized(void)
{
  return is_initialized;
}

nrfx_err_t
clock_arch_get_init_error(void)
{
  return init_error_code;
}

void
clock_arch_check_and_recover(void)
{
}

#elif defined(TRUSTZONE_NONSECURE)

static nrfx_grtc_channel_t tick_channel;

static uint64_t
clock_grtc_now(void)
{
  uint64_t counter = 0;

  if(nrfx_grtc_init_check()) {
    (void)nrfx_grtc_syscounter_get(&counter);
  }

  return counter;
}

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
/*---------------------------------------------------------------------------*/
static void
schedule_next_tick(void)
{
  nrfx_err_t err;

  if(!is_initialized) {
    return;
  }

  err = nrfx_grtc_syscounter_cc_relative_set(&tick_channel,
                                             tick_interval_us,
                                             true,
                                             NRFX_GRTC_CC_RELATIVE_SYSCOUNTER);
  last_schedule_err = err;
  if(err == NRFX_ERROR_INTERNAL) {
    wait_for_syscounter_ready();
    err = nrfx_grtc_syscounter_cc_relative_set(&tick_channel,
                                               tick_interval_us,
                                               true,
                                               NRFX_GRTC_CC_RELATIVE_SYSCOUNTER);
    last_schedule_err = err;
  }

  if(err != NRFX_SUCCESS) {
    schedule_failure_count++;
  }

  if(tick_channel_id < NRF_GRTC_SYSCOUNTER_CC_COUNT &&
     NRF_GRTC->CC[tick_channel_id].CCEN != GRTC_CC_CCEN_ACTIVE_Enable) {
    NRF_GRTC->CC[tick_channel_id].CCEN = GRTC_CC_CCEN_ACTIVE_Enable;
    ccen_fix_count++;
  }
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
  ccen_fix_count = 0;
  recover_count = 0;
  tick_interval_us = (uint32_t)(((uint64_t)GRTC_TICK_FREQUENCY_HZ +
                                 (CLOCK_SECOND / 2)) / CLOCK_SECOND);
  if(tick_interval_us == 0) {
    tick_interval_us = 1;
  }

  init_error_code = NRFX_ERROR_INTERNAL;
  last_schedule_err = NRFX_SUCCESS;

  {
    nrfx_err_t err = nrfx_grtc_init(GRTC_IRQ_PRIORITY);

    if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
      init_error_code = err;
      return;
    }

    NVIC_SetPriority(GRTC_IRQn, GRTC_IRQ_PRIORITY);
    NVIC_ClearPendingIRQ(GRTC_IRQn);
    NVIC_EnableIRQ(GRTC_IRQn);

    wait_for_syscounter_ready();

    {
      uint8_t channel = 0;

      err = nrfx_grtc_channel_alloc(&channel);
      if(err != NRFX_SUCCESS) {
        init_error_code = err;
        return;
      }

      err = nrfx_grtc_syscounter_cc_int_enable(channel);
      if(err != NRFX_SUCCESS) {
        init_error_code = err;
        return;
      }

      tick_channel.channel = channel;
      tick_channel.handler = grtc_tick_handler;
      tick_channel.p_context = NULL;
      tick_channel_id = channel;
    }
  }

  is_initialized = true;
  schedule_next_tick();
  init_error_code = last_schedule_err;
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
  return clock_grtc_now();
}

uint32_t
clock_arch_get_ccen_fix_count(void)
{
  return ccen_fix_count;
}

uint32_t
clock_arch_get_grtc_inten(void)
{
  return NRF_GRTC->INTENSET1;
}

uint32_t
clock_arch_get_grtc_ccen(void)
{
  if(tick_channel_id >= NRF_GRTC_SYSCOUNTER_CC_COUNT) {
    return 0;
  }

  return NRF_GRTC->CC[tick_channel_id].CCEN;
}

bool
clock_arch_is_initialized(void)
{
  return is_initialized;
}

nrfx_err_t
clock_arch_get_init_error(void)
{
  return init_error_code;
}

void
clock_arch_check_and_recover(void)
{
  static clock_time_t last_check_ticks;
  static uint32_t idle_count;
  clock_time_t current;

  if(!is_initialized) {
    return;
  }

  current = ticks;
  if(current != last_check_ticks) {
    last_check_ticks = current;
    idle_count = 0;
    return;
  }

  idle_count++;
  if(idle_count > 5000) {
    recover_count++;
    idle_count = 0;
    schedule_failure_count++;
    schedule_next_tick();
  }
}

#else

static nrfx_grtc_channel_t tick_channel;

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

static void
schedule_next_tick(void)
{
  nrfx_err_t err = nrfx_grtc_syscounter_cc_relative_set(&tick_channel,
                                                        tick_interval_us,
                                                        true,
                                                        NRFX_GRTC_CC_RELATIVE_SYSCOUNTER);

  last_schedule_err = err;
  if(err == NRFX_ERROR_INTERNAL) {
    wait_for_syscounter_ready();
    err = nrfx_grtc_syscounter_cc_relative_set(&tick_channel,
                                               tick_interval_us,
                                               true,
                                               NRFX_GRTC_CC_RELATIVE_SYSCOUNTER);
    last_schedule_err = err;
  }

  if(err != NRFX_SUCCESS) {
    schedule_failure_count++;
  }

  if(NRF_GRTC->CC[tick_channel_id].CCEN != GRTC_CC_CCEN_ACTIVE_Enable) {
    NRF_GRTC->CC[tick_channel_id].CCEN = GRTC_CC_CCEN_ACTIVE_Enable;
    ccen_fix_count++;
  }
}

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

  NVIC_SetPriority(GRTC_IRQn, GRTC_IRQ_PRIORITY);
  NVIC_ClearPendingIRQ(GRTC_IRQn);
  NVIC_EnableIRQ(GRTC_IRQn);

  {
    uint8_t main_cc_channel = 0;

    err = nrfx_grtc_syscounter_start(true, &main_cc_channel);
    if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
      init_error_code = err;
      return;
    }
  }

  grtc_apply_sleep_workaround();
  wait_for_syscounter_ready();
  nrfx_grtc_active_request_set(true);

  {
    uint8_t channel = 0;

    err = nrfx_grtc_channel_alloc(&channel);
    if(err != NRFX_SUCCESS) {
      init_error_code = err;
      return;
    }

    nrfx_grtc_syscounter_cc_int_enable(channel);
    tick_channel.channel = channel;
    tick_channel.handler = grtc_tick_handler;
    tick_channel.p_context = NULL;
    tick_channel_id = channel;
  }

  is_initialized = true;
  init_error_code = NRFX_SUCCESS;
  schedule_next_tick();
}

clock_time_t
clock_time(void)
{
  return ticks;
}

unsigned long
clock_seconds(void)
{
  return (unsigned long)(ticks / CLOCK_SECOND);
}

void
clock_wait(clock_time_t i)
{
  clock_time_t start = clock_time();

  while(clock_time() - start < i) {
    __WFE();
  }
}

void
clock_delay_usec(uint16_t dt)
{
  nrfx_coredep_delay_us(dt);
}

void
clock_delay(unsigned int i)
{
  clock_delay_usec(i);
}

uint32_t
clock_arch_get_irq_count(void)
{
  return grtc_irq_count;
}

nrfx_err_t
clock_arch_get_last_schedule_err(void)
{
  return last_schedule_err;
}

uint32_t
clock_arch_get_schedule_failures(void)
{
  return schedule_failure_count;
}

uint8_t
clock_arch_get_tick_channel(void)
{
  return tick_channel_id;
}

uint32_t
clock_arch_get_tick_interval_us(void)
{
  return tick_interval_us;
}

uint64_t
clock_arch_get_syscounter(void)
{
  return grtc_syscounter_read_active(GRTC_IRQ_GROUP);
}
/*---------------------------------------------------------------------------*/
uint32_t
clock_arch_get_ccen_fix_count(void)
{
  return ccen_fix_count;
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
  clock_time_t current;

  if(!is_initialized) {
    return;
  }

  current = ticks;
  if(current != last_check_ticks) {
    last_check_ticks = current;
    idle_count = 0;
    return;
  }

  idle_count++;
  if(idle_count > 5000) {
    recover_count++;
    idle_count = 0;
    schedule_next_tick();
  }
}

#endif
