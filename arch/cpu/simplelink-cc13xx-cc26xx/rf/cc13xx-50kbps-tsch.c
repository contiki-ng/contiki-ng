/*
 * Copyright (c) 2018, University of Bristol - http://www.bristol.ac.uk/
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
 * \file
 *         IEEE 802.15.4 TSCH timeslot timings for CC13xx chips at 50kbps datarate
 * \author
 *         Atis Elsts <atis.elsts@bristol.ac.uk>
 *
 */

#include "contiki.h"
#include "net/mac/tsch/tsch.h"

#define CC13XX_TSCH_DEFAULT_TS_CCA_OFFSET         1800
#define CC13XX_TSCH_DEFAULT_TS_CCA                128
#define CC13XX_TSCH_DEFAULT_TS_TX_OFFSET          3800
#define CC13XX_TSCH_DEFAULT_TS_RX_OFFSET          (CC13XX_TSCH_DEFAULT_TS_TX_OFFSET - (TSCH_CONF_RX_WAIT / 2))
#define CC13XX_TSCH_DEFAULT_TS_RX_ACK_DELAY       2000
#define CC13XX_TSCH_DEFAULT_TS_TX_ACK_DELAY       3000
#define CC13XX_TSCH_DEFAULT_TS_RX_WAIT            TSCH_CONF_RX_WAIT
#define CC13XX_TSCH_DEFAULT_TS_ACK_WAIT           3000
#define CC13XX_TSCH_DEFAULT_TS_RX_TX              192
#define CC13XX_TSCH_DEFAULT_TS_MAX_ACK            10000
#define CC13XX_TSCH_DEFAULT_TS_MAX_TX             21600

/* Timeslot length: 40000 usec */
#define CC13XX_TSCH_DEFAULT_TS_TIMESLOT_LENGTH  40000

/* TSCH timeslot timing (microseconds) */
const tsch_timeslot_timing_usec tsch_timing_cc13xx_50kbps = {
  CC13XX_TSCH_DEFAULT_TS_CCA_OFFSET,
  CC13XX_TSCH_DEFAULT_TS_CCA,
  CC13XX_TSCH_DEFAULT_TS_TX_OFFSET,
  CC13XX_TSCH_DEFAULT_TS_RX_OFFSET,
  CC13XX_TSCH_DEFAULT_TS_RX_ACK_DELAY,
  CC13XX_TSCH_DEFAULT_TS_TX_ACK_DELAY,
  CC13XX_TSCH_DEFAULT_TS_RX_WAIT,
  CC13XX_TSCH_DEFAULT_TS_ACK_WAIT,
  CC13XX_TSCH_DEFAULT_TS_RX_TX,
  CC13XX_TSCH_DEFAULT_TS_MAX_ACK,
  CC13XX_TSCH_DEFAULT_TS_MAX_TX,
  CC13XX_TSCH_DEFAULT_TS_TIMESLOT_LENGTH,
};
