/*
 * Copyright (c) 2015, SICS Swedish ICT.
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
* \addtogroup tsch
* @{
 * \file
 *         TSCH configuration
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 */

#ifndef __TSCH_CONF_H__
#define __TSCH_CONF_H__

/********** Includes **********/

#include "contiki.h"

/******** Configuration: synchronization *******/

/* Max time before sending a unicast keep-alive message to the time source */
#ifdef TSCH_CONF_KEEPALIVE_TIMEOUT
#define TSCH_KEEPALIVE_TIMEOUT TSCH_CONF_KEEPALIVE_TIMEOUT
#else
/* Time to desynch assuming a drift of 40 PPM (80 PPM between two nodes) and guard time of +/-1ms: 12.5s. */
#define TSCH_KEEPALIVE_TIMEOUT (12 * CLOCK_SECOND)
#endif

/* With TSCH_ADAPTIVE_TIMESYNC enabled: keep-alive timeout used after reaching
 * accurate drift compensation. */
#ifdef TSCH_CONF_MAX_KEEPALIVE_TIMEOUT
#define TSCH_MAX_KEEPALIVE_TIMEOUT TSCH_CONF_MAX_KEEPALIVE_TIMEOUT
#else
#define TSCH_MAX_KEEPALIVE_TIMEOUT (60 * CLOCK_SECOND)
#endif

/* Max time without synchronization before leaving the PAN */
#ifdef TSCH_CONF_DESYNC_THRESHOLD
#define TSCH_DESYNC_THRESHOLD TSCH_CONF_DESYNC_THRESHOLD
#else
#define TSCH_DESYNC_THRESHOLD (2 * TSCH_MAX_KEEPALIVE_TIMEOUT)
#endif

/* Period between two consecutive EBs */
#ifdef TSCH_CONF_EB_PERIOD
#define TSCH_EB_PERIOD TSCH_CONF_EB_PERIOD
#else
#define TSCH_EB_PERIOD (16 * CLOCK_SECOND)
#endif

/* Max Period between two consecutive EBs */
#ifdef TSCH_CONF_MAX_EB_PERIOD
#define TSCH_MAX_EB_PERIOD TSCH_CONF_MAX_EB_PERIOD
#else
#define TSCH_MAX_EB_PERIOD (16 * CLOCK_SECOND)
#endif

/* Use SFD timestamp for synchronization? By default we merely rely on rtimer and busy wait
 * until SFD is high, which we found to provide greater accuracy on JN516x and CC2420.
 * Note: for association, however, we always use SFD timestamp to know the time of arrival
 * of the EB (because we do not busy-wait for the whole scanning process)
 * */
#ifdef TSCH_CONF_RESYNC_WITH_SFD_TIMESTAMPS
#define TSCH_RESYNC_WITH_SFD_TIMESTAMPS TSCH_CONF_RESYNC_WITH_SFD_TIMESTAMPS
#else
#define TSCH_RESYNC_WITH_SFD_TIMESTAMPS 0
#endif

/* If enabled, remove jitter due to measurement errors */
#ifdef TSCH_CONF_TIMESYNC_REMOVE_JITTER
#define TSCH_TIMESYNC_REMOVE_JITTER TSCH_CONF_TIMESYNC_REMOVE_JITTER
#else
#define TSCH_TIMESYNC_REMOVE_JITTER TSCH_RESYNC_WITH_SFD_TIMESTAMPS
#endif

/* Base drift value.
 * Used to compensate locally know inaccuracies, such as
 * the effect of having a binary 32.768 kHz timer as the TSCH time base. */
#ifdef TSCH_CONF_BASE_DRIFT_PPM
#define TSCH_BASE_DRIFT_PPM TSCH_CONF_BASE_DRIFT_PPM
#else
#define TSCH_BASE_DRIFT_PPM 0
#endif

/* Estimate the drift of the time-source neighbor and compensate for it? */
#ifdef TSCH_CONF_ADAPTIVE_TIMESYNC
#define TSCH_ADAPTIVE_TIMESYNC TSCH_CONF_ADAPTIVE_TIMESYNC
#else
#define TSCH_ADAPTIVE_TIMESYNC 1
#endif

/* An ad-hoc mechanism to have TSCH select its time source without the
 * help of an upper-layer, simply by collecting statistics on received
 * EBs and their join priority. Disabled by default as we recomment
 * mapping the time source on the RPL preferred parent
 * (via tsch_rpl_callback_parent_switch) */
#ifdef TSCH_CONF_AUTOSELECT_TIME_SOURCE
#define TSCH_AUTOSELECT_TIME_SOURCE TSCH_CONF_AUTOSELECT_TIME_SOURCE
#else
#define TSCH_AUTOSELECT_TIME_SOURCE 0
#endif /* TSCH_CONF_EB_AUTOSELECT */

/******** Configuration: channel hopping *******/

/* Default hopping sequence, used in case hopping sequence ID == 0 */
#ifdef TSCH_CONF_DEFAULT_HOPPING_SEQUENCE
#define TSCH_DEFAULT_HOPPING_SEQUENCE TSCH_CONF_DEFAULT_HOPPING_SEQUENCE
#else
#define TSCH_DEFAULT_HOPPING_SEQUENCE TSCH_HOPPING_SEQUENCE_4_4
#endif

/* Hopping sequence used for joining (scan channels) */
#ifdef TSCH_CONF_JOIN_HOPPING_SEQUENCE
#define TSCH_JOIN_HOPPING_SEQUENCE TSCH_CONF_JOIN_HOPPING_SEQUENCE
#else
#define TSCH_JOIN_HOPPING_SEQUENCE TSCH_DEFAULT_HOPPING_SEQUENCE
#endif

/* Maximum length of the TSCH channel hopping sequence. Must be greater or
 * equal to the length of TSCH_DEFAULT_HOPPING_SEQUENCE. */
#ifdef TSCH_CONF_HOPPING_SEQUENCE_MAX_LEN
#define TSCH_HOPPING_SEQUENCE_MAX_LEN TSCH_CONF_HOPPING_SEQUENCE_MAX_LEN
#else
#define TSCH_HOPPING_SEQUENCE_MAX_LEN 16
#endif

/******** Configuration: association *******/

/* Start TSCH automatically after init? If not, the upper layers
 * must call NETSTACK_MAC.on() to start it. Useful when the
 * application needs to control when the nodes are to start
 * scanning or advertising.*/
#ifdef TSCH_CONF_AUTOSTART
#define TSCH_AUTOSTART TSCH_CONF_AUTOSTART
#else
#define TSCH_AUTOSTART 1
#endif

/* Max acceptable join priority */
#ifdef TSCH_CONF_MAX_JOIN_PRIORITY
#define TSCH_MAX_JOIN_PRIORITY TSCH_CONF_MAX_JOIN_PRIORITY
#else
#define TSCH_MAX_JOIN_PRIORITY 32
#endif

/* Join only secured networks? (discard EBs with security disabled) */
#ifdef TSCH_CONF_JOIN_SECURED_ONLY
#define TSCH_JOIN_SECURED_ONLY TSCH_CONF_JOIN_SECURED_ONLY
#else
/* By default, set if LLSEC802154_ENABLED is also non-zero */
#define TSCH_JOIN_SECURED_ONLY LLSEC802154_ENABLED
#endif

/* By default, join any PAN ID. Otherwise, wait for an EB from IEEE802154_PANID */
#ifdef TSCH_CONF_JOIN_MY_PANID_ONLY
#define TSCH_JOIN_MY_PANID_ONLY TSCH_CONF_JOIN_MY_PANID_ONLY
#else
#define TSCH_JOIN_MY_PANID_ONLY 1
#endif

/* The radio polling frequency (in Hz) during association process */
#ifdef TSCH_CONF_ASSOCIATION_POLL_FREQUENCY
#define TSCH_ASSOCIATION_POLL_FREQUENCY TSCH_CONF_ASSOCIATION_POLL_FREQUENCY
#else
#define TSCH_ASSOCIATION_POLL_FREQUENCY 100
#endif

/* When associating, check ASN against our own uptime (time in minutes)..
 * Useful to force joining only with nodes started roughly at the same time.
 * Set to the max number of minutes acceptable. */
#ifdef TSCH_CONF_CHECK_TIME_AT_ASSOCIATION
#define TSCH_CHECK_TIME_AT_ASSOCIATION TSCH_CONF_CHECK_TIME_AT_ASSOCIATION
#else
#define TSCH_CHECK_TIME_AT_ASSOCIATION 0
#endif

/* By default: initialize schedule from EB when associating, using the
 * slotframe and links Information Element */
#ifdef TSCH_CONF_INIT_SCHEDULE_FROM_EB
#define TSCH_INIT_SCHEDULE_FROM_EB TSCH_CONF_INIT_SCHEDULE_FROM_EB
#else
#define TSCH_INIT_SCHEDULE_FROM_EB 1
#endif

/* How long to scan each channel in the scanning phase */
#ifdef TSCH_CONF_CHANNEL_SCAN_DURATION
#define TSCH_CHANNEL_SCAN_DURATION TSCH_CONF_CHANNEL_SCAN_DURATION
#else
#define TSCH_CHANNEL_SCAN_DURATION CLOCK_SECOND
#endif

/* TSCH EB: include timeslot timing Information Element? */
#ifdef TSCH_PACKET_CONF_EB_WITH_TIMESLOT_TIMING
#define TSCH_PACKET_EB_WITH_TIMESLOT_TIMING TSCH_PACKET_CONF_EB_WITH_TIMESLOT_TIMING
#else
#define TSCH_PACKET_EB_WITH_TIMESLOT_TIMING 0
#endif

/* TSCH EB: include hopping sequence Information Element? */
#ifdef TSCH_PACKET_CONF_EB_WITH_HOPPING_SEQUENCE
#define TSCH_PACKET_EB_WITH_HOPPING_SEQUENCE TSCH_PACKET_CONF_EB_WITH_HOPPING_SEQUENCE
#else
#define TSCH_PACKET_EB_WITH_HOPPING_SEQUENCE 0
#endif

/* TSCH EB: include slotframe and link Information Element? */
#ifdef TSCH_PACKET_CONF_EB_WITH_SLOTFRAME_AND_LINK
#define TSCH_PACKET_EB_WITH_SLOTFRAME_AND_LINK TSCH_PACKET_CONF_EB_WITH_SLOTFRAME_AND_LINK
#else
#define TSCH_PACKET_EB_WITH_SLOTFRAME_AND_LINK 0
#endif

/******** Configuration: queues  *******/

/* Size of the ring buffer storing dequeued outgoing packets (only an array of pointers).
 * Must be power of two, and greater or equal to QUEUEBUF_NUM */
#ifdef TSCH_CONF_DEQUEUED_ARRAY_SIZE
#define TSCH_DEQUEUED_ARRAY_SIZE TSCH_CONF_DEQUEUED_ARRAY_SIZE
#else
/* By default, round QUEUEBUF_CONF_NUM to next power of two
 * (in the range [4;256]) */
#if QUEUEBUF_CONF_NUM <= 4
#define TSCH_DEQUEUED_ARRAY_SIZE 4
#elif QUEUEBUF_CONF_NUM <= 8
#define TSCH_DEQUEUED_ARRAY_SIZE 8
#elif QUEUEBUF_CONF_NUM <= 16
#define TSCH_DEQUEUED_ARRAY_SIZE 16
#elif QUEUEBUF_CONF_NUM <= 32
#define TSCH_DEQUEUED_ARRAY_SIZE 32
#elif QUEUEBUF_CONF_NUM <= 64
#define TSCH_DEQUEUED_ARRAY_SIZE 64
#elif QUEUEBUF_CONF_NUM <= 128
#define TSCH_DEQUEUED_ARRAY_SIZE 128
#else
#define TSCH_DEQUEUED_ARRAY_SIZE 256
#endif
#endif

/* Size of the ring buffer storing incoming packets.
 * Must be power of two */
#ifdef TSCH_CONF_MAX_INCOMING_PACKETS
#define TSCH_MAX_INCOMING_PACKETS TSCH_CONF_MAX_INCOMING_PACKETS
#else
#define TSCH_MAX_INCOMING_PACKETS 4
#endif

/* The maximum number of outgoing packets towards each neighbor
 * Must be power of two to enable atomic ringbuf operations.
 * Note: the total number of outgoing packets in the system (for
 * all neighbors) is defined via QUEUEBUF_CONF_NUM */
#ifdef TSCH_QUEUE_CONF_NUM_PER_NEIGHBOR
#define TSCH_QUEUE_NUM_PER_NEIGHBOR TSCH_QUEUE_CONF_NUM_PER_NEIGHBOR
#else
/* By default, round QUEUEBUF_CONF_NUM to next power of two
 * (in the range [4;256]) */
#if QUEUEBUF_CONF_NUM <= 4
#define TSCH_QUEUE_NUM_PER_NEIGHBOR 4
#elif QUEUEBUF_CONF_NUM <= 8
#define TSCH_QUEUE_NUM_PER_NEIGHBOR 8
#elif QUEUEBUF_CONF_NUM <= 16
#define TSCH_QUEUE_NUM_PER_NEIGHBOR 16
#elif QUEUEBUF_CONF_NUM <= 32
#define TSCH_QUEUE_NUM_PER_NEIGHBOR 32
#elif QUEUEBUF_CONF_NUM <= 64
#define TSCH_QUEUE_NUM_PER_NEIGHBOR 64
#elif QUEUEBUF_CONF_NUM <= 128
#define TSCH_QUEUE_NUM_PER_NEIGHBOR 128
#else
#define TSCH_QUEUE_NUM_PER_NEIGHBOR 256
#endif
#endif

/* The number of neighbor queues. There are two queues allocated at all times:
 * one for EBs, one for broadcasts. Other queues are for unicast to neighbors */
#ifdef TSCH_QUEUE_CONF_MAX_NEIGHBOR_QUEUES
#define TSCH_QUEUE_MAX_NEIGHBOR_QUEUES TSCH_QUEUE_CONF_MAX_NEIGHBOR_QUEUES
#else
#define TSCH_QUEUE_MAX_NEIGHBOR_QUEUES ((NBR_TABLE_CONF_MAX_NEIGHBORS) + 2)
#endif

/******** Configuration: scheduling  *******/

/* Initializes TSCH with a 6TiSCH minimal schedule */
#ifdef TSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL
#define TSCH_SCHEDULE_WITH_6TISCH_MINIMAL TSCH_SCHEDULE_CONF_WITH_6TISCH_MINIMAL
#else
#define TSCH_SCHEDULE_WITH_6TISCH_MINIMAL (!(BUILD_WITH_ORCHESTRA))
#endif

/* Set an upper bound on burst length. Set to 0 to never set the frame pending
 * bit, i.e., never trigger a burst. Note that receiver-side support for burst
 * is always enabled, as it is part of IEEE 802.1.5.4-2015 (Section 7.2.1.3)*/
#ifdef TSCH_CONF_BURST_MAX_LEN
#define TSCH_BURST_MAX_LEN TSCH_CONF_BURST_MAX_LEN
#else
#define TSCH_BURST_MAX_LEN 32
#endif

/* 6TiSCH Minimal schedule slotframe length */
#ifdef TSCH_SCHEDULE_CONF_DEFAULT_LENGTH
#define TSCH_SCHEDULE_DEFAULT_LENGTH TSCH_SCHEDULE_CONF_DEFAULT_LENGTH
#else
#define TSCH_SCHEDULE_DEFAULT_LENGTH 7
#endif

/* Max number of TSCH slotframes */
#ifdef TSCH_SCHEDULE_CONF_MAX_SLOTFRAMES
#define TSCH_SCHEDULE_MAX_SLOTFRAMES TSCH_SCHEDULE_CONF_MAX_SLOTFRAMES
#else
#define TSCH_SCHEDULE_MAX_SLOTFRAMES 4
#endif

/* Max number of links */
#ifdef TSCH_SCHEDULE_CONF_MAX_LINKS
#define TSCH_SCHEDULE_MAX_LINKS TSCH_SCHEDULE_CONF_MAX_LINKS
#else
#define TSCH_SCHEDULE_MAX_LINKS 32
#endif

/* To include Sixtop Implementation */
#ifdef TSCH_CONF_WITH_SIXTOP
#define TSCH_WITH_SIXTOP TSCH_CONF_WITH_SIXTOP
#else
#define TSCH_WITH_SIXTOP 0
#endif

/* A custom feature allowing upper layers to assign packets to
 * a specific slotframe and link */
#ifdef TSCH_CONF_WITH_LINK_SELECTOR
#define TSCH_WITH_LINK_SELECTOR TSCH_CONF_WITH_LINK_SELECTOR
#else /* TSCH_CONF_WITH_LINK_SELECTOR */
#define TSCH_WITH_LINK_SELECTOR (BUILD_WITH_ORCHESTRA)
#endif /* TSCH_CONF_WITH_LINK_SELECTOR */

/******** Configuration: CSMA *******/

/* TSCH CSMA-CA parameters, see IEEE 802.15.4e-2012 */
/* Min backoff exponent */
#ifdef TSCH_CONF_MAC_MIN_BE
#define TSCH_MAC_MIN_BE TSCH_CONF_MAC_MIN_BE
#else
#define TSCH_MAC_MIN_BE 1
#endif
/* Max backoff exponent */
#ifdef TSCH_CONF_MAC_MAX_BE
#define TSCH_MAC_MAX_BE TSCH_CONF_MAC_MAX_BE
#else
#define TSCH_MAC_MAX_BE 5
#endif
/* Max number of re-transmissions */
#ifdef TSCH_CONF_MAC_MAX_FRAME_RETRIES
#define TSCH_MAC_MAX_FRAME_RETRIES TSCH_CONF_MAC_MAX_FRAME_RETRIES
#else
#define TSCH_MAC_MAX_FRAME_RETRIES 7
#endif

/* Include source address in ACK? */
#ifdef TSCH_PACKET_CONF_EACK_WITH_SRC_ADDR
#define TSCH_PACKET_EACK_WITH_SRC_ADDR TSCH_PACKET_CONF_EACK_WITH_SRC_ADDR
#else
#define TSCH_PACKET_EACK_WITH_SRC_ADDR 0
#endif

/* Include destination address in ACK? */
#ifdef TSCH_PACKET_CONF_EACK_WITH_DEST_ADDR
#define TSCH_PACKET_EACK_WITH_DEST_ADDR TSCH_PACKET_CONF_EACK_WITH_DEST_ADDR
#else
#define TSCH_PACKET_EACK_WITH_DEST_ADDR 1 /* Include destination address
by default, useful in case of duplicate seqno */
#endif

/******** Configuration: hardware-specific settings *******/

/* HW frame filtering enabled */
#ifdef TSCH_CONF_HW_FRAME_FILTERING
#define TSCH_HW_FRAME_FILTERING TSCH_CONF_HW_FRAME_FILTERING
#else /* TSCH_CONF_HW_FRAME_FILTERING */
#define TSCH_HW_FRAME_FILTERING 1
#endif /* TSCH_CONF_HW_FRAME_FILTERING */

/* Keep radio always on within TSCH timeslot (1) or turn it off between packet and ACK? (0) */
#ifdef TSCH_CONF_RADIO_ON_DURING_TIMESLOT
#define TSCH_RADIO_ON_DURING_TIMESLOT TSCH_CONF_RADIO_ON_DURING_TIMESLOT
#else
#define TSCH_RADIO_ON_DURING_TIMESLOT 0
#endif



/* Timeslot timing */
#ifndef TSCH_CONF_DEFAULT_TIMESLOT_LENGTH
#define TSCH_CONF_DEFAULT_TIMESLOT_LENGTH 10000
#endif /* TSCH_CONF_DEFAULT_TIMESLOT_LENGTH */

/* Configurable Rx guard time is micro-seconds */
#ifndef TSCH_CONF_RX_WAIT
#define TSCH_CONF_RX_WAIT 2200
#endif /* TSCH_CONF_RX_WAIT */

#endif /* __TSCH_CONF_H__ */
/** @} */
