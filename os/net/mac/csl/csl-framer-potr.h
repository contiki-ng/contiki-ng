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
 *         A CSL-enabled version of practical on-the-fly rejection (POTR)
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CSL_FRAMER_POTR_H_
#define CSL_FRAMER_POTR_H_

#include "contiki.h"
#include "lib/leaky-bucket.h"
#include "services/akes/akes-mac.h"
#include "net/mac/wake-up-counter.h"
#include "net/mac/llsec802154.h"

/* define the lengths of various fields */
#ifdef CSL_FRAMER_POTR_CONF_OTP_LEN
#define CSL_FRAMER_POTR_OTP_LEN CSL_FRAMER_POTR_CONF_OTP_LEN
#else /* CSL_FRAMER_POTR_CONF_OTP_LEN */
#define CSL_FRAMER_POTR_OTP_LEN 2
#endif /* CSL_FRAMER_POTR_CONF_OTP_LEN */
#define CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN (1)
#define CSL_FRAMER_POTR_LONG_RENDEZVOUS_TIME_LEN (2)
#define CSL_FRAMER_POTR_SHORT_RENDEZVOUS_TIME_LEN (1)
#define CSL_FRAMER_POTR_PAYLOAD_FRAMES_LEN_LEN (1)
#define CSL_FRAMER_POTR_SEQUENCE_NUMBER_LEN (1)
#define CSL_FRAMER_POTR_SOURCE_INDEX_LEN (1)
#define CSL_FRAMER_POTR_PAN_ID_LEN (2)
#define CSL_FRAMER_POTR_PHASE_LEN (2)

/* define the maximum length of wake-up frames */
#define CSL_FRAMER_POTR_HELLO_WAKE_UP_FRAME_LEN (RADIO_PHY_HEADER_LEN \
    + CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN \
    + CSL_FRAMER_POTR_PAN_ID_LEN \
    + CSL_FRAMER_POTR_LONG_RENDEZVOUS_TIME_LEN)
#define CSL_FRAMER_POTR_HELLOACK_WAKE_UP_FRAME_LEN (RADIO_PHY_HEADER_LEN \
    + CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN \
    + CSL_FRAMER_POTR_PAN_ID_LEN \
    + CSL_FRAMER_POTR_SHORT_RENDEZVOUS_TIME_LEN)
#define CSL_FRAMER_POTR_ACK_WAKE_UP_FRAME_LEN (RADIO_PHY_HEADER_LEN \
    + CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN \
    + CSL_FRAMER_POTR_SOURCE_INDEX_LEN \
    + CSL_FRAMER_POTR_PAYLOAD_FRAMES_LEN_LEN \
    + CSL_FRAMER_POTR_OTP_LEN \
    + CSL_FRAMER_POTR_SHORT_RENDEZVOUS_TIME_LEN)
#define CSL_FRAMER_POTR_NORMAL_WAKE_UP_FRAME_LEN (CSL_FRAMER_POTR_ACK_WAKE_UP_FRAME_LEN)
#define CSL_FRAMER_POTR_MIN_WAKE_UP_FRAME_LEN \
    MIN(CSL_FRAMER_POTR_HELLO_WAKE_UP_FRAME_LEN, \
    MIN(CSL_FRAMER_POTR_HELLOACK_WAKE_UP_FRAME_LEN, \
    MIN(CSL_FRAMER_POTR_ACK_WAKE_UP_FRAME_LEN, CSL_FRAMER_POTR_NORMAL_WAKE_UP_FRAME_LEN)))
#define CSL_FRAMER_POTR_MAX_WAKE_UP_FRAME_LEN \
    MAX(CSL_FRAMER_POTR_HELLO_WAKE_UP_FRAME_LEN, \
    MAX(CSL_FRAMER_POTR_HELLOACK_WAKE_UP_FRAME_LEN, \
    MAX(CSL_FRAMER_POTR_ACK_WAKE_UP_FRAME_LEN, CSL_FRAMER_POTR_NORMAL_WAKE_UP_FRAME_LEN)))

/* define how many bytes should be received before parsing wake-up frames */
#define CSL_FRAMER_POTR_MIN_BYTES_FOR_PARSING_WAKE_UP_FRAMES (1)

/* define the maximum length of acknowledgement frames */
#define CSL_FRAMER_POTR_MAX_ACKNOWLEDGEMENT_LEN (CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN \
    + CSL_FRAMER_POTR_PHASE_LEN \
    + AKES_MAC_UNICAST_MIC_LEN)

/* define the lengths of piggybacked data */
#if MAC_CONF_WITH_CSL && !LLSEC802154_USES_FRAME_COUNTER
#define CSL_FRAMER_POTR_HELLO_PIGGYBACK_LEN (WAKE_UP_COUNTER_LEN)
#define CSL_FRAMER_POTR_HELLOACK_PIGGYBACK_LEN (CSL_FRAMER_POTR_PHASE_LEN \
    + WAKE_UP_COUNTER_LEN \
    + AKES_NBR_CHALLENGE_LEN)
#define CSL_FRAMER_POTR_ACK_PIGGYBACK_LEN (CSL_FRAMER_POTR_PHASE_LEN + AKES_NBR_CHALLENGE_LEN)
#else /* MAC_CONF_WITH_CSL && !LLSEC802154_USES_FRAME_COUNTER */
#define CSL_FRAMER_POTR_HELLO_PIGGYBACK_LEN (0)
#define CSL_FRAMER_POTR_HELLOACK_PIGGYBACK_LEN (0)
#define CSL_FRAMER_POTR_ACK_PIGGYBACK_LEN (0)
#endif /* MAC_CONF_WITH_CSL && !LLSEC802154_USES_FRAME_COUNTER */

/**
 * \brief Writes a CSL phase to the specified memory location
 */
void csl_framer_potr_write_phase(uint8_t *dst, rtimer_clock_t phase);

/**
 * \brief Reads a CSL phase from the specified memory location
 */
rtimer_clock_t csl_framer_potr_parse_phase(uint8_t *src);

/** used for rate-limiting incoming HELLOs */
extern struct leaky_bucket csl_framer_potr_hello_inc_bucket;
/** used for rate-limiting incoming HELLOACKs */
extern struct leaky_bucket csl_framer_potr_helloack_inc_bucket;

#endif /* CSL_FRAMER_POTR_H_ */

/** @} */
