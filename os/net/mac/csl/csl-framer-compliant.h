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
 *         Assembles IEEE 802.15.4-compliant frames
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CSL_FRAMER_COMPLIANT_H_
#define CSL_FRAMER_COMPLIANT_H_

#include "contiki.h"
#include "net/mac/framer/crc16-framer.h"
#include "net/linkaddr.h"
#include "services/akes/akes-mac.h"

/* define the maximum length of wake-up frames */
#define CSL_FRAMER_COMPLIANT_RENDEZVOUS_TIME_IE_LEN (4)
#define CSL_FRAMER_COMPLIANT_PAN_ID_LEN (2)
#define CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN (RADIO_PHY_HEADER_LEN \
    + 2 /* MP Frame Control field */ \
    + CSL_FRAMER_COMPLIANT_PAN_ID_LEN /* Destination PAN ID field */ \
    + LINKADDR_SIZE /* Destination Address field */ \
    + CSL_FRAMER_COMPLIANT_RENDEZVOUS_TIME_IE_LEN \
    + CRC16_FRAMER_CHECKSUM_LEN)
#define CSL_FRAMER_COMPLIANT_MAX_WAKE_UP_FRAME_LEN CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN

/* define how many bytes should be received before parsing wake-up frames */
#define CSL_FRAMER_COMPLIANT_MIN_BYTES_FOR_PARSING_WAKE_UP_FRAMES \
    (CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN - CRC16_FRAMER_CHECKSUM_LEN)

/* define the maximum length of acknowledgement frames */
#define CSL_FRAMER_COMPLIANT_CSL_IE_LEN (6)
#define CSL_FRAMER_COMPLIANT_ACKNOWLEDGEMENT_SEC_LVL (AKES_MAC_UNICAST_SEC_LVL)
#define CSL_FRAMER_COMPLIANT_ACKNOWLEDGEMENT_MIC_LEN (AKES_MAC_UNICAST_MIC_LEN)
#define CSL_FRAMER_COMPLIANT_MAX_ACKNOWLEDGEMENT_LEN (2 /* frame type */ \
    + LINKADDR_SIZE /* destination */ \
    + 5 /* auxiliary security header */ \
    + CSL_FRAMER_COMPLIANT_CSL_IE_LEN \
    + CSL_FRAMER_COMPLIANT_ACKNOWLEDGEMENT_MIC_LEN \
    + CRC16_FRAMER_CHECKSUM_LEN)

#endif /* CSL_FRAMER_COMPLIANT_H_ */

/** @} */
