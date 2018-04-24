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
 * \ingroup link-layer
 * \defgroup csl 802.15.4 CSL
 *
 * \brief        The IEEE 802.15.4-2015 Coordinated Sampled Listening (CSL) protocol.
 *
 * CSL works without network-wide time synchronization. For improved
 * reliability, energy efficiency, and throughput CSL supports channel hopping,
 * synchronized transmissions, and burst forwarding, respectively. Furthermore,
 * CSL enables mains-powered devices to disable duty cycling so as to reduce
 * latencies, but that feature is not implemented, yet.
 *
 * @{
 * \file
 *         Coordinated Sampled Listening (CSL)
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CSL_H_
#define CSL_H_

#include "net/mac/wake-up-counter.h"
#include "net/mac/csl/csl-framer-compliant.h"
#include "net/mac/csl/csl-framer-potr.h"
#include "dev/radio.h"
#include "sys/rtimer.h"
#include "lib/aes-128.h"
#include "lib/ccm-star.h"
#include "net/linkaddr.h"
#include "net/packetbuf.h"
#ifdef LPM_CONF_ENABLE
#include "lpm.h"
#endif /* LPM_CONF_ENABLE */

/**
 * This CSL implementation can either be configured to operate standards
 * -compliant or to use a whole lot of third-parts security enhancements.
 */
#ifdef CSL_CONF_COMPLIANT
#define CSL_COMPLIANT CSL_CONF_COMPLIANT
#else /* CSL_CONF_COMPLIANT */
#define CSL_COMPLIANT 1
#endif /* CSL_CONF_COMPLIANT */

#ifdef CSL_CONF_MAX_BURST_INDEX
#define CSL_MAX_BURST_INDEX CSL_CONF_MAX_BURST_INDEX
#else /* CSL_CONF_MAX_BURST_INDEX */
#define CSL_MAX_BURST_INDEX (3)
#endif /* CSL_CONF_MAX_BURST_INDEX */

#if CSL_COMPLIANT
#define CSL_FRAMER \
    csl_framer_compliant_csl_framer
#define CSL_MAX_WAKE_UP_FRAME_LEN \
    CSL_FRAMER_COMPLIANT_MAX_WAKE_UP_FRAME_LEN
#define CSL_MAX_ACKNOWLEDGEMENT_LEN \
    CSL_FRAMER_COMPLIANT_MAX_ACKNOWLEDGEMENT_LEN
#define CSL_MIN_BYTES_FOR_PARSING_WAKE_UP_FRAMES \
    CSL_FRAMER_COMPLIANT_MIN_BYTES_FOR_PARSING_WAKE_UP_FRAMES
#else /* CSL_COMPLIANT */
#define CSL_FRAMER \
    csl_framer_potr_csl_framer
#define CSL_MAX_WAKE_UP_FRAME_LEN \
    CSL_FRAMER_POTR_MAX_WAKE_UP_FRAME_LEN
#define CSL_MAX_ACKNOWLEDGEMENT_LEN \
    CSL_FRAMER_POTR_MAX_ACKNOWLEDGEMENT_LEN
#define CSL_MIN_BYTES_FOR_PARSING_WAKE_UP_FRAMES \
    CSL_FRAMER_POTR_MIN_BYTES_FOR_PARSING_WAKE_UP_FRAMES
#endif /* CSL_COMPLIANT */

/* TODO handle these CC2538-specific adjustments in rtimer.c */
#define CSL_LPM_SWITCHING (2)
#define CSL_LPM_DEEP_SWITCHING (2)
#ifdef LPM_CONF_ENABLE
#if LPM_CONF_ENABLE
#if (LPM_CONF_MAX_PM == LPM_PM0)
#elif (LPM_CONF_MAX_PM == LPM_PM1)
#undef CSL_LPM_SWITCHING
#define CSL_LPM_SWITCHING (9)
#undef CSL_LPM_DEEP_SWITCHING
#define CSL_LPM_DEEP_SWITCHING (13)
#elif (LPM_CONF_MAX_PM == LPM_PM2)
#undef CSL_LPM_SWITCHING
#define CSL_LPM_SWITCHING (13)
#undef CSL_LPM_DEEP_SWITCHING
#define CSL_LPM_DEEP_SWITCHING (13)
#else
#warning unsupported power mode
#endif
#endif /* LPM_CONF_ENABLE */
#endif /* LPM_CONF_ENABLE */

#define CSL_ACKNOWLEDGEMENT_WINDOW_MIN \
    (RADIO_RECEIVE_CALIBRATION_TIME - 1 + RADIO_SHR_TIME - 1)
#define CSL_ACKNOWLEDGEMENT_WINDOW_MAX \
    (RADIO_RECEIVE_CALIBRATION_TIME + RADIO_SHR_TIME + 1)
#define CSL_ACKNOWLEDGEMENT_WINDOW (CSL_ACKNOWLEDGEMENT_WINDOW_MAX \
    - CSL_ACKNOWLEDGEMENT_WINDOW_MIN \
    + 1)
#define CSL_COLLISION_AVOIDANCE_DURATION \
    (RADIO_RECEIVE_CALIBRATION_TIME + RADIO_CCA_TIME - 2)
#define CSL_WAKE_UP_SEQUENCE_GUARD_TIME (CSL_LPM_SWITCHING \
    + CSL_COLLISION_AVOIDANCE_DURATION \
    + RADIO_TRANSMIT_CALIBRATION_TIME \
    - 1)
#define CSL_CLOCK_TOLERANCE (15) /* ppm */
#define CSL_NEGATIVE_SYNC_GUARD_TIME (2 /* sender side */ \
    + 2 /* receiver side */ \
    + CSL_ACKNOWLEDGEMENT_WINDOW /* allow for pulse-delay attacks */)
#define CSL_POSITIVE_SYNC_GUARD_TIME (2 + 2)

#if !CSL_COMPLIANT
#define CSL_MAX_RETRANSMISSION_DELAY (15) /* seconds */
#define CSL_COMPENSATION_TOLERANCE (3) /* ppm */
#define CSL_MIN_TIME_BETWEEN_DRIFT_UPDATES (50) /* seconds */
#define CSL_MAX_OVERALL_UNCERTAINTY (US_TO_RTIMERTICKS(2000) \
    + CSL_NEGATIVE_SYNC_GUARD_TIME \
    + CSL_POSITIVE_SYNC_GUARD_TIME)
#define CSL_INITIAL_UPDATE_THRESHOLD ( \
    RTIMERTICKS_TO_S( \
    ((CSL_MAX_OVERALL_UNCERTAINTY - CSL_NEGATIVE_SYNC_GUARD_TIME - CSL_POSITIVE_SYNC_GUARD_TIME) \
    * (uint32_t)1000000) / (2 * CSL_CLOCK_TOLERANCE)) \
    - CSL_MAX_RETRANSMISSION_DELAY)
#define CSL_SUBSEQUENT_UPDATE_THRESHOLD MIN(300, \
    RTIMERTICKS_TO_S( \
    ((CSL_MAX_OVERALL_UNCERTAINTY - CSL_NEGATIVE_SYNC_GUARD_TIME - CSL_POSITIVE_SYNC_GUARD_TIME) \
    * (uint32_t)1000000) / CSL_COMPENSATION_TOLERANCE) \
    - CSL_MAX_RETRANSMISSION_DELAY)
#endif /* !CSL_COMPLIANT */

#if !CSL_COMPLIANT
enum csl_subtype {
  CSL_SUBTYPE_HELLO = 0,
  CSL_SUBTYPE_HELLOACK = 1,
  CSL_SUBTYPE_ACK = 2,
  CSL_SUBTYPE_NORMAL = 3,
};
#endif /* !CSL_COMPLIANT */

/** Either stores data about ongoing receptions or ongoing transmissions */
typedef union {
  struct {
#if CSL_COMPLIANT
    linkaddr_t receiver;
#else /* CSL_COMPLIANT */
    enum csl_subtype subtype;
    uint8_t next_frames_len;
#endif /* CSL_COMPLIANT */
    uint8_t min_bytes_for_filtering;
    uint8_t frame_pending;
    uint16_t remaining_wake_up_frames;
    rtimer_clock_t rendezvous_time;
    int got_wake_up_frames_shr;
    int waiting_for_wake_up_frames_shr;
    int left_radio_on;
    int waiting_for_unwanted_shr;
    int got_rendezvous_time;
#if !CSL_COMPLIANT
    int skip_to_rendezvous;
#endif /* !CSL_COMPLIANT */
    int waiting_for_payload_frames_shr;
    int got_payload_frames_shr;
    int rejected_payload_frame;
    rtimer_clock_t wake_up_frame_timeout;
    linkaddr_t sender;
    int shall_send_acknowledgement;
    int received_frame;
    uint8_t last_burst_index;
    uint8_t acknowledgement[1 /* Frame Length */ + CSL_MAX_ACKNOWLEDGEMENT_LEN];
    struct packetbuf local_packetbuf[CSL_MAX_BURST_INDEX + 1];
    struct packetbuf *actual_packetbuf[CSL_MAX_BURST_INDEX + 1];
    rtimer_clock_t wake_up_frame_sfd_timestamp;
  } duty_cycle;

  struct {
#if CSL_COMPLIANT && LLSEC802154_USES_FRAME_COUNTER
    uint8_t expected_mic_len;
    linkaddr_t receiver;
    uint8_t acknowledgement_key[AES_128_KEY_LENGTH];
    frame802154_frame_counter_t his_unicast_counter;
#endif /* CSL_COMPLIANT && LLSEC802154_USES_FRAME_COUNTER */
#if !CSL_COMPLIANT
    enum csl_subtype subtype;
    uint8_t rendezvous_time_len;
    uint8_t acknowledgement_key[AES_128_KEY_LENGTH];
    uint8_t acknowledgement_nonce[CCM_STAR_NONCE_LENGTH];
    wake_up_counter_t receivers_wake_up_counter;
#endif /* !CSL_COMPLIANT */
    int is_broadcast;
    uint8_t wake_up_frame_len;
    struct buffered_frame *bf[CSL_MAX_BURST_INDEX + 1];
    int result[CSL_MAX_BURST_INDEX + 1];
    uint8_t last_burst_index;
    uint8_t burst_index;
    rtimer_clock_t wake_up_sequence_start;
    uint16_t remaining_wake_up_frames;
    uint8_t wrote_payload_frames_phy_header;
    uint8_t remaining_payload_frame_bytes;
    uint8_t next_wake_up_frames[RADIO_MAX_SEQUENCE_LEN];
    uint32_t wake_up_sequence_pos;
    rtimer_clock_t payload_frame_start;
    rtimer_clock_t next_rendezvous_time_update;
    uint8_t payload_frame[CSL_MAX_BURST_INDEX + 1][1 /* Frame Length */ + RADIO_MAX_FRAME_LEN];
    rtimer_clock_t acknowledgement_sfd_timestamp;
    rtimer_clock_t acknowledgement_phase;
    int waiting_for_acknowledgement_shr;
    int got_acknowledgement_shr;
    int is_waiting_for_txdone;
  } transmit;
} csl_state_t;


#if !CSL_COMPLIANT
/**
 * \brief Retrieves the currently configured radio channel
 */
uint8_t csl_get_channel(void);

/**
 * \brief Calculates the wake-up counter at time t
 */
wake_up_counter_t csl_get_wake_up_counter(rtimer_clock_t t);

/**
 * \brief Predicts the receiver's wake-up counter when receiving our wake-up sequence
 */
wake_up_counter_t csl_predict_wake_up_counter(void);

/**
 * \brief Restores the sender's wake-up counter at time of transmitting the HELLO
 */
wake_up_counter_t csl_restore_wake_up_counter(void);

/** This node's wake-up counter */
extern wake_up_counter_t csl_wake_up_counter;

/** The number of wake-up frames that precede a HELLO */
extern const uint32_t csl_hello_wake_up_sequence_length;

/** The time it takes to transmit the wake-up sequence that precedes a HELLO */
const rtimer_clock_t csl_hello_wake_up_sequence_tx_time;
#endif /* !CSL_COMPLIANT */

/**
 * \brief Returns the time when the last channel sample was done
 */
rtimer_clock_t csl_get_last_wake_up_time(void);

/**
 * \brief Returns when the transmission of the next payload frame's SHR is done
 */
rtimer_clock_t csl_get_payload_frames_shr_end(void);

/**
 * \brief Returns when the reception of the last payload frame's SHR ended
 */
rtimer_clock_t csl_get_sfd_timestamp_of_last_payload_frame(void);

/**
 * \brief Returns the time between t and the next channel sample
 */
rtimer_clock_t csl_get_phase(rtimer_clock_t t);

/** The marshaling and parsing of frames is delegated to this external component */
extern const struct csl_framer CSL_FRAMER;

/** The state of CSL is public, but should only be accessed from its components */
extern csl_state_t csl_state;

/** The actual MAC driver */
extern const struct mac_driver csl_driver;

#endif /* CSL_H_ */

/** @} */
/** @} */
