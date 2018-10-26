/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
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
 */

/**
 * \addtogroup akes
 * @{
 * \file
 *         Deletes expired neighbors.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "services/akes/akes-delete.h"
#include "services/akes/akes.h"
#include "net/packetbuf.h"

#ifdef AKES_DELETE_CONF_UPDATE_CHECK_INTERVAL
#define UPDATE_CHECK_INTERVAL AKES_DELETE_CONF_UPDATE_CHECK_INTERVAL
#else /* AKES_DELETE_CONF_UPDATE_CHECK_INTERVAL */
#define UPDATE_CHECK_INTERVAL (1) /* seconds */
#endif /* AKES_DELETE_CONF_UPDATE_CHECK_INTERVAL */

#ifdef AKES_DELETE_CONF_ENABLED
#define ENABLED AKES_DELETE_CONF_ENABLED
#else /* AKES_DELETE_CONF_ENABLED */
#define ENABLED 1
#endif /* AKES_DELETE_CONF_ENABLED */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-delete"
#define LOG_LEVEL LOG_LEVEL_MAC

#if ENABLED
PROCESS(delete_process, "delete_process");

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(delete_process, ev, data)
{
  static struct etimer update_check_timer;
  struct akes_nbr_entry *next;

  PROCESS_BEGIN();

  while(1) {
    /* randomize the transmission time of UPDATEs to avoid collisions */
    etimer_set(&update_check_timer, akes_mac_random_clock_time(
        (UPDATE_CHECK_INTERVAL * CLOCK_SECOND) - (CLOCK_SECOND / 2),
        (UPDATE_CHECK_INTERVAL * CLOCK_SECOND) + (CLOCK_SECOND / 2)));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&update_check_timer));
    next = akes_nbr_head();
    while(next) {
      if(!next->permanent
          || !AKES_DELETE_STRATEGY.is_nbr_expired(next, AKES_NBR_PERMANENT)
          || next->permanent->is_receiving_update) {
        next = akes_nbr_next(next);
        continue;
      }

      /* send UPDATE */
      akes_send_update(next);
      next->permanent->is_receiving_update = 1;
      PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
      next = akes_nbr_head();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void
akes_delete_on_update_sent(void *ptr, int status, int transmissions)
{
  struct akes_nbr_entry *entry;

  entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_ERR("neighbor has gone\n");
    return;
  }

  if(AKES_DELETE_STRATEGY.is_nbr_expired(entry, AKES_NBR_PERMANENT)) {
    LOG_INFO("deleting neighbor\n");
    akes_nbr_delete(entry, AKES_NBR_PERMANENT);
  } else {
    entry->permanent->is_receiving_update = 0;
  }

  process_poll(&delete_process);
}
/*---------------------------------------------------------------------------*/
void
akes_delete_init(void)
{
  process_start(&delete_process, NULL);
}
/*---------------------------------------------------------------------------*/
#else /* ENABLED */
void
akes_delete_on_update_sent(void *ptr, int status, int transmissions)
{

}
/*---------------------------------------------------------------------------*/
void
akes_delete_init(void)
{

}
#endif /* ENABLED */
/*---------------------------------------------------------------------------*/
void
akes_delete_expired_tentatives(void)
{
  struct akes_nbr_entry *next;
  struct akes_nbr_entry *current;

  next = akes_nbr_head();
  while(next) {
    current = next;
    next = akes_nbr_next(current);
    if(current->tentative
        && AKES_DELETE_STRATEGY.is_nbr_expired(current, AKES_NBR_TENTATIVE)) {
      akes_nbr_delete(current, AKES_NBR_TENTATIVE);
    }
  }
}
/*---------------------------------------------------------------------------*/
#ifndef AKES_DELETE_CONF_STRATEGY
static int
is_nbr_expired(struct akes_nbr_entry *entry, enum akes_nbr_status status)
{
  return entry->refs[status]->expiration_time < clock_seconds();
}
/*---------------------------------------------------------------------------*/
static void
prolong_tentative(struct akes_nbr *tentative, uint16_t seconds)
{
  tentative->expiration_time = clock_seconds() + seconds;
}
/*---------------------------------------------------------------------------*/
static void
prolong_permanent(struct akes_nbr *permanent)
{
  permanent->expiration_time = clock_seconds() + AKES_NBR_LIFETIME;
}
/*---------------------------------------------------------------------------*/
const struct akes_delete_strategy akes_delete_strategy_default = {
  is_nbr_expired,
  prolong_tentative,
  prolong_permanent
};
#endif /* AKES_DELETE_CONF_STRATEGY */
/*---------------------------------------------------------------------------*/

/** @} */
