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
 *         Coordinated Sampled Listening (CSL)
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/csl/csl.h"
#include "net/mac/csl/csl-framer.h"
#include "net/mac/csl/csl-framer-compliant.h"
#include "net/mac/csl/csl-framer-potr.h"
#include "net/mac/csl/csl-ccm-inputs.h"
#include "net/mac/csl/csl-strategy.h"
#include "net/mac/csl/csl-synchronizer.h"
#include "net/mac/csl/csl-nbr.h"
#include "net/mac/mac.h"
#include "services/akes/akes.h"
#include "services/akes/akes-mac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "net/mac/mac-sequence.h"
#include "dev/crypto.h"
#include "lib/memb.h"
#include "lib/list.h"
#include "lib/aes-128.h"
#include "net/nbr-table.h"
#include "lib/random.h"

#ifdef CSL_CONF_MAX_RETRANSMISSIONS
#define MAX_RETRANSMISSIONS CSL_CONF_MAX_RETRANSMISSIONS
#else /* CSL_CONF_MAX_RETRANSMISSIONS */
#define MAX_RETRANSMISSIONS 5
#endif /* CSL_CONF_MAX_RETRANSMISSIONS */

/**
 * If standards-compliance is on, the number of channels must be one because
 * standards-compliant channel hopping is not implemented, yet. If standards-
 * compliance is off, the number of channels must be a power of two. Note
 * that the ordering of the channels does not matter because they are mixed
 * pseudo-randomly.
 */
#ifdef CSL_CONF_CHANNELS
#define CHANNELS CSL_CONF_CHANNELS
#else /* CSL_CONF_CHANNELS */
#if CSL_COMPLIANT
#define CHANNELS { IEEE802154_DEFAULT_CHANNEL }
#else /*  CSL_COMPLIANT */
#define CHANNELS { 11 , 12 , 13 , 14 , 15 , 16 , 17 , 18 , 19 , 20 , 21 , 22 , 23 , 24 , 25 , 26 }
#endif /*  CSL_COMPLIANT */
#endif /* CSL_CONF_CHANNELS */

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

#define NEGATIVE_RENDEZVOUS_TIME_ACCURACY (2)
#define POSITIVE_RENDEZVOUS_TIME_ACCURACY (2)
#define RENDEZVOUS_GUARD_TIME (CSL_LPM_SWITCHING \
    + NEGATIVE_RENDEZVOUS_TIME_ACCURACY \
    + RADIO_RECEIVE_CALIBRATION_TIME)
#define MIN_BACK_OFF_EXPONENT 2
#define MAX_BACK_OFF_EXPONENT 5
#define SCAN_DURATION (RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE \
    * (CSL_MAX_WAKE_UP_FRAME_LEN + RADIO_SHR_LEN)) + 2)
#define MIN_PREPARE_LEAD_OVER_LOOP (10)
#define CCA_SLEEP_DURATION (RADIO_RECEIVE_CALIBRATION_TIME \
    + RADIO_CCA_TIME \
    - 3)

#if !CSL_COMPLIANT
/**
 * For caching information about wake-up frames with extremely late
 * rendezvous times such that we can do something else in the meantime.
 */
struct late_rendezvous {
  struct late_rendezvous *next;
  rtimer_clock_t time;
  enum csl_subtype subtype;
  uint8_t channel;
};
#define LATE_WAKE_UP_GUARD_TIME (US_TO_RTIMERTICKS(10000))
#define LATE_RENDEZVOUS_TRESHOLD (US_TO_RTIMERTICKS(20000))

#define HELLO_WAKE_UP_SEQUENCE_LENGTH CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH( \
    WAKE_UP_COUNTER_INTERVAL * sizeof(channels), \
    CSL_FRAMER_POTR_HELLO_WAKE_UP_FRAME_LEN)
#define CSL_HELLO_WAKE_UP_SEQUENCE_TX_TIME RADIO_TIME_TO_TRANSMIT( \
    HELLO_WAKE_UP_SEQUENCE_LENGTH \
    * CSL_FRAMER_POTR_HELLO_WAKE_UP_FRAME_LEN \
    * RADIO_SYMBOLS_PER_BYTE)
#endif /* !CSL_COMPLIANT */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL"
#define LOG_LEVEL LOG_LEVEL_MAC

struct buffered_frame {
  struct buffered_frame *next;
  struct queuebuf *qb;
  mac_callback_t sent;
  int transmissions;
  rtimer_clock_t next_attempt;
  void *ptr;
};

static void schedule_duty_cycle(rtimer_clock_t time);
static int schedule_duty_cycle_precise(rtimer_clock_t time);
static void duty_cycle_wrapper(struct rtimer *t, void *ptr);
static char duty_cycle(void);
static void on_shr(void);
static void on_wake_up_frame_fifop(void);
static void on_payload_frame_fifop(void);
static void on_final_payload_frame_fifop(void);
static void on_txdone(void);
static void delay_any_frames_to(const linkaddr_t *receiver, rtimer_clock_t next_attempt);
static struct buffered_frame *select_next_frame_to_transmit(void);
static struct buffered_frame *select_next_burst_frame(struct buffered_frame *bf);
static uint8_t prepare_next_wake_up_frames(uint8_t space);
static void schedule_transmission(rtimer_clock_t time);
static int schedule_transmission_precise(rtimer_clock_t time);
static void transmit_wrapper(struct rtimer *rt, void *ptr);
static char transmit(void);
static void on_transmitted(void);
static void try_skip_to_send(void);
static void queue_frame(mac_callback_t sent, void *ptr);

csl_state_t csl_state;
static const uint8_t channels[] = CHANNELS;
MEMB(buffered_frames_memb, struct buffered_frame, QUEUEBUF_NUM); /* holds outgoing frames */
LIST(buffered_frames_list);
static struct rtimer timer;
static rtimer_clock_t duty_cycle_next; /* stores when the next channel sample will be made */
static struct pt pt;
static int is_duty_cycling;
static int is_transmitting;
static int can_skip;
static int skipped;
PROCESS(post_processing, "post processing");
static rtimer_clock_t last_payload_frame_sfd_timestamp;
#if !CSL_COMPLIANT
MEMB(late_rendezvous_memb, struct late_rendezvous, sizeof(channels));
LIST(late_rendezvous_list);
static rtimer_clock_t wake_up_counter_last_increment;
wake_up_counter_t csl_wake_up_counter;
const uint32_t csl_hello_wake_up_sequence_length
    = HELLO_WAKE_UP_SEQUENCE_LENGTH;
const rtimer_clock_t csl_hello_wake_up_sequence_tx_time
    = CSL_HELLO_WAKE_UP_SEQUENCE_TX_TIME;
#elif !LLSEC802154_USES_FRAME_COUNTER
static uint8_t mac_dsn;
#endif /* !LLSEC802154_USES_FRAME_COUNTER */

/*---------------------------------------------------------------------------*/
#if !CSL_COMPLIANT
static int
is_anything_locked(void)
{
  return aes_128_locked || akes_nbr_locked || nbr_table_locked;
}
/*---------------------------------------------------------------------------*/
wake_up_counter_t
csl_get_wake_up_counter(rtimer_clock_t t)
{
  rtimer_clock_t delta;
  wake_up_counter_t wuc;

  delta = RTIMER_CLOCK_DIFF(t, wake_up_counter_last_increment);
  wuc = csl_wake_up_counter;
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
  struct akes_nbr_entry *entry;
  wake_up_counter_t wuc;
  rtimer_clock_t delta;
  int32_t drift;
  uint32_t seconds_since_last_sync;
  int32_t compensation;
  csl_nbr_t *csl_nbr;

  entry = akes_nbr_get_sender_entry();
  if(!entry || !entry->permanent) {
    wuc.u32 = 0;
    LOG_ERR("could not restore wake-up counter\n");
    return wuc;
  }
  csl_nbr = csl_nbr_get(entry->permanent);

  drift = csl_nbr->drift;
  if(drift == AKES_NBR_UNINITIALIZED_DRIFT) {
    compensation = 0;
  } else {
    seconds_since_last_sync = RTIMERTICKS_TO_S(
        RTIMER_CLOCK_DIFF(csl_get_sfd_timestamp_of_last_payload_frame(),
        csl_nbr->sync_data.t));
    compensation = ((int64_t)drift
        * (int64_t)seconds_since_last_sync / (int64_t)1000000);
  }

  delta = csl_get_sfd_timestamp_of_last_payload_frame()
      - csl_nbr->sync_data.t
      + compensation
      - (WAKE_UP_COUNTER_INTERVAL / 2);
  wuc.u32 = csl_nbr->sync_data.his_wake_up_counter_at_t.u32
      + wake_up_counter_round_increments(delta);
  return wuc;
}
/*---------------------------------------------------------------------------*/
static void
set_channel(wake_up_counter_t wuc, const linkaddr_t *addr)
{
  uint8_t i;
  uint8_t xored;

  xored = wuc.u8[0];
  for(i = 0; i < LINKADDR_SIZE; i++) {
    xored ^= addr->u8[i];
  }

  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL,
      channels[xored & (sizeof(channels) - 1)]);
}
/*---------------------------------------------------------------------------*/
uint8_t
csl_get_channel(void)
{
  radio_value_t rv;

  NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &rv);
  return (uint8_t)rv;
}
/*---------------------------------------------------------------------------*/
static void
delete_late_rendezvous(struct late_rendezvous *lr)
{
  list_remove(late_rendezvous_list, lr);
  memb_free(&late_rendezvous_memb, lr);
}
/*---------------------------------------------------------------------------*/
static void
clear_missed_late_rendezvous(void)
{
  struct late_rendezvous *next;
  struct late_rendezvous *current;

  next = list_head(late_rendezvous_list);
  while(next) {
    current = next;
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
  struct late_rendezvous *nearest;
  struct late_rendezvous *next;

  clear_missed_late_rendezvous();
  nearest = next = list_head(late_rendezvous_list);
  while(nearest && ((next = list_item_next(next)))) {
    if(RTIMER_CLOCK_LT_OR_EQ(next->time, nearest->time)) {
      nearest = next;
    }
  }
  return nearest;
}
/*---------------------------------------------------------------------------*/
static int
has_late_rendezvous_on_channel(uint8_t channel)
{
  struct late_rendezvous *lr;

  clear_missed_late_rendezvous();
  lr = list_head(late_rendezvous_list);
  while(lr) {
    if(lr->channel == channel) {
      return 1;
    }
    lr = list_item_next(lr);
  }
  return 0;
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
disable_local_packetbuf(uint8_t burst_index)
{
  packetbuf = csl_state.duty_cycle.actual_packetbuf[burst_index];
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  NETSTACK_RADIO.enter_async_mode();
#if CSL_COMPLIANT
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channels[0]);
#if !LLSEC802154_USES_FRAME_COUNTER
  mac_dsn = random_rand();
#endif /* !LLSEC802154_USES_FRAME_COUNTER */
#else /* CSL_COMPLIANT */
  LOG_INFO("t_u = %lu\n", CSL_MAX_OVERALL_UNCERTAINTY);
  memb_init(&late_rendezvous_memb);
  list_init(late_rendezvous_list);
#endif /* CSL_COMPLIANT */
  CSL_SYNCHRONIZER.init();
  CSL_FRAMER.init();
  memb_init(&buffered_frames_memb);
  list_init(buffered_frames_list);
  NETSTACK_RADIO.async_set_shr_callback(on_shr);
  NETSTACK_RADIO.async_set_txdone_callback(on_txdone);
  NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, OUTPUT_POWER);
  process_start(&post_processing, NULL);
  PT_INIT(&pt);
  duty_cycle_next = RTIMER_NOW() + WAKE_UP_COUNTER_INTERVAL;
  schedule_duty_cycle(duty_cycle_next - CSL_LPM_DEEP_SWITCHING);
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
 * receiving payload frames, and sending acknowledgement frames.
 */
static char
duty_cycle(void)
{
#if !CSL_COMPLIANT
  rtimer_clock_t last_wake_up_time;
  struct late_rendezvous *lr;
#endif /* !CSL_COMPLIANT */

  PT_BEGIN(&pt);
#ifdef LPM_CONF_ENABLE
  lpm_set_max_pm(LPM_PM1);
#endif /* LPM_CONF_ENABLE */
  can_skip = 0;
  is_duty_cycling = 1;

  if(skipped) {
    skipped = 0;
#if !CSL_COMPLIANT
  } else if(!csl_state.duty_cycle.skip_to_rendezvous
      && has_late_rendezvous_on_channel(csl_get_channel())) {
    /* skipping duty cycle */
#endif /* !CSL_COMPLIANT */
  } else {
    if(!SHALL_SKIP_TO_RENDEZVOUS) {

#if !CSL_COMPLIANT
      last_wake_up_time = csl_get_last_wake_up_time();
      csl_wake_up_counter = csl_get_wake_up_counter(last_wake_up_time);
      wake_up_counter_last_increment = last_wake_up_time;
      set_channel(csl_wake_up_counter, &linkaddr_node_addr);
#endif /* !CSL_COMPLIANT */
      NETSTACK_RADIO.async_set_fifop_callback(on_wake_up_frame_fifop,
          1 /* Frame Length */ + CSL_MIN_BYTES_FOR_PARSING_WAKE_UP_FRAMES);

      /* if we come from PM0, we will be too early */
      while(!rtimer_has_timed_out(duty_cycle_next));

      NETSTACK_RADIO.async_on();
      csl_state.duty_cycle.waiting_for_wake_up_frames_shr = 1;
      csl_state.duty_cycle.wake_up_frame_timeout = RTIMER_NOW()
          + RADIO_RECEIVE_CALIBRATION_TIME
          + SCAN_DURATION;
      schedule_duty_cycle(csl_state.duty_cycle.wake_up_frame_timeout);
      /* wait until timeout or on_wake_up_frame_fifop, whatever comes first */
      PT_YIELD(&pt);
      csl_state.duty_cycle.waiting_for_wake_up_frames_shr = 0;
    }
    if(!SHALL_SKIP_TO_RENDEZVOUS && !csl_state.duty_cycle.got_wake_up_frames_shr) {
      NETSTACK_RADIO.async_off();
    } else {
#if CRYPTO_CONF_INIT
      crypto_enable();
#endif /* CRYPTO_CONF_INIT */
      if(!SHALL_SKIP_TO_RENDEZVOUS) {
        /* wait until timeout or on_wake_up_frame_fifop, whatever comes last */
        PT_YIELD(&pt);
      }
      if(csl_state.duty_cycle.got_rendezvous_time) {
#if !CSL_COMPLIANT
        if(!csl_state.duty_cycle.left_radio_on
            && !SHALL_SKIP_TO_RENDEZVOUS
            && !RTIMER_CLOCK_LT_OR_EQ(
                csl_state.duty_cycle.rendezvous_time,
                RTIMER_NOW() + LATE_RENDEZVOUS_TRESHOLD)) {
          lr = memb_alloc(&late_rendezvous_memb);
          if(lr) {
            lr->time = csl_state.duty_cycle.rendezvous_time;
            lr->subtype = csl_state.duty_cycle.subtype;
            lr->channel = csl_get_channel();
            list_add(late_rendezvous_list, lr);
          } else {
            LOG_ERR("late_rendezvous_memb is full\n");
          }
        } else
#endif /* !CSL_COMPLIANT */
        {
          csl_state.duty_cycle.min_bytes_for_filtering
              = CSL_FRAMER.get_min_bytes_for_filtering();
          NETSTACK_RADIO.async_set_fifop_callback(on_payload_frame_fifop,
              1 + csl_state.duty_cycle.min_bytes_for_filtering);
          if(!csl_state.duty_cycle.left_radio_on) {
            if(!SHALL_SKIP_TO_RENDEZVOUS
                && (schedule_duty_cycle_precise(csl_state.duty_cycle.rendezvous_time - RENDEZVOUS_GUARD_TIME) == RTIMER_OK)) {
              PT_YIELD(&pt); /* wait until rendezvous */
            }
            /* if we come from PM0 we will be too early */
            while(!rtimer_has_timed_out(csl_state.duty_cycle.rendezvous_time
                - NEGATIVE_RENDEZVOUS_TIME_ACCURACY
                - RADIO_RECEIVE_CALIBRATION_TIME));
            NETSTACK_RADIO.async_on();
          }
          csl_state.duty_cycle.waiting_for_payload_frames_shr = 1;
          schedule_duty_cycle(csl_state.duty_cycle.rendezvous_time
              + RADIO_SHR_TIME
              + POSITIVE_RENDEZVOUS_TIME_ACCURACY);
          while(1) {
            /* wait until timeout */
            PT_YIELD(&pt);
            csl_state.duty_cycle.waiting_for_payload_frames_shr = 0;

            if(!csl_state.duty_cycle.got_payload_frames_shr) {
              LOG_ERR("missed %spayload frame %i\n",
                     csl_state.duty_cycle.last_burst_index ? "bursted "
                  : (SHALL_SKIP_TO_RENDEZVOUS ? "late "
                  : (csl_state.duty_cycle.left_radio_on ? "early "
                  : "")), csl_state.duty_cycle.remaining_wake_up_frames);
              NETSTACK_RADIO.async_off();
              csl_state.duty_cycle.last_burst_index = csl_state.duty_cycle.last_burst_index
                  ? csl_state.duty_cycle.last_burst_index - 1
                  : 0;
              break;
            }

            PT_YIELD(&pt); /* wait until on_payload_frame_fifop */
            if(csl_state.duty_cycle.rejected_payload_frame) {
              csl_state.duty_cycle.last_burst_index = csl_state.duty_cycle.last_burst_index
                  ? csl_state.duty_cycle.last_burst_index - 1
                  : 0;
              break;
            }

            PT_YIELD(&pt); /* wait either until on_final_payload_frame_fifop or on_txdone */
            if(!csl_state.duty_cycle.frame_pending) {
              break;
            }

            csl_state.duty_cycle.last_burst_index++;
            csl_state.duty_cycle.min_bytes_for_filtering
                = CSL_FRAMER.get_min_bytes_for_filtering();
            NETSTACK_RADIO.async_set_fifop_callback(on_payload_frame_fifop,
                1 + csl_state.duty_cycle.min_bytes_for_filtering);
            csl_state.duty_cycle.got_payload_frames_shr = 0;
            csl_state.duty_cycle.waiting_for_payload_frames_shr = 1;
            csl_state.duty_cycle.left_radio_on = 0;
            csl_state.duty_cycle.remaining_wake_up_frames = 0;
            schedule_duty_cycle(RTIMER_NOW() + CSL_ACKNOWLEDGEMENT_WINDOW_MAX);
          }
        }
      }
    }
    NETSTACK_RADIO.async_set_fifop_callback(NULL, 0);
  }

  is_duty_cycling = 0;
  process_poll(&post_processing);

  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
/** Called when a start-of-frame delimiter was received or transmitted */
static void
on_shr(void)
{
  rtimer_clock_t now;
  uint8_t wake_up_frame_len;
  uint8_t wake_up_frame[CSL_MAX_WAKE_UP_FRAME_LEN - RADIO_PHY_HEADER_LEN];

  now = RTIMER_NOW();

  if(is_duty_cycling) {
    if(csl_state.duty_cycle.waiting_for_unwanted_shr) {
      csl_state.duty_cycle.waiting_for_unwanted_shr = 0;
    } else if(csl_state.duty_cycle.waiting_for_wake_up_frames_shr) {
      csl_state.duty_cycle.got_wake_up_frames_shr = 1;
      csl_state.duty_cycle.wake_up_frame_sfd_timestamp = now;
      rtimer_cancel();
    } else if(csl_state.duty_cycle.waiting_for_payload_frames_shr) {
      if(csl_state.duty_cycle.left_radio_on && csl_state.duty_cycle.remaining_wake_up_frames) {
        wake_up_frame_len = NETSTACK_RADIO.async_read_phy_header();
        if((wake_up_frame_len > CSL_MAX_WAKE_UP_FRAME_LEN)
            || !NETSTACK_RADIO.async_read_payload(wake_up_frame, wake_up_frame_len)) {
          LOG_WARN("something went wrong while scanning for the payload frame\n");
          return;
        }
      }
      csl_state.duty_cycle.got_payload_frames_shr = 1;
      last_payload_frame_sfd_timestamp = now;
    }
  } else if(is_transmitting) {
    if(csl_state.transmit.waiting_for_acknowledgement_shr) {
      csl_state.transmit.got_acknowledgement_shr = 1;
      if(!csl_state.transmit.burst_index) {
        csl_state.transmit.acknowledgement_sfd_timestamp = now;
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
  csl_state.duty_cycle.got_rendezvous_time
      = CSL_FRAMER.parse_wake_up_frame() != FRAMER_FAILED;

  if(!csl_state.duty_cycle.got_rendezvous_time
      || (csl_state.duty_cycle.remaining_wake_up_frames >= 2)) {
    NETSTACK_RADIO.async_off();
  } else {
    csl_state.duty_cycle.left_radio_on = 1;
    if(csl_state.duty_cycle.remaining_wake_up_frames == 1) {
      csl_state.duty_cycle.waiting_for_unwanted_shr = 1;
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
  NETSTACK_RADIO.async_set_fifop_callback(NULL, RADIO_MAX_FRAME_LEN);
  enable_local_packetbuf(csl_state.duty_cycle.last_burst_index);
  packetbuf_set_attr(PACKETBUF_ATTR_RSSI, radio_get_rssi());

  if(csl_state.duty_cycle.last_burst_index) {
    packetbuf_set_attr(PACKETBUF_ATTR_BURST_INDEX, csl_state.duty_cycle.last_burst_index);
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &csl_state.duty_cycle.sender);
  }

#if CSL_COMPLIANT
  if(0
#else /* CSL_COMPLIANT */
  if(is_anything_locked()
#endif /* CSL_COMPLIANT */
      || (NETSTACK_RADIO.async_read_phy_header_to_packetbuf()
          < csl_state.duty_cycle.min_bytes_for_filtering)
      || !NETSTACK_RADIO.async_read_payload_to_packetbuf(csl_state.duty_cycle.min_bytes_for_filtering)
      || (CSL_FRAMER.filter(csl_state.duty_cycle.acknowledgement) == FRAMER_FAILED)) {
    NETSTACK_RADIO.async_off();
    LOG_INFO("rejected payload frame of length %i\n", packetbuf_datalen());
    csl_state.duty_cycle.rejected_payload_frame = 1;
  } else {
    csl_state.duty_cycle.frame_pending = packetbuf_attr(PACKETBUF_ATTR_PENDING)
        && (csl_state.duty_cycle.last_burst_index < CSL_MAX_BURST_INDEX);
    linkaddr_copy(&csl_state.duty_cycle.sender, packetbuf_addr(PACKETBUF_ADDR_SENDER));
    csl_state.duty_cycle.shall_send_acknowledgement = !packetbuf_holds_broadcast();
    if(csl_state.duty_cycle.shall_send_acknowledgement) {
      NETSTACK_RADIO.async_prepare(csl_state.duty_cycle.acknowledgement);
    }
    NETSTACK_RADIO.async_set_fifop_callback(on_final_payload_frame_fifop,
        NETSTACK_RADIO.async_remaining_payload_bytes());
  }

  disable_local_packetbuf(csl_state.duty_cycle.last_burst_index);

  duty_cycle();
}
/*---------------------------------------------------------------------------*/
/** Called as soon as the whole payload frame was received */
static void
on_final_payload_frame_fifop(void)
{
#if !CSL_COMPLIANT
  struct akes_nbr_entry *entry;
#endif /* !CSL_COMPLIANT */
  int successful;

  /* avoid that on_final_payload_frame_fifop is called twice */
  NETSTACK_RADIO.async_set_fifop_callback(NULL, 0);

  if(csl_state.duty_cycle.shall_send_acknowledgement) {
    NETSTACK_RADIO.async_transmit(csl_state.duty_cycle.frame_pending);
  } else if(!csl_state.duty_cycle.frame_pending) {
    NETSTACK_RADIO.async_off();
  }

  enable_local_packetbuf(csl_state.duty_cycle.last_burst_index);

  if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(NETSTACK_RADIO.async_remaining_payload_bytes())) {
    LOG_ERR("NETSTACK_RADIO.async_read_payload_to_packetbuf failed\n");
    successful = 0;
  } else if(NETSTACK_FRAMER.parse() == FRAMER_FAILED) {
    LOG_ERR("NETSTACK_FRAMER.parse failed\n");
    successful = 0;
#if CSL_COMPLIANT
  } else {
    successful = 1;
  }
#else /* CSL_COMPLIANT */
  } else if((csl_state.duty_cycle.subtype == CSL_SUBTYPE_HELLOACK)
      || !csl_state.duty_cycle.shall_send_acknowledgement) {
    successful = 1;
  } else if(is_anything_locked()) {
    LOG_ERR("something is locked\n");
    successful = 0;
  } else if(!((entry = akes_nbr_get_sender_entry()))) {
    LOG_ERR("sender not found\n");
    successful = 0;
  } else if(csl_state.duty_cycle.subtype == CSL_SUBTYPE_ACK) {
    successful = entry->tentative
        && !memcmp(((uint8_t *)packetbuf_dataptr()) + 1 /* command frame identifier */ + 1 /* neighbor index */ + CSL_FRAMER_POTR_PHASE_LEN,
            csl_nbr_get_tentative(entry->tentative->meta)->q,
            AKES_NBR_CHALLENGE_LEN)
        && !AKES_MAC_STRATEGY.verify(entry->tentative);
  } else {
    successful = entry->permanent
        && !AKES_MAC_STRATEGY.verify(entry->permanent);
  }
#endif /* CSL_COMPLIANT */

  disable_local_packetbuf(csl_state.duty_cycle.last_burst_index);

  if(successful) {
    csl_state.duty_cycle.received_frame = 1;
  }

  if(!csl_state.duty_cycle.shall_send_acknowledgement) {
    duty_cycle();
  } else if(!successful) {
    /* abort ongoing acknowledgement transmission */
    NETSTACK_RADIO.async_off();
    csl_state.duty_cycle.frame_pending = 0;
    LOG_INFO("flushing unicast frame\n");
    duty_cycle();
  }
}
/*---------------------------------------------------------------------------*/
/** Called right after the transmission of an acknowledgement or payload frame */
static void
on_txdone(void)
{
  if(is_duty_cycling) {
    duty_cycle();
  } else if(is_transmitting && csl_state.transmit.is_waiting_for_txdone) {
    transmit();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * Passes received frames to upper-layer protocols, as well as prepares
 * outgoing transmissions.
 */
PROCESS_THREAD(post_processing, ev, data)
{
  struct buffered_frame *next;
  uint8_t burst_index;
  uint8_t i;
  int create_result;
  uint8_t wake_up_frame[CSL_MAX_WAKE_UP_FRAME_LEN];
  int sent_once;
  rtimer_clock_t end_of_transmission;
  uint8_t prepared_bytes;
#if !CSL_COMPLIANT
  struct late_rendezvous *lr;
#endif /* !CSL_COMPLIANT */

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    if(csl_state.duty_cycle.received_frame) {
      for(burst_index = 0; burst_index <= csl_state.duty_cycle.last_burst_index; burst_index++) {
        enable_local_packetbuf(burst_index);
#if AKES_MAC_ENABLED
        NETSTACK_MAC.input();
#else /* AKES_MAC_ENABLED */
        if(mac_sequence_is_duplicate()) {
          LOG_ERR("received duplicate\n");
        } else {
          mac_sequence_register_seqno();
          NETSTACK_NETWORK.input();
        }
#endif /* AKES_MAC_ENABLED */
        disable_local_packetbuf(burst_index);
      }
    }

#if CRYPTO_CONF_INIT
    crypto_disable();
#endif /* CRYPTO_CONF_INIT */
    PROCESS_PAUSE();

    /* send queued frames */
    sent_once = 0;
    while((next = select_next_frame_to_transmit())) {
#if CRYPTO_CONF_INIT
      crypto_enable();
#endif /* CRYPTO_CONF_INIT */
      memset(&csl_state.transmit, 0, sizeof(csl_state.transmit));
      csl_state.transmit.bf[0] = next;
      queuebuf_to_packetbuf(csl_state.transmit.bf[0]->qb);

      /* what kind of payload frame do we have here? */
      csl_state.transmit.is_broadcast = packetbuf_holds_broadcast();
#if !CSL_COMPLIANT
      if(akes_mac_is_hello()) {
        csl_state.transmit.subtype = CSL_SUBTYPE_HELLO;
      } else if(akes_mac_is_helloack()) {
        csl_state.transmit.subtype = CSL_SUBTYPE_HELLOACK;
      } else {
        csl_state.transmit.subtype = akes_mac_is_ack()
            ? CSL_SUBTYPE_ACK
            : CSL_SUBTYPE_NORMAL;
      }
#endif /* !CSL_COMPLIANT */
      csl_state.transmit.wake_up_frame_len = CSL_FRAMER.get_length_of_wake_up_frame()
          + RADIO_PHY_HEADER_LEN;

      /* schedule */
      if(!CSL_SYNCHRONIZER.schedule()) {
        LOG_ERR("CSL_SYNCHRONIZER.schedule failed\n");
        csl_state.transmit.result[0] = MAC_TX_ERR_FATAL;
        on_transmitted();
        continue;
      }

      /* TODO compute precisely */
      end_of_transmission = csl_state.transmit.payload_frame_start + US_TO_RTIMERTICKS(6000);
      if(sent_once && !RTIMER_CLOCK_LT_OR_EQ(end_of_transmission,
          wake_up_counter_shift_to_future(duty_cycle_next))) {
        /* cancel second transmission to avoid skipping over the next wake up */
        break;
      }

#if !CSL_COMPLIANT
      /* set channel */
      if(csl_state.transmit.subtype == CSL_SUBTYPE_HELLO) {
        set_channel(csl_get_wake_up_counter(csl_get_payload_frames_shr_end()), &linkaddr_node_addr);
      } else {
        set_channel(csl_predict_wake_up_counter(), packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
      }
      lr = get_nearest_late_rendezvous();
      if(has_late_rendezvous_on_channel(csl_get_channel())
          || (lr && (csl_state.transmit.subtype == CSL_SUBTYPE_HELLO))
          || (lr && !RTIMER_CLOCK_LT_OR_EQ(end_of_transmission, lr->time))) {
        if(csl_state.transmit.subtype == CSL_SUBTYPE_HELLO) {
          csl_state.transmit.bf[0]->next_attempt = RTIMER_NOW()
              + csl_hello_wake_up_sequence_tx_time;
        } else {
          delay_any_frames_to(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
              RTIMER_NOW() + WAKE_UP_COUNTER_INTERVAL);
        }
        continue;
      }
#endif /* !CSL_COMPLIANT */

      if(!CSL_FRAMER.prepare_acknowledgement_parsing()) {
        LOG_ERR("CSL_FRAMER.prepare_acknowledgement_parsing failed\n");
        csl_state.transmit.result[0] = MAC_TX_ERR_FATAL;
        on_transmitted();
        continue;
      }

#if !CSL_COMPLIANT
      if(csl_state.transmit.subtype == CSL_SUBTYPE_NORMAL)
#endif /* !CSL_COMPLIANT */
      {
        /* check if we can burst more payload frames */
        while(csl_state.transmit.last_burst_index < CSL_MAX_BURST_INDEX) {
          /* TODO compute precisely */
          end_of_transmission = csl_state.transmit.payload_frame_start
              + ((csl_state.transmit.last_burst_index + 1) * US_TO_RTIMERTICKS(6000));
#if !CSL_COMPLIANT
          if(lr && !RTIMER_CLOCK_LT_OR_EQ(end_of_transmission, lr->time)) {
            /* we do not want to miss our late rendezvous */
            break;
          }
#endif /* !CSL_COMPLIANT */
          if(sent_once && !RTIMER_CLOCK_LT_OR_EQ(end_of_transmission, wake_up_counter_shift_to_future(duty_cycle_next))) {
            /* we do not want to skip over our next wake up */
            break;
          }

          csl_state.transmit.bf[csl_state.transmit.last_burst_index + 1] =
              select_next_burst_frame(csl_state.transmit.bf[csl_state.transmit.last_burst_index]);
          if(!csl_state.transmit.bf[csl_state.transmit.last_burst_index + 1]) {
            break;
          }
          csl_state.transmit.last_burst_index++;
        }
      }

      /* create payload frame(s) */
      i = csl_state.transmit.last_burst_index;
      do {
        queuebuf_to_packetbuf(csl_state.transmit.bf[i]->qb);
        packetbuf_set_attr(PACKETBUF_ATTR_BURST_INDEX, i);
        packetbuf_set_attr(PACKETBUF_ATTR_PENDING,
            ((i < CSL_MAX_BURST_INDEX) && csl_state.transmit.bf[i + 1])
                ? csl_state.transmit.payload_frame[i + 1][0]
                : 0);
        create_result = NETSTACK_FRAMER.create();
        if(create_result == FRAMER_FAILED) {
          break;
        }
        csl_state.transmit.payload_frame[i][0] = packetbuf_totlen();
        memcpy(csl_state.transmit.payload_frame[i] + 1,
            packetbuf_hdrptr(),
            packetbuf_totlen());
      } while(i--);
      if(create_result == FRAMER_FAILED) {
        LOG_ERR("NETSTACK_FRAMER.create failed\n");
        csl_state.transmit.result[0] = MAC_TX_ERR_FATAL;
        on_transmitted();
        continue;
      }
      csl_state.transmit.remaining_payload_frame_bytes = csl_state.transmit.payload_frame[0][0];

      /* prepare wake-up sequence */
      memcpy(wake_up_frame, radio_shr, RADIO_SHR_LEN);
      if(CSL_FRAMER.create_wake_up_frame(wake_up_frame + RADIO_SHR_LEN)
          == FRAMER_FAILED) {
        LOG_ERR("wake-up frame creation failed\n");
        csl_state.transmit.result[0] = MAC_TX_ERR_FATAL;
        on_transmitted();
        continue;
      }
      for(i = 0; i <= (RADIO_MAX_SEQUENCE_LEN - csl_state.transmit.wake_up_frame_len); i += csl_state.transmit.wake_up_frame_len) {
        memcpy(csl_state.transmit.next_wake_up_frames + i, wake_up_frame, csl_state.transmit.wake_up_frame_len);
      }
      prepared_bytes = prepare_next_wake_up_frames(RADIO_MAX_SEQUENCE_LEN);
      NETSTACK_RADIO.async_prepare_sequence(csl_state.transmit.next_wake_up_frames, prepared_bytes);

      /* schedule transmission */
      if(schedule_transmission_precise(csl_state.transmit.wake_up_sequence_start - CSL_WAKE_UP_SEQUENCE_GUARD_TIME) != RTIMER_OK) {
        LOG_ERR("Transmission is not schedulable\n");
        csl_state.transmit.result[0] = MAC_TX_ERR;
        on_transmitted();
        continue;
      }
      PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
      on_transmitted();
      PROCESS_PAUSE();
      sent_once = 1;
    }

    /* prepare next duty cycle */
#if CRYPTO_CONF_INIT
    crypto_disable();
#endif /* CRYPTO_CONF_INIT */
#ifdef LPM_CONF_ENABLE
    lpm_set_max_pm(LPM_CONF_MAX_PM);
#endif /* LPM_CONF_ENABLE */

    duty_cycle_next = wake_up_counter_shift_to_future(duty_cycle_next);
    while(1) {
      memset(&csl_state.duty_cycle, 0, sizeof(csl_state.duty_cycle));
#if !CSL_COMPLIANT
      lr = get_nearest_late_rendezvous();
      if(!lr
          || (RTIMER_CLOCK_LT_OR_EQ(
              duty_cycle_next + LATE_WAKE_UP_GUARD_TIME, lr->time))) {
#endif /* !CSL_COMPLIANT */
        if(schedule_duty_cycle_precise(duty_cycle_next - CSL_LPM_DEEP_SWITCHING) != RTIMER_OK) {
          duty_cycle_next += WAKE_UP_COUNTER_INTERVAL;
          continue;
        }
#if !CSL_COMPLIANT
      } else {
        csl_state.duty_cycle.rendezvous_time = lr->time;
        csl_state.duty_cycle.got_rendezvous_time = 1;
        csl_state.duty_cycle.subtype = lr->subtype;
        csl_state.duty_cycle.skip_to_rendezvous = 1;
        if(schedule_duty_cycle_precise(lr->time
            - RENDEZVOUS_GUARD_TIME
            - (CSL_LPM_DEEP_SWITCHING - CSL_LPM_SWITCHING)) != RTIMER_OK) {
          delete_late_rendezvous(lr);
          LOG_ERR("missed late rendezvous\n");
          continue;
        }
        NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, lr->channel);
        delete_late_rendezvous(lr);
      }
#endif /* !CSL_COMPLIANT */
      break;
    }
#if CSL_COMPLIANT
    can_skip = 0;
#else /* CSL_COMPLIANT */
    can_skip = lr ? 0 : 1;
#endif /* CSL_COMPLIANT */
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * This function delays the transmission of all outgoing frames to a
 * particular neighbor, e.g., as a result of a collision.
 */
static void
delay_any_frames_to(const linkaddr_t *receiver, rtimer_clock_t next_attempt)
{
  struct buffered_frame *next;

  next = list_head(buffered_frames_list);
  while(next) {
    if(linkaddr_cmp(receiver, queuebuf_addr(next->qb, PACKETBUF_ADDR_RECEIVER))) {
      next->next_attempt = next_attempt;
    }
    next = list_item_next(next);
  }
}
/*---------------------------------------------------------------------------*/
static struct buffered_frame *
select_next_frame_to_transmit(void)
{
  rtimer_clock_t now;
  struct buffered_frame *next;

  now = RTIMER_NOW();
  next = list_head(buffered_frames_list);
  while(next) {
    if(RTIMER_CLOCK_LT_OR_EQ(next->next_attempt, now)) {
      if(next->transmissions) {
        LOG_INFO("retransmission %i\n", next->transmissions);
      }
      return next;
    }
    next = list_item_next(next);
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static struct buffered_frame *
select_next_burst_frame(struct buffered_frame *bf)
{
  rtimer_clock_t now;
  linkaddr_t *receiver;

  now = RTIMER_NOW();
  receiver = queuebuf_addr(bf->qb, PACKETBUF_ADDR_RECEIVER);
  while((bf = list_item_next(bf))) {
    if(linkaddr_cmp(receiver, queuebuf_addr(bf->qb, PACKETBUF_ADDR_RECEIVER))) {
      return RTIMER_CLOCK_LT_OR_EQ(bf->next_attempt, now) ? bf : NULL;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
static uint8_t
prepare_next_wake_up_frames(uint8_t space)
{
  uint8_t prepared_bytes;
  uint8_t number_of_wake_up_frames;
  uint8_t i;
  uint8_t bytes;

  /* append the next wake-up frames */
  number_of_wake_up_frames = MIN(csl_state.transmit.remaining_wake_up_frames,
      space / csl_state.transmit.wake_up_frame_len);
  for(i = 0; i < number_of_wake_up_frames; i++) {
    csl_state.transmit.remaining_wake_up_frames--;

    /* update the wake-up frame's rendezvous time, as well as its checksum (if any) */
    CSL_FRAMER.update_rendezvous_time(csl_state.transmit.next_wake_up_frames
        + (i * csl_state.transmit.wake_up_frame_len)
        + RADIO_SHR_LEN);
  }
  prepared_bytes = number_of_wake_up_frames * csl_state.transmit.wake_up_frame_len;
  space -= prepared_bytes;
  csl_state.transmit.wake_up_sequence_pos += prepared_bytes;

  /*
   * append the first payload frame - the first payload
   * frame is sent right after the last wake-up frame
   */
  if(!csl_state.transmit.remaining_wake_up_frames
      && (space >= RADIO_PHY_HEADER_LEN)) {
    if(!csl_state.transmit.wrote_payload_frames_phy_header) {
      memcpy(csl_state.transmit.next_wake_up_frames + prepared_bytes, radio_shr, RADIO_SHR_LEN);
      prepared_bytes += RADIO_SHR_LEN;
      csl_state.transmit.next_wake_up_frames[prepared_bytes] = csl_state.transmit.payload_frame[0][0];
      prepared_bytes += 1;
      space -= RADIO_PHY_HEADER_LEN;
      csl_state.transmit.wake_up_sequence_pos += RADIO_PHY_HEADER_LEN;
      csl_state.transmit.wrote_payload_frames_phy_header = 1;
    }

    bytes = MIN(space, csl_state.transmit.remaining_payload_frame_bytes);
    memcpy(csl_state.transmit.next_wake_up_frames + prepared_bytes,
        csl_state.transmit.payload_frame[0]
        + 1 /* Frame Length */
        + csl_state.transmit.payload_frame[0][0]
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
 * payload frames, and receiving acknowledgement frames.
 */
static char
transmit(void)
{
  uint8_t prepared_bytes;

  PT_BEGIN(&pt);
  is_transmitting = 1;

  csl_state.transmit.bf[0]->transmissions++;
  /* if we come from PM0 we will be too early */
  while(!rtimer_has_timed_out(csl_state.transmit.wake_up_sequence_start
      - (CSL_WAKE_UP_SEQUENCE_GUARD_TIME - CSL_LPM_SWITCHING)));
  NETSTACK_RADIO.async_on();
  schedule_transmission(RTIMER_NOW() + CCA_SLEEP_DURATION);
  PT_YIELD(&pt);
  if(radio_get_rssi() >= CCA_THRESHOLD) {
    NETSTACK_RADIO.async_off();
    LOG_INFO("collision\n");
    csl_state.transmit.result[0] = MAC_TX_COLLISION;
  } else {
    /* send the wake-up sequence, as well as the first payload frame */
    NETSTACK_RADIO.async_transmit_sequence();
    while(1) {
      csl_state.transmit.next_rendezvous_time_update = csl_state.transmit.wake_up_sequence_start
          + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE
              * (csl_state.transmit.wake_up_sequence_pos - (MIN_PREPARE_LEAD_OVER_LOOP / 2)));
      if(!csl_state.transmit.remaining_wake_up_frames && !csl_state.transmit.remaining_payload_frame_bytes) {
        break;
      }
      schedule_transmission(csl_state.transmit.next_rendezvous_time_update);
      PT_YIELD(&pt);
      prepared_bytes = prepare_next_wake_up_frames(RADIO_MAX_SEQUENCE_LEN - MIN_PREPARE_LEAD_OVER_LOOP);
      NETSTACK_RADIO.async_append_to_sequence(csl_state.transmit.next_wake_up_frames, prepared_bytes);
    }
    if(schedule_transmission_precise(csl_state.transmit.next_rendezvous_time_update) == RTIMER_OK) {
      PT_YIELD(&pt);
    }
    NETSTACK_RADIO.async_finish_sequence();
    if(!csl_state.transmit.is_broadcast) {
      NETSTACK_RADIO.async_on();
    }

    while(1) {
      if(!csl_state.transmit.is_broadcast) {
        /* wait for acknowledgement */
        csl_state.transmit.waiting_for_acknowledgement_shr = 1;
        csl_state.transmit.got_acknowledgement_shr = 0;
        schedule_transmission(RTIMER_NOW() + CSL_ACKNOWLEDGEMENT_WINDOW_MAX);
        PT_YIELD(&pt);
        csl_state.transmit.waiting_for_acknowledgement_shr = 0;
        if(!csl_state.transmit.got_acknowledgement_shr) {
          NETSTACK_RADIO.async_off();
          LOG_ERR("received no acknowledgement\n");
          csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_NOACK;
          break;
        }
        if(CSL_FRAMER.parse_acknowledgement() == FRAMER_FAILED) {
          NETSTACK_RADIO.async_off();
          csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_COLLISION;
          break;
        }
        NETSTACK_RADIO.async_off();
      }
      csl_state.transmit.result[csl_state.transmit.burst_index] = MAC_TX_OK;

      /* check if we burst more payload frames */
      if(++csl_state.transmit.burst_index > csl_state.transmit.last_burst_index) {
        break;
      }

      /* transmit next payload frame */
      NETSTACK_RADIO.async_transmit(!csl_state.transmit.is_broadcast);
      csl_state.transmit.bf[csl_state.transmit.burst_index]->transmissions++;

      /* move next payload frame to radio */
      NETSTACK_RADIO.async_prepare(csl_state.transmit.payload_frame[csl_state.transmit.burst_index]);

      /* wait for on_txdone */
      csl_state.transmit.is_waiting_for_txdone = 1;
      PT_YIELD(&pt);
      csl_state.transmit.is_waiting_for_txdone = 0;
    }
  }

  is_transmitting = 0;
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
  linkaddr_t *receiver;
  rtimer_clock_t next_attempt;
  uint8_t i;
  uint8_t back_off_exponent;
  uint8_t back_off_periods;

  i = 0;
  do {
    queuebuf_to_packetbuf(csl_state.transmit.bf[i]->qb);

    if(!i && !csl_state.transmit.is_broadcast) {
      CSL_FRAMER.on_unicast_transmitted();
      CSL_SYNCHRONIZER.on_unicast_transmitted();
    }

    switch(csl_state.transmit.result[i]) {
    case MAC_TX_COLLISION:
    case MAC_TX_NOACK:
    case MAC_TX_ERR:
      if(csl_state.transmit.bf[i]->transmissions
          >= queuebuf_attr(csl_state.transmit.bf[i]->qb, PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS)) {
        /* intentionally no break; */
      } else {
        back_off_exponent = MIN(csl_state.transmit.bf[i]->transmissions + MIN_BACK_OFF_EXPONENT,
            MAX_BACK_OFF_EXPONENT);
        back_off_periods = ((1 << back_off_exponent) - 1) & random_rand();
        next_attempt = RTIMER_NOW() + (WAKE_UP_COUNTER_INTERVAL * back_off_periods);
        receiver = queuebuf_addr(csl_state.transmit.bf[i]->qb, PACKETBUF_ADDR_RECEIVER);
        delay_any_frames_to(receiver, next_attempt);
        break;
      }
    case MAC_TX_OK:
    case MAC_TX_ERR_FATAL:
      queuebuf_free(csl_state.transmit.bf[i]->qb);
      mac_call_sent_callback(csl_state.transmit.bf[i]->sent,
          csl_state.transmit.bf[i]->ptr,
          csl_state.transmit.result[i],
          csl_state.transmit.bf[i]->transmissions);
      list_remove(buffered_frames_list, csl_state.transmit.bf[i]);
      memb_free(&buffered_frames_memb, csl_state.transmit.bf[i]);
      break;
    }
  } while((csl_state.transmit.result[i] == MAC_TX_OK)
      && (++i <= csl_state.transmit.last_burst_index));
}
/*---------------------------------------------------------------------------*/
/** Called by upper layers when a frame shall be sent. */
static void
send(mac_callback_t sent, void *ptr)
{
#if CSL_COMPLIANT && !AKES_MAC_ENABLED
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
#endif /* CSL_COMPLIANT && !AKES_MAC_ENABLED */
#if CSL_COMPLIANT && !LLSEC802154_USES_FRAME_COUNTER
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, ++mac_dsn);
#endif /* CSL_COMPLIANT && !LLSEC802154_USES_FRAME_COUNTER */
  queue_frame(sent, ptr);
  try_skip_to_send();
}
/*---------------------------------------------------------------------------*/
/**
 * If possible, this function accelerates the transmission of buffer frames
 * by jumping to the protohread "post_processing" directly.
 */
static void
try_skip_to_send(void)
{
  if(!skipped && can_skip && rtimer_cancel()) {
    skipped = 1;
  }
}
/*---------------------------------------------------------------------------*/
/** Buffers outgoing frames */
static void
queue_frame(mac_callback_t sent, void *ptr)
{
  struct buffered_frame *bf;
  struct buffered_frame *next;

  if(!packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS)) {
    packetbuf_set_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS,
        MAX_RETRANSMISSIONS + 1);
  }

  bf = memb_alloc(&buffered_frames_memb);
  if(!bf) {
    LOG_ERR("buffer is full\n");
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
    return;
  }
  bf->qb = queuebuf_new_from_packetbuf();
  if(!bf->qb) {
    LOG_ERR("queubuf is full\n");
    memb_free(&buffered_frames_memb, bf);
    mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
    return;
  }

  bf->ptr = ptr;
  bf->sent = sent;
  bf->transmissions = 0;
  bf->next_attempt = RTIMER_NOW();
  /* do not send earlier than other frames for that receiver */
  next = list_head(buffered_frames_list);
  while(next) {
    if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
        queuebuf_addr(next->qb, PACKETBUF_ADDR_RECEIVER))) {
      bf->next_attempt = next->next_attempt;
      break;
    }
    next = list_item_next(next);
  }
  list_add(buffered_frames_list, bf);
}
/*---------------------------------------------------------------------------*/
/** This function is never called since we operate in polling mode throughout */
static void
input(void)
{
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
  return RADIO_MAX_FRAME_LEN - NETSTACK_FRAMER.length();
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
csl_get_last_wake_up_time(void)
{
  return duty_cycle_next + RADIO_RECEIVE_CALIBRATION_TIME;
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
  rtimer_clock_t result;

  result = RTIMER_CLOCK_DIFF(t, csl_get_last_wake_up_time());
  while(result >= WAKE_UP_COUNTER_INTERVAL) {
    result -= WAKE_UP_COUNTER_INTERVAL;
  }
  return WAKE_UP_COUNTER_INTERVAL - result;
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
