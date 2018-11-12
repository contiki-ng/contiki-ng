/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * Copyright (c) 2017, University of Bristol - http://www.bristol.ac.uk
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
 * \author Simon Duquennoy <simonduq@sics.se>
 *         Atis Elsts <atis.elsts@bristol.ac.uk>
 */

#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

#include <stdint.h>
#include <stdbool.h>

/* Set to enable TSCH security */
#ifndef WITH_SECURITY
#define WITH_SECURITY 0
#endif /* WITH_SECURITY */

/* USB serial takes space, free more space elsewhere */
#define SICSLOWPAN_CONF_FRAG 0
#define UIP_CONF_BUFFER_SIZE 160

/*******************************************************/
/******************* Configure TSCH ********************/
/*******************************************************/

/* IEEE802.15.4 PANID */
#define IEEE802154_CONF_PANID 0x81a5

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0

/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and saves energy. */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 3

#if WITH_SECURITY

/* Enable security */
#define LLSEC802154_CONF_ENABLED 1

#endif /* WITH_SECURITY */

/* Enable TSCH statistics: must be on for channel selection to work */
#define TSCH_STATS_CONF_ON 1

/* Enable periodic RSSI sampling for TSCH statistics */
#define TSCH_STATS_CONF_SAMPLE_NOISE_RSSI 1

/* Reduce the TSCH stat "decay to normal" period to get printouts more often */
#define TSCH_STATS_CONF_DECAY_INTERVAL (60 * CLOCK_SECOND)

/* For adaptive channel selection */
extern void tsch_cs_channel_stats_updated(uint8_t updated_channel, uint16_t old_busyness_metric);
extern bool tsch_cs_process(void);
/* These will be called from the core TSCH code */
#define TSCH_CALLBACK_CHANNEL_STATS_UPDATED tsch_cs_channel_stats_updated
#define TSCH_CALLBACK_SELECT_CHANNELS tsch_cs_process

/* The coordinator will update the network nodes with new hopping sequences */
#define TSCH_PACKET_CONF_EB_WITH_HOPPING_SEQUENCE 1

/* Reduce the EB period in order to update the network nodes with more agility */
#define TSCH_CONF_EB_PERIOD     (4 * CLOCK_SECOND)
#define TSCH_CONF_MAX_EB_PERIOD (4 * CLOCK_SECOND)

/*******************************************************/
/************* Other system configuration **************/
/*******************************************************/

/* Logging */
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_INFO
#define TSCH_LOG_CONF_PER_SLOT                     1

#endif /* __PROJECT_CONF_H__ */
