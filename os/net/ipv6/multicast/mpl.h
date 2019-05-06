/*
 * Copyright (c) 2018, University of Bristol - http://www.bristol.ac.uk/
 * All rights reserved
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
 */

/**
 * \addtogroup uip-multicast
 * @{
 */
/**
 * \defgroup mpl Multicast Protocol for Low Power and Lossy Networks
 *
 * IPv6 multicast according to the algorithm in RFC7731
 *
 * The current version of the specification can be found in
 * https://tools.ietf.org/html/rfc7731
 *
 * @{
 */
/**
 * \file
 *    Header file for the implementation of the MPL protocol
 * \author
 *    Ed Rose - <er15406@bris.ac.uk>
 */

#ifndef MPL_H
#define MPL_H
#include "contiki.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/multicast/uip-mcast6-stats.h"

/*---------------------------------------------------------------------------*/
/* Protocol Constants */
/*---------------------------------------------------------------------------*/
#define ALL_MPL_FORWARDERS(a, r)   uip_ip6addr(a, 0xFF00 + r,0x00,0x00,0x00,0x00,0x00,0x00,0xFC)
#define HBHO_OPT_TYPE_MPL          0x6D
#define MPL_IP_HOP_LIMIT           0xFF   /**< Hop limit for ICMP messages */
#define HBHO_BASE_LEN              8
#define HBHO_S0_LEN                0
#define HBHO_S1_LEN                0
#define HBHO_S2_LEN                8
#define HBHO_S3_LEN                16
#define MPL_OPT_LEN_S0             2
#define MPL_OPT_LEN_S1             4
#define MPL_OPT_LEN_S2             10
#define MPL_OPT_LEN_S3             18
#define MPL_DGRAM_OUT              0
#define MPL_DGRAM_IN               1

/* Trickle timer configuration */
#ifndef MPL_CONF_DATA_MESSAGE_IMIN
#define MPL_DATA_MESSAGE_IMIN               32
#else
#define MPL_DATA_MESSAGE_IMIN MPL_CONF_DATA_MESSAGE_IMIN
#endif

#ifndef MPL_CONF_DATA_MESSAGE_IMAX
#define MPL_DATA_MESSAGE_IMAX               MPL_CONTROL_MESSAGE_IMIN
#else
#define MPL_DATA_MESSAGE_IMAX MPL_CONF_DATA_MESSAGE_IMAX
#endif

#ifndef MPL_CONF_DATA_MESSAGE_K
#define MPL_DATA_MESSAGE_K                  1
#else
#define MPL_CONF_DATA_MESSAGE_K MPL_DATA_MESSAGE_K
#endif

#ifndef MPL_CONF_CONTROL_MESSAGE_IMIN
#define MPL_CONTROL_MESSAGE_IMIN            32
#else
#define MPL_CONTROL_MESSAGE_IMIN MPL_CONF_CONTROL_MESSAGE_IMIN
#endif

#ifndef MPL_CONF_CONTROL_MESSAGE_IMAX
#define MPL_CONTROL_MESSAGE_IMAX            32
#else
#define MPL_CONTROL_MESSAGE_IMAX MPL_CONF_CONTROL_MESSAGE_IMAX
#endif

#ifndef MPL_CONF_CONTROL_MESSAGE_K
#define MPL_CONTROL_MESSAGE_K               1
#else
#define MPL_CONTROL_MESSAGE_K MPL_CONF_CONTROL_MESSAGE_K
#endif

/*---------------------------------------------------------------------------*/
/* Protocol Configuration */
/*---------------------------------------------------------------------------*/
/**
 * Seed ID Length
 * The MPL Protocol requires that each seed is identified by an ID that is
 * unique to the MPL domain. The Seed ID can be either a 16 bit, 64 bit,
 * or 128 bit unsigned integer. It it's an 128 bit unsigned integer then the
 * IPv6 address may be used. The format of the Seed ID is set in the
 * three-bit 'S' field in MPL header options, and the value below mirrors this.
 * 0 - The seed id will be the IPv6 address of this device. The values of
 *      MPL_CONF_SEED_ID_L and MPL_CONF_SEED_ID_H are ignored.
 * 1 - The seed id will be a 16 bit unsigned integer defined
 *      in MPL_CONF_SEED_ID_L
 * 2 - The seed id will be a 64 bit unsigned integer defined in
 *      in MPL_CONF_SEED_ID_L
 * 3 - The seed id will be an 128 bit unsigned integer defined in both
 *      MPL_CONF_SEED_ID_L and MPL_CONF_SEED_ID_H
 */
#ifndef MPL_CONF_SEED_ID_TYPE
#define MPL_SEED_ID_TYPE                    0
#else
#define MPL_SEED_ID_TYPE MPL_CONF_SEED_ID_TYPE
#endif
/*---------------------------------------------------------------------------*/
/**
 * Seed ID Alias
 * Points to MPL_CONF_SEED_ID_L
 */
#ifdef MPL_CONF_SEED_ID
#define MPL_CONF_SEED_ID_L MPL_CONF_SEED_ID
#endif
/*---------------------------------------------------------------------------*/
/**
* Seed ID Low Bits
 * If the Seed ID Length setting is 1 or 2, this setting defines the seed
 * id for this seed. If the seed id setting is 3, then this defines the lower
 * 64 bits of the seed id.
 */
#ifndef MPL_CONF_SEED_ID_L
#define MPL_SEED_ID_L                       0x00
#else
#define MPL_SEED_ID_L MPL_CONF_SEED_ID_L
#endif
/*---------------------------------------------------------------------------*/
/**
* Seed ID High Bits
 * If the Seed ID Length setting is 3, this setting defines the upper 64 bits
 * for the seed id. Else it's ignored.
 */
#ifndef MPL_CONF_SEED_ID_H
#define MPL_SEED_ID_H                       0x00
#else
#define MPL_SEED_ID_H MPL_CONF_SEED_ID_H
#endif
/*---------------------------------------------------------------------------*/
/**
 * Subscribe to All MPL Forwarders
 *  By default, an MPL forwarder will subscribe to the ALL_MPL_FORWARDERS
 *  address with realm-local scope: FF03::FC. This behaviour can be disabled
 *  using the macro below.
 */
#ifndef MPL_CONF_SUB_TO_ALL_FORWARDERS
#define MPL_SUB_TO_ALL_FORWARDERS           1
#else
#define MPL_SUB_TO_ALL_FORWARDERS MPL_CONF_SUB_TO_ALL_FORWARDERS
#endif
/*---------------------------------------------------------------------------*/
/**
 * Domain Set Size
 * MPL Forwarders maintain a Domain Set which maps MPL domains to trickle
 * timers. The size of this should reflect the number of MPL domains this
 * forwarder is participating in.
 */
#ifndef MPL_CONF_DOMAIN_SET_SIZE
#define MPL_DOMAIN_SET_SIZE                 1
#else
#define MPL_DOMAIN_SET_SIZE MPL_CONF_DOMAIN_SET_SIZE
#endif
/*---------------------------------------------------------------------------*/
/**
 * Seed Set Size
 * MPL Forwarders maintain a Seed Set to keep track of the MPL messages that a
 * particular seed has sent recently. Seeds remain on the Seed Set for the
 * time specified in the Seed Set Entry Lifetime setting.
 */
#ifndef MPL_CONF_SEED_SET_SIZE
#define MPL_SEED_SET_SIZE                   2
#else
#define MPL_SEED_SET_SIZE MPL_CONF_SEED_SET_SIZE
#endif
/*---------------------------------------------------------------------------*/
/**
 * Buffered Message Set Size
 * MPL Forwarders maintain a buffer of data messages that are periodically
 * forwarded around the MPL domain. These are forwarded when trickle timers
 * expire, and remain in the buffer for the number of timer expirations
 * set in Data Message Timer Expirations.
 */
#ifndef MPL_CONF_BUFFERED_MESSAGE_SET_SIZE
#define MPL_BUFFERED_MESSAGE_SET_SIZE       6
#else
#define MPL_BUFFERED_MESSAGE_SET_SIZE MPL_CONF_BUFFERED_MESSAGE_SET_SIZE
#endif
/*---------------------------------------------------------------------------*/
/**
 * MPL Forwarding Strategy
 * Two forwarding strategies are defined for MPL. With Proactive forwarding
 * enabled, the forwarder will schedule transmissions of new messages
 * before any control messages are received to indicate that neighbouring nodes
 * have yet to receive the messages. With Reactive forwarding enabled, MPL
 * forwarders will only schedule transmissions of new MPL messages once control
 * messages have been received indicating that they are missing those messages.
 * 1 - Indicates that proactive forwarding be enabled
 * 0 - Indicates that proactive forwarding be disabled
 */
#ifndef MPL_CONF_PROACTIVE_FORWARDING
#define MPL_PROACTIVE_FORWARDING            0
#else
#define MPL_PROACTIVE_FORWARDING MPL_CONF_PROACTIVE_FORWARDING
#endif
/*---------------------------------------------------------------------------*/
/**
 * Seed Set Entry Lifetime
 * MPL Seed set entries remain in the seed set for a set period of time after
 * the last message that was received by that seed. This is called the
 * lifetime of the seed, and is defined in minutes.
 */
#ifndef MPL_CONF_SEED_SET_ENTRY_LIFETIME
#define MPL_SEED_SET_ENTRY_LIFETIME         30
#else
#define MPL_SEED_SET_ENTRY_LIFETIME MPL_CONF_SEED_SET_ENTRY_LIFETIME
#endif
/*---------------------------------------------------------------------------*/
/**
 * Data Message Timer Expirations
 * MPL data message trickle timers are stopped after they expire a set number
 * of times. The number of times they expire before being stopped is set below.
 */
#ifndef MPL_CONF_DATA_MESSAGE_TIMER_EXPIRATIONS
#define MPL_DATA_MESSAGE_TIMER_EXPIRATIONS  5
#else
#define MPL_DATA_MESSAGE_TIMER_EXPIRATIONS MPL_CONF_DATA_MESSAGE_TIMER_EXPIRATIONS
#endif
/*---------------------------------------------------------------------------*/
/**
 * Control Message Timer Expirations
 * An MPL Forwarder forwards MPL messages for a particular domain using a
 * trickle timer which is mapped using the Domain Set. MPL domains remain in
 * the Domain Set for the number of timer expirations below. New messages on
 * that domain cause the expirations counter to reset.
 */
#ifndef MPL_CONF_CONTROL_MESSAGE_TIMER_EXPIRATIONS
#define MPL_CONTROL_MESSAGE_TIMER_EXPIRATIONS 10
#else
#define MPL_CONTROL_MESSAGE_TIMER_EXPIRATIONS MPL_CONF_CONTROL_MESSAGE_TIMER_EXPIRATIONS
#endif
/*---------------------------------------------------------------------------*/
/* Misc System Config */
/*---------------------------------------------------------------------------*/

/* Configure the correct number of multicast addresses for MPL */
#define UIP_CONF_DS6_MADDR_NBU MPL_DOMAIN_SET_SIZE * 2

/*---------------------------------------------------------------------------*/
/* Stats datatype */
/*---------------------------------------------------------------------------*/
/**
 * \brief Multicast stats extension for the MPL engine
 */
struct mpl_stats {
  /** Number of received ICMP datagrams */
  UIP_MCAST6_STATS_DATATYPE icmp_in;

  /** Number of ICMP datagrams sent */
  UIP_MCAST6_STATS_DATATYPE icmp_out;

  /** Number of malformed ICMP datagrams seen by us */
  UIP_MCAST6_STATS_DATATYPE icmp_bad;
};
#endif
/*---------------------------------------------------------------------------*/
/** @} */
/** @} */