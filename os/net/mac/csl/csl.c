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
 *         Coordinated Sampled Listening (CSL)
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "contiki.h"
#include "net/mac/csl/csl.h"
#include "dev/crypto/cc/cc-crypto.h"
#include "lib/aes-128.h"
#include "lib/assert.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "net/mac/csl/csl-ccm-inputs.h"
#include "net/mac/csl/csl-framer-compliant.h"
#include "net/mac/csl/csl-framer-potr.h"
#include "net/mac/csl/csl-framer.h"
#include "net/mac/csl/csl-nbr.h"
#include "net/mac/csl/csl-strategy.h"
#include "net/mac/csl/csl-synchronizer.h"
#include "net/mac/frame-queue.h"
#include "net/mac/mac-sequence.h"
#include "net/mac/mac.h"
#include "net/nbr-table.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "services/akes/akes-mac.h"
#include "services/akes/akes.h"
#include "sys/atomic.h"
#include <stddef.h>

#if CSL_COMPLIANT
#define SHALL_SKIP_TO_RENDEZVOUS 0
#else /* CSL_COMPLIANT */
#define SHALL_SKIP_TO_RENDEZVOUS csl_state.duty_cycle.skip_to_rendezvous
#endif /* CSL_COMPLIANT */

#ifdef CSL_CONF_OUTPUT_POWER
#define OUTPUT_POWER CSL_CONF_OUTPUT_POWER
#else /* CSL_CONF_OUTPUT_POWER */
#define OUTPUT_POWER (0)
#endif /* CSL_CONF_OUTPUT_POWER */

#ifdef CSL_CONF_CCA_THRESHOLD
#define CCA_THRESHOLD CSL_CONF_CCA_THRESHOLD
#else /* CSL_CONF_CCA_THRESHOLD */
#ifdef CSL_CONF_NO_CCA
#define CCA_THRESHOLD (0)
#else /* CSL_CONF_NO_CCA */
#define CCA_THRESHOLD (-81)
#endif /* CSL_CONF_NO_CCA */
#endif /* CSL_CONF_CCA_THRESHOLD */

#ifdef CSL_CONF_MIN_PREPARE_LEAD_OVER_LOOP
#define MIN_PREPARE_LEAD_OVER_LOOP CSL_CONF_MIN_PREPARE_LEAD_OVER_LOOP
#else /* CSL_CONF_MIN_PREPARE_LEAD_OVER_LOOP */
#if CONTIKI_TARGET_COOJA
/* I guess I hardcoded this somewhere in Cooja */
#define MIN_PREPARE_LEAD_OVER_LOOP (10)
#else /* CONTIKI_TARGET_COOJA */
#define MIN_PREPARE_LEAD_OVER_LOOP (15)
#endif /* CONTIKI_TARGET_COOJA */
#endif /* CSL_CONF_MIN_PREPARE_LEAD_OVER_LOOP */

#ifdef CSL_CONF_MIN_PREPARE_LEAD_OVER_LOOP_MID
#define MIN_PREPARE_LEAD_OVER_LOOP_MID CSL_CONF_MIN_PREPARE_LEAD_OVER_LOOP_MID
#else /* CSL_CONF_MIN_PREPARE_LEAD_OVER_LOOP_MID */
#if CONTIKI_TARGET_COOJA
/* I guess I hardcoded this somewhere in Cooja */
#define MIN_PREPARE_LEAD_OVER_LOOP_MID (MIN_PREPARE_LEAD_OVER_LOOP / 2)
#else /* CONTIKI_TARGET_COOJA */
#define MIN_PREPARE_LEAD_OVER_LOOP_MID ((MIN_PREPARE_LEAD_OVER_LOOP / 2) + 1)
#endif /* CONTIKI_TARGET_COOJA */
#endif /* CSL_CONF_MIN_PREPARE_LEAD_OVER_LOOP_MID */

#define NEGATIVE_RENDEZVOUS_TIME_ACCURACY (2)
#define POSITIVE_RENDEZVOUS_TIME_ACCURACY (2)
#define RENDEZVOUS_GUARD_TIME \
  (CSL_LPM_SWITCHING \
   + NEGATIVE_RENDEZVOUS_TIME_ACCURACY \
   + RADIO_RECEIVE_CALIBRATION_TIME)
#define SCAN_DURATION \
  (RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE \
                          * (CSL_MAX_WAKE_UP_FRAME_LEN + RADIO_SHR_LEN)) \
   + 2)
#define CCA_SLEEP_DURATION \
  (RADIO_RECEIVE_CALIBRATION_TIME + RADIO_CCA_TIME - 3)
#define MAX_TRANSMISSION_TIME \
  (RADIO_TRANSMIT_CALIBRATION_TIME \
   + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE \
                            * (RADIO_MAX_PAYLOAD + RADIO_SHR_LEN)) \
   + RADIO_RECEIVE_CALIBRATION_TIME \
   + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE \
                            * (CSL_MAX_ACKNOWLEDGMENT_LEN + RADIO_SHR_LEN)))

#if !CSL_COMPLIANT
/**
 * Structure for caching information about wake-up frames with extremely late
 * rendezvous times. It enables us to do something else in the meantime.
 */
struct late_rendezvous {
  struct late_rendezvous *next;
  rtimer_clock_t time;
  enum csl_subtype subtype;
  uint_fast8_t channel_index;
};
#define LATE_WAKE_UP_GUARD_TIME (WAKE_UP_COUNTER_INTERVAL / 2)
#define LATE_RENDEZVOUS_TRESHOLD (US_TO_RTIMERTICKS(20000))

#define HELLO_WAKE_UP_SEQUENCE_LENGTH \
  CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH(WAKE_UP_COUNTER_INTERVAL \
                                     * CSL_CHANNELS_COUNT, \
                                     CSL_FRAMER_POTR_HELLO_WAKE_UP_FRAME_LEN)
#define CSL_HELLO_WAKE_UP_SEQUENCE_TX_TIME \
  RADIO_TIME_TO_TRANSMIT((uint64_t)HELLO_WAKE_UP_SEQUENCE_LENGTH \
                         * CSL_FRAMER_POTR_HELLO_WAKE_UP_FRAME_LEN \
                         * RADIO_SYMBOLS_PER_BYTE)
#endif /* !CSL_COMPLIANT */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL"
#define LOG_LEVEL LOG_LEVEL_MAC

static void schedule_duty_cycle(rtimer_clock_t time);
static int schedule_duty_cycle_precise(rtimer_clock_t time);
static void duty_cycle_wrapper(struct rtimer *t, void *ptr);
static PT_THREAD(duty_cycle(void));
static void on_shr(void);
static void on_wake_up_frame_fifop(void);
static void on_payload_frame_fifop(void);
static void on_final_payload_frame_fifop(void);
static void on_txdone(void);
static uint_fast16_t prepare_next_wake_up_frames(uint_fast16_t space);
static void schedule_transmission(rtimer_clock_t time);
static int schedule_transmission_precise(rtimer_clock_t time);
static void transmit_wrapper(struct rtimer *rt, void *ptr);
static PT_THREAD(transmit(void));
static void on_transmitted(void);

csl_state_t csl_state;
rtimer_clock_t csl_max_frame_creation_time =
    US_TO_RTIMERTICKS(2000)
    + US_TO_RTIMERTICKS((CSL_MAX_BURST_INDEX + 1) * 1300);
static const uint8_t channels[] = CSL_CHANNELS;
static uint_fast8_t current_channel_index;
static struct rtimer timer;
static rtimer_clock_t last_wake_up_time;
static struct pt pt;
static bool is_duty_cycling;
static bool is_transmitting;
static bool can_skip;
static uint8_t skip_lock;
PROCESS(post_processing, "post processing");
static rtimer_clock_t last_payload_frame_sfd_timestamp;
#if !CSL_COMPLIANT
MEMB(late_rendezvous_memb, struct late_rendezvous, CSL_CHANNELS_COUNT);
LIST(late_rendezvous_list);
wake_up_counter_t csl_wake_up_counter;
const uint_fast16_t csl_hello_wake_up_sequence_length =
    HELLO_WAKE_UP_SEQUENCE_LENGTH;
const rtimer_clock_t csl_hello_wake_up_sequence_tx_time =
    CSL_HELLO_WAKE_UP_SEQUENCE_TX_TIME;
const int64_t csl_drift_unit = INT64_C(1) << 40;
#endif /* !CSL_COMPLIANT */

/*---------------------------------------------------------------------------*/
#if !CSL_COMPLIANT
static bool
is_anything_locked(void)
{
  return !ccm_star_can_use_asynchronously()
         || !akes_nbr_can_query_asynchronously();
}
/*---------------------------------------------------------------------------*/
uint_fast8_t
csl_forecast_channel_index(wake_up_counter_t wuc, const linkaddr_t *addr)
{
  uint_fast8_t xored = wuc.u8[0];
  for(size_t i = 0; i < LINKADDR_SIZE; i++) {
    xored ^= addr->u8[i];
  }
  return xored & (CSL_CHANNELS_COUNT - 1);
}
/*---------------------------------------------------------------------------*/
wake_up_counter_t
csl_get_wake_up_counter(rtimer_clock_t t)
{
  rtimer_clock_t delta = t - csl_get_last_wake_up_time();
  wake_up_counter_t wuc = csl_wake_up_counter;
  wuc.u32 += wake_up_counter_increments(delta, NULL);
  return wuc;
}
/*---------------------------------------------------------------------------*/
wake_up_counter_t
csl_predict_wake_up_counter(void)
{
  return csl_state.transmit.receivers_wake_up_counter;
}
/*---------------------------------------------------------------------------*/
wake_up_counter_t
csl_restore_wake_up_counter(void)
{
  const akes_nbr_entry_t *entry = akes_nbr_get_sender_entry();
  if(!entry || !entry->permanent) {
    wake_up_counter_t wuc = {
      wuc.u32 = 0
    };
    LOG_ERR("could not restore wake-up counter\n");
    return wuc;
  }

  const csl_nbr_t *csl_nbr = csl_nbr_get(entry->permanent);
  const int32_t drift = csl_nbr->drift;
  int32_t compensation;
  if(drift == AKES_NBR_UNINITIALIZED_DRIFT) {
    compensation = 0;
  } else {
    const rtimer_clock_t ticks_since_last_sync =
        csl_get_sfd_timestamp_of_last_payload_frame() - csl_nbr->sync_data.t;
    compensation = ((int64_t)drift * (int64_t)ticks_since_last_sync)
                   / csl_drift_unit;
    LOG_DBG("compensated ticks: %" PRIi32 "\n", compensation);
  }

  const rtimer_clock_t delta = csl_get_sfd_timestamp_of_last_payload_frame()
                               - csl_nbr->sync_data.t
                               + compensation
                               - (WAKE_UP_COUNTER_INTERVAL / 2);
  wake_up_counter_t wuc = {
    wuc.u32 = csl_nbr->sync_data.his_wake_up_counter_at_t.u32
              + wake_up_counter_round_increments(delta)
  };
  return wuc;
}
/*---------------------------------------------------------------------------*/
static void
delete_late_rendezvous(struct late_rendezvous *lr)
{
  list_remove(late_rendezvous_list, lr);
  if(memb_free(&late_rendezvous_memb, lr) == -1) {
    assert(false);
  }
}
/*---------------------------------------------------------------------------*/
static void
clear_missed_late_rendezvous(void)
{
  struct late_rendezvous *next = list_head(late_rendezvous_list);
  while(next) {
    struct late_rendezvous *current = next;
    next = list_item_next(current);
    if(rtimer_has_timed_out(current->time
                            - RENDEZVOUS_GUARD_TIME
                            - (CSL_LPM_DEEP_SWITCHING - CSL_LPM_SWITCHING))) {
      delete_late_rendezvous(current);
      LOG_ERR("forgot late rendezvous\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static struct late_rendezvous *
get_nearest_late_rendezvous(void)
{
  clear_missed_late_rendezvous();
  struct late_rendezvous *nearest = list_head(late_rendezvous_list);
  struct late_rendezvous *next = nearest;
  while(nearest && ((next = list_item_next(next)))) {
    if(RTIMER_CLOCK_LT(next->time, nearest->time)) {
      nearest = next;
    }
  }
  return nearest;
}
/*---------------------------------------------------------------------------*/
static bool
has_late_rendezvous_on_current_channel(void)
{
  clear_missed_late_rendezvous();

  for(struct late_rendezvous *lr = list_head(late_rendezvous_list);
      lr;
      lr = list_item_next(lr)) {
    if(lr->channel_index == current_channel_index) {
      return true;
    }
  }
  return false;
}
/*---------------------------------------------------------------------------*/
static void
set_channel(wake_up_counter_t wuc, const linkaddr_t *addr)
{
  current_channel_index = csl_forecast_channel_index(wuc, addr);
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, csl_get_channel());
}
#endif /* !CSL_COMPLIANT */
/*---------------------------------------------------------------------------*/
static void
enable_local_packetbuf(uint8_t burst_index)
{
  csl_state.duty_cycle.actual_packetbuf[burst_index] = packetbuf;
  packetbuf = &csl_state.duty_cycle.local_packetbuf[burst_index];
}
/*---------------------------------------------------------------------------*/
static void
disable_local_packetbuf(uint_fast8_t burst_index)
{
  packetbuf = csl_state.duty_cycle.actual_packetbuf[burst_index];
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  if(NETSTACK_RADIO.async_enter()) {
    LOG_ERR("async_enter failed\n");
    return;
  }
#if !AKES_MAC_ENABLED
  mac_sequence_init();
#endif /* !AKES_MAC_ENABLED */
#if CSL_COMPLIANT
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channels[0]);
#else /* CSL_COMPLIANT */
  LOG_INFO("t_u = %" RTIMER_PRI "\n", CSL_MAX_OVERALL_UNCERTAINTY);
  memb_init(&late_rendezvous_memb);
  list_init(late_rendezvous_list);
#endif /* CSL_COMPLIANT */
  CSL_SYNCHRONIZER.init();
  CSL_FRAMER.init();
  frame_queue_init();
  NETSTACK_RADIO.async_set_shr_callback(on_shr);
  NETSTACK_RADIO.async_set_txdone_callback(on_txdone);
  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, OUTPUT_POWER);
  process_start(&post_processing, NULL);
  PT_INIT(&pt);
  schedule_duty_cycle(RTIMER_NOW() + WAKE_UP_COUNTER_INTERVAL);
}
/*---------------------------------------------------------------------------*/
static void
schedule_duty_cycle(rtimer_clock_t time)
{
  if(rtimer_set(&timer, time, 1, duty_cycle_wrapper, NULL) != RTIMER_OK) {
    LOG_ERR("rtimer_set failed\n");
  }
}
/*---------------------------------------------------------------------------*/
static int
schedule_duty_cycle_precise(rtimer_clock_t time)
{
  timer.time = time;
  timer.func = duty_cycle_wrapper;
  timer.ptr = NULL;
  return rtimer_set_precise(&timer);
}
/*---------------------------------------------------------------------------*/
static void
duty_cycle_wrapper(struct rtimer *rt, void *ptr)
{
  duty_cycle();
}
/*---------------------------------------------------------------------------*/
/**
 * Handles the whole process of sampling the channel, receiving wake-up frames,
 * receiving payload frames, and sending acknowledgment frames.
 */
static
PT_THREAD(duty_cycle(void))
{
  PT_BEGIN(&pt);

#if defined(LPM_CONF_ENABLE) && LPM_CONF_ENABLE
  lpm_set_max_pm(LPM_PM1);
#endif /* defined(LPM_CONF_ENABLE) && LPM_CONF_ENABLE */
  is_duty_cycling = true;

  if(atomic_cas_uint8(&skip_lock, 0, 1) || SHALL_SKIP_TO_RENDEZVOUS) {
    if(!SHALL_SKIP_TO_RENDEZVOUS) {
      /* this is to avoid detecting an SHR in a previous wake-up frame */
      NETSTACK_RADIO.set_value(RADIO_PARAM_SHR_SENSITIVITY,
                               RADIO_SHR_SENSITIVITY_HIGH);
#if !CSL_COMPLIANT
      csl_wake_up_counter = csl_get_wake_up_counter(
                                timer.time
                                + CSL_LPM_DEEP_SWITCHING
                                + RADIO_RECEIVE_CALIBRATION_TIME);
#endif /* !CSL_COMPLIANT */
      last_wake_up_time = timer.time
                          + CSL_LPM_DEEP_SWITCHING
                          + RADIO_RECEIVE_CALIBRATION_TIME;
      NETSTACK_RADIO.async_set_fifop_callback(
          on_wake_up_frame_fifop,
          CSL_MIN_BYTES_FOR_PARSING_WAKE_UP_FRAMES);

      /* if we come from PM0, we will be too early */
      RTIMER_BUSYWAIT_UNTIL_TIMEOUT(timer.time + CSL_LPM_DEEP_SWITCHING);

      NETSTACK_RADIO.async_on();
      csl_state.duty_cycle.waiting_for_wake_up_frames_shr = true;
      csl_state.duty_cycle.wake_up_frame_timeout =
          RTIMER_NOW() + RADIO_RECEIVE_CALIBRATION_TIME + SCAN_DURATION;
      schedule_duty_cycle(csl_state.duty_cycle.wake_up_frame_timeout);
      /* wait until timeout or on_wake_up_frame_fifop, whatever comes first */
      PT_YIELD(&pt);
      csl_state.duty_cycle.waiting_for_wake_up_frames_shr = false;
    }
    if(!SHALL_SKIP_TO_RENDEZVOUS
       && !csl_state.duty_cycle.got_wake_up_frames_shr) {
      NETSTACK_RADIO.async_off();
    } else {
#if CC_CRYPTO_ENABLED
      cc_crypto_enable();
#endif /* CC_CRYPTO_ENABLED */
      if(!SHALL_SKIP_TO_RENDEZVOUS) {
        /* wait until timeout or on_wake_up_frame_fifop, whatever comes last */
        PT_YIELD(&pt);
      }
      if(csl_state.duty_cycle.got_rendezvous_time) {
#if !CSL_COMPLIANT
        if(!csl_state.duty_cycle.left_radio_on
           && !SHALL_SKIP_TO_RENDEZVOUS
           && !RTIMER_CLOCK_LT(csl_state.duty_cycle.rendezvous_time,
                               RTIMER_NOW() + LATE_RENDEZVOUS_TRESHOLD)) {
          struct late_rendezvous *lr = memb_alloc(&late_rendezvous_memb);
          if(lr) {
            lr->time = csl_state.duty_cycle.rendezvous_time;
            lr->subtype = csl_state.duty_cycle.subtype;
            lr->channel_index = current_channel_index;
            list_add(late_rendezvous_list, lr);
          } else {
            LOG_ERR("late_rendezvous_memb is full\n");
          }
        } else
#endif /* !CSL_COMPLIANT */
        {
          csl_state.duty_cycle.min_bytes_for_filtering =
              CSL_FRAMER.get_min_bytes_for_filtering();
          NETSTACK_RADIO.async_set_fifop_callback(
              on_payload_frame_fifop,
              csl_state.duty_cycle.min_bytes_for_filtering);
          if(!csl_state.duty_cycle.left_radio_on) {
            if(!SHALL_SKIP_TO_RENDEZVOUS
               && (schedule_duty_cycle_precise(
                       csl_state.duty_cycle.rendezvous_time
                       - RENDEZVOUS_GUARD_TIME)
                   == RTIMER_OK)) {
              PT_YIELD(&pt); /* wait until rendezvous */
            }
            /* if we come from PM0 we will be too early */
            RTIMER_BUSYWAIT_UNTIL_TIMEOUT(csl_state.duty_cycle.rendezvous_time
                                          - NEGATIVE_RENDEZVOUS_TIME_ACCURACY
                                          - RADIO_RECEIVE_CALIBRATION_TIME);
            NETSTACK_RADIO.async_on();
          }
          csl_state.duty_cycle.waiting_for_payload_frames_shr = true;
          schedule_duty_cycle(csl_state.duty_cycle.rendezvous_time
                              + RADIO_SHR_TIME
                              + POSITIVE_RENDEZVOUS_TIME_ACCURACY);
          while(1) {
            /* wait until timeout */
            PT_YIELD(&pt);
            csl_state.duty_cycle.waiting_for_payload_frames_shr = false;

            if(!csl_state.duty_cycle.got_payload_frames_shr) {
              LOG_ERR("missed %spayload frame %i\n",
                      csl_state.duty_cycle.last_burst_index
                      ? "bursted "
                      : (SHALL_SKIP_TO_RENDEZVOUS
                         ? "late "
                         : (csl_state.duty_cycle.left_radio_on
                            ? "early "
                            : "")),
                      csl_state.duty_cycle.remaining_wake_up_frames);
              NETSTACK_RADIO.async_off();
              if(csl_state.duty_cycle.last_burst_index) {
                csl_state.duty_cycle.last_burst_index--;
              }
              break;
            }

            /* wait until on_payload_frame_fifop */
            PT_YIELD(&pt);
            if(csl_state.duty_cycle.rejected_payload_frame) {
              if(csl_state.duty_cycle.last_burst_index) {
                csl_state.duty_cycle.last_burst_index--;
              }
              break;
            }

            /* wait either until on_final_payload_frame_fifop or on_txdone */
            PT_YIELD(&pt);
            if(!csl_state.duty_cycle.frame_pending) {
              if(!csl_state.duty_cycle.received_frame
                 && csl_state.duty_cycle.last_burst_index) {
                csl_state.duty_cycle.last_burst_index--;
                csl_state.duty_cycle.received_frame = true;
              }
              break;
            }

            /* prepare for receiving bursted payload frame */
            csl_state.duty_cycle.last_burst_index++;
            csl_state.duty_cycle.min_bytes_for_filtering =
                CSL_FRAMER.get_min_bytes_for_filtering();
            NETSTACK_RADIO.async_set_fifop_callback(
                on_payload_frame_fifop,
                csl_state.duty_cycle.min_bytes_for_filtering);
            csl_state.duty_cycle.got_payload_frames_shr = false;
            csl_state.duty_cycle.waiting_for_payload_frames_shr = true;
            csl_state.duty_cycle.left_radio_on = false;
            csl_state.duty_cycle.remaining_wake_up_frames = 0;
            schedule_duty_cycle(RTIMER_NOW() + CSL_ACKNOWLEDGMENT_WINDOW_MAX);
          }
        }
      }
    }
    NETSTACK_RADIO.async_set_fifop_callback(NULL, 0);
  }

  is_duty_cycling = false;
  process_poll(&post_processing);

  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
/** Called when a start-of-frame delimiter was received or transmitted */
static void
on_shr(void)
{
  rtimer_clock_t now = RTIMER_NOW();
  if(is_duty_cycling) {
    if(csl_state.duty_cycle.waiting_for_unwanted_shr
       && !RTIMER_CLOCK_LT(csl_state.duty_cycle.unwanted_shr_timeout, now)) {
      csl_state.duty_cycle.waiting_for_unwanted_shr = false;
    } else if(csl_state.duty_cycle.waiting_for_wake_up_frames_shr) {
      csl_state.duty_cycle.got_wake_up_frames_shr = true;
      csl_state.duty_cycle.wake_up_frame_sfd_timestamp = now;
      NETSTACK_RADIO.set_value(RADIO_PARAM_SHR_SENSITIVITY,
                               RADIO_SHR_SENSITIVITY_MEDIUM);
      rtimer_cancel();
    } else if(csl_state.duty_cycle.waiting_for_payload_frames_shr) {
#if !CONTIKI_TARGET_COOJA
      if(csl_state.duty_cycle.left_radio_on
         && csl_state.duty_cycle.remaining_wake_up_frames
         && !csl_state.duty_cycle.waiting_for_unwanted_shr) {
        uint_fast16_t wake_up_frame_len =
            NETSTACK_RADIO.async_read_phy_header();
        uint8_t wake_up_frame[CSL_MAX_WAKE_UP_FRAME_LEN
                              - RADIO_SHR_LEN
                              - RADIO_HEADER_LEN];
        if((wake_up_frame_len > CSL_MAX_WAKE_UP_FRAME_LEN)
           || NETSTACK_RADIO.async_read_payload(wake_up_frame,
                                                wake_up_frame_len)) {
          LOG_WARN("error while scanning for the payload frame\n");
          return;
        }
      }
#endif /* !CONTIKI_TARGET_COOJA */
      csl_state.duty_cycle.got_payload_frames_shr = true;
      last_payload_frame_sfd_timestamp = now;
    }
  } else if(is_transmitting) {
    if(csl_state.transmit.waiting_for_acknowledgment_shr) {
      csl_state.transmit.got_acknowledgment_shr = true;
      csl_state.transmit.acknowledgment_rssi[csl_state.transmit.burst_index] =
          radio_get_rssi();
      if(!csl_state.transmit.burst_index) {
        csl_state.transmit.acknowledgment_sfd_timestamp = now;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
/** Called as soon as first bytes of a wake-up frame were received */
static void
on_wake_up_frame_fifop(void)
{
  if(!csl_state.duty_cycle.got_wake_up_frames_shr) {
    return;
  }

  /* avoid that on_fifop is called twice */
  NETSTACK_RADIO.async_set_fifop_callback(NULL, 0);
  enable_local_packetbuf(0);
#if !CSL_COMPLIANT
  if(is_anything_locked()) {
    csl_state.duty_cycle.got_rendezvous_time = false;
  } else
#endif /* !CSL_COMPLIANT */
  if(CSL_FRAMER.parse_wake_up_frame() == FRAMER_FAILED) {
    csl_state.duty_cycle.got_rendezvous_time = false;
  } else {
    csl_state.duty_cycle.got_rendezvous_time = true;
  }

  if(!csl_state.duty_cycle.got_rendezvous_time
     || (csl_state.duty_cycle.remaining_wake_up_frames >= 2)) {
    NETSTACK_RADIO.async_off();
  } else {
    csl_state.duty_cycle.left_radio_on = true;
    if(csl_state.duty_cycle.remaining_wake_up_frames == 1) {
      csl_state.duty_cycle.unwanted_shr_timeout =
          csl_state.duty_cycle.wake_up_frame_sfd_timestamp
          + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE
                                   * (RADIO_HEADER_LEN
                                      + packetbuf_totlen()
                                      + RADIO_SHR_LEN
                                      + (packetbuf_totlen() / 2)));
      csl_state.duty_cycle.waiting_for_unwanted_shr = true;
    }
  }
  disable_local_packetbuf(0);

  duty_cycle();
}
/*---------------------------------------------------------------------------*/
/** Called as soon as first bytes of a payload frame were received */
static void
on_payload_frame_fifop(void)
{
  if(!csl_state.duty_cycle.got_payload_frames_shr) {
    return;
  }

  /* avoid that on_payload_frame_fifop is called twice */
  NETSTACK_RADIO.async_set_fifop_callback(NULL, RADIO_MAX_PAYLOAD);
  enable_local_packetbuf(csl_state.duty_cycle.last_burst_index);
  packetbuf_set_attr(PACKETBUF_ATTR_CHANNEL, csl_get_channel());

  if(csl_state.duty_cycle.last_burst_index) {
    packetbuf_set_attr(PACKETBUF_ATTR_BURST_INDEX,
                       csl_state.duty_cycle.last_burst_index);
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &csl_state.duty_cycle.sender);
  }

#if CSL_COMPLIANT
  if(false
#else /* CSL_COMPLIANT */
  if(is_anything_locked()
#endif /* CSL_COMPLIANT */
     || (radio_read_phy_header_to_packetbuf()
         < csl_state.duty_cycle.min_bytes_for_filtering)
     || radio_read_payload_to_packetbuf(
         csl_state.duty_cycle.min_bytes_for_filtering)
     || (CSL_FRAMER.filter() == FRAMER_FAILED)) {
    NETSTACK_RADIO.async_off();
    LOG_INFO("rejected payload frame of length %i\n", packetbuf_datalen());
    csl_state.duty_cycle.rejected_payload_frame = true;
  } else {
    csl_state.duty_cycle.frame_pending =
        packetbuf_attr(PACKETBUF_ATTR_PENDING)
        && (csl_state.duty_cycle.last_burst_index < CSL_MAX_BURST_INDEX);
    linkaddr_copy(&csl_state.duty_cycle.sender,
                  packetbuf_addr(PACKETBUF_ADDR_SENDER));
    csl_state.duty_cycle.shall_send_acknowledgment =
        !packetbuf_holds_broadcast();
    if(csl_state.duty_cycle.shall_send_acknowledgment
       && NETSTACK_RADIO.async_prepare(
           csl_state.duty_cycle.acknowledgment,
           csl_state.duty_cycle.acknowledgment_len)) {
      NETSTACK_RADIO.async_off();
      LOG_ERR("async_prepare failed\n");
      csl_state.duty_cycle.rejected_payload_frame = true;
    } else {
      NETSTACK_RADIO.async_set_fifop_callback(on_final_payload_frame_fifop,
                                              radio_remaining_payload_bytes());
    }
  }

  disable_local_packetbuf(csl_state.duty_cycle.last_burst_index);

  duty_cycle();
}
/*---------------------------------------------------------------------------*/
/** Called as soon as the whole payload frame was received */
static void
on_final_payload_frame_fifop(void)
{
  /* avoid that on_final_payload_frame_fifop is called twice */
  NETSTACK_RADIO.async_set_fifop_callback(NULL, 0);

  enable_local_packetbuf(csl_state.duty_cycle.last_burst_index);
  packetbuf_set_attr(PACKETBUF_ATTR_RSSI, (int16_t)radio_get_rssi());

  if(csl_state.duty_cycle.shall_send_acknowledgment) {
    if(NETSTACK_RADIO.async_transmit(csl_state.duty_cycle.frame_pending)) {
      LOG_WARN("async_transmit failed\n");
      csl_state.duty_cycle.received_frame = false;
    }
    csl_state.duty_cycle.is_waiting_for_txdone = true;
  } else if(!csl_state.duty_cycle.frame_pending) {
    NETSTACK_RADIO.async_off();
  }

#if !CSL_COMPLIANT
  const akes_nbr_entry_t *entry;
#endif /* !CSL_COMPLIANT */
  if(radio_read_payload_to_packetbuf(radio_remaining_payload_bytes())) {
    LOG_ERR("radio_read_payload_to_packetbuf failed\n");
    csl_state.duty_cycle.received_frame = false;
  } else if(NETSTACK_FRAMER.parse() == FRAMER_FAILED) {
    LOG_ERR("NETSTACK_FRAMER.parse failed\n");
    csl_state.duty_cycle.received_frame = false;
#if CSL_COMPLIANT
  } else {
    csl_state.duty_cycle.received_frame = true;
  }
#else /* CSL_COMPLIANT */
  } else if((csl_state.duty_cycle.subtype == CSL_SUBTYPE_HELLOACK)
            || !csl_state.duty_cycle.shall_send_acknowledgment) {
    csl_state.duty_cycle.received_frame = true;
  } else if(is_anything_locked()) {
    LOG_ERR("something is locked\n");
    csl_state.duty_cycle.received_frame = false;
  } else if(!((entry = akes_nbr_get_sender_entry()))) {
    LOG_ERR("sender not found\n");
    csl_state.duty_cycle.received_frame = false;
  } else if(csl_state.duty_cycle.subtype == CSL_SUBTYPE_ACK) {
    csl_state.duty_cycle.received_frame =
        entry->tentative
        && !memcmp(((uint8_t *)packetbuf_dataptr())
                   + AKES_ACK_PIGGYBACK_OFFSET
                   + CSL_FRAMER_POTR_PHASE_LEN,
                   csl_nbr_get_tentative(entry->tentative->meta)->q,
                   sizeof(csl_nbr_get_tentative(entry->tentative->meta)->q))
        && !AKES_MAC_STRATEGY.verify(entry->tentative);
  } else {
    csl_state.duty_cycle.received_frame =
        entry->permanent && !AKES_MAC_STRATEGY.verify(entry->permanent);
  }
#endif /* CSL_COMPLIANT */

  disable_local_packetbuf(csl_state.duty_cycle.last_burst_index);

  if(!csl_state.duty_cycle.shall_send_acknowledgment) {
    duty_cycle();
  } else if(!csl_state.duty_cycle.received_frame) {
    /* abort ongoing acknowledgment transmission */
    NETSTACK_RADIO.async_off();
    csl_state.duty_cycle.is_waiting_for_txdone = false;
    csl_state.duty_cycle.frame_pending = false;
    LOG_INFO("flushing unicast frame\n");
    duty_cycle();
  }
}
/*---------------------------------------------------------------------------*/
/** Called right after transmitting an acknowledgment or payload frame */
static void
on_txdone(void)
{
  if(is_duty_cycling && csl_state.duty_cycle.is_waiting_for_txdone) {
    csl_state.duty_cycle.is_waiting_for_txdone = false;
    duty_cycle();
  } else if(is_transmitting && csl_state.transmit.is_waiting_for_txdone) {
    transmit();
  } else {
    LOG_ERR("unwanted TXDONE\n");
    NETSTACK_RADIO.async_off();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * Passes received frames to upper-layer protocols, as well as prepares
 * outgoing transmissions.
 */
PROCESS_THREAD(post_processing, ev, data)
{
  bool sent_once;
  rtimer_clock_t schedule_start;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    can_skip = true;

    if(csl_state.duty_cycle.received_frame) {
      for(uint_fast8_t burst_index = 0;
          burst_index <= csl_state.duty_cycle.last_burst_index;
          burst_index++) {
        enable_local_packetbuf(burst_index);
#if AKES_MAC_ENABLED
        NETSTACK_MAC.input();
#else /* AKES_MAC_ENABLED */
        if(!packetbuf_holds_broadcast() && mac_sequence_is_duplicate()) {
          LOG_ERR("received duplicate\n");
        } else {
          mac_sequence_register_seqno();
          NETSTACK_NETWORK.input();
        }
#endif /* AKES_MAC_ENABLED */
        disable_local_packetbuf(burst_index);
      }
    }

#if CC_CRYPTO_ENABLED
    cc_crypto_disable();
#endif /* CC_CRYPTO_ENABLED */
    NETSTACK_RADIO.set_value(RADIO_PARAM_SHR_SENSITIVITY,
                             RADIO_SHR_SENSITIVITY_MEDIUM);
    PROCESS_PAUSE();

    /* send queued frames */
    sent_once = false;
    frame_queue_entry_t *next;
    while((next = frame_queue_pick())) {
#if CC_CRYPTO_ENABLED
      cc_crypto_enable();
#endif /* CC_CRYPTO_ENABLED */
      memset(&csl_state.transmit, 0, sizeof(csl_state.transmit));
      csl_state.transmit.fqe[0] = next;

      /* what kind of payload frame do we have here? */
#if CSL_COMPLIANT
      csl_state.transmit.is_broadcast = packetbuf_holds_broadcast();
#else /* CSL_COMPLIANT */
      if(akes_mac_is_hello()) {
        csl_state.transmit.is_broadcast = true;
        csl_state.transmit.subtype = CSL_SUBTYPE_HELLO;
      } else if(akes_mac_is_helloack()) {
        csl_state.transmit.subtype = CSL_SUBTYPE_HELLOACK;
      } else {
        csl_state.transmit.subtype =
            akes_mac_is_ack() ? CSL_SUBTYPE_ACK : CSL_SUBTYPE_NORMAL;
      }
#endif /* CSL_COMPLIANT */
      csl_state.transmit.wake_up_frame_len =
          RADIO_SHR_LEN
          + RADIO_HEADER_LEN
          + CSL_FRAMER.get_length_of_wake_up_frame();

      /* schedule */
      schedule_start = RTIMER_NOW();
#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
      linkaddr_copy(&csl_state.transmit.next_hop_address,
                    packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */
      csl_state.transmit.result[0] = CSL_SYNCHRONIZER.schedule();
      assert(csl_state.transmit.result[0] != MAC_TX_ERR);
      if(csl_state.transmit.result[0] != MAC_TX_OK) {
        LOG_ERR("CSL_SYNCHRONIZER.schedule failed\n");
        on_transmitted();
        continue;
      }

      /* avoid skipping too many wake ups */
      rtimer_clock_t start_of_transmission =
          csl_state.transmit.wake_up_sequence_start
          - csl_max_frame_creation_time
          - CSL_WAKE_UP_SEQUENCE_GUARD_TIME
          - US_TO_RTIMERTICKS(1000) /* leeway */;
      rtimer_clock_t end_of_transmission =
          csl_state.transmit.payload_frame_start
          + MAX_TRANSMISSION_TIME
          - RADIO_TRANSMIT_CALIBRATION_TIME
          + US_TO_RTIMERTICKS(1000) /* leeway */;
      rtimer_clock_t start_of_wake_up =
          wake_up_counter_shift_to_future(last_wake_up_time
                                          - CSL_LPM_DEEP_SWITCHING
                                          - RADIO_RECEIVE_CALIBRATION_TIME);
      rtimer_clock_t end_of_wake_up = start_of_wake_up
                                      + CSL_LPM_DEEP_SWITCHING
                                      + RADIO_RECEIVE_CALIBRATION_TIME
                                      + SCAN_DURATION;
      if(RTIMER_CLOCK_LT(end_of_wake_up, start_of_transmission)
         || (sent_once
             && RTIMER_CLOCK_LT(start_of_wake_up, end_of_transmission))) {
        break;
      }

#if !CSL_COMPLIANT
      /* set channel */
      if(csl_state.transmit.subtype == CSL_SUBTYPE_HELLO) {
        set_channel(csl_get_wake_up_counter(csl_get_payload_frames_shr_end()),
                    &linkaddr_node_addr);
      } else {
        set_channel(csl_predict_wake_up_counter(),
                    packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
      }
      struct late_rendezvous *lr1 = get_nearest_late_rendezvous();
      if(has_late_rendezvous_on_current_channel()
         || (lr1 && (csl_state.transmit.subtype == CSL_SUBTYPE_HELLO))
         || (lr1 && !RTIMER_CLOCK_LT(end_of_transmission, lr1->time))) {
        if(csl_state.transmit.subtype == CSL_SUBTYPE_HELLO) {
          /*
           * give the sender of the late rendezvous a chance to receive our
           * HELLO; TODO rate-limit to avoid suppression of communication
           */
          frame_queue_postpone((CSL_HELLO_WAKE_UP_SEQUENCE_TX_TIME
                                * CLOCK_SECOND)
                               / RTIMER_SECOND);
        }
        break;
      }
#endif /* !CSL_COMPLIANT */

      if(!CSL_FRAMER.prepare_acknowledgment_parsing()) {
        LOG_ERR("CSL_FRAMER.prepare_acknowledgment_parsing failed\n");
        csl_state.transmit.result[0] = MAC_TX_ERR_FATAL;
        on_transmitted();
        continue;
      }

      if(!csl_channel_selector_take_feedback_is_exploring()
#if !CSL_COMPLIANT
         && (csl_state.transmit.subtype == CSL_SUBTYPE_NORMAL)
#endif /* !CSL_COMPLIANT */
         ) {
        /* check if we can burst more payload frames */
        while(csl_state.transmit.last_burst_index < CSL_MAX_BURST_INDEX) {
#if !CSL_COMPLIANT
          end_of_transmission += MAX_TRANSMISSION_TIME;
          if(lr1 && RTIMER_CLOCK_LT(lr1->time - RENDEZVOUS_GUARD_TIME,
                                    end_of_transmission)) {
            /* TODO rate-limit to avoid suppression of communication */
            /* we do not want to miss our late rendezvous */
            break;
          }
#endif /* !CSL_COMPLIANT */

          if(!((csl_state.transmit
                    .fqe[csl_state.transmit.last_burst_index + 1] =
                    frame_queue_burst(
                        csl_state.transmit
                            .fqe[csl_state.transmit.last_burst_index])))) {
            break;
          }
          csl_state.transmit.last_burst_index++;
        }
      }

      /* create payload frame(s) */
      uint_fast8_t i = csl_state.transmit.last_burst_index;
      int create_result;
      do {
        queuebuf_to_packetbuf(csl_state.transmit.fqe[i]->qb);
#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER,
                           &csl_state.transmit.next_hop_address);
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */
        packetbuf_set_attr(PACKETBUF_ATTR_BURST_INDEX, i);
        packetbuf_set_attr(PACKETBUF_ATTR_PENDING,
                           (i < CSL_MAX_BURST_INDEX)
                           && csl_state.transmit.fqe[i + 1]
                           ? csl_state.transmit.payload_frame_lens[i + 1]
                           : 0);
        create_result = NETSTACK_FRAMER.create();
        if(create_result == FRAMER_FAILED) {
          LOG_ERR("NETSTACK_FRAMER.create failed\n");
          csl_state.transmit.result[0] = MAC_TX_ERR_FATAL;
          csl_state.transmit.fqe[0] = csl_state.transmit.fqe[i];
          on_transmitted();
          break;
        }
        csl_state.transmit.payload_frame_lens[i] = packetbuf_totlen();
        memcpy(csl_state.transmit.payload_frame[i],
               packetbuf_hdrptr(),
               packetbuf_totlen());
      } while(i--);
      if(create_result == FRAMER_FAILED) {
        continue;
      }
      csl_state.transmit.remaining_payload_frame_bytes =
          csl_state.transmit.payload_frame_lens[0];

      /* prepare wake-up sequence */
      uint8_t wake_up_frame[CSL_MAX_WAKE_UP_FRAME_LEN];
      memcpy(wake_up_frame, radio_shr, RADIO_SHR_LEN);
      if(CSL_FRAMER.create_wake_up_frame(wake_up_frame + RADIO_SHR_LEN)
         == FRAMER_FAILED) {
        LOG_ERR("wake-up frame creation failed\n");
        csl_state.transmit.result[0] = MAC_TX_ERR_FATAL;
        on_transmitted();
        continue;
      }
      for(uint_fast16_t j = 0;
          j <= (RADIO_MAX_SEQUENCE_LEN - csl_state.transmit.wake_up_frame_len);
          j += csl_state.transmit.wake_up_frame_len) {
        memcpy(csl_state.transmit.next_wake_up_frames + j,
               wake_up_frame,
               csl_state.transmit.wake_up_frame_len);
      }
      uint_fast16_t prepared_bytes =
          prepare_next_wake_up_frames(RADIO_MAX_SEQUENCE_LEN);
      if(NETSTACK_RADIO.async_prepare_sequence(
             csl_state.transmit.next_wake_up_frames,
             prepared_bytes)) {
        LOG_ERR("async_prepare_sequence failed\n");
        csl_state.transmit.result[0] = MAC_TX_ERR;
        on_transmitted();
        continue;
      }

      /* schedule transmission */
      if(schedule_transmission_precise(
             csl_state.transmit.wake_up_sequence_start
             - CSL_WAKE_UP_SEQUENCE_GUARD_TIME)
         != RTIMER_OK) {
        /* update max_frame_creation_time */
        rtimer_clock_t frame_creation_time =
            US_TO_RTIMERTICKS(200) /* leeway */
            + (RTIMER_NOW() - schedule_start);
        LOG_ERR("Transmission is not schedulable %" RTIMER_PRI "\n",
                frame_creation_time);
        csl_max_frame_creation_time = MAX(csl_max_frame_creation_time,
                                          frame_creation_time);
        continue;
      }
      PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
      on_transmitted();
      PROCESS_PAUSE();
      sent_once = true;
    }

    /* prepare next duty cycle */
#if CC_CRYPTO_ENABLED
    cc_crypto_disable();
#endif /* CC_CRYPTO_ENABLED */
#if defined(LPM_CONF_ENABLE) && LPM_CONF_ENABLE
    lpm_set_max_pm(LPM_CONF_MAX_PM);
#endif /* defined(LPM_CONF_ENABLE) && LPM_CONF_ENABLE */

    rtimer_clock_t next_wake_up_time = wake_up_counter_shift_to_future(
                                           last_wake_up_time
                                           - CSL_LPM_DEEP_SWITCHING
                                           - RADIO_RECEIVE_CALIBRATION_TIME);
    memset(&csl_state.duty_cycle, 0, sizeof(csl_state.duty_cycle));
    while(1) {
#if !CSL_COMPLIANT
      struct late_rendezvous *lr2 = get_nearest_late_rendezvous();
      if(!lr2
         || (RTIMER_CLOCK_LT(next_wake_up_time + LATE_WAKE_UP_GUARD_TIME,
                             lr2->time))) {
        set_channel(csl_get_wake_up_counter(next_wake_up_time
                                            + CSL_LPM_DEEP_SWITCHING
                                            + RADIO_RECEIVE_CALIBRATION_TIME),
                    &linkaddr_node_addr);
        if(has_late_rendezvous_on_current_channel()) {
          next_wake_up_time += WAKE_UP_COUNTER_INTERVAL;
          continue;
        }
#endif /* !CSL_COMPLIANT */
        atomic_cas_uint8(&skip_lock, 1, 0);
        if(schedule_duty_cycle_precise(next_wake_up_time) != RTIMER_OK) {
          next_wake_up_time += WAKE_UP_COUNTER_INTERVAL;
          atomic_cas_uint8(&skip_lock, 0, 1);
          continue;
        }
#if !CSL_COMPLIANT
      } else {
        csl_state.duty_cycle.rendezvous_time = lr2->time;
        csl_state.duty_cycle.got_rendezvous_time = true;
        csl_state.duty_cycle.subtype = lr2->subtype;
        csl_state.duty_cycle.skip_to_rendezvous = true;
        current_channel_index = lr2->channel_index;
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, csl_get_channel());
        delete_late_rendezvous(lr2);
        if(schedule_duty_cycle_precise(
               csl_state.duty_cycle.rendezvous_time
               - RENDEZVOUS_GUARD_TIME
               - (CSL_LPM_DEEP_SWITCHING - CSL_LPM_SWITCHING))
           != RTIMER_OK) {
          LOG_ERR("missed late rendezvous\n");
          memset(&csl_state.duty_cycle, 0, sizeof(csl_state.duty_cycle));
          continue;
        }
      }
#endif /* !CSL_COMPLIANT */
      break;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static uint_fast16_t
prepare_next_wake_up_frames(uint_fast16_t space)
{
  /* append the next wake-up frames */
  uint_fast16_t number_of_wake_up_frames =
      MIN(csl_state.transmit.remaining_wake_up_frames,
          space / csl_state.transmit.wake_up_frame_len);
  for(uint_fast16_t i = 0; i < number_of_wake_up_frames; i++) {
    csl_state.transmit.remaining_wake_up_frames--;

    /* update the wake-up frame's rendezvous time and its checksum (if any) */
    CSL_FRAMER.update_rendezvous_time(
        csl_state.transmit.next_wake_up_frames
        + (i * csl_state.transmit.wake_up_frame_len)
        + RADIO_SHR_LEN);
  }
  uint_fast16_t prepared_bytes =
      number_of_wake_up_frames * csl_state.transmit.wake_up_frame_len;
  space -= prepared_bytes;
  csl_state.transmit.wake_up_sequence_pos += prepared_bytes;

  /*
   * append the first payload frame - the first payload
   * frame is sent right after the last wake-up frame
   */
  if(!csl_state.transmit.remaining_wake_up_frames
     && (space >= (RADIO_SHR_LEN + RADIO_HEADER_LEN))) {
    if(!csl_state.transmit.wrote_payload_frames_phy_header) {
      memcpy(csl_state.transmit.next_wake_up_frames + prepared_bytes,
             radio_shr,
             RADIO_SHR_LEN);
      prepared_bytes += RADIO_SHR_LEN;
      csl_state.transmit.next_wake_up_frames[prepared_bytes] =
          csl_state.transmit.payload_frame_lens[0];
      prepared_bytes += RADIO_HEADER_LEN;
      space -= RADIO_SHR_LEN + RADIO_HEADER_LEN;
      csl_state.transmit.wake_up_sequence_pos +=
          RADIO_SHR_LEN + RADIO_HEADER_LEN;
      csl_state.transmit.wrote_payload_frames_phy_header = true;
    }

    uint_fast16_t bytes =
        MIN(space, csl_state.transmit.remaining_payload_frame_bytes);
    memcpy(csl_state.transmit.next_wake_up_frames + prepared_bytes,
           csl_state.transmit.payload_frame[0]
           + csl_state.transmit.payload_frame_lens[0]
           - csl_state.transmit.remaining_payload_frame_bytes,
           bytes);
    csl_state.transmit.remaining_payload_frame_bytes -= bytes;
    prepared_bytes += bytes;
    csl_state.transmit.wake_up_sequence_pos += bytes;
  }

  return prepared_bytes;
}
/*---------------------------------------------------------------------------*/
static void
schedule_transmission(rtimer_clock_t time)
{
  if(rtimer_set(&timer, time, 1, transmit_wrapper, NULL) != RTIMER_OK) {
    LOG_ERR("rtimer_set failed\n");
  }
}
/*---------------------------------------------------------------------------*/
static int
schedule_transmission_precise(rtimer_clock_t time)
{
  timer.time = time;
  timer.func = transmit_wrapper;
  timer.ptr = NULL;
  return rtimer_set_precise(&timer);
}
/*---------------------------------------------------------------------------*/
static void
transmit_wrapper(struct rtimer *rt, void *ptr)
{
  transmit();
}
/*---------------------------------------------------------------------------*/
/**
 * Handles the whole process of transmitting wake-up sequences, transmitting
 * payload frames, and receiving acknowledgment frames.
 */
static
PT_THREAD(transmit(void))
{
  PT_BEGIN(&pt);
  is_transmitting = true;

  /* if we come from PM0 we will be too early */
  RTIMER_BUSYWAIT_UNTIL_TIMEOUT(
      csl_state.transmit.wake_up_sequence_start
      - (CSL_WAKE_UP_SEQUENCE_GUARD_TIME - CSL_LPM_SWITCHING));
  NETSTACK_RADIO.async_on();
  schedule_transmission(RTIMER_NOW() + CCA_SLEEP_DURATION);
  PT_YIELD(&pt);
  if(radio_get_rssi() >= CCA_THRESHOLD) {
    NETSTACK_RADIO.async_off();
    LOG_INFO("collision\n");
    csl_state.transmit.result[0] = MAC_TX_COLLISION;
    goto error;
  }

  /* send the wake-up sequence, as well as the first payload frame */
  if(NETSTACK_RADIO.async_transmit_sequence()) {
    NETSTACK_RADIO.async_off();
    LOG_ERR("async_transmit_sequence failed\n");
    csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_ERR;
    goto error;
  }
  while(1) {
    csl_state.transmit.next_rendezvous_time_update =
        csl_state.transmit.wake_up_sequence_start
        + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE
                                 * (csl_state.transmit.wake_up_sequence_pos
                                    - MIN_PREPARE_LEAD_OVER_LOOP_MID));
    if(!csl_state.transmit.remaining_wake_up_frames
       && !csl_state.transmit.remaining_payload_frame_bytes) {
      break;
    }
    schedule_transmission(csl_state.transmit.next_rendezvous_time_update);
    PT_YIELD(&pt);
    uint_fast16_t prepared_bytes = prepare_next_wake_up_frames(
                                       RADIO_MAX_SEQUENCE_LEN
                                       - MIN_PREPARE_LEAD_OVER_LOOP);
    if(NETSTACK_RADIO.async_append_to_sequence(
           csl_state.transmit.next_wake_up_frames,
           prepared_bytes)) {
      NETSTACK_RADIO.async_off();
      LOG_ERR("async_append_to_sequence failed\n");
      csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_ERR;
      goto error;
    }
  }
  if(schedule_transmission_precise(
         csl_state.transmit.next_rendezvous_time_update)
     == RTIMER_OK) {
    PT_YIELD(&pt);
  }
  if(NETSTACK_RADIO.async_finish_sequence()) {
    NETSTACK_RADIO.async_off();
    LOG_ERR("async_finish_sequence failed\n");
    csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_ERR;
    goto error;
  }
  if(!csl_state.transmit.is_broadcast) {
    NETSTACK_RADIO.async_on();
  }

  while(1) {
    if(!csl_state.transmit.is_broadcast) {
      /* wait for acknowledgment */
      csl_state.transmit.waiting_for_acknowledgment_shr = true;
      csl_state.transmit.got_acknowledgment_shr = false;
      schedule_transmission(RTIMER_NOW() + CSL_ACKNOWLEDGMENT_WINDOW_MAX);
      PT_YIELD(&pt);
      csl_state.transmit.waiting_for_acknowledgment_shr = false;
      if(!csl_state.transmit.got_acknowledgment_shr) {
        NETSTACK_RADIO.async_off();
        LOG_ERR("received no acknowledgment\n");
        csl_state.transmit.result[csl_state.transmit.burst_index] =
            MAC_TX_NOACK;
        break;
      }
      if(CSL_FRAMER.parse_acknowledgment() == FRAMER_FAILED) {
        NETSTACK_RADIO.async_off();
        csl_state.transmit.result[csl_state.transmit.burst_index] =
            MAC_TX_COLLISION;
        break;
      }
      NETSTACK_RADIO.async_off();
    }
    csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_OK;

    /* check if we burst more payload frames */
    if(++csl_state.transmit.burst_index
       > csl_state.transmit.last_burst_index) {
      break;
    }

    /* transmit next payload frame */
    if(NETSTACK_RADIO.async_transmit(!csl_state.transmit.is_broadcast)) {
      NETSTACK_RADIO.async_off();
      LOG_ERR("async_transmit failed\n");
      csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_ERR;
      break;
    }

    /* move next payload frame to radio */
    if(NETSTACK_RADIO.async_prepare(
           csl_state.transmit.payload_frame[csl_state.transmit.burst_index],
           csl_state.transmit
               .payload_frame_lens[csl_state.transmit.burst_index])) {
      NETSTACK_RADIO.async_off();
      LOG_ERR("async_prepare failed\n");
      csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_ERR;
      break;
    }

    /* wait for on_txdone */
    csl_state.transmit.is_waiting_for_txdone = true;
    PT_YIELD(&pt);
    csl_state.transmit.is_waiting_for_txdone = false;
  }
error:
  is_transmitting = false;
  process_poll(&post_processing);
  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
/**
 * Performs things that have to be done after transmissions, namely backing off
 * next attempts, udpating synchronization data, and invoking callbacks.
 */
static void
on_transmitted(void)
{
  uint_fast8_t i = 0;
  do {
    bool successful = csl_state.transmit.result[i] == MAC_TX_OK;
    queuebuf_to_packetbuf(csl_state.transmit.fqe[i]->qb);
#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER,
                       &csl_state.transmit.next_hop_address);
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */

    if(!csl_state.transmit.is_broadcast) {
      if(!i) {
        CSL_FRAMER.on_unicast_transmitted();
      }
      csl_channel_selector_take_feedback(successful, i);
      CSL_SYNCHRONIZER.on_unicast_transmitted(successful, i);
    }

    packetbuf_set_attr(PACKETBUF_ATTR_CHANNEL, csl_get_channel());
    if(successful && !csl_state.transmit.is_broadcast) {
      packetbuf_set_attr(PACKETBUF_ATTR_RSSI,
                         csl_state.transmit.acknowledgment_rssi[i]);
    }
    frame_queue_on_transmitted(csl_state.transmit.result[i],
                               csl_state.transmit.fqe[i]);
  } while((csl_state.transmit.result[i] == MAC_TX_OK)
          && (++i <= csl_state.transmit.last_burst_index));
}
/*---------------------------------------------------------------------------*/
/** Called by upper layers when a frame shall be sent. */
static void
send(mac_callback_t sent, void *ptr)
{
#if !AKES_MAC_ENABLED
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  if(!packetbuf_holds_broadcast()) {
    mac_sequence_set_dsn();
  }
#endif /* !AKES_MAC_ENABLED */
  if(frame_queue_add(sent, ptr)
     && can_skip
     && atomic_cas_uint8(&skip_lock, 0, 1)) {
    /* fire up the protohread "post_processing" right away */
    rtimer_cancel();
  }
}
/*---------------------------------------------------------------------------*/
/** This function is never called as we operate in polling mode throughout */
static void
input(void)
{
  assert(0);
}
/*---------------------------------------------------------------------------*/
/** TODO implement if needed */
static int
on(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
/** TODO implement if needed  */
static int
off(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  return RADIO_MAX_PAYLOAD - NETSTACK_FRAMER.length();
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
csl_get_last_wake_up_time(void)
{
  return last_wake_up_time;
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
csl_get_payload_frames_shr_end(void)
{
  return csl_state.transmit.payload_frame_start + RADIO_SHR_TIME;
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
csl_get_sfd_timestamp_of_last_payload_frame(void)
{
  return last_payload_frame_sfd_timestamp;
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
csl_get_phase(rtimer_clock_t t)
{
  rtimer_clock_t result = t - csl_get_last_wake_up_time();
  while(result >= WAKE_UP_COUNTER_INTERVAL) {
    result -= WAKE_UP_COUNTER_INTERVAL;
  }
  return WAKE_UP_COUNTER_INTERVAL - result;
}
/*---------------------------------------------------------------------------*/
bool
csl_can_schedule_wake_up_sequence(void)
{
  return !rtimer_has_timed_out(
             csl_state.transmit.wake_up_sequence_start
             - csl_max_frame_creation_time
             - CSL_WAKE_UP_SEQUENCE_GUARD_TIME);
}
/*---------------------------------------------------------------------------*/
uint_fast8_t
csl_get_channel_index(void)
{
  return current_channel_index;
}
/*---------------------------------------------------------------------------*/
uint_fast8_t
csl_get_channel(void)
{
  return channels[current_channel_index];
}
/*---------------------------------------------------------------------------*/
const struct mac_driver csl_driver = {
  "CSL",
  init,
  send,
  input,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/

/** @} */
