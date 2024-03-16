/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 *         Creates and parses the ContikiMAC header.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CONTIKIMAC_FRAMER_ORIGINAL_H_
#define CONTIKIMAC_FRAMER_ORIGINAL_H_

#include "contiki.h"
#include "net/linkaddr.h"
#include "net/mac/anti-replay.h"
#include "net/mac/contikimac/contikimac-framer.h"
#include "net/mac/framer/crc16-framer.h"
#include "net/mac/framer/framer.h"
#include "net/mac/llsec802154.h"
#include "services/akes/akes-mac.h"

#define CONTIKIMAC_FRAMER_ORIGINAL_MIN_BYTES_FOR_FILTERING \
  (2 /* Frame Control */ \
   + (AKES_MAC_ENABLED ? 0 : 1) /* Sequence Number */ \
   + 2 /* Destination PAN ID */ \
   + LINKADDR_SIZE /* Destination Address */ \
   + LINKADDR_SIZE /* Source Address */ \
   + (AKES_MAC_ENABLED ? 5 : 0) /* Auxiliary Security Header */)
#define CONTIKIMAC_FRAMER_ORIGINAL_UNAUTHENTICATED_ACKNOWLEDGMENT_LEN \
  (2 /* Frame Control */ \
   + ((LINKADDR_SIZE == 2) ? 0 : 2) /* Destination PAN ID */ \
   + LINKADDR_SIZE /* Destination Address */ \
   + CRC16_FRAMER_CHECKSUM_LEN)
#define CONTIKIMAC_FRAMER_AUTHENTICATED_ACKNOWLEDGMENT_LEN \
  (CONTIKIMAC_FRAMER_ORIGINAL_UNAUTHENTICATED_ACKNOWLEDGMENT_LEN \
   + (AKES_MAC_UNICAST_MIC_LEN) + 5 /* Auxiliary Security Header */)

int contikimac_framer_filter(uint8_t *acknowledgment);
extern const struct framer contikimac_framer_original;
extern const struct contikimac_framer
    contikimac_framer_original_contikimac_framer;

#endif /* CONTIKIMAC_FRAMER_ORIGINAL_H_ */
