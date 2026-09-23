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
 *         Practical On-the-fly Rejection (POTR).
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CONTIKIMAC_FRAMER_POTR_H_
#define CONTIKIMAC_FRAMER_POTR_H_

#include "contiki.h"
#include "lib/leaky-bucket.h"
#include "net/linkaddr.h"
#include "net/mac/anti-replay.h"
#include "net/mac/contikimac/contikimac-framer.h"
#include "net/mac/framer/framer.h"
#include "net/mac/llsec802154.h"
#include "sys/rtimer.h"

#ifdef CONTIKIMAC_FRAMER_POTR_CONF_ENABLED
#define CONTIKIMAC_FRAMER_POTR_ENABLED CONTIKIMAC_FRAMER_POTR_CONF_ENABLED
#else /* CONTIKIMAC_FRAMER_POTR_CONF_ENABLED */
#define CONTIKIMAC_FRAMER_POTR_ENABLED 0
#endif /* CONTIKIMAC_FRAMER_POTR_CONF_ENABLED */

#ifdef CONTIKIMAC_FRAMER_POTR_ILOS_CONF_ENABLED
#define CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED \
  CONTIKIMAC_FRAMER_POTR_ILOS_CONF_ENABLED
#else /* CONTIKIMAC_FRAMER_POTR_ILOS_CONF_ENABLED */
#define CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED 0
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_CONF_ENABLED */

#ifdef CONTIKIMAC_FRAMER_POTR_CONF_OTP_LEN
#define CONTIKIMAC_FRAMER_POTR_OTP_LEN CONTIKIMAC_FRAMER_POTR_CONF_OTP_LEN
#else /* CONTIKIMAC_FRAMER_POTR_CONF_OTP_LEN */
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
#define CONTIKIMAC_FRAMER_POTR_OTP_LEN 2
#else /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#define CONTIKIMAC_FRAMER_POTR_OTP_LEN 3
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#endif /* CONTIKIMAC_FRAMER_POTR_CONF_OTP_LEN */

#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
#define CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN (4)
#else /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#define CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN (0)
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
#define CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN 0
#else /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#if ANTI_REPLAY_WITH_SUPPRESSION
#define CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN 1
#else /* ANTI_REPLAY_WITH_SUPPRESSION */
#define CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN 4
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

#define CONTIKIMAC_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN (1)
#define CONTIKIMAC_FRAMER_POTR_DELTA_LEN (1)
#define CONTIKIMAC_FRAMER_POTR_PAN_ID_LEN (2)
#define CONTIKIMAC_FRAMER_POTR_PHASE_LEN (RTIMER_CLOCK_SIZE)

enum potr_frame_type {
  CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_DATA =
      (0x7 /* extended */
       | (0x0 << 3) /* unused extended frame type 000 */
       | (0x0 << 6)),
  CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_COMMAND =
      (0x7 /* extended */
       | (0x0 << 3) /* unused extended frame type 000 */
       | (0x1 << 6)),
  CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK =
      (0x7 /* extended */
       | (0x0 << 3) /* unused extended frame type 000 */
       | (0x2 << 6)),
  CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK =
      (0x7 /* extended */
       | (0x0 << 3) /* unused extended frame type 000 */
       | (0x3 << 6)),
  CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_DATA =
      (0x7 /* extended */
       | (0x1 << 3) /* unused extended frame type 001 */
       | (0x0 << 6)),
  CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_COMMAND =
      (0x7 /* extended */
       | (0x1 << 3) /* unused extended frame type 001 */
       | (0x1 << 6)),
  CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLO =
      (0x7 /* extended */
       | (0x1 << 3) /* unused extended frame type 001 */
       | (0x2 << 6)),
  CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACKNOWLEDGMENT =
      (0x7 /* extended */
       | (0x1 << 3) /* unused extended frame type 001 */
       | (0x3 << 6))
};

typedef union {
  uint8_t u8[CONTIKIMAC_FRAMER_POTR_OTP_LEN];
} contikimac_framer_potr_otp_t;

extern leaky_bucket_t contikimac_framer_potr_hello_inc_bucket;
extern leaky_bucket_t contikimac_framer_potr_helloack_inc_bucket;
extern const struct framer contikimac_framer_potr;
extern const struct contikimac_framer contikimac_framer_potr_contikimac_framer;

uint_fast16_t contikimac_framer_potr_get_strobe_index_offset(
    enum potr_frame_type type);
int contikimac_framer_potr_update_contents(void);

#endif /* CONTIKIMAC_FRAMER_POTR_H_ */
