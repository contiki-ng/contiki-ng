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
 *         Learns wake-up times, as well as long-term clock drifts
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "contiki.h"
#include "net/mac/csl/csl-synchronizer.h"
#include "net/mac/csl/csl-synchronizer-splo.h"
#include "net/mac/csl/csl-framer.h"
#include "net/mac/csl/csl-nbr.h"
#include "net/mac/mac.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL-synchronizer-splo"
#define LOG_LEVEL LOG_LEVEL_MAC

#define FRAME_CREATION_TIME (US_TO_RTIMERTICKS(1000))

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
  struct akes_nbr_entry *entry;
  csl_nbr_t *csl_nbr;
  int32_t drift;
  uint32_t seconds_since_last_sync;
  rtimer_clock_t negative_uncertainty;
  rtimer_clock_t positive_uncertainty;
  struct csl_synchronizer_splo_data *sync_data;
  int32_t compensation;

  if(csl_state.transmit.subtype == CSL_SUBTYPE_HELLO) {
    /* the transmission of a HELLO's SHR has to coincide with a wake up */
    csl_state.transmit.payload_frame_start = csl_get_last_wake_up_time()
        - RADIO_SHR_TIME
        + (WAKE_UP_COUNTER_INTERVAL / 2);
    csl_state.transmit.remaining_wake_up_frames = csl_hello_wake_up_sequence_length;
    do {
      csl_state.transmit.payload_frame_start += WAKE_UP_COUNTER_INTERVAL;
      csl_state.transmit.wake_up_sequence_start = csl_state.transmit.payload_frame_start
          - csl_hello_wake_up_sequence_tx_time;
    } while(rtimer_has_timed_out(csl_state.transmit.wake_up_sequence_start
        - FRAME_CREATION_TIME
        - CSL_WAKE_UP_SEQUENCE_GUARD_TIME));
  } else {
    entry = akes_nbr_get_receiver_entry();
    if(!entry) {
      LOG_ERR("receiver has gone\n");
      return 0;
    }

    switch(csl_state.transmit.subtype) {
    case CSL_SUBTYPE_HELLOACK:
      if(!entry->tentative) {
        LOG_ERR("tentative neighbor not present\n");
        return 0;
      }
      csl_nbr = csl_nbr_get(entry->tentative);
      sync_data = &csl_nbr->sync_data;
      drift = AKES_NBR_UNINITIALIZED_DRIFT;
      break;
    default:
      if(!entry->permanent) {
        LOG_ERR("permanent neighbor not present\n");
        return 0;
      }
      csl_nbr = csl_nbr_get(entry->permanent);
      sync_data = &csl_nbr->sync_data;
      drift = csl_nbr->drift;
      break;
    }

    /* calculate uncertainty */
    seconds_since_last_sync = RTIMERTICKS_TO_S(RTIMER_CLOCK_DIFF(RTIMER_NOW(), sync_data->t));
    negative_uncertainty = positive_uncertainty =
        ((seconds_since_last_sync
        * ((drift == AKES_NBR_UNINITIALIZED_DRIFT)
            ? CSL_CLOCK_TOLERANCE
            : CSL_COMPENSATION_TOLERANCE)
        * RTIMER_ARCH_SECOND) / (1000000)) + 1;
    negative_uncertainty += CSL_NEGATIVE_SYNC_GUARD_TIME;
    positive_uncertainty += CSL_POSITIVE_SYNC_GUARD_TIME;

    /* compensate for clock drift if known */
    if(drift == AKES_NBR_UNINITIALIZED_DRIFT) {
      compensation = 0;
    } else {
      compensation = ((int64_t)drift * (int64_t)seconds_since_last_sync / (int64_t)1000000);
    }

    /* set variables */
    csl_state.transmit.wake_up_sequence_start = wake_up_counter_shift_to_future(sync_data->t + compensation - negative_uncertainty);
    while(rtimer_has_timed_out(csl_state.transmit.wake_up_sequence_start
        - FRAME_CREATION_TIME
        - CSL_WAKE_UP_SEQUENCE_GUARD_TIME)) {
      csl_state.transmit.wake_up_sequence_start += WAKE_UP_COUNTER_INTERVAL;
    }
    csl_state.transmit.remaining_wake_up_frames = CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH(
        negative_uncertainty + positive_uncertainty,
        csl_state.transmit.wake_up_frame_len);
    csl_state.transmit.payload_frame_start = csl_state.transmit.wake_up_sequence_start
        + RADIO_TIME_TO_TRANSMIT(
            (uint32_t)csl_state.transmit.remaining_wake_up_frames
            * csl_state.transmit.wake_up_frame_len
            * RADIO_SYMBOLS_PER_BYTE);
    csl_state.transmit.receivers_wake_up_counter.u32 = sync_data->his_wake_up_counter_at_t.u32
        + wake_up_counter_round_increments(
            RTIMER_CLOCK_DIFF(csl_state.transmit.wake_up_sequence_start - compensation + negative_uncertainty, sync_data->t));
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
  struct akes_nbr_entry *entry;
  csl_nbr_t *csl_nbr;
  uint32_t seconds_since_historical_sync;
  struct csl_synchronizer_splo_data new_sync_data;
  rtimer_clock_t expected_diff;
  rtimer_clock_t actual_diff;

  if(csl_state.transmit.result[0] != MAC_TX_OK) {
    return;
  }

  entry = akes_nbr_get_receiver_entry();
  switch(csl_state.transmit.subtype) {
  case CSL_SUBTYPE_ACK:
  case CSL_SUBTYPE_NORMAL:
    if(!entry || !entry->permanent) {
      LOG_ERR("receiver not found\n");
    } else {
      new_sync_data.his_wake_up_counter_at_t = csl_state.transmit.receivers_wake_up_counter;
      new_sync_data.t = csl_state.transmit.acknowledgement_sfd_timestamp
          - (WAKE_UP_COUNTER_INTERVAL - csl_state.transmit.acknowledgement_phase);

      csl_nbr = csl_nbr_get(entry->permanent);
      if(csl_state.transmit.subtype == CSL_SUBTYPE_ACK) {
        csl_nbr->historical_sync_data = new_sync_data;
      } else {
        seconds_since_historical_sync = RTIMERTICKS_TO_S(
            RTIMER_CLOCK_DIFF(new_sync_data.t, csl_nbr->historical_sync_data.t));
        if(seconds_since_historical_sync >= CSL_MIN_TIME_BETWEEN_DRIFT_UPDATES) {
          expected_diff = WAKE_UP_COUNTER_INTERVAL
              * (new_sync_data.his_wake_up_counter_at_t.u32
              - csl_nbr->historical_sync_data.his_wake_up_counter_at_t.u32);
          actual_diff = new_sync_data.t - csl_nbr->historical_sync_data.t;
          csl_nbr->drift =
            (((int64_t)actual_diff - (int64_t)expected_diff) * (int64_t)1000000)
            / seconds_since_historical_sync;
          csl_nbr->historical_sync_data = csl_nbr->sync_data;
        }
      }

      csl_nbr->sync_data = new_sync_data;
    }
    break;
  default:
    break;
  }
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
