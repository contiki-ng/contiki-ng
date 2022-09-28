/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 *
 * This file is part of the Contiki operating system.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/**
 * \addtogroup etimer
 * @{
 */

/**
 * \file
 * Event timer library implementation.
 * \author
 * Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"

#include "sys/etimer.h"
#include "sys/process.h"

static struct etimer *next_etimer;
PROCESS(etimer_process, "Event timer");
/*---------------------------------------------------------------------------*/
static void
remove_etimer_from_list(struct etimer *previous, struct etimer *et)
{
  et->p = PROCESS_NONE;
  if(previous) {
    previous->next = et->next;
  } else {
    next_etimer = et->next;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(etimer_process, ev, data)
{
  struct etimer *current;
  struct etimer *previous;

  PROCESS_BEGIN();

  next_etimer = NULL;

  while(1) {
    PROCESS_YIELD();

    current = next_etimer;
    previous = NULL;

    if(ev == PROCESS_EVENT_EXITED) {
      while(current) {
        if(current->p == ((struct process *)data)) {
          remove_etimer_from_list(previous, current);
          current = previous ? previous->next : next_etimer;
          continue;
        }
        previous = current;
        current = current->next;
      }
    } else if(ev == PROCESS_EVENT_POLL) {
      while(current && timer_expired(&current->timer)) {
        if(process_post(current->p, PROCESS_EVENT_TIMER, current)
            == PROCESS_ERR_OK) {
          remove_etimer_from_list(previous, current);
          current = previous ? previous->next : next_etimer;
          continue;
        } else {
          /* retry later */
          etimer_request_poll();
        }
        previous = current;
        current = current->next;
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
etimer_request_poll(void)
{
  process_poll(&etimer_process);
}
/*---------------------------------------------------------------------------*/
static void
add_timer(struct etimer *et)
{
  clock_time_t expiration_time;
  struct etimer *current;
  struct etimer *previous;

  /* remove et from the list */
  etimer_stop(et);

  /* locate insertion point */
  expiration_time = etimer_expiration_time(et);
  current = next_etimer;
  previous = NULL;
  while(current
      && CLOCK_LT(etimer_expiration_time(current), expiration_time)) {
    previous = current;
    current = current->next;
  }

  /* insert et */
  et->p = PROCESS_CURRENT();
  if(previous) {
    et->next = previous->next;
    previous->next = et;
  } else {
    et->next = next_etimer;
    next_etimer = et;
    etimer_request_poll();
  }
}
/*---------------------------------------------------------------------------*/
void
etimer_set(struct etimer *et, clock_time_t interval)
{
  timer_set(&et->timer, interval);
  add_timer(et);
}
/*---------------------------------------------------------------------------*/
void
etimer_reset_with_new_interval(struct etimer *et, clock_time_t interval)
{
  timer_reset(&et->timer);
  et->timer.interval = interval;
  add_timer(et);
}
/*---------------------------------------------------------------------------*/
void
etimer_reset(struct etimer *et)
{
  timer_reset(&et->timer);
  add_timer(et);
}
/*---------------------------------------------------------------------------*/
void
etimer_restart(struct etimer *et)
{
  timer_restart(&et->timer);
  add_timer(et);
}
/*---------------------------------------------------------------------------*/
void
etimer_adjust(struct etimer *et, int timediff)
{
  et->timer.start += timediff;
  add_timer(et);
}
/*---------------------------------------------------------------------------*/
int
etimer_expired(struct etimer *et)
{
  return et->p == PROCESS_NONE;
}
/*---------------------------------------------------------------------------*/
clock_time_t
etimer_expiration_time(struct etimer *et)
{
  return et->timer.start + et->timer.interval;
}
/*---------------------------------------------------------------------------*/
clock_time_t
etimer_start_time(struct etimer *et)
{
  return et->timer.start;
}
/*---------------------------------------------------------------------------*/
int
etimer_pending(void)
{
  return next_etimer != NULL;
}
/*---------------------------------------------------------------------------*/
clock_time_t
etimer_next_expiration_time(void)
{
  return next_etimer ? etimer_expiration_time(next_etimer) : 0;
}
/*---------------------------------------------------------------------------*/
void
etimer_stop(struct etimer *et)
{
  struct etimer *current;
  struct etimer *previous;

  if(et->p == PROCESS_NONE) {
    return;
  }

  current = next_etimer;
  previous = NULL;
  while(current) {
    if(current == et) {
      remove_etimer_from_list(previous, current);
      break;
    }
    previous = current;
    current = current->next;
  }
  et->p = PROCESS_NONE;
}
/*---------------------------------------------------------------------------*/
bool
etimer_check_ordering(void)
{
  struct etimer *current;

  current = next_etimer;
  while(current && current->next) {
    if(CLOCK_LT(etimer_expiration_time(current->next),
        etimer_expiration_time(current))) {
      return false;
    }
    current = current->next;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
/** @} */
