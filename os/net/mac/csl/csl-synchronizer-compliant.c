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
 * \addtogroup csl
 * @{
 *
 * \file
 *         Learns wake-up times and assumes a worst-case clock drift
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/csl/csl-synchronizer-compliant.h"
#include "net/mac/csl/csl-framer.h"
#include "net/mac/csl/csl-nbr.h"
#include "net/mac/csl/csl-synchronizer.h"
#include "net/mac/csl/csl.h"
#include "net/nbr-table.h"
#include "services/akes/akes.h"
#include "sys/rtimer.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL-synchronizer-compliant"
#define LOG_LEVEL LOG_LEVEL_MAC

#if !LLSEC802154_USES_FRAME_COUNTER
NBR_TABLE(struct csl_synchronizer_compliant_data, sync_data_table);
#endif /* !LLSEC802154_USES_FRAME_COUNTER */

/*---------------------------------------------------------------------------*/
static struct csl_synchronizer_compliant_data *
get_sync_data_of_receiver(void)
{
#if LLSEC802154_USES_FRAME_COUNTER
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry) {
    return NULL;
  }
  akes_nbr_t *nbr;
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
    const rtimer_clock_t ticks_since_last_sync = RTIMER_NOW() - sync_data->t;
    rtimer_clock_t positive_uncertainty;
    rtimer_clock_t negative_uncertainty = positive_uncertainty =
        ((ticks_since_last_sync * CSL_CLOCK_TOLERANCE) / 1000000) + 1;
    negative_uncertainty += CSL_NEGATIVE_SYNC_GUARD_TIME;
    positive_uncertainty += CSL_POSITIVE_SYNC_GUARD_TIME;
    csl_state.transmit.wake_up_sequence_start =
        wake_up_counter_shift_to_future(sync_data->t - negative_uncertainty);
    while(!csl_can_schedule_wake_up_sequence()) {
      csl_state.transmit.wake_up_sequence_start += WAKE_UP_COUNTER_INTERVAL;
    }
    csl_state.transmit.remaining_wake_up_frames =
        CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH(
            negative_uncertainty + positive_uncertainty,
            csl_state.transmit.wake_up_frame_len);
    csl_state.transmit.payload_frame_start =
        csl_state.transmit.wake_up_sequence_start
        + RADIO_TIME_TO_TRANSMIT(
            (uint_fast32_t)csl_state.transmit.remaining_wake_up_frames
            * csl_state.transmit.wake_up_frame_len
            * RADIO_SYMBOLS_PER_BYTE);
  } else {
    /* unsynchronized transmission */
    csl_state.transmit.wake_up_sequence_start =
        RTIMER_NOW()
        + csl_max_frame_creation_time
        + CSL_WAKE_UP_SEQUENCE_GUARD_TIME;
    csl_state.transmit.remaining_wake_up_frames =
        CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH(
            WAKE_UP_COUNTER_INTERVAL,
            csl_state.transmit.wake_up_frame_len);
    csl_state.transmit.payload_frame_start =
        csl_state.transmit.wake_up_sequence_start
        + RADIO_TIME_TO_TRANSMIT(
            (uint_fast32_t)csl_state.transmit.remaining_wake_up_frames
            * csl_state.transmit.wake_up_frame_len
            * RADIO_SYMBOLS_PER_BYTE);
  }

  return MAC_TX_OK;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(bool successful, uint_fast8_t burst_index)
{
  if(!successful || burst_index) {
    return;
  }

  struct csl_synchronizer_compliant_data *sync_data =
      get_sync_data_of_receiver();
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
  sync_data->t =
      csl_state.transmit.acknowledgment_sfd_timestamp
      - (WAKE_UP_COUNTER_INTERVAL - csl_state.transmit.acknowledgment_phase);
}
/*---------------------------------------------------------------------------*/
const struct csl_synchronizer csl_synchronizer_compliant = {
  init,
  schedule,
  on_unicast_transmitted
};
/*---------------------------------------------------------------------------*/

/** @} */
