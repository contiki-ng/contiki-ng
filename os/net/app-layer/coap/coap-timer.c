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
 *         CoAP timer implementation.
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

/**
 * \addtogroup coap-timer
 * @{
 */

#include "coap-timer.h"
#include "lib/list.h"
#include "sys/cc.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap-timer"
#define LOG_LEVEL  LOG_LEVEL_NONE

LIST(timer_list);
static uint8_t is_initialized;
/*---------------------------------------------------------------------------*/
static void
add_timer(coap_timer_t *timer)
{
  coap_timer_t *n, *l, *p;

  if(!is_initialized) {
    /* The coap_timer system has not yet been initialized */
    coap_timer_init();
  }

  LOG_DBG("adding timer %p at %lu\n", timer,
          (unsigned long)timer->expiration_time);

  p = list_head(timer_list);

  /* Make sure the timer is not already added to the timer list */
  list_remove(timer_list, timer);

  for(l = NULL, n = list_head(timer_list); n != NULL; l = n, n = n->next) {
    if(timer->expiration_time < n->expiration_time) {
      list_insert(timer_list, l, timer);
      timer = NULL;
      break;
    }
  }

  if(timer != NULL) {
    list_insert(timer_list, l, timer);
  }

  if(p != list_head(timer_list)) {
    /* The next timer to expire has changed so we need to notify the driver */
    COAP_TIMER_DRIVER.update();
  }
}
/*---------------------------------------------------------------------------*/
void
coap_timer_stop(coap_timer_t *timer)
{
  LOG_DBG("stopping timer %p\n", timer);

  /* Mark timer as expired right now */
  timer->expiration_time = coap_timer_uptime();

  list_remove(timer_list, timer);
}
/*---------------------------------------------------------------------------*/
void
coap_timer_set(coap_timer_t *timer, uint64_t time)
{
  timer->expiration_time = coap_timer_uptime() + time;
  add_timer(timer);
}
/*---------------------------------------------------------------------------*/
void
coap_timer_reset(coap_timer_t *timer, uint64_t time)
{
  timer->expiration_time += time;
  add_timer(timer);
}
/*---------------------------------------------------------------------------*/
uint64_t
coap_timer_time_to_next_expiration(void)
{
  uint64_t now;
  coap_timer_t *next;

  next = list_head(timer_list);
  if(next == NULL) {
    /* No pending timers - return a time in the future */
    return 60000;
  }

  now = coap_timer_uptime();
  if(now < next->expiration_time) {
    return next->expiration_time - now;
  }
  /* The next timer should already have expired */
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_timer_run(void)
{
  uint64_t now;
  coap_timer_t *next;

  /* Always get the current time because it might trigger clock updates */
  now = coap_timer_uptime();

  next = list_head(timer_list);
  if(next == NULL) {
    /* No pending timers */
    return 0;
  }

  if(next->expiration_time <= now) {
    LOG_DBG("timer %p expired at %lu\n", next, (unsigned long)now);

    /* This timer should expire now */
    list_remove(timer_list, next);

    if(next->callback) {
      next->callback(next);
    }

    /* The next timer has changed */
    COAP_TIMER_DRIVER.update();

    /* Check if there is another pending timer */
    next = list_head(timer_list);
    if(next != NULL && next->expiration_time <= coap_timer_uptime()) {
      /* Need to run again */
      return 1;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
void
coap_timer_init(void)
{
  if(is_initialized) {
    return;
  }
  is_initialized = 1;
  list_init(timer_list);
  if(COAP_TIMER_DRIVER.init) {
    COAP_TIMER_DRIVER.init();
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
