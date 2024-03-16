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
 *         Defines the interface to framing-related tasks
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CSL_FRAMER_H_
#define CSL_FRAMER_H_

#include "dev/radio.h"
#include "sys/rtimer.h"
#include <stdint.h>

#define CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH(uncertainty, wake_up_frame_len) \
  (((((uint64_t)(uncertainty)) * 1000 * 1000 / RTIMER_SECOND) \
    / (RADIO_BYTE_PERIOD * (wake_up_frame_len))) \
   + 1 /* round up */ \
   + 1 /* once more */)

/**
 * Strategy for creating and parsing of IEEE 802.15.4 frames
 */
struct csl_framer {

  /**
   * Returns the number of bytes that are necessary to filter out unwanted
   * payload frames
   */
  uint8_t (* get_min_bytes_for_filtering)(void);

  /**
   * Parses and validates incoming payload frames;
   * Creates corresponding acknowledgment frame
   */
  int (* filter)(void);

  /**
   * Returns the length of the current wake-up frame,
   * excluding the PHY header
   */
  uint_fast16_t (* get_length_of_wake_up_frame)(void);

  /**
   * Creates a wake-up frame
   */
  int (* create_wake_up_frame)(uint8_t *dst);

  /**
   * Updates the rendezvous time of the created wake-up frame
   */
  void (* update_rendezvous_time)(uint8_t *frame_length);

  /**
   * Parses and validates the incoming payload frame
   */
  int (* parse_wake_up_frame)(void);

  /**
   * Prepares for parsing upcoming acknowledgment frames within interrupt
   * contexts
   */
  int (* prepare_acknowledgment_parsing)(void);

  /**
   * Parses and validates the incoming acknowledgment frame
   */
  int (* parse_acknowledgment)(void);

  /**
   * Does bookkeeping work
   */
  void (* on_unicast_transmitted)(void);

  /**
   * Performs initialization tasks
   */
  void (*init)(void);
};

#endif /* CSL_FRAMER_H_ */

/** @} */
