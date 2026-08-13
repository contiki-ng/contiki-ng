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
 *      Temperature platform for nrf_802154 on the nRF5340 network core.
 *
 *      The library uses the die temperature to compensate the RSSI/ED
 *      readings and the CCA energy-detection threshold (nrf_802154_rssi.c),
 *      so an accurate value improves RSSI/LQI reporting and CCA behavior
 *      across the operating range. The network core's TEMP peripheral is
 *      sampled periodically; nrf_802154_temperature_get() returns the cached
 *      value and the library is notified via nrf_802154_temperature_changed()
 *      only when it moves, so the hot RSSI path never blocks on a conversion.
 *
 *      Set NRF_802154_TEMPERATURE_UPDATE_ENABLED to 0 to fall back to a fixed
 *      value (e.g. to leave the TEMP peripheral free for another use).
 */
/*---------------------------------------------------------------------------*/
#include "platform/nrf_802154_temperature.h"
#include "hal/nrf_temp.h"
#include "nrf.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
#ifndef NRF_802154_TEMPERATURE_UPDATE_ENABLED
#define NRF_802154_TEMPERATURE_UPDATE_ENABLED 1
#endif

/* Reading [degrees C] used before the first sample completes and whenever
 * periodic updates are disabled. */
#ifndef NRF_802154_TEMPERATURE_DEFAULT
#define NRF_802154_TEMPERATURE_DEFAULT 20
#endif

#if NRF_802154_TEMPERATURE_UPDATE_ENABLED
#include "sys/ctimer.h"

/* Re-sample interval. The correction does not need fast updates; one second
 * matches the default of Nordic's Zephyr port. */
#ifndef NRF_802154_TEMPERATURE_UPDATE_INTERVAL
#define NRF_802154_TEMPERATURE_UPDATE_INTERVAL CLOCK_SECOND
#endif

static struct ctimer update_timer;
#endif /* NRF_802154_TEMPERATURE_UPDATE_ENABLED */

static int8_t cached_temperature = NRF_802154_TEMPERATURE_DEFAULT;
/*---------------------------------------------------------------------------*/
#if NRF_802154_TEMPERATURE_UPDATE_ENABLED
static int8_t
temperature_measure(void)
{
  int32_t raw;

  /* A single conversion completes in tens of microseconds; this is only
   * reached from the periodic ctimer (process context), never the radio
   * hot path, so the short busy-wait does not delay frame handling. */
  nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY);
  nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_START);
  while(!nrf_temp_event_check(NRF_TEMP, NRF_TEMP_EVENT_DATARDY)) {
  }
  nrf_temp_event_clear(NRF_TEMP, NRF_TEMP_EVENT_DATARDY);
  nrf_temp_task_trigger(NRF_TEMP, NRF_TEMP_TASK_STOP);

  /* The result register holds the temperature in 0.25 degree C steps.
   * Round to the nearest degree instead of truncating toward zero. */
  raw = nrf_temp_result_get(NRF_TEMP);
  return (int8_t)((raw >= 0 ? raw + 2 : raw - 2) / 4);
}
/*---------------------------------------------------------------------------*/
static void
temperature_update(void *ptr)
{
  int8_t now = temperature_measure();

  (void)ptr;

  if(now != cached_temperature) {
    cached_temperature = now;
    nrf_802154_temperature_changed();
  }

  ctimer_reset(&update_timer);
}
#endif /* NRF_802154_TEMPERATURE_UPDATE_ENABLED */
/*---------------------------------------------------------------------------*/
void
nrf_802154_temperature_init(void)
{
#if NRF_802154_TEMPERATURE_UPDATE_ENABLED
  /* Prime the cache with a real reading, then poll for changes. The initial
   * sample intentionally does not invoke nrf_802154_temperature_changed():
   * the library has not cached anything to invalidate yet. */
  cached_temperature = temperature_measure();
  ctimer_set(&update_timer, NRF_802154_TEMPERATURE_UPDATE_INTERVAL,
             temperature_update, NULL);
#endif
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_temperature_deinit(void)
{
#if NRF_802154_TEMPERATURE_UPDATE_ENABLED
  ctimer_stop(&update_timer);
#endif
}
/*---------------------------------------------------------------------------*/
int8_t
nrf_802154_temperature_get(void)
{
  return cached_temperature;
}
/*---------------------------------------------------------------------------*/
