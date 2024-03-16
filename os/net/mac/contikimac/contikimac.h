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

#ifndef CONTIKIMAC_H_
#define CONTIKIMAC_H_

#include "contiki.h"
#include "dev/radio.h"
#ifdef LPM_CONF_ENABLE
#include "lpm.h"
#endif /* LPM_CONF_ENABLE */
#include "net/mac/anti-replay.h"
#include "net/mac/contikimac/contikimac-framer-original.h"
#include "net/mac/contikimac/contikimac-framer-potr.h"
#include "net/mac/contikimac/contikimac-framer.h"
#include "net/mac/frame-queue.h"
#include "net/mac/mac.h"
#include "net/mac/wake-up-counter.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "services/akes/akes-mac.h"
#include "sys/rtimer.h"

#if CONTIKIMAC_FRAMER_POTR_ENABLED
#define CONTIKIMAC_FRAMER contikimac_framer_potr_contikimac_framer
#else /* CONTIKIMAC_FRAMER_POTR_ENABLED */
#define CONTIKIMAC_FRAMER contikimac_framer_original_contikimac_framer
#endif /* CONTIKIMAC_FRAMER_POTR_ENABLED */

#ifdef CONTIKIMAC_CONF_MIN_FRAME_LENGTH
#define CONTIKIMAC_MIN_FRAME_LENGTH CONTIKIMAC_CONF_MIN_FRAME_LENGTH
#else /* CONTIKIMAC_CONF_MIN_FRAME_LENGTH */
#define CONTIKIMAC_MIN_FRAME_LENGTH 34
#endif /* CONTIKIMAC_CONF_MIN_FRAME_LENGTH */

#ifdef CONTIKIMAC_CONF_WITH_INTRA_COLLISION_AVOIDANCE
#define CONTIKIMAC_WITH_INTRA_COLLISION_AVOIDANCE \
  CONTIKIMAC_CONF_WITH_INTRA_COLLISION_AVOIDANCE
#else /* CONTIKIMAC_CONF_WITH_INTRA_COLLISION_AVOIDANCE */
#define CONTIKIMAC_WITH_INTRA_COLLISION_AVOIDANCE 1
#endif /* CONTIKIMAC_CONF_WITH_INTRA_COLLISION_AVOIDANCE */

#ifdef CONTIKIMAC_CONF_WITH_INTER_COLLISION_AVOIDANCE
#define CONTIKIMAC_WITH_INTER_COLLISION_AVOIDANCE \
  CONTIKIMAC_CONF_WITH_INTER_COLLISION_AVOIDANCE
#else /* CONTIKIMAC_CONF_WITH_INTER_COLLISION_AVOIDANCE */
#define CONTIKIMAC_WITH_INTER_COLLISION_AVOIDANCE 0
#endif /* CONTIKIMAC_CONF_WITH_INTER_COLLISION_AVOIDANCE */

#if !CONTIKIMAC_FRAMER_POTR_ENABLED
#define CONTIKIMAC_WITH_SECURE_PHASE_LOCK 0
#else /* !CONTIKIMAC_FRAMER_POTR_ENABLED */
#ifdef CONTIKIMAC_CONF_WITH_SECURE_PHASE_LOCK
#define CONTIKIMAC_WITH_SECURE_PHASE_LOCK \
  CONTIKIMAC_CONF_WITH_SECURE_PHASE_LOCK
#else /* CONTIKIMAC_CONF_WITH_SECURE_PHASE_LOCK */
#define CONTIKIMAC_WITH_SECURE_PHASE_LOCK 1
#endif /* CONTIKIMAC_CONF_WITH_SECURE_PHASE_LOCK */
#endif /* !CONTIKIMAC_FRAMER_POTR_ENABLED */

#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
#define CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK 0
#if RTIMER_CLOCK_SIZE == 8
#define CONTIKIMAC_DELTA_SHIFT 5
#else /* RTIMER_CLOCK_SIZE == 8 */
#define CONTIKIMAC_DELTA_SHIFT 0
#endif /* RTIMER_CLOCK_SIZE == 8 */
#else /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
#ifdef CONTIKIMAC_CONF_WITH_ORIGINAL_PHASE_LOCK
#define CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK \
  CONTIKIMAC_CONF_WITH_ORIGINAL_PHASE_LOCK
#else /* CONTIKIMAC_CONF_WITH_ORIGINAL_PHASE_LOCK */
#define CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK 1
#endif /* CONTIKIMAC_CONF_WITH_ORIGINAL_PHASE_LOCK */
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */

#define CONTIKIMAC_WITH_PHASE_LOCK \
  (CONTIKIMAC_WITH_SECURE_PHASE_LOCK || CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK)
#define CONTIKIMAC_Q_LEN (CONTIKIMAC_WITH_SECURE_PHASE_LOCK ? 8 : 0)

#if CONTIKIMAC_FRAMER_POTR_ENABLED
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
#define CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN \
   + CONTIKIMAC_FRAMER_POTR_DELTA_LEN \
   + AKES_MAC_UNICAST_MIC_LEN)
#define CONTIKIMAC_HELLOACK_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN)
#else /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
#define CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN)
#define CONTIKIMAC_HELLOACK_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN)
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
#else /* CONTIKIMAC_FRAMER_POTR_ENABLED */
#if AKES_MAC_ENABLED
#define CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_AUTHENTICATED_ACKNOWLEDGMENT_LEN)
#define CONTIKIMAC_HELLOACK_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_ORIGINAL_UNAUTHENTICATED_ACKNOWLEDGMENT_LEN)
#else /* AKES_MAC_ENABLED */
#define CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_ORIGINAL_UNAUTHENTICATED_ACKNOWLEDGMENT_LEN)
#define CONTIKIMAC_HELLOACK_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_ORIGINAL_UNAUTHENTICATED_ACKNOWLEDGMENT_LEN)
#endif /* AKES_MAC_ENABLED */
#endif /* CONTIKIMAC_FRAMER_POTR_ENABLED */

#define CONTIKIMAC_UPDATE_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN \
   + (ANTI_REPLAY_WITH_SUPPRESSION ? 4 : 0))
#if ANTI_REPLAY_WITH_SUPPRESSION && !CONTIKIMAC_WITH_SECURE_PHASE_LOCK
#error forbidden config
#endif /* ANTI_REPLAY_WITH_SUPPRESSION && !CONTIKIMAC_WITH_SECURE_PHASE_LOCK */

/* TODO handle these platform-specific adjustments in rtimer.ch */
#if CONTIKI_TARGET_COOJA
#define CONTIKIMAC_LPM_SWITCHING (0)
#define CONTIKIMAC_LPM_DEEP_SWITCHING (0)
#else /* CONTIKI_TARGET_COOJA */
#define CONTIKIMAC_LPM_SWITCHING (1)
#define CONTIKIMAC_LPM_DEEP_SWITCHING (1)
#ifdef LPM_CONF_ENABLE
#if LPM_CONF_ENABLE
#if (LPM_CONF_MAX_PM == LPM_PM1)
#undef CONTIKIMAC_LPM_SWITCHING
#define CONTIKIMAC_LPM_SWITCHING (9)
#undef CONTIKIMAC_LPM_DEEP_SWITCHING
#define CONTIKIMAC_LPM_DEEP_SWITCHING (9)
#elif (LPM_CONF_MAX_PM == LPM_PM2)
#undef CONTIKIMAC_LPM_SWITCHING
#define CONTIKIMAC_LPM_SWITCHING (9)
#undef CONTIKIMAC_LPM_DEEP_SWITCHING
#define CONTIKIMAC_LPM_DEEP_SWITCHING (13)
#else
#warning unsupported power mode
#endif
#endif /* LPM_CONF_ENABLE */
#endif /* LPM_CONF_ENABLE */
#endif /* CONTIKI_TARGET_COOJA */

#ifdef CONTIKIMAC_CONF_MAX_CCAS
#define CONTIKIMAC_MAX_CCAS CONTIKIMAC_CONF_MAX_CCAS
#else /* CONTIKIMAC_CONF_MAX_CCAS */
#define CONTIKIMAC_MAX_CCAS 2
#endif /* CONTIKIMAC_CONF_MAX_CCAS */

#ifdef CONTIKIMAC_CONF_ACKNOWLEDGMENT_WINDOW_MAX
#define CONTIKIMAC_ACKNOWLEDGMENT_WINDOW_MAX \
  CONTIKIMAC_CONF_ACKNOWLEDGMENT_WINDOW_MAX
#else /* CONTIKIMAC_CONF_ACKNOWLEDGMENT_WINDOW_MAX */
#define CONTIKIMAC_ACKNOWLEDGMENT_WINDOW_MAX US_TO_RTIMERTICKS(427)
#endif /* CONTIKIMAC_CONF_ACKNOWLEDGMENT_WINDOW_MAX */

#define CONTIKIMAC_MAX_ACKNOWLEDGMENT_LEN \
  (MAX(CONTIKIMAC_UPDATE_ACKNOWLEDGMENT_LEN, \
       MAX(CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN, \
           CONTIKIMAC_HELLOACK_ACKNOWLEDGMENT_LEN)))
#define CONTIKIMAC_INTER_FRAME_PERIOD (US_TO_RTIMERTICKS(1068))
#define CONTIKIMAC_INTER_CCA_PERIOD \
  ((CONTIKIMAC_INTER_FRAME_PERIOD - RADIO_RECEIVE_CALIBRATION_TIME + 4) \
   / (CONTIKIMAC_MAX_CCAS - 1))
#define CONTIKIMAC_INTRA_COLLISION_AVOIDANCE_DURATION \
  (CONTIKIMAC_WITH_INTRA_COLLISION_AVOIDANCE \
       ? ((2 * (RADIO_RECEIVE_CALIBRATION_TIME + RADIO_CCA_TIME)) \
          + CONTIKIMAC_INTER_CCA_PERIOD) \
       : 0)
#define CONTIKIMAC_STROBE_GUARD_TIME \
  (CONTIKIMAC_LPM_SWITCHING \
   + CONTIKIMAC_INTRA_COLLISION_AVOIDANCE_DURATION \
   + RADIO_TRANSMIT_CALIBRATION_TIME)
#define CONTIKIMAC_ACKNOWLEDGMENT_WINDOW_MIN (US_TO_RTIMERTICKS(336))
#define CONTIKIMAC_ACKNOWLEDGMENT_WINDOW \
  (CONTIKIMAC_ACKNOWLEDGMENT_WINDOW_MAX \
   - CONTIKIMAC_ACKNOWLEDGMENT_WINDOW_MIN + 1)

struct contikimac_phase {
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  rtimer_clock_t t;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  wake_up_counter_t his_wake_up_counter_at_t;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#if CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK
  uint8_t fail_streak;
  rtimer_clock_t t0;
  rtimer_clock_t t1;
#endif /* CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK */
};

typedef union {
  struct {
    uint_fast8_t cca_count;
    rtimer_clock_t silence_timeout;
    bool set_silence_timeout;
    bool got_shr;
    bool waiting_for_shr;
    bool rejected_frame;
    struct packetbuf local_packetbuf;
    struct packetbuf *actual_packetbuf;
    bool shall_send_acknowledgment;
    bool got_frame;
    uint8_t acknowledgment[CONTIKIMAC_MAX_ACKNOWLEDGMENT_LEN];
    uint_fast16_t acknowledgment_len;
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    bool read_and_parsed;
    bool is_helloack;
    bool is_ack;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
  } duty_cycle;

  struct {
    uint_fast8_t cca_count;
    bool is_waiting_for_acknowledgment_shr;
    bool got_acknowledgment_shr;
    uint8_t prepared_frame[RADIO_MAX_PAYLOAD];
    bool is_broadcast;
    int result;
    rtimer_clock_t next_transmission;
    rtimer_clock_t timeout;
    frame_queue_entry_t *fqe;
    uint_fast8_t sent_once_more;
    uint8_t acknowledgment[CONTIKIMAC_MAX_ACKNOWLEDGMENT_LEN];
    uint8_t acknowledgment_len;
#if AKES_MAC_ENABLED
    bool is_hello;
    bool is_helloack;
    bool is_ack;
    uint8_t nonce[CCM_STAR_NONCE_LENGTH];
    uint8_t acknowledgment_key[AES_128_KEY_LENGTH];
#if !CONTIKIMAC_FRAMER_POTR_ENABLED
    uint32_t last_unicast_counter;
#endif /* !CONTIKIMAC_FRAMER_POTR_ENABLED */
#endif /* AKES_MAC_ENABLED */
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    wake_up_counter_t receivers_wake_up_counter;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#if CONTIKIMAC_WITH_PHASE_LOCK
    rtimer_clock_t t1[2];
#endif /* CONTIKIMAC_WITH_PHASE_LOCK */
#if CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK
    rtimer_clock_t t0[2];
#endif /* CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK */
    uint8_t strobes;
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    uint8_t shall_encrypt;
    uint8_t a_len;
    uint16_t m_len;
    uint_fast8_t mic_len;
    uint16_t totlen;
    uint8_t unsecured_frame[RADIO_MAX_PAYLOAD];
    uint_fast16_t strobe_index_offset;
    uint8_t phase_offset;
    uint8_t key[AES_128_KEY_LENGTH];
    bool update_error_occurred;
#else /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    uint8_t seqno;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
  } strobe;
} contikimac_state_t;

#if CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK
rtimer_clock_t contikimac_get_last_but_one_t0(void);
#endif /* CONTIKIMAC_WITH_ORIGINAL_PHASE_LOCK */
#if CONTIKIMAC_WITH_PHASE_LOCK
rtimer_clock_t contikimac_get_last_but_one_t1(void);
#endif /* CONTIKIMAC_WITH_PHASE_LOCK */
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
rtimer_clock_t contikimac_get_sfd_timestamp(void);
rtimer_clock_t contikimac_get_phase(void);
uint8_t contikimac_get_last_delta(void);
uint8_t contikimac_get_last_strobe_index(void);
bool potr_has_strobe_index(enum potr_frame_type type);
rtimer_clock_t contikimac_get_last_wake_up_time(void);
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
rtimer_clock_t contikimac_get_next_strobe_start(void);
wake_up_counter_t contikimac_get_wake_up_counter(rtimer_clock_t t);
wake_up_counter_t contikimac_predict_wake_up_counter(void);
wake_up_counter_t contikimac_restore_wake_up_counter(void);
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */

extern const struct mac_driver contikimac_driver;
extern contikimac_state_t contikimac_state;
extern const struct contikimac_framer CONTIKIMAC_FRAMER;

#endif /* CONTIKIMAC_H_ */
