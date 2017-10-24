/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/**
 * \file
 *         CoAP timer API.
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef COAP_TIMER_H_
#define COAP_TIMER_H_

#include "contiki.h"
#include <stdint.h>

typedef struct coap_timer coap_timer_t;
struct coap_timer {
  coap_timer_t *next;
  void (* callback)(coap_timer_t *);
  void *user_data;
  uint64_t expiration_time;
};

typedef struct {
  void     (* init)(void);
  uint64_t (* uptime)(void);
  void     (* update)(void);
} coap_timer_driver_t;

#ifndef COAP_TIMER_DRIVER
#ifdef COAP_TIMER_CONF_DRIVER
#define COAP_TIMER_DRIVER COAP_TIMER_CONF_DRIVER
#else /* COAP_TIMER_CONF_DRIVER */
#define COAP_TIMER_DRIVER coap_timer_default_driver
#endif /* COAP_TIMER_CONF_DRIVER */
#endif /* COAP_TIMER_DRIVER */

extern const coap_timer_driver_t COAP_TIMER_DRIVER;

/*
 * milliseconds since boot
 */
static inline uint64_t
coap_timer_uptime(void)
{
  return COAP_TIMER_DRIVER.uptime();
}

/*
 * seconds since boot
 */
static inline uint32_t
coap_timer_seconds(void)
{
  return (uint32_t)(COAP_TIMER_DRIVER.uptime() / 1000);
}

static inline void
coap_timer_set_callback(coap_timer_t *timer, void (* callback)(coap_timer_t *))
{
  timer->callback = callback;
}

static inline void *
coap_timer_get_user_data(coap_timer_t *timer)
{
  return timer->user_data;
}

static inline void
coap_timer_set_user_data(coap_timer_t *timer, void *data)
{
  timer->user_data = data;
}

static inline int
coap_timer_expired(const coap_timer_t *timer)
{
  return timer->expiration_time <= coap_timer_uptime();
}

void coap_timer_stop(coap_timer_t *timer);

void coap_timer_set(coap_timer_t *timer, uint64_t time);

/**
 * Set the CoAP timer to expire the specified time after the previous
 * expiration time. If the new expiration time has already passed, the
 * timer will expire as soon as possible.
 *
 * If the timer has not yet expired when this function is called, the
 * time until the timer expires will be extended by the specified time.
 */
void coap_timer_reset(coap_timer_t *timer, uint64_t time);

/**
 * Returns the time until next timer expires or 0 if there already
 * exists expired timers that have not yet been processed.
 * Returns a time in the future if there are no timers pending.
 */
uint64_t coap_timer_time_to_next_expiration(void);

/**
 * Must be called periodically to process any expired CoAP timers.
 * Returns non-zero if it needs to run again to process more timers.
 */
int coap_timer_run(void);

void coap_timer_init(void);

#endif /* COAP_TIMER_H_ */
