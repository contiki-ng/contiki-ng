/*
 * Copyright (c) 2023, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

#include "mbedtls/private_access.h"
#include "timing_alt.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "DTLS"
#define LOG_LEVEL LOG_LEVEL_NONE

volatile int mbedtls_timing_alarmed;
static struct ctimer alarm_timer;

/*---------------------------------------------------------------------------*/
static void
alarm_callback(void *ptr)
{
  LOG_DBG("Triggered an alarm\n");
  mbedtls_timing_alarmed = 1;
}
/*---------------------------------------------------------------------------*/
void
mbedtls_set_alarm(int seconds)
{
  LOG_DBG("Set alarm in %d seconds\n", seconds);
  mbedtls_timing_alarmed = 0;
  ctimer_set(&alarm_timer, seconds * CLOCK_SECOND, alarm_callback, NULL);
}
/*---------------------------------------------------------------------------*/
int
mbedtls_timing_get_delay(void *data)
{
  struct mbedtls_timing_delay_context *ctx =
    (struct mbedtls_timing_delay_context *)data;
  unsigned long elapsed_ms =
    mbedtls_timing_get_timer(&ctx->MBEDTLS_PRIVATE(timer), 0);

  if(ctx->MBEDTLS_PRIVATE(fin_ms) == 0) {
    return -1;
  }

  if(elapsed_ms > ctx->MBEDTLS_PRIVATE(fin_ms)) {
    /* The final delay has passed. */
    LOG_DBG("%s: the final delay has passed. %lu > %"PRIu32"\n",
            __func__, elapsed_ms, ctx->MBEDTLS_PRIVATE(fin_ms));
    return 2;
  }

  if(elapsed_ms > ctx->MBEDTLS_PRIVATE(int_ms)) {
    /* The intermediate delay has passed. */
    LOG_DBG("%s: the intermediate delay has passed. %lu > %"PRIu32"\n",
            __func__, elapsed_ms, ctx->MBEDTLS_PRIVATE(int_ms));
    return 1;
  }

  /* None of the delays have passed. */
  LOG_DBG("%s: no delay has passed. elapsed ms %lu, fin ms %"PRIu32", int ms %"PRIu32"\n",
          __func__, elapsed_ms,
          ctx->MBEDTLS_PRIVATE(fin_ms), ctx->MBEDTLS_PRIVATE(int_ms));
  return 0;
}
/*---------------------------------------------------------------------------*/
unsigned long
mbedtls_timing_get_timer(struct mbedtls_timing_hr_time *val, int reset)
{
  if(reset) {
    timer_reset(&val->timer);
  }
  unsigned long ret = ((clock_time() - val->timer.start) * 1000) / CLOCK_SECOND;
  return ret;
}
/*---------------------------------------------------------------------------*/
unsigned long
mbedtls_timing_hardclock(void)
{
  /* This should preferably be a CPU cycle counter, but we use the rtimer
     value instead. */
  return RTIMER_NOW();
}
/*---------------------------------------------------------------------------*/
void
mbedtls_timing_set_delay(void *data, uint32_t int_ms, uint32_t fin_ms)
{
  struct mbedtls_timing_delay_context *ctx;

  ctx = (struct mbedtls_timing_delay_context *)data;
  struct timer *timer = (struct timer *)&ctx->MBEDTLS_PRIVATE(timer);

  timer_set(timer, fin_ms * CLOCK_SECOND / 1000);
  ctx->MBEDTLS_PRIVATE(int_ms) = int_ms;
  ctx->MBEDTLS_PRIVATE(fin_ms) = fin_ms;
}
/*---------------------------------------------------------------------------*/
