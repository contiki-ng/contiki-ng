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
* \ingroup link-layer
* \defgroup tsch 802.15.4 TSCH
The IEEE 802.15.4-2015 TimeSlotted Channel Hopping (TSCH) protocol. Provides
scheduled communication on top of a globally-synchronized network. Performs
frequency hopping for enhanced reliability.
* @{
* \file
*	Main API declarations for TSCH.
*/

#ifndef __TSCH_H__
#define __TSCH_H__

/********** Includes **********/

#include "contiki.h"
#include "net/mac/mac.h"
#include "net/linkaddr.h"

#include "net/mac/tsch/tsch-conf.h"
#include "net/mac/tsch/tsch-const.h"
#include "net/mac/tsch/tsch-types.h"
#include "net/mac/tsch/tsch-adaptive-timesync.h"
#include "net/mac/tsch/tsch-slot-operation.h"
#include "net/mac/tsch/tsch-queue.h"
#include "net/mac/tsch/tsch-log.h"
#include "net/mac/tsch/tsch-packet.h"
#include "net/mac/tsch/tsch-security.h"
#include "net/mac/tsch/tsch-schedule.h"
#if UIP_CONF_IPV6_RPL
#include "net/mac/tsch/tsch-rpl.h"
#endif /* UIP_CONF_IPV6_RPL */

#if CONTIKI_TARGET_COOJA
#include "lib/simEnvChange.h"
#include "sys/cooja_mt.h"
#endif /* CONTIKI_TARGET_COOJA */

/*********** Macros *********/

/* Wait for a condition with timeout t0+offset. */
#if CONTIKI_TARGET_COOJA
#define BUSYWAIT_UNTIL_ABS(cond, t0, offset) \
  while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), (t0) + (offset))) { \
    simProcessRunValue = 1; \
    cooja_mt_yield(); \
  };
#else
#define BUSYWAIT_UNTIL_ABS(cond, t0, offset) \
  while(!(cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), (t0) + (offset))) ;
#endif /* CONTIKI_TARGET_COOJA */

/*********** Callbacks *********/

/* Link callbacks to RPL in case RPL is enabled */
#if UIP_CONF_IPV6_RPL

#ifndef TSCH_CALLBACK_JOINING_NETWORK
#define TSCH_CALLBACK_JOINING_NETWORK tsch_rpl_callback_joining_network
#endif /* TSCH_CALLBACK_JOINING_NETWORK */

#ifndef TSCH_CALLBACK_LEAVING_NETWORK
#define TSCH_CALLBACK_LEAVING_NETWORK tsch_rpl_callback_leaving_network
#endif /* TSCH_CALLBACK_LEAVING_NETWORK */

#ifndef TSCH_CALLBACK_KA_SENT
#define TSCH_CALLBACK_KA_SENT tsch_rpl_callback_ka_sent
#endif /* TSCH_CALLBACK_KA_SENT */

#endif /* UIP_CONF_IPV6_RPL */

#if BUILD_WITH_ORCHESTRA

#ifndef TSCH_CALLBACK_NEW_TIME_SOURCE
#define TSCH_CALLBACK_NEW_TIME_SOURCE orchestra_callback_new_time_source
#endif /* TSCH_CALLBACK_NEW_TIME_SOURCE */

#ifndef TSCH_CALLBACK_PACKET_READY
#define TSCH_CALLBACK_PACKET_READY orchestra_callback_packet_ready
#endif /* TSCH_CALLBACK_PACKET_READY */

#endif /* BUILD_WITH_ORCHESTRA */

/* Called by TSCH when joining a network */
#ifdef TSCH_CALLBACK_JOINING_NETWORK
void TSCH_CALLBACK_JOINING_NETWORK();
#endif

/* Called by TSCH when leaving a network */
#ifdef TSCH_CALLBACK_LEAVING_NETWORK
void TSCH_CALLBACK_LEAVING_NETWORK();
#endif

/* Called by TSCH after sending a keep-alive */
#ifdef TSCH_CALLBACK_KA_SENT
void TSCH_CALLBACK_KA_SENT();
#endif

/* Called by TSCH form interrupt after receiving a frame, enabled upper-layer to decide
 * whether to ACK or NACK */
#ifdef TSCH_CALLBACK_DO_NACK
int TSCH_CALLBACK_DO_NACK(struct tsch_link *link, linkaddr_t *src, linkaddr_t *dst);
#endif

/* Called by TSCH when switching time source */
#ifdef TSCH_CALLBACK_NEW_TIME_SOURCE
struct tsch_neighbor;
void TSCH_CALLBACK_NEW_TIME_SOURCE(const struct tsch_neighbor *old, const struct tsch_neighbor *new);
#endif

/* Called by TSCH every time a packet is ready to be added to the send queue */
#ifdef TSCH_CALLBACK_PACKET_READY
void TSCH_CALLBACK_PACKET_READY(void);
#endif

/***** External Variables *****/

/* Are we coordinator of the TSCH network? */
extern int tsch_is_coordinator;
/* Are we associated to a TSCH network? */
extern int tsch_is_associated;
/* Is the PAN running link-layer security? */
extern int tsch_is_pan_secured;
/* The TSCH MAC driver */
extern const struct mac_driver tschmac_driver;
/* 802.15.4 broadcast MAC address */
extern const linkaddr_t tsch_broadcast_address;
/* The address we use to identify EB queue */
extern const linkaddr_t tsch_eb_address;
/* The current Absolute Slot Number (ASN) */
extern struct tsch_asn_t tsch_current_asn;
extern uint8_t tsch_join_priority;
extern struct tsch_link *current_link;
/* If we are inside a slot, this tells the current channel */
extern uint8_t tsch_current_channel;
/* TSCH channel hopping sequence */
extern uint8_t tsch_hopping_sequence[TSCH_HOPPING_SEQUENCE_MAX_LEN];
extern struct tsch_asn_divisor_t tsch_hopping_sequence_length;
/* TSCH timeslot timing (in rtimer ticks) */
extern rtimer_clock_t tsch_timing[tsch_ts_elements_count];
/* Statistics on the current session */
extern unsigned long tx_count;
extern unsigned long rx_count;
extern unsigned long sync_count;
extern int32_t min_drift_seen;
extern int32_t max_drift_seen;

/* TSCH processes */
PROCESS_NAME(tsch_process);
PROCESS_NAME(tsch_send_eb_process);
PROCESS_NAME(tsch_pending_events_process);


/********** Functions *********/

/**
 * Set the TSCH join priority (JP)
 *
 * \param jp the new join priority
 */
void tsch_set_join_priority(uint8_t jp);
/**
 * Set the period at wich TSCH enhanced beacons (EBs) are sent. The period can
 * not be set to exceed TSCH_MAX_EB_PERIOD. Set to 0 to stop sending EBs.
 * Actual transmissions are jittered, spaced by a random number within
 * [period*0.75, period[
 *
 * \param period The period in Clock ticks.
 */
void tsch_set_eb_period(uint32_t period);
/**
 * Set the desynchronization timeout after which a node sends a unicasst
 * keep-alive (KA) to its time source. Set to 0 to stop sending KAs. The
 * actual timeout is a random number within
 * [timeout*0.9, timeout[
 *
 * \param timeout The timeout in Clock ticks.
 */
void tsch_set_ka_timeout(uint32_t timeout);
/**
 * Set the node as PAN coordinator
 *
 * \param enable 1 to be coordinator, 0 to be a node
 */
void tsch_set_coordinator(int enable);
/**
 * Enable/disable security. If done at the coordinator, the Information
 * will be included in EBs, and all nodes will adopt the same security level.
 * Enabling requires compilation with LLSEC802154_ENABLED set.
 * Note: when LLSEC802154_ENABLED is set, nodes boot with security enabled.
 *
 * \param enable 1 to enable security, 0 to disable it
 */
void tsch_set_pan_secured(int enable);
/**
  * Schedule a keep-alive transmission within [timeout*0.9, timeout[
  * @see tsch_set_ka_timeout
  */
void tsch_schedule_keepalive(void);
/**
  * Schedule a keep-alive immediately
  */
void tsch_schedule_keepalive_immediately(void);
/**
  * Leave the TSCH network we are currently in
  */
void tsch_disassociate(void);

#endif /* __TSCH_H__ */
/** @} */
