/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         Orchestra configuration
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#ifndef __ORCHESTRA_CONF_H__
#define __ORCHESTRA_CONF_H__

#ifdef ORCHESTRA_CONF_RULES
#define ORCHESTRA_RULES ORCHESTRA_CONF_RULES
#else /* ORCHESTRA_CONF_RULES */
/* A default configuration with:
 * - a sender-based slotframe for EB transmission
 * - a sender-based or receiver-based slotframe for unicast to RPL parents and children
 * - a common shared slotframe for any other traffic (mostly broadcast)
 *  */
#define ORCHESTRA_RULES { &eb_per_time_source, \
                          &unicast_per_neighbor_rpl_ns, \
                          &default_common }
/* Example configuration for RPL storing mode: */
/* #define ORCHESTRA_RULES { &eb_per_time_source, \
                             &unicast_per_neighbor_rpl_storing, \
                             &default_common } */

#endif /* ORCHESTRA_CONF_RULES */

/* Length of the various slotframes. Tune to balance network capacity,
 * contention, energy, latency. */
#ifdef ORCHESTRA_CONF_EBSF_PERIOD
#define ORCHESTRA_EBSF_PERIOD                     ORCHESTRA_CONF_EBSF_PERIOD
#else /* ORCHESTRA_CONF_EBSF_PERIOD */
#define ORCHESTRA_EBSF_PERIOD                     397
#endif /* ORCHESTRA_CONF_EBSF_PERIOD */

#ifdef ORCHESTRA_CONF_COMMON_SHARED_PERIOD
#define ORCHESTRA_COMMON_SHARED_PERIOD            ORCHESTRA_CONF_COMMON_SHARED_PERIOD
#else /* ORCHESTRA_CONF_COMMON_SHARED_PERIOD */
#define ORCHESTRA_COMMON_SHARED_PERIOD            31
#endif /* ORCHESTRA_CONF_COMMON_SHARED_PERIOD */

#ifdef ORCHESTRA_CONF_UNICAST_PERIOD
#define ORCHESTRA_UNICAST_PERIOD                  ORCHESTRA_CONF_UNICAST_PERIOD
#else /* ORCHESTRA_CONF_UNICAST_PERIOD */
#define ORCHESTRA_UNICAST_PERIOD                  17
#endif /* ORCHESTRA_CONF_UNICAST_PERIOD */

/* Is the per-neighbor unicast slotframe sender-based (if not, it is receiver-based).
 * Note: sender-based works only with RPL storing mode as it relies on DAO and
 * routing entries to keep track of children and parents. */
#ifdef ORCHESTRA_CONF_UNICAST_SENDER_BASED
#define ORCHESTRA_UNICAST_SENDER_BASED            ORCHESTRA_CONF_UNICAST_SENDER_BASED
#else /* ORCHESTRA_CONF_UNICAST_SENDER_BASED */
#define ORCHESTRA_UNICAST_SENDER_BASED            0
#endif /* ORCHESTRA_CONF_UNICAST_SENDER_BASED */

/* The hash function used to assign timeslot to a given node (based on its link-layer address).
 * For rules with multiple channel offsets, it is also used to select the channel offset. */
#ifdef ORCHESTRA_CONF_LINKADDR_HASH
#define ORCHESTRA_LINKADDR_HASH                   ORCHESTRA_CONF_LINKADDR_HASH
#else /* ORCHESTRA_CONF_LINKADDR_HASH */
#define ORCHESTRA_LINKADDR_HASH(addr)             ((addr != NULL) ? (addr)->u8[LINKADDR_SIZE - 1] : -1)
#endif /* ORCHESTRA_CONF_LINKADDR_HASH */

/* The hash function used to assign timeslot for a pair of given nodes. */
#ifdef ORCHESTRA_CONF_LINKADDR_HASH2
#define ORCHESTRA_LINKADDR_HASH2                  ORCHESTRA_CONF_LINKADDR_HASH2
#else /* ORCHESTRA_CONF_LINKADDR_HASH2 */
#define ORCHESTRA_LINKADDR_HASH2(addr1, addr2)    ((addr1)->u8[LINKADDR_SIZE - 1] + 256 * (addr2)->u8[LINKADDR_SIZE - 1])
#endif /* ORCHESTRA_CONF_LINKADDR_HASH2 */

/* The maximum hash */
#ifdef ORCHESTRA_CONF_MAX_HASH
#define ORCHESTRA_MAX_HASH                        ORCHESTRA_CONF_MAX_HASH
#else /* ORCHESTRA_CONF_MAX_HASH */
#define ORCHESTRA_MAX_HASH                        0x7fff
#endif /* ORCHESTRA_CONF_MAX_HASH */

/* Is the "hash" function collision-free? (e.g. it maps to unique node-ids) */
#ifdef ORCHESTRA_CONF_COLLISION_FREE_HASH
#define ORCHESTRA_COLLISION_FREE_HASH             ORCHESTRA_CONF_COLLISION_FREE_HASH
#else /* ORCHESTRA_CONF_COLLISION_FREE_HASH */
#define ORCHESTRA_COLLISION_FREE_HASH             0 /* Set to 1 if ORCHESTRA_LINKADDR_HASH returns unique hashes */
#endif /* ORCHESTRA_CONF_COLLISION_FREE_HASH */

/* Channel offset for the EB rule, default 0 */
#ifdef ORCHESTRA_CONF_EB_CHANNEL_OFFSET
#define ORCHESTRA_EB_CHANNEL_OFFSET               ORCHESTRA_CONF_EB_CHANNEL_OFFSET
#else
#define ORCHESTRA_EB_CHANNEL_OFFSET               0
#endif

/* Channel offset for the default common rule, default 1 */
#ifdef ORCHESTRA_CONF_DEFAULT_COMMON_CHANNEL_OFFSET
#define ORCHESTRA_DEFAULT_COMMON_CHANNEL_OFFSET   ORCHESTRA_CONF_DEFAULT_COMMON_CHANNEL_OFFSET
#else
#define ORCHESTRA_DEFAULT_COMMON_CHANNEL_OFFSET   1
#endif

/* Min channel offset for the unicast rules; the default min/max range is [2, 255] */
#ifdef ORCHESTRA_CONF_UNICAST_MIN_CHANNEL_OFFSET
#define ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET       ORCHESTRA_CONF_UNICAST_MIN_CHANNEL_OFFSET
#else
#define ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET       2
#endif

/* Max channel offset for the unicast rules */
#ifdef ORCHESTRA_CONF_UNICAST_MAX_CHANNEL_OFFSET
#define ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET       ORCHESTRA_CONF_UNICAST_MAX_CHANNEL_OFFSET
#else
#define ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET       255
#endif

#endif /* __ORCHESTRA_CONF_H__ */
