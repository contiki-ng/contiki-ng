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
 * \addtogroup nrf-sys System drivers
 * @{
 *
 * \addtogroup nrf-clock Clock driver
 * @{
 *
 * \file
 *         Software clock implementation for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "nrfx_config.h"
#include "nrfx_rtc.h"
#include "nrfx_clock.h"

/*---------------------------------------------------------------------------*/
const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(0); /**< RTC instance used for platform clock */
/*---------------------------------------------------------------------------*/
static volatile uint32_t ticks;
void clock_update(void);

static void 
clock_handler(nrfx_clock_evt_type_t event)
{
  (void) event;
}

/**
 * @brief Function for handling the RTC0 interrupts
 * @param int_type Type of interrupt to be handled
 */
static void
rtc_handler(nrfx_rtc_int_type_t int_type)
{
  if(int_type == NRFX_RTC_INT_TICK) {
    clock_update();
  }
}
/** 
 * @brief Function starting the internal LFCLK XTAL oscillator.
 */
static void
lfclk_config(void)
{
  nrfx_err_t err_code = nrfx_clock_init(clock_handler);

  if(err_code != NRFX_SUCCESS) {
    return;
  }

  nrfx_clock_enable();

  nrfx_clock_lfclk_start();
}
/**
 * @brief Function initialization and configuration of RTC driver instance.
 */
static void
rtc_config(void)
{
  nrfx_err_t err_code;

  /*Initialize RTC instance */
  nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
  config.prescaler = 255;
  config.interrupt_priority = 6;
  config.reliable = 0;

  err_code = nrfx_rtc_init(&rtc, &config, rtc_handler);

  if(err_code != NRFX_SUCCESS) {
    return;
  }

  /*Enable tick event & interrupt */
  nrfx_rtc_tick_enable(&rtc, true);

  /*Power on RTC instance */
  nrfx_rtc_enable(&rtc);
}
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  ticks = 0;
  lfclk_config();
  rtc_config();
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  return (clock_time_t)(ticks & 0xFFFFFFFF);
}
/*---------------------------------------------------------------------------*/
void
clock_update(void)
{
  ticks++;
  if(etimer_pending()) {
    etimer_request_poll();
  }
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  return (unsigned long)ticks / CLOCK_CONF_SECOND;
}
/*---------------------------------------------------------------------------*/
void
clock_wait(clock_time_t i)
{
  clock_time_t start;
  start = clock_time();
  while(clock_time() - start < (clock_time_t)i) {
    __WFE();
  }
}
/*---------------------------------------------------------------------------*/
void
clock_delay_usec(uint16_t dt)
{
  NRFX_DELAY_US(dt);
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Obsolete delay function but we implement it here since some code
 * still uses it
 */
void
clock_delay(unsigned int i)
{
  clock_delay_usec(i);
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
