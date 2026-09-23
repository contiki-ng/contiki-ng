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

/**
 * \addtogroup akes
 * @{
 *
 * \file
 *         Deletes expired neighbors.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "akes/akes-delete.h"
#include "akes/akes.h"
#include "net/packetbuf.h"
#include "sys/clock.h"

#ifdef AKES_DELETE_CONF_CHECK_INTERVAL
#define CHECK_INTERVAL AKES_DELETE_CONF_CHECK_INTERVAL
#else /* AKES_DELETE_CONF_CHECK_INTERVAL */
#define CHECK_INTERVAL (1) /* seconds */
#endif /* AKES_DELETE_CONF_CHECK_INTERVAL */

#ifdef AKES_DELETE_CONF_WITH_UPDATES
#define WITH_UPDATES AKES_DELETE_CONF_WITH_UPDATES
#else /* AKES_DELETE_CONF_WITH_UPDATES */
#define WITH_UPDATES (1)
#endif /* AKES_DELETE_CONF_WITH_UPDATES */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-delete"
#define LOG_LEVEL LOG_LEVEL_MAC

#if AKES_NBR_WITH_SEQNOS || WITH_UPDATES
PROCESS(delete_process, "delete_process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(delete_process, ev, data)
{
  static struct etimer check_timer;

  PROCESS_BEGIN();

  while(1) {
    /* randomize the transmission time of UPDATEs to avoid collisions */
    etimer_set(&check_timer,
               (CHECK_INTERVAL * CLOCK_SECOND)
               - (CLOCK_SECOND / 2)
               + clock_random(CLOCK_SECOND));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&check_timer));

#if AKES_NBR_WITH_SEQNOS
    uint8_t now = clock_seconds();
#endif /* AKES_NBR_WITH_SEQNOS */
    for(akes_nbr_entry_t *entry = akes_nbr_head(AKES_NBR_PERMANENT);
        entry;
        entry = akes_nbr_next(entry, AKES_NBR_PERMANENT)) {
      akes_nbr_t *nbr = entry->permanent;
#if AKES_NBR_WITH_SEQNOS
      if(nbr->has_active_seqno
         && ((now - nbr->seqno_timestamp) > AKES_NBR_SEQNO_LIFETIME)) {
        nbr->has_active_seqno = false;
      }
#endif /* AKES_NBR_WITH_SEQNOS */
#if WITH_UPDATES
      if(!nbr->is_receiving_update
         && AKES_DELETE_STRATEGY.is_permanent_neighbor_expired(nbr)) {
        nbr->is_receiving_update = true;
        akes_send_update(entry);
      }
#endif /* WITH_UPDATES */
    }
  }

  PROCESS_END();
}
#endif /* AKES_NBR_WITH_SEQNOS || WITH_UPDATES */
/*---------------------------------------------------------------------------*/
void
akes_delete_on_update_sent(void *ptr, int status, int transmissions)
{
#if WITH_UPDATES
  if(status == MAC_TX_DEFERRED) {
    /* we expect another callback at a later point in time */
    return;
  }

  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_ERR("neighbor has gone\n");
    return;
  }

  if(!entry->permanent->is_receiving_update) {
    LOG_WARN("apparently a new session began\n");
    return;
  }

  akes_mac_report_to_network_layer(status, transmissions);

  if(AKES_DELETE_STRATEGY.is_permanent_neighbor_expired(entry->permanent)
     && (status != MAC_TX_QUEUE_FULL)) {
    LOG_INFO("deleting neighbor\n");
    akes_nbr_delete(entry, AKES_NBR_PERMANENT);
  } else {
    entry->permanent->is_receiving_update = false;
  }
#endif /* WITH_UPDATES */
}
/*---------------------------------------------------------------------------*/
void
akes_delete_init(void)
{
#if AKES_NBR_WITH_SEQNOS || WITH_UPDATES
  process_start(&delete_process, NULL);
#endif /* AKES_NBR_WITH_SEQNOS || WITH_UPDATES */
}
/*---------------------------------------------------------------------------*/
#if AKES_NBR_WITH_PROLONGATION_TIME
static bool
is_permanent_neighbor_expired(akes_nbr_t *nbr)
{
  uint16_t now = clock_seconds();
  return (now - nbr->prolongation_time) > AKES_NBR_LIFETIME;
}
/*---------------------------------------------------------------------------*/
static void
prolong_permanent_neighbor(akes_nbr_t *nbr)
{
  nbr->prolongation_time = clock_seconds();
}
/*---------------------------------------------------------------------------*/
const struct akes_delete_strategy akes_delete_strategy_default = {
  is_permanent_neighbor_expired,
  prolong_permanent_neighbor
};
#endif /* AKES_NBR_WITH_PROLONGATION_TIME */
/*---------------------------------------------------------------------------*/

/** @} */
