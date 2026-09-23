/*
 * Copyright (c) 2018, Hasso-Plattner-Institut.
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
 * \file
 *         ContikiMAC's original phase-lock optimization.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/linkaddr.h"
#include "net/mac/contikimac/contikimac-nbr.h"
#include "net/mac/contikimac/contikimac-synchronizer.h"
#include "net/mac/mac.h"
#include "net/nbr-table.h"
#include "net/packetbuf.h"
#include "services/akes/akes-nbr.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ContikiMAC-Phase-Lock"
#define LOG_LEVEL LOG_LEVEL_MAC

#define PHASE_LOCK_GUARD_TIME (US_TO_RTIMERTICKS(1000))
#define MAX_FAIL_STREAK (10)

#if CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK

#if !AKES_MAC_ENABLED
NBR_TABLE(struct contikimac_phase, sync_data_table);
#endif /* !AKES_MAC_ENABLED */
/*---------------------------------------------------------------------------*/
static void
init(void)
{
#if !AKES_MAC_ENABLED
  nbr_table_register(sync_data_table, NULL);
#endif /* !AKES_MAC_ENABLED */
}
/*---------------------------------------------------------------------------*/
static struct contikimac_phase *
obtain_phase_lock_data(void)
{
#if AKES_MAC_ENABLED
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry) {
    LOG_ERR("no entry found\n");
    return NULL;
  }
  akes_nbr_t *nbr;
  if(akes_mac_is_helloack()) {
    nbr = entry->tentative;
  } else {
    nbr = entry->permanent;
  }
  if(!nbr) {
    LOG_ERR("could not obtain phase-lock data\n");
    return NULL;
  }
  contikimac_nbr_t *contikimac_nbr = contikimac_nbr_get(nbr);
  if(akes_mac_is_helloack() || akes_mac_is_ack()) {
    contikimac_nbr->phase.t0 = contikimac_nbr->phase.t1 = 0;
  }
  return &contikimac_nbr->phase;
#else /* AKES_MAC_ENABLED */
  struct contikimac_phase *cp;
  const linkaddr_t *address;

  address = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  cp = nbr_table_get_from_lladdr(sync_data_table, address);
  if(!cp) {
    cp = nbr_table_add_lladdr(sync_data_table,
                              address,
                              NBR_TABLE_REASON_MAC,
                              NULL);
    if(!cp) {
      LOG_WARN("nbr-table is full\n");
    } else {
      cp->t0 = cp->t1 = 0;
      cp->fail_streak = 0;
    }
  }
  return cp;
#endif /* AKES_MAC_ENABLED */
}
/*---------------------------------------------------------------------------*/
static int
schedule(void)
{
  if(contikimac_state.strobe.is_broadcast) {
    contikimac_synchronizer_strobe_soon();
  } else {
    struct contikimac_phase *phase = obtain_phase_lock_data();
    if(!phase) {
      return MAC_TX_ERR_FATAL;
    }
    if(phase->t0 == phase->t1) {
      /* no phase-lock information stored, yet */
      contikimac_synchronizer_strobe_soon();
    } else {
      contikimac_state.strobe.next_transmission =
          wake_up_counter_shift_to_future(phase->t0 - PHASE_LOCK_GUARD_TIME);
      contikimac_state.strobe.timeout =
          contikimac_state.strobe.next_transmission
          + (phase->t1 - phase->t0)
          + PHASE_LOCK_GUARD_TIME;
    }
  }
  return MAC_TX_OK;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
  struct contikimac_phase *phase = obtain_phase_lock_data();
  if(!phase) {
    return;
  }

  if(contikimac_state.strobe.result == MAC_TX_OK) {
    phase->t0 = contikimac_get_last_but_one_t0();
    phase->t1 = contikimac_get_last_but_one_t1();
    phase->fail_streak = 0;
  } else {
    phase->fail_streak++;
    if(phase->fail_streak == MAX_FAIL_STREAK) {
      LOG_WARN("deleting sync data of ");
      LOG_LLADDR(LOG_LEVEL_WARN, packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
      LOG_WARN_("\n");
      phase->t0 = phase->t1 = 0;
      phase->fail_streak = 0;
    }
  }
}
/*---------------------------------------------------------------------------*/
const struct contikimac_synchronizer contikimac_synchronizer_original = {
  init,
  schedule,
  on_unicast_transmitted,
};
/*---------------------------------------------------------------------------*/
#endif /* CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK */
