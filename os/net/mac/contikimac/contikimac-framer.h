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
 * \file
 *         Defines the interface to framing-related tasks
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CONTIKIMAC_FRAMER_H_
#define CONTIKIMAC_FRAMER_H_

#include <stdint.h>

/** Strategy for creating and parsing of IEEE 802.15.4 frames */
struct contikimac_framer {

  /** Returns how many bytes have to be received when calling filter() */
  uint8_t (* get_min_bytes_for_filtering)(void);

  /** Parses and validates incoming frames; Creates acknowledgment frame */
  int (* filter)(void);

  /** Prepares for parsing acknowledgment frames within interrupt contexts */
  int (* prepare_acknowledgment_parsing)(void);

  /** Parses and validates the incoming acknowledgment frame */
  int (* parse_acknowledgment)(void);

  /** Does bookkeeping work */
  void (* on_unicast_transmitted)(void);

  /** Performs initialization tasks */
  void (* init)(void);
};

#endif /* CONTIKIMAC_FRAMER_H_ */
