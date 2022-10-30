/*
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
 * \addtogroup nrf-dev Device drivers
 * @{
 *
 * \addtogroup nrf-watchdog Watchdog driver
 * @{
 *
 * \file
 *         Watchdog implementation for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "nrfx_config.h"
#include "nrfx_wdt.h"

#include "leds.h"

/*---------------------------------------------------------------------------*/
static const nrfx_wdt_t wdt = NRFX_WDT_INSTANCE(0);
static nrfx_wdt_channel_id wdt_channel_id;
static uint8_t wdt_initialized = 0;
/*---------------------------------------------------------------------------*/
/**
 * @brief WDT events handler.
 */
static void
wdt_event_handler(void)
{
  leds_off(LEDS_ALL);
}
/*---------------------------------------------------------------------------*/
void
watchdog_init(void)
{
  nrfx_wdt_config_t config = NRFX_WDT_DEFAULT_CONFIG;
  nrfx_err_t err_code = nrfx_wdt_init(&wdt, &config, &wdt_event_handler);

  if(err_code != NRFX_SUCCESS) {
    return;
  }

  err_code = nrfx_wdt_channel_alloc(&wdt, &wdt_channel_id);

  if(err_code != NRFX_SUCCESS) {
    return;
  }

  wdt_initialized = 1;
}
/*---------------------------------------------------------------------------*/
void
watchdog_start(void)
{
  if(wdt_initialized) {
    nrfx_wdt_enable(&wdt);
  }
}
/*---------------------------------------------------------------------------*/
void
watchdog_periodic(void)
{
  if(wdt_initialized) {
    nrfx_wdt_channel_feed(&wdt, wdt_channel_id);
  }
}
/*---------------------------------------------------------------------------*/
void
watchdog_reboot(void)
{
  NVIC_SystemReset();
}
/**
 * @}
 * @}
 * @}
 */
