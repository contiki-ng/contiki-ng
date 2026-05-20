/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Minimal rtimer implementation backed by the nRF54L15 GRTC syscounter.
 */

#include "sys/rtimer.h"

#include "nrfx_grtc.h"

#include <stdbool.h>
#include <stdint.h>

#if defined(TRUSTZONE_SECURE)

void
rtimer_arch_init(void)
{
}

rtimer_clock_t
rtimer_arch_now(void)
{
  return 0;
}

void
rtimer_arch_schedule(rtimer_clock_t t)
{
  (void)t;
}

#elif defined(TRUSTZONE_NONSECURE)

extern uint64_t clock_arch_get_syscounter(void);

static nrfx_grtc_channel_t rtimer_channel;
static bool rtimer_channel_active;

static void
rtimer_grtc_handler(int32_t id, uint64_t cc_value, void *context)
{
  (void)id;
  (void)cc_value;
  (void)context;

  rtimer_run_next();
}

void
rtimer_arch_init(void)
{
  if(!nrfx_grtc_init_check() || rtimer_channel_active) {
    return;
  }

  {
    uint8_t channel;

    if(nrfx_grtc_channel_alloc(&channel) != NRFX_SUCCESS) {
      return;
    }

    rtimer_channel.channel = channel;
    rtimer_channel.handler = rtimer_grtc_handler;
    rtimer_channel.p_context = NULL;
  }

  rtimer_channel_active = true;
}

rtimer_clock_t
rtimer_arch_now(void)
{
  return (rtimer_clock_t)clock_arch_get_syscounter();
}

void
rtimer_arch_schedule(rtimer_clock_t t)
{
  uint64_t now;
  uint64_t target;

  if(!rtimer_channel_active) {
    return;
  }

  now = clock_arch_get_syscounter();
  target = (uint64_t)t;
  if((int64_t)(target - now) <= 0) {
    rtimer_run_next();
    return;
  }

  nrfx_grtc_syscounter_cc_absolute_set(&rtimer_channel, target, true);
}

#else

static nrfx_grtc_channel_t rtimer_channel;
static bool rtimer_channel_active;
/*---------------------------------------------------------------------------*/
static void
rtimer_grtc_handler(int32_t id, uint64_t cc_value, void *context)
{
  (void)id;
  (void)cc_value;
  (void)context;

  rtimer_run_next();
}
/*---------------------------------------------------------------------------*/
static void
ensure_grtc_started(void)
{
  if(!nrfx_grtc_init_check()) {
    nrfx_err_t err = nrfx_grtc_init(6);
    if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY) {
      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
  ensure_grtc_started();

  if(rtimer_channel_active) {
    return;
  }

  {
    uint8_t channel;

    if(nrfx_grtc_channel_alloc(&channel) != NRFX_SUCCESS) {
      return;
    }

    rtimer_channel.channel = channel;
    rtimer_channel.handler = rtimer_grtc_handler;
    rtimer_channel.p_context = NULL;
    rtimer_channel_active = true;
  }
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
rtimer_arch_now(void)
{
  uint64_t counter;

  nrfx_grtc_syscounter_get(&counter);
  return (rtimer_clock_t)counter;
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_schedule(rtimer_clock_t t)
{
  uint64_t target;
  uint64_t now;

  if(!rtimer_channel_active) {
    return;
  }

  target = (uint64_t)t;
  nrfx_grtc_syscounter_get(&now);
  if((int64_t)(target - now) <= 0) {
    rtimer_run_next();
    return;
  }

  nrfx_grtc_syscounter_cc_absolute_set(&rtimer_channel, target, true);
}
/*---------------------------------------------------------------------------*/

#endif
