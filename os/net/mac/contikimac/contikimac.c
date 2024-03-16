/*
 * Copyright (c) 2016, Hasso-Plattner-Institut.
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
 *         A denial-of-sleep-resilient version of ContikiMAC.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/contikimac/contikimac.h"
#include "dev/crypto/cc/cc-crypto.h"
#include "lib/aes-128.h"
#include "lib/random.h"
#include "net/mac/contikimac/contikimac-ccm-inputs.h"
#include "net/mac/contikimac/contikimac-framer-original.h"
#include "net/mac/contikimac/contikimac-nbr.h"
#include "net/mac/contikimac/contikimac-synchronizer.h"
#include "net/mac/frame-queue.h"
#include "net/mac/framer/crc16-framer.h"
#include "net/mac/mac-sequence.h"
#include "net/mac/mac.h"
#include "net/nbr-table.h"
#include "net/netstack.h"
#include "net/queuebuf.h"
#include "services/akes/akes-nbr.h"
#include "services/akes/akes.h"
#include "sys/atomic.h"
#include <string.h>

#ifdef CONTIKIMAC_CONF_CCA_THRESHOLD_TRANSMISSION_DETECTION
#define CCA_THRESHOLD_TRANSMISSION_DETECTION \
  CONTIKIMAC_CONF_CCA_THRESHOLD_TRANSMISSION_DETECTION
#else /* CONTIKIMAC_CONF_CCA_THRESHOLD_TRANSMISSION_DETECTION */
#define CCA_THRESHOLD_TRANSMISSION_DETECTION (-80)
#endif /* CONTIKIMAC_CONF_CCA_THRESHOLD_TRANSMISSION_DETECTION */

#ifdef CONTIKIMAC_CONF_CCA_THRESHOLD_SILENCE_DETECTION
#define CCA_THRESHOLD_SILENCE_DETECTION \
  CONTIKIMAC_CONF_CCA_THRESHOLD_SILENCE_DETECTION
#else /* CONTIKIMAC_CONF_CCA_THRESHOLD_SILENCE_DETECTION */
#define CCA_THRESHOLD_SILENCE_DETECTION (CCA_THRESHOLD_TRANSMISSION_DETECTION)
#endif /* CONTIKIMAC_CONF_CCA_THRESHOLD_SILENCE_DETECTION */

#ifdef CONTIKIMAC_CONF_CCA_THRESHOLD_COLLISION_AVOIDANCE
#define CCA_THRESHOLD_COLLISION_AVOIDANCE \
  CONTIKIMAC_CONF_CCA_THRESHOLD_COLLISION_AVOIDANCE
#else /* CONTIKIMAC_CONF_CCA_THRESHOLD_COLLISION_AVOIDANCE */
#define CCA_THRESHOLD_COLLISION_AVOIDANCE (-70)
#endif /* CONTIKIMAC_CONF_CCA_THRESHOLD_COLLISION_AVOIDANCE */

#ifdef CONTIKIMAC_CONF_WITH_DOZING
#define WITH_DOZING CONTIKIMAC_CONF_WITH_DOZING
#else /* CONTIKIMAC_CONF_WITH_DOZING */
#define WITH_DOZING 1
#endif /* CONTIKIMAC_CONF_WITH_DOZING */

#define MAX_NOISE \
  RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE \
                         * (RADIO_SHR_LEN \
                            + RADIO_HEADER_LEN \
                            + RADIO_MAX_PAYLOAD))
#define CCA_SLEEP_DURATION \
  (RADIO_RECEIVE_CALIBRATION_TIME + RADIO_CCA_TIME - 3)
#define SILENCE_CHECK_PERIOD (US_TO_RTIMERTICKS(250))
#define DOZING_PERIOD \
  (CONTIKIMAC_INTER_FRAME_PERIOD \
   - RADIO_RECEIVE_CALIBRATION_TIME \
   - RADIO_CCA_TIME)

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ContikiMAC"
#define LOG_LEVEL LOG_LEVEL_MAC

enum cca_reason {
  TRANSMISSION_DETECTION = 0,
  SILENCE_DETECTION,
  COLLISION_AVOIDANCE
};

static void schedule_duty_cycle(rtimer_clock_t time);
static int schedule_duty_cycle_precise(rtimer_clock_t time);
static void duty_cycle_wrapper(struct rtimer *t, void *ptr);
static PT_THREAD(duty_cycle(void));
static void on_shr(void);
static void on_fifop(void);
static void on_final_fifop(void);
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
static bool received_authentic_unicast(void);
static bool is_valid_ack(akes_nbr_entry_t *entry);
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
static void on_txdone(void);
static void schedule_strobe(rtimer_clock_t time);
static int schedule_strobe_precise(rtimer_clock_t time);
static void strobe_wrapper(struct rtimer *rt, void *ptr);
static PT_THREAD(strobe(void));
static bool should_strobe_again(void);
static radio_async_result_t transmit(void);
static void on_strobed(void);

contikimac_state_t contikimac_state;
static const radio_value_t cca_thresholds[] = {
  CCA_THRESHOLD_TRANSMISSION_DETECTION,
  CCA_THRESHOLD_SILENCE_DETECTION,
  CCA_THRESHOLD_COLLISION_AVOIDANCE
};
static struct rtimer timer;
static rtimer_clock_t last_wake_up_time;
static struct pt pt;
static bool is_duty_cycling;
static bool is_strobing;
static bool can_skip;
static uint8_t skip_lock;
PROCESS(post_processing, "post processing");
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
static rtimer_clock_t sfd_timestamp;
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
static wake_up_counter_t my_wake_up_counter;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */

/*---------------------------------------------------------------------------*/
static bool
channel_clear(enum cca_reason reason)
{
  return radio_get_rssi() < cca_thresholds[reason];
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
  LOG_INFO("t_i = %lu\n", (long unsigned)CONTIKIMAC_INTER_FRAME_PERIOD);
  LOG_INFO("t_c = %lu\n", (long unsigned)CONTIKIMAC_INTER_CCA_PERIOD);
  LOG_INFO("t_w = %lu\n", (long unsigned)WAKE_UP_COUNTER_INTERVAL);
  frame_queue_init();
  CONTIKIMAC_FRAMER.init();
  CONTIKIMAC_SYNCHRONIZER.init();
  NETSTACK_RADIO.async_set_txdone_callback(on_txdone);
  NETSTACK_RADIO.async_set_shr_callback(on_shr);
  NETSTACK_RADIO.set_value(RADIO_PARAM_SHR_SENSITIVITY,
                           RADIO_SHR_SENSITIVITY_HIGH);
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
static
PT_THREAD(duty_cycle(void))
{
  PT_BEGIN(&pt);

#ifdef LPM_CONF_ENABLE
  lpm_set_max_pm(LPM_PM1);
#endif /* LPM_CONF_ENABLE */
  is_duty_cycling = true;

  if(atomic_cas_uint8(&skip_lock, 0, 1)) {
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    my_wake_up_counter = contikimac_get_wake_up_counter(
                             timer.time
                             + CONTIKIMAC_LPM_DEEP_SWITCHING
                             + RADIO_RECEIVE_CALIBRATION_TIME);
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
    last_wake_up_time = timer.time
                        + CONTIKIMAC_LPM_DEEP_SWITCHING
                        + RADIO_RECEIVE_CALIBRATION_TIME;

    NETSTACK_RADIO.async_set_fifop_callback(
        on_fifop,
        CONTIKIMAC_FRAMER.get_min_bytes_for_filtering());
    NETSTACK_RADIO.set_value(RADIO_PARAM_SHR_SEARCH, RADIO_SHR_SEARCH_DIS);

    /* if we come from PM0, we will be too early */
    RTIMER_BUSYWAIT_UNTIL_TIMEOUT(timer.time + CONTIKIMAC_LPM_DEEP_SWITCHING);

    /* CCAs */
    while(1) {
      NETSTACK_RADIO.async_on();
      schedule_duty_cycle(RTIMER_NOW() + CCA_SLEEP_DURATION);
      PT_YIELD(&pt);
      if(channel_clear(TRANSMISSION_DETECTION)) {
        NETSTACK_RADIO.async_off();
        if(++contikimac_state.duty_cycle.cca_count != CONTIKIMAC_MAX_CCAS) {
          schedule_duty_cycle(RTIMER_NOW()
                              + CONTIKIMAC_INTER_CCA_PERIOD
                              - CONTIKIMAC_LPM_SWITCHING);
          PT_YIELD(&pt);
          /* if we come from PM0, we will be too early */
          RTIMER_BUSYWAIT_UNTIL_TIMEOUT(timer.time);
          continue;
        }
      } else {
        contikimac_state.duty_cycle.silence_timeout = RTIMER_NOW()
                                                      + MAX_NOISE
                                                      + RADIO_CCA_TIME;
        contikimac_state.duty_cycle.set_silence_timeout = true;
      }
      break;
    }

    /* fast-sleep optimization */
    if(contikimac_state.duty_cycle.set_silence_timeout) {
      while(1) {

        /* look for silence period */
#if WITH_DOZING
        NETSTACK_RADIO.async_off();
        schedule_duty_cycle(RTIMER_NOW()
                            + DOZING_PERIOD
                            - CONTIKIMAC_LPM_SWITCHING
                            - 2);
        PT_YIELD(&pt);
        NETSTACK_RADIO.async_on();
        schedule_duty_cycle(RTIMER_NOW() + CCA_SLEEP_DURATION);
        PT_YIELD(&pt);
#else /* WITH_DOZING */
        schedule_duty_cycle(RTIMER_NOW() + SILENCE_CHECK_PERIOD);
        PT_YIELD(&pt);
#endif /* WITH_DOZING */
        if(channel_clear(SILENCE_DETECTION)) {
          NETSTACK_RADIO.set_value(RADIO_PARAM_SHR_SEARCH,
                                   RADIO_SHR_SEARCH_EN);

          /* wait for SHR */
          contikimac_state.duty_cycle.waiting_for_shr = true;
          schedule_duty_cycle(RTIMER_NOW()
                              + CONTIKIMAC_INTER_FRAME_PERIOD
                              + RADIO_SHR_TIME
                              + 3 /* some tolerance */);
          /* wait for timeout or on_fifop, whatever comes first */
          PT_YIELD(&pt);
          if(contikimac_state.duty_cycle.rejected_frame) {
            rtimer_cancel();
          }
          contikimac_state.duty_cycle.waiting_for_shr = false;
          if(!contikimac_state.duty_cycle.got_shr) {
            NETSTACK_RADIO.async_off();
            LOG_WARN("no SHR detected\n");
          } else {
            /* wait for timeout or on_fifop, whatever comes last */
            PT_YIELD(&pt);
            if(!contikimac_state.duty_cycle.rejected_frame) {
              /* wait for on_final_fifop or on_txdone */
              PT_YIELD(&pt);
            }
          }
          break;
        } else if(rtimer_has_timed_out(
                      contikimac_state.duty_cycle.silence_timeout)) {
          NETSTACK_RADIO.async_off();
          LOG_WARN("noise too long\n");
          break;
        }
      }
    }
  }

  NETSTACK_RADIO.async_set_fifop_callback(NULL, 0);
  NETSTACK_RADIO.set_value(RADIO_PARAM_SHR_SEARCH, RADIO_SHR_SEARCH_EN);
  is_duty_cycling = false;
  process_poll(&post_processing);

  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
/**
 * Here, we assume that rtimer and radio interrupts have equal priorities,
 * such that they do not preempt each other.
 */
static void
on_shr(void)
{
  if(is_duty_cycling && contikimac_state.duty_cycle.waiting_for_shr) {
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    sfd_timestamp = RTIMER_NOW();
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    contikimac_state.duty_cycle.got_shr = true;
#if CC_CRYPTO_ENABLED
    cc_crypto_enable();
#endif /* CC_CRYPTO_ENABLED */
  } else if(is_strobing) {
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    sfd_timestamp = RTIMER_NOW();
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    if(contikimac_state.strobe.is_waiting_for_acknowledgment_shr) {
      contikimac_state.strobe.got_acknowledgment_shr = true;
    } else {
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
      if(!contikimac_framer_potr_update_contents()) {
        NETSTACK_RADIO.async_off();
        LOG_WARN("contikimac_framer_potr_update_contents failed\n");
        contikimac_state.strobe.update_error_occurred = true;
        strobe();
      }
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
enable_local_packetbuf(void)
{
  contikimac_state.duty_cycle.actual_packetbuf = packetbuf;
  packetbuf = &contikimac_state.duty_cycle.local_packetbuf;
}
/*---------------------------------------------------------------------------*/
static void
disable_local_packetbuf(void)
{
  packetbuf = contikimac_state.duty_cycle.actual_packetbuf;
}
/*---------------------------------------------------------------------------*/
#if AKES_MAC_ENABLED
static bool
is_anything_locked(void)
{
  return !ccm_star_can_use_asynchronously()
         || !akes_nbr_can_query_asynchronously();
}
#endif /* !AKES_MAC_ENABLED */
/*---------------------------------------------------------------------------*/
static void
on_fifop(void)
{
  if(!contikimac_state.duty_cycle.got_shr) {
    return;
  }

  /* avoid that on_fifop is called twice if FIFOP_THRESHOLD is very low */
  NETSTACK_RADIO.async_set_fifop_callback(NULL, RADIO_MAX_PAYLOAD);
  enable_local_packetbuf();
  if(false
#if AKES_MAC_ENABLED
     || is_anything_locked()
#endif /* !AKES_MAC_ENABLED */
     || !radio_read_phy_header_to_packetbuf()
     || (CONTIKIMAC_FRAMER.filter() == FRAMER_FAILED)) {
    NETSTACK_RADIO.async_off();
    LOG_ERR("rejected frame of length %i\n", packetbuf_datalen());
    contikimac_state.duty_cycle.rejected_frame = true;
  } else {
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, (int16_t)radio_get_rssi());
    contikimac_state.duty_cycle.shall_send_acknowledgment =
        !packetbuf_holds_broadcast();
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    uint8_t *hdrptr = packetbuf_hdrptr();
    contikimac_state.duty_cycle.is_helloack =
        hdrptr[0] == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK;
    contikimac_state.duty_cycle.is_ack =
        hdrptr[0] == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */

    if(contikimac_state.duty_cycle.shall_send_acknowledgment
       && NETSTACK_RADIO.async_prepare(
           contikimac_state.duty_cycle.acknowledgment,
           contikimac_state.duty_cycle.acknowledgment_len)) {
      NETSTACK_RADIO.async_off();
      LOG_ERR("async_prepare failed\n");
      contikimac_state.duty_cycle.rejected_frame = true;
    } else {
      NETSTACK_RADIO.async_set_fifop_callback(on_final_fifop,
                                              radio_remaining_payload_bytes());
    }
  }
  disable_local_packetbuf();
  duty_cycle();
}
/*---------------------------------------------------------------------------*/
static void
on_final_fifop(void)
{
  /* avoid that on_final_fifop is called twice */
  NETSTACK_RADIO.async_set_fifop_callback(NULL, 0);

  contikimac_state.duty_cycle.got_frame = true;
  if(!contikimac_state.duty_cycle.shall_send_acknowledgment) {
    NETSTACK_RADIO.async_off();
    duty_cycle();
    return;
  }

  if(NETSTACK_RADIO.async_transmit(false)) {
    NETSTACK_RADIO.async_off();
    LOG_ERR("async_transmit failed\n");
    duty_cycle();
    return;
  }
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  if(!received_authentic_unicast()) {
    NETSTACK_RADIO.async_off();
    contikimac_state.duty_cycle.got_frame = false;
    LOG_ERR("aborted transmission of acknowledgment frame\n");
    duty_cycle();
  }
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
}
/*---------------------------------------------------------------------------*/
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
static bool
received_authentic_unicast(void)
{
  if(contikimac_state.duty_cycle.is_helloack) {
    /* HELLOACKs are parsed and verified later */
    return true;
  }

  enable_local_packetbuf();

  akes_nbr_entry_t *entry;
  contikimac_state.duty_cycle.read_and_parsed =
      !is_anything_locked()
      && !radio_read_payload_to_packetbuf(radio_remaining_payload_bytes())
      && (NETSTACK_FRAMER.parse() != FRAMER_FAILED)
      && ((entry = akes_nbr_get_sender_entry()))
      && ((!contikimac_state.duty_cycle.is_ack
           && entry->permanent
           && !AKES_MAC_STRATEGY.verify(entry->permanent))
          || (contikimac_state.duty_cycle.is_ack
              && is_valid_ack(entry)));

  disable_local_packetbuf();
  return contikimac_state.duty_cycle.read_and_parsed;
}
/*---------------------------------------------------------------------------*/
static bool
is_valid_ack(akes_nbr_entry_t *entry)
{
  uint8_t *dataptr = packetbuf_dataptr();
  dataptr += AKES_ACK_PIGGYBACK_OFFSET
             + (ANTI_REPLAY_WITH_SUPPRESSION ? 4 : 0);
  packetbuf_set_attr(PACKETBUF_ATTR_UNENCRYPTED_BYTES,
                     packetbuf_datalen()
                     - AES_128_KEY_LENGTH
                     - AKES_MAC_UNICAST_MIC_LEN);
  contikimac_nbr_tentative_t *contikimac_nbr_tentative =
      contikimac_nbr_get_tentative(entry->tentative->meta);
  if((dataptr[CONTIKIMAC_Q_LEN] != contikimac_nbr_tentative->strobe_index)
     || memcmp(dataptr, contikimac_nbr_tentative->q, CONTIKIMAC_Q_LEN)
     || !akes_mac_unsecure(entry->tentative->tentative_pairwise_key)) {
    LOG_ERR("invalid ACK\n");
    akes_nbr_delete(entry, AKES_NBR_TENTATIVE);
    return false;
  }
  return true;
}
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
/*---------------------------------------------------------------------------*/
static void
on_txdone(void)
{
  if(is_duty_cycling) {
    duty_cycle();
  } else if(is_strobing) {
#if CONTIKIMAC_WITH_PHASE_LOCK
    contikimac_state.strobe.t1[0] = contikimac_state.strobe.t1[1];
    contikimac_state.strobe.t1[1] = RTIMER_NOW();
#endif /* CONTIKIMAC_WITH_PHASE_LOCK */
    strobe();
  }
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
contikimac_get_last_wake_up_time(void)
{
  return last_wake_up_time;
}
/*---------------------------------------------------------------------------*/
#if CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK
rtimer_clock_t
contikimac_get_last_but_one_t0(void)
{
  return contikimac_state.strobe.t0[0];
}
#endif /* CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK */
/*---------------------------------------------------------------------------*/
#if CONTIKIMAC_WITH_PHASE_LOCK
rtimer_clock_t
contikimac_get_last_but_one_t1(void)
{
  return contikimac_state.strobe.t1[0];
}
#endif /* CONTIKIMAC_WITH_PHASE_LOCK */
/*---------------------------------------------------------------------------*/
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
rtimer_clock_t
contikimac_get_sfd_timestamp(void)
{
  return sfd_timestamp;
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
contikimac_get_phase(void)
{
  rtimer_clock_t result = sfd_timestamp - contikimac_get_last_wake_up_time();
  while(result >= WAKE_UP_COUNTER_INTERVAL) {
    result -= WAKE_UP_COUNTER_INTERVAL;
  }
  return WAKE_UP_COUNTER_INTERVAL - result;
}
/*---------------------------------------------------------------------------*/
uint8_t
contikimac_get_last_delta(void)
{
  return (sfd_timestamp
          - contikimac_get_last_wake_up_time()
          - CONTIKIMAC_INTER_FRAME_PERIOD
          - RADIO_SHR_TIME)
         >> CONTIKIMAC_DELTA_SHIFT;
}
/*---------------------------------------------------------------------------*/
uint8_t
contikimac_get_last_strobe_index(void)
{
  return contikimac_state.strobe.strobes;
}
/*---------------------------------------------------------------------------*/
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
rtimer_clock_t
contikimac_get_next_strobe_start(void)
{
  return contikimac_state.strobe.next_transmission;
}
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(post_processing, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    can_skip = true;

    bool just_received_broadcast = false;

    /* read received frame */
    if(contikimac_state.duty_cycle.got_frame) {
      enable_local_packetbuf();
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
      if(!contikimac_state.duty_cycle.read_and_parsed
#else /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
      if(1
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
         && ((radio_read_payload_to_packetbuf(radio_remaining_payload_bytes())
              || (NETSTACK_FRAMER.parse() == FRAMER_FAILED)))) {
        LOG_ERR("something went wrong while reading\n");
      } else {
        just_received_broadcast = packetbuf_holds_broadcast();
#if AKES_MAC_ENABLED
        NETSTACK_MAC.input();
#else /* AKES_MAC_ENABLED */
        if(mac_sequence_is_duplicate()) {
          LOG_WARN("received duplicate\n");
        } else {
          mac_sequence_register_seqno();
          NETSTACK_NETWORK.input();
        }
#endif /* AKES_MAC_ENABLED */
      }
      disable_local_packetbuf();
    }

    /* send queued frames */
    if(!just_received_broadcast) {
      frame_queue_entry_t *next;
      while((next = frame_queue_pick())) {
#if CC_CRYPTO_ENABLED
        cc_crypto_enable();
#endif /* CC_CRYPTO_ENABLED */
        memset(&contikimac_state.strobe, 0, sizeof(contikimac_state.strobe));
        contikimac_state.strobe.fqe = next;
        contikimac_state.strobe.is_broadcast = packetbuf_holds_broadcast();

#if AKES_MAC_ENABLED
        if(akes_mac_is_hello()) {
          contikimac_state.strobe.is_hello = true;
        } else if(akes_mac_is_helloack()) {
          contikimac_state.strobe.is_helloack = true;
          contikimac_state.strobe.acknowledgment_len =
              CONTIKIMAC_HELLOACK_ACKNOWLEDGMENT_LEN;
        } else if(akes_mac_is_ack()) {
          contikimac_state.strobe.is_ack = true;
          contikimac_state.strobe.acknowledgment_len =
              CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN;
        } else if(akes_mac_is_update()) {
          contikimac_state.strobe.acknowledgment_len =
              CONTIKIMAC_UPDATE_ACKNOWLEDGMENT_LEN;
        } else {
          contikimac_state.strobe.acknowledgment_len =
              CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN;
        }
#else /* AKES_MAC_ENABLED */
        contikimac_state.strobe.acknowledgment_len =
            CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN;
#endif /* AKES_MAC_ENABLED */

        if(!contikimac_state.strobe.is_broadcast
           && !CONTIKIMAC_FRAMER.prepare_acknowledgment_parsing()) {
          LOG_ERR("prepare_acknowledgment_parsing failed\n");
          contikimac_state.strobe.result = MAC_TX_ERR_FATAL;
          on_strobed();
          continue;
        }

#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
        /* schedule strobe */
        contikimac_state.strobe.result = CONTIKIMAC_SYNCHRONIZER.schedule();
        if(contikimac_state.strobe.result != MAC_TX_OK) {
          on_strobed();
          continue;
        }
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

        /* create frame */
#if !CONTIKIMAC_FRAMER_POTR_ENABLED
        packetbuf_set_attr(PACKETBUF_ATTR_MAC_ACK, 1);
        packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
#endif /* !CONTIKIMAC_FRAMER_POTR_ENABLED */
        if(NETSTACK_FRAMER.create() == FRAMER_FAILED) {
          contikimac_state.strobe.result = MAC_TX_ERR_FATAL;
          on_strobed();
          continue;
        }

        /* is this a broadcast? */
#if !CONTIKIMAC_WITH_SECURE_PHASE_LOCK
#if CONTIKIMAC_FRAMER_POTR_ENABLED
        contikimac_state.strobe.seqno = anti_replay_get_counter_lsbs();
#else /* CONTIKIMAC_FRAMER_POTR_ENABLED */
        contikimac_state.strobe.seqno =
            packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
#endif /* CONTIKIMAC_FRAMER_POTR_ENABLED */
#endif /* !CONTIKIMAC_WITH_SECURE_PHASE_LOCK */

        /* move frame to radio */
        memcpy(contikimac_state.strobe.prepared_frame,
               packetbuf_hdrptr(),
               packetbuf_totlen());
        if(NETSTACK_RADIO.async_prepare(contikimac_state.strobe.prepared_frame,
                                        packetbuf_totlen())) {
          LOG_ERR("async_prepare failed\n");
          contikimac_state.strobe.result = MAC_TX_ERR;
          on_strobed();
          continue;
        }

#if !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
        /* schedule strobe */
        contikimac_state.strobe.result = CONTIKIMAC_SYNCHRONIZER.schedule();
        if(contikimac_state.strobe.result != MAC_TX_OK) {
          on_strobed();
          continue;
        }
#endif /* !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

#if CONTIKIMAC_WITH_PHASE_LOCK
        /* check if there is time for a duty cycle beforehand */
        rtimer_clock_t end_of_next_potential_reception =
            wake_up_counter_shift_to_future(last_wake_up_time
                                            - CONTIKIMAC_LPM_DEEP_SWITCHING
                                            - RADIO_RECEIVE_CALIBRATION_TIME)
            + CONTIKIMAC_LPM_DEEP_SWITCHING
            + RADIO_RECEIVE_CALIBRATION_TIME
            + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE
                                     * RADIO_MAX_PAYLOAD)
            + CONTIKIMAC_INTER_FRAME_PERIOD
            + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE
                                     * RADIO_MAX_PAYLOAD)
            + RADIO_RECEIVE_CALIBRATION_TIME
            + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE
                                     * CONTIKIMAC_MAX_ACKNOWLEDGMENT_LEN)
            + US_TO_RTIMERTICKS(1000) /* leeway */;
        if(RTIMER_CLOCK_LT(end_of_next_potential_reception,
                           contikimac_state.strobe.next_transmission
                           - CONTIKIMAC_STROBE_GUARD_TIME)) {
          break;
        }
#endif /* CONTIKIMAC_WITH_PHASE_LOCK */

#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
        if(schedule_strobe_precise(contikimac_state.strobe.next_transmission
                                   - CONTIKIMAC_STROBE_GUARD_TIME)
           != RTIMER_OK) {
          LOG_ERR("strobe starts too early\n");
          contikimac_state.strobe.result = MAC_TX_ERR;
          on_strobed();
          continue;
        }
#else /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
        while(schedule_strobe_precise(contikimac_state.strobe.next_transmission
                                      - CONTIKIMAC_STROBE_GUARD_TIME)
              != RTIMER_OK) {
          contikimac_state.strobe.next_transmission +=
              WAKE_UP_COUNTER_INTERVAL;
          contikimac_state.strobe.timeout += WAKE_UP_COUNTER_INTERVAL;
        }
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

        /* process strobe result */
        PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
        on_strobed();
      }
    }
#ifdef LPM_CONF_ENABLE
    lpm_set_max_pm(LPM_CONF_MAX_PM);
#endif /* LPM_CONF_ENABLE */

    /* prepare next duty cycle */
#if CC_CRYPTO_ENABLED
    cc_crypto_disable();
#endif /* CC_CRYPTO_ENABLED */
    memset(&contikimac_state.duty_cycle,
           0,
           sizeof(contikimac_state.duty_cycle));
    rtimer_clock_t next_wake_up_time =
        wake_up_counter_shift_to_future(last_wake_up_time
                                        - CONTIKIMAC_LPM_DEEP_SWITCHING
                                        - RADIO_RECEIVE_CALIBRATION_TIME);
    atomic_cas_uint8(&skip_lock, 1, 0);
    while(schedule_duty_cycle_precise(next_wake_up_time) != RTIMER_OK) {
      next_wake_up_time += WAKE_UP_COUNTER_INTERVAL;
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static void
schedule_strobe(rtimer_clock_t time)
{
  if(rtimer_set(&timer, time, 1, strobe_wrapper, NULL) != RTIMER_OK) {
    LOG_ERR("rtimer_set failed\n");
  }
}
/*---------------------------------------------------------------------------*/
static int
schedule_strobe_precise(rtimer_clock_t time)
{
  timer.time = time;
  timer.func = strobe_wrapper;
  timer.ptr = NULL;
  return rtimer_set_precise(&timer);
}
/*---------------------------------------------------------------------------*/
static void
strobe_wrapper(struct rtimer *rt, void *ptr)
{
  strobe();
}
/*---------------------------------------------------------------------------*/
static
PT_THREAD(strobe(void))
{
  PT_BEGIN(&pt);

  is_strobing = true;

  while(1) {
    if(!contikimac_state.strobe.strobes) {
#if CONTIKIMAC_WITH_INTRA_COLLISION_AVOIDANCE
      /* if we come from PM0, we will be too early */
      RTIMER_BUSYWAIT_UNTIL_TIMEOUT(
          contikimac_state.strobe.next_transmission
          - CONTIKIMAC_INTRA_COLLISION_AVOIDANCE_DURATION
          - RADIO_TRANSMIT_CALIBRATION_TIME);

      /* CCAs */
      while(1) {
        NETSTACK_RADIO.async_on();
        schedule_strobe(RTIMER_NOW() + CCA_SLEEP_DURATION);
        PT_YIELD(&pt);
        if(channel_clear(COLLISION_AVOIDANCE)) {
          NETSTACK_RADIO.async_off();
          if(++contikimac_state.strobe.cca_count != CONTIKIMAC_MAX_CCAS) {
            schedule_strobe(RTIMER_NOW()
                            + CONTIKIMAC_INTER_CCA_PERIOD
                            - CONTIKIMAC_LPM_SWITCHING);
            PT_YIELD(&pt);
            /* if we come from PM0, we will be too early */
            RTIMER_BUSYWAIT_UNTIL_TIMEOUT(timer.time);
            continue;
          }
        } else {
          LOG_INFO("collision\n");
          contikimac_state.strobe.result = MAC_TX_COLLISION;
        }
        break;
      }
      if(contikimac_state.strobe.result == MAC_TX_COLLISION) {
        break;
      }
#endif /* CONTIKIMAC_WITH_INTRA_COLLISION_AVOIDANCE */
    } else {
#if CONTIKIMAC_WITH_INTER_COLLISION_AVOIDANCE
      if(!channel_clear(COLLISION_AVOIDANCE)) {
        LOG_INFO("collision\n");
        contikimac_state.strobe.result = MAC_TX_COLLISION;
        break;
      }
#endif /* CONTIKIMAC_WITH_INTER_COLLISION_AVOIDANCE */
    }

    /* busy waiting for better timing */
    RTIMER_BUSYWAIT_UNTIL_TIMEOUT(contikimac_state.strobe.next_transmission
                                  - RADIO_TRANSMIT_CALIBRATION_TIME);

    if(transmit()) {
      LOG_ERR("NETSTACK_RADIO.async_transmit failed\n");
      contikimac_state.strobe.result = MAC_TX_ERR;
      break;
    }
    PT_YIELD(&pt);
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    if(contikimac_state.strobe.update_error_occurred) {
      contikimac_state.strobe.result = MAC_TX_ERR;
      break;
    }
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    contikimac_state.strobe.next_transmission = RTIMER_NOW()
                                                + CONTIKIMAC_INTER_FRAME_PERIOD;

    if(contikimac_state.strobe.is_broadcast
       || !contikimac_state.strobe.strobes /* little tweak */) {
      if(!should_strobe_again()) {
        contikimac_state.strobe.result = MAC_TX_OK;
        break;
      }
      schedule_strobe(contikimac_state.strobe.next_transmission
                      - CONTIKIMAC_LPM_SWITCHING
#if CONTIKIMAC_WITH_INTER_COLLISION_AVOIDANCE
                      - RADIO_RECEIVE_CALIBRATION_TIME
                      - RADIO_CCA_TIME
#endif /* CONTIKIMAC_WITH_INTER_COLLISION_AVOIDANCE */
                      - RADIO_TRANSMIT_CALIBRATION_TIME);
      PT_YIELD(&pt);
#if CONTIKIMAC_WITH_INTER_COLLISION_AVOIDANCE
      NETSTACK_RADIO.async_on();
#endif /* CONTIKIMAC_WITH_INTER_COLLISION_AVOIDANCE */
    } else {
      /* wait for acknowledgment */
      schedule_strobe(RTIMER_NOW() + CONTIKIMAC_ACKNOWLEDGMENT_WINDOW_MAX);
      contikimac_state.strobe.is_waiting_for_acknowledgment_shr = true;
      PT_YIELD(&pt);
      contikimac_state.strobe.is_waiting_for_acknowledgment_shr = false;
      if(contikimac_state.strobe.got_acknowledgment_shr) {
        if(NETSTACK_RADIO.async_read_phy_header()
           != contikimac_state.strobe.acknowledgment_len) {
          LOG_ERR("unexpected frame\n");
          contikimac_state.strobe.result = MAC_TX_COLLISION;
          break;
        }

        /* read acknowledgment */
        if(NETSTACK_RADIO.async_read_payload(
               contikimac_state.strobe.acknowledgment,
               contikimac_state.strobe.acknowledgment_len)) {
          LOG_ERR("could not read acknowledgment\n");
          contikimac_state.strobe.result = MAC_TX_ERR_FATAL;
          break;
        }
        if(!CONTIKIMAC_FRAMER.parse_acknowledgment()) {
          LOG_ERR("invalid acknowledgment\n");
          contikimac_state.strobe.result = MAC_TX_COLLISION;
          break;
        }

        NETSTACK_RADIO.async_off();
        contikimac_state.strobe.result = MAC_TX_OK;
        break;
      }

      /* schedule next transmission */
      if(!should_strobe_again()) {
        contikimac_state.strobe.result = MAC_TX_NOACK;
        break;
      }

      /* go back to sleep if time allows */
      if(schedule_strobe_precise(contikimac_state.strobe.next_transmission
                                 - CONTIKIMAC_LPM_SWITCHING
                                 - RADIO_TRANSMIT_CALIBRATION_TIME)
         == RTIMER_OK) {
        PT_YIELD(&pt);
      }
    }
    contikimac_state.strobe.strobes++;
  }

  if(contikimac_state.strobe.result != MAC_TX_OK) {
    NETSTACK_RADIO.async_off();
  }
  is_strobing = false;
  process_poll(&post_processing);
  PT_END(&pt);
}
/*---------------------------------------------------------------------------*/
static bool
should_strobe_again(void)
{
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  if(contikimac_state.strobe.strobes == 0xFE) {
    LOG_ERR("strobe index reached maximum\n");
    return 0;
  }
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
  return RTIMER_CLOCK_LT(contikimac_state.strobe.next_transmission
                         - CONTIKIMAC_INTER_FRAME_PERIOD,
                         contikimac_state.strobe.timeout)
         || !contikimac_state.strobe.sent_once_more++;
}
/*---------------------------------------------------------------------------*/
static radio_async_result_t
transmit(void)
{
#if CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK
  contikimac_state.strobe.t0[0] = contikimac_state.strobe.t0[1];
  contikimac_state.strobe.t0[1] = contikimac_state.strobe.next_transmission;
#endif /* CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK */
  return NETSTACK_RADIO.async_transmit(!contikimac_state.strobe.is_broadcast
                                       && contikimac_state.strobe.strobes);
}
/*---------------------------------------------------------------------------*/
static void
on_strobed(void)
{
  if(LOG_INFO_ENABLED && !contikimac_state.strobe.is_broadcast) {
    LOG_INFO("strobed %i times with %s\n",
             contikimac_state.strobe.strobes + 1,
             contikimac_state.strobe.result == MAC_TX_OK
             ? "success"
             : "error");
  }

  queuebuf_to_packetbuf(contikimac_state.strobe.fqe->qb);
  if(!contikimac_state.strobe.is_broadcast) {
    CONTIKIMAC_FRAMER.on_unicast_transmitted();
    CONTIKIMAC_SYNCHRONIZER.on_unicast_transmitted();
  }

  frame_queue_on_transmitted(contikimac_state.strobe.result,
                             contikimac_state.strobe.fqe);
}
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
#if !AKES_MAC_ENABLED
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  mac_sequence_set_dsn();
#endif /* !AKES_MAC_ENABLED */
  if(frame_queue_add(sent, ptr)
     && can_skip
     && atomic_cas_uint8(&skip_lock, 0, 1)) {
    /* fire up the protohread "post_processing" right away */
    rtimer_cancel();
  }
}
/*---------------------------------------------------------------------------*/
static void
input(void)
{
  /* we operate in polling mode throughout */
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  /* TODO implement if needed */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  /* TODO implement if needed  */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  return RADIO_MAX_PAYLOAD - NETSTACK_FRAMER.length();
}
/*---------------------------------------------------------------------------*/
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
wake_up_counter_t
contikimac_get_wake_up_counter(rtimer_clock_t t)
{
  rtimer_clock_t delta = t - contikimac_get_last_wake_up_time();
  wake_up_counter_t wuc = my_wake_up_counter;
  wuc.u32 += wake_up_counter_increments(delta, NULL);
  return wuc;
}
/*---------------------------------------------------------------------------*/
wake_up_counter_t
contikimac_predict_wake_up_counter(void)
{
  return contikimac_state.strobe.receivers_wake_up_counter;
}
/*---------------------------------------------------------------------------*/
wake_up_counter_t
contikimac_restore_wake_up_counter(void)
{
  akes_nbr_entry_t *entry = akes_nbr_get_sender_entry();
  if(!entry || !entry->permanent) {
    wake_up_counter_t wuc = {
      wuc.u32 = 0
    };
    LOG_ERR("could not restore wake-up counter\n");
    return wuc;
  }

  contikimac_nbr_t *contikimac_nbr = contikimac_nbr_get(entry->permanent);
  rtimer_clock_t delta =
      contikimac_get_last_wake_up_time() - contikimac_nbr->phase.t;
  uint32_t mod;
  uint32_t increments = wake_up_counter_increments(delta, &mod);
  wake_up_counter_t wuc = {
    wuc.u32 = contikimac_nbr->phase.his_wake_up_counter_at_t.u32 + increments
  };

  if(wuc.u32 & 1) {
    /* odd --> we need to round */
    if(mod < (WAKE_UP_COUNTER_INTERVAL / 2)) {
      wuc.u32--;
    } else {
      wuc.u32++;
    }
  }

  return wuc;
}
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
/*---------------------------------------------------------------------------*/
const struct mac_driver contikimac_driver = {
  "ContikiMAC",
  init,
  send,
  input,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/
