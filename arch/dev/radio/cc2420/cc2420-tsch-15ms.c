/*
 * Copyright (c) 2018, RISE SICS.
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
 *         IEEE 802.15.4 TSCH timeslot timings for 15ms slots
 * \author
 *         Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */

#include "contiki.h"
#include "net/mac/tsch/tsch.h"

/**
 * \brief 15ms TSCH timeslot timings, required for cc2420 platforms as
 * they are unable to keep up with the defulat 10ms timeslots.
 */
const tsch_timeslot_timing_usec tsch_timeslot_timing_us_15000= {
  1800, /* CCAOffset */
   128, /* CCA */
  4000, /* TxOffset */
 (4000 - (TSCH_CONF_RX_WAIT / 2)), /* RxOffset */
  3600, /* RxAckDelay */
  4000, /* TxAckDelay */
 TSCH_CONF_RX_WAIT, /* RxWait */
   800, /* AckWait */
  2072, /* RxTx */
  2400, /* MaxAck */
  4256, /* MaxTx */
 15000, /* TimeslotLength */
};
