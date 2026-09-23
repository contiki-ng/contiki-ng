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
 *         Learns wake-up times, as well as long-term clock drifts
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/csl/csl-synchronizer-splo.h"
#include "net/mac/csl/csl.h"
#include "net/mac/csl/csl-framer.h"
#include "net/mac/csl/csl-nbr.h"
#include "net/mac/csl/csl-synchronizer.h"
#include "net/mac/mac.h"
#include "sys/rtimer.h"
#include <stdint.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL-synchronizer-splo"
#define LOG_LEVEL LOG_LEVEL_MAC

#if !CSL_COMPLIANT

/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static int
schedule(void)
{
  if(csl_state.transmit.subtype == CSL_SUBTYPE_HELLO) {
    /* the transmission of a HELLO's SHR has to coincide with a wake up */
    csl_state.transmit.payload_frame_start = csl_get_last_wake_up_time()
                                             - RADIO_SHR_TIME
                                             + (WAKE_UP_COUNTER_INTERVAL / 2);
    csl_state.transmit.remaining_wake_up_frames =
        csl_hello_wake_up_sequence_length;
    do {
      csl_state.transmit.payload_frame_start += WAKE_UP_COUNTER_INTERVAL;
      csl_state.transmit.wake_up_sequence_start =
          csl_state.transmit.payload_frame_start
          - csl_hello_wake_up_sequence_tx_time;
    } while(!csl_can_schedule_wake_up_sequence());
  } else {
    csl_nbr_t *csl_nbr = csl_nbr_get_receiver();
    if(!csl_nbr) {
      LOG_ERR("receiver not found\n");
      return MAC_TX_ERR_FATAL;
    }
    const struct csl_synchronizer_splo_data *const sync_data =
        &csl_nbr->sync_data;
    int32_t drift;
    if(csl_state.transmit.subtype == CSL_SUBTYPE_HELLOACK) {
      drift = AKES_NBR_UNINITIALIZED_DRIFT;
    } else {
      drift = csl_nbr->drift;
    }

    /* calculate uncertainty */
    const rtimer_clock_t ticks_since_last_sync = RTIMER_NOW() - sync_data->t;
    rtimer_clock_t positive_uncertainty;
    rtimer_clock_t negative_uncertainty = positive_uncertainty =
        (ticks_since_last_sync
         * (drift == AKES_NBR_UNINITIALIZED_DRIFT
            ? CSL_CLOCK_TOLERANCE
            : CSL_COMPENSATION_TOLERANCE)
         / 1000000)
        + 1;
    negative_uncertainty += CSL_NEGATIVE_SYNC_GUARD_TIME;
    positive_uncertainty += CSL_POSITIVE_SYNC_GUARD_TIME;

    /* compensate for clock drift if known */
    int32_t compensation;
    if(drift == AKES_NBR_UNINITIALIZED_DRIFT) {
      compensation = 0;
    } else {
      compensation = ((int64_t)drift * (int64_t)ticks_since_last_sync)
                     / csl_drift_unit;
      LOG_DBG("compensated ticks: %" PRIi32 "\n", compensation);
    }

    /* set variables */
    csl_state.transmit.wake_up_sequence_start =
        wake_up_counter_shift_to_future(sync_data->t
                                        + compensation
                                        - negative_uncertainty);
    while(!csl_can_schedule_wake_up_sequence()) {
      csl_state.transmit.wake_up_sequence_start += WAKE_UP_COUNTER_INTERVAL;
    }
    uint_fast16_t proposed_channels =
        CSL_CHANNEL_SELECTOR.propose_channels(csl_nbr);
    while(1) {
      csl_state.transmit.receivers_wake_up_counter.u32 =
          sync_data->his_wake_up_counter_at_t.u32
          + wake_up_counter_round_increments(
              (csl_state.transmit.wake_up_sequence_start
               - compensation
               + negative_uncertainty)
              - sync_data->t);
      uint_fast8_t forecast_channel_index =
          csl_forecast_channel_index(
              csl_state.transmit.receivers_wake_up_counter,
              packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
      if(proposed_channels & (1 << forecast_channel_index)) {
        break;
      }
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
  }
  return MAC_TX_OK;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(bool successful, uint_fast8_t burst_index)
{
  if(burst_index
     || !successful
     || (csl_state.transmit.subtype == CSL_SUBTYPE_HELLOACK)) {
    return;
  }

  csl_nbr_t *csl_nbr = csl_nbr_get_receiver();
  if(!csl_nbr) {
    LOG_ERR("receiver not found\n");
    return;
  }

  struct csl_synchronizer_splo_data new_sync_data;
  new_sync_data.his_wake_up_counter_at_t =
      csl_state.transmit.receivers_wake_up_counter;
  new_sync_data.t =
      csl_state.transmit.acknowledgment_sfd_timestamp
      - (WAKE_UP_COUNTER_INTERVAL - csl_state.transmit.acknowledgment_phase);
  if(csl_state.transmit.subtype == CSL_SUBTYPE_ACK) {
    csl_nbr->historical_sync_data = new_sync_data;
  } else {
    const rtimer_clock_t ticks_since_historical_sync =
        new_sync_data.t - csl_nbr->historical_sync_data.t;
    if(ticks_since_historical_sync >= CSL_MIN_TIME_BETWEEN_DRIFT_UPDATES
                                      * RTIMER_SECOND) {
      const rtimer_clock_t expected_diff =
          WAKE_UP_COUNTER_INTERVAL
          * (new_sync_data.his_wake_up_counter_at_t.u32
             - csl_nbr->historical_sync_data.his_wake_up_counter_at_t.u32);
      const rtimer_clock_t actual_diff =
          new_sync_data.t - csl_nbr->historical_sync_data.t;
      const int64_t offset = (int64_t)actual_diff - (int64_t)expected_diff;
      int64_t drift = (offset * csl_drift_unit)
                      / (int64_t)ticks_since_historical_sync;
      csl_nbr->drift = drift;
      LOG_DBG("computed drift: %" PRIi32 "\n", csl_nbr->drift);
      csl_nbr->historical_sync_data = csl_nbr->sync_data;
    }
  }

  csl_nbr->sync_data = new_sync_data;
}
/*---------------------------------------------------------------------------*/
const struct csl_synchronizer csl_synchronizer_splo = {
  init,
  schedule,
  on_unicast_transmitted
};
/*---------------------------------------------------------------------------*/
#endif /* !CSL_COMPLIANT */

/** @} */
