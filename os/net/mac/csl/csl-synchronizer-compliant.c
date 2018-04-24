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
 * \addtogroup csl
 * @{
 * \file
 *         Learns wake-up times and assumes a worst-case clock drift
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/csl/csl-synchronizer-compliant.h"
#include "net/mac/csl/csl.h"
#include "net/mac/csl/csl-synchronizer.h"
#include "net/mac/csl/csl-framer.h"
#include "net/mac/csl/csl-nbr.h"
#include "net/nbr-table.h"
#include "services/akes/akes.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL-synchronizer-compliant"
#define LOG_LEVEL LOG_LEVEL_MAC
#define FRAME_CREATION_TIME (US_TO_RTIMERTICKS(4000))

#if !LLSEC802154_USES_FRAME_COUNTER
NBR_TABLE(struct csl_synchronizer_compliant_data, sync_data_table);
#endif /* !LLSEC802154_USES_FRAME_COUNTER */

/*---------------------------------------------------------------------------*/
static struct csl_synchronizer_compliant_data *
get_sync_data_of_receiver(void)
{
#if LLSEC802154_USES_FRAME_COUNTER
  struct akes_nbr_entry *entry;
  struct akes_nbr *nbr;

  entry = akes_nbr_get_receiver_entry();
  if(!entry) {
    return NULL;
  }
  if(akes_mac_is_helloack()) {
    nbr = entry->tentative;
  } else {
    nbr = entry->permanent;
  }
  if(!nbr) {
    return NULL;
  }
  return &csl_nbr_get(nbr)->sync_data;
#else /* LLSEC802154_USES_FRAME_COUNTER */
  return nbr_table_get_from_lladdr(sync_data_table,
      packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
#endif /* LLSEC802154_USES_FRAME_COUNTER */
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
#if !LLSEC802154_USES_FRAME_COUNTER
  nbr_table_register(sync_data_table, NULL);
#endif /* !LLSEC802154_USES_FRAME_COUNTER */
}
/*---------------------------------------------------------------------------*/
static int
schedule(void)
{
  struct csl_synchronizer_compliant_data *sync_data;
  uint32_t seconds_since_last_sync;
  rtimer_clock_t negative_uncertainty;
  rtimer_clock_t positive_uncertainty;

#if LLSEC802154_USES_FRAME_COUNTER
  if(akes_mac_is_helloack() || akes_mac_is_ack()) {
    sync_data = NULL;
  } else
#endif /* LLSEC802154_USES_FRAME_COUNTER */
  {
    sync_data = get_sync_data_of_receiver();
  }

  if(sync_data) {
    /* synchronized transmission */
    seconds_since_last_sync
        = RTIMERTICKS_TO_S(RTIMER_CLOCK_DIFF(RTIMER_NOW(), sync_data->t));
    negative_uncertainty = positive_uncertainty
        = ((seconds_since_last_sync * CSL_CLOCK_TOLERANCE * RTIMER_ARCH_SECOND)
            / (1000000)) + 1;
    negative_uncertainty += CSL_NEGATIVE_SYNC_GUARD_TIME;
    positive_uncertainty += CSL_POSITIVE_SYNC_GUARD_TIME;
    csl_state.transmit.wake_up_sequence_start
        = wake_up_counter_shift_to_future(sync_data->t - negative_uncertainty);
    while(rtimer_has_timed_out(csl_state.transmit.wake_up_sequence_start
        - FRAME_CREATION_TIME
        - CSL_WAKE_UP_SEQUENCE_GUARD_TIME)) {
      csl_state.transmit.wake_up_sequence_start += WAKE_UP_COUNTER_INTERVAL;
    }
    csl_state.transmit.remaining_wake_up_frames
        = CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH(
            negative_uncertainty + positive_uncertainty,
            csl_state.transmit.wake_up_frame_len);
    csl_state.transmit.payload_frame_start
        = csl_state.transmit.wake_up_sequence_start
            + RADIO_TIME_TO_TRANSMIT(
                (uint32_t)csl_state.transmit.remaining_wake_up_frames
                * csl_state.transmit.wake_up_frame_len
                * RADIO_SYMBOLS_PER_BYTE);
  } else {
    /* unsynchronized transmission */
    csl_state.transmit.wake_up_sequence_start = RTIMER_NOW()
        + FRAME_CREATION_TIME
        + CSL_WAKE_UP_SEQUENCE_GUARD_TIME;
    csl_state.transmit.remaining_wake_up_frames
        = CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH(
            WAKE_UP_COUNTER_INTERVAL, csl_state.transmit.wake_up_frame_len);
    csl_state.transmit.payload_frame_start
        = csl_state.transmit.wake_up_sequence_start
            + RADIO_TIME_TO_TRANSMIT(
                (uint32_t)csl_state.transmit.remaining_wake_up_frames
                * csl_state.transmit.wake_up_frame_len
                * RADIO_SYMBOLS_PER_BYTE);
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
  struct csl_synchronizer_compliant_data *sync_data;

  switch(csl_state.transmit.result[0]) {
  case MAC_TX_OK:
    sync_data = get_sync_data_of_receiver();
#if LLSEC802154_USES_FRAME_COUNTER
    if(!sync_data) {
      LOG_ERR("sync_data is NULL\n");
      return;
    }
#else /* LLSEC802154_USES_FRAME_COUNTER */
    if(!sync_data) {
      /* allocate memory for storing sync data about that neighbor */
      sync_data = nbr_table_add_lladdr(sync_data_table,
          packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
          NBR_TABLE_REASON_MAC,
          NULL);
      if(!sync_data) {
        LOG_WARN("nbr-table is full\n");
        return;
      }
    }
#endif /* LLSEC802154_USES_FRAME_COUNTER */
    /* update stored wake-up time */
    sync_data->t = csl_state.transmit.acknowledgement_sfd_timestamp
        - (WAKE_UP_COUNTER_INTERVAL - csl_state.transmit.acknowledgement_phase);
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
const struct csl_synchronizer csl_synchronizer_compliant = {
  init,
  schedule,
  on_unicast_transmitted
};
/*---------------------------------------------------------------------------*/

/** @} */
