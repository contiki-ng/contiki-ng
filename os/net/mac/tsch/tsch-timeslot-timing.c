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
 *         IEEE 802.15.4 TSCH timeslot timings
 * \author
 *         Simon Duquennoy <simon.duquennoy@ri.se>
 *
 */

/**
 * \addtogroup tsch
 * @{
*/

#include "contiki.h"
#include "net/mac/tsch/tsch.h"

/**
 * \brief TSCH timing attributes and description. All timings are in usec.
 *
 * CCAOffset   -> time between the beginning of timeslot and start of CCA
 * CCA         -> duration of CCA (CCA is NOT ENABLED by default)
 * TxOffset    -> time between beginning of the timeslot and start of frame TX (end of SFD)
 * RxOffset    -> beginning of the timeslot to when the receiver shall be listening
 * RxAckDelay  -> end of frame to when the transmitter shall listen for ACK
 * TxAckDelay  -> end of frame to the start of ACK tx
 * RxWait      -> time to wait for start of frame (Guard time)
 * AckWait     -> min time to wait for start of an ACK frame
 * RxTx        -> receive-to-transmit switch time (NOT USED)
 * MaxAck      -> TX time to send a max length ACK
 * MaxTx       -> TX time to send the max length frame
 *
 * The TSCH timeslot structure is described in the IEEE 802.15.4-2015 standard,
 * in particular in the Figure 6-30.
 *
 * The default timeslot timing in the standard is a guard time of
 * 2200 us, a Tx offset of 2120 us and a Rx offset of 1120 us.
 * As a result, the listening device has a guard time not centered
 * on the expected Tx time. This is to be fixed in the next iteration
 * of the standard. This can be enabled with:
 * TxOffset: 2120
 * RxOffset: 1120
 * RxWait:   2200
 *
 * Instead, we align the Rx guard time on expected Tx time. The Rx
 * guard time is user-configurable with TSCH_CONF_RX_WAIT.
 * (TxOffset - (RxWait / 2)) instead
 */

const tsch_timeslot_timing_usec tsch_timeslot_timing_us_10000 = {
   1800, /* CCAOffset */
    128, /* CCA */
   2120, /* TxOffset */
  (2120 - (TSCH_CONF_RX_WAIT / 2)), /* RxOffset */
    800, /* RxAckDelay */
   1000, /* TxAckDelay */
  TSCH_CONF_RX_WAIT, /* RxWait */
    400, /* AckWait */
    192, /* RxTx */
   2400, /* MaxAck */
   4256, /* MaxTx */
  10000, /* TimeslotLength */
};

/** @} */
