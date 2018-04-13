/*
 * Copyright (c) 2014, Swedish Institute of Computer Science.
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
 */

#ifndef __COMMON_CONF_H__
#define __COMMON_CONF_H__

/* Global config flags */

#define WITH_TSCH_SECURITY 0
#define WITH_COAP_RESOURCES 0

#define ENABLE_COOJA_DEBUG 0

#define IEEE802154_CONF_PANID 0x5254

#define TSCH_HOPPING_SEQUENCE_MY_SEQUENCE (uint8_t[]){17, 23, 15, 25, 19, 11, 13, 21}
#define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE TSCH_HOPPING_SEQUENCE_MY_SEQUENCE

#define TSCH_CONF_JOIN_MY_PANID_ONLY 1

#define TSCH_CONF_AUTOSTART 0

/* RPL Trickle timer tuning */
#define RPL_CONF_DIO_INTERVAL_MIN 12 /* 4.096 s */

#define RPL_CONF_DIO_INTERVAL_DOUBLINGS 2 /* Max factor: x4. 4.096 s * 4 = 16.384 s */

#define TSCH_CONF_EB_PERIOD (4 * CLOCK_SECOND)
#define TSCH_CONF_KEEPALIVE_TIMEOUT (24 * CLOCK_SECOND)

/* Dimensioning */
#define ORCHESTRA_CONF_EBSF_PERIOD                     41
#define ORCHESTRA_CONF_COMMON_SHARED_PERIOD             7 /* Common shared slot, 7 is a very short slotframe (high energy, high capacity). Must be prime and at least equal to number of nodes (incl. BR) */
#define ORCHESTRA_CONF_UNICAST_PERIOD                  11 /* First prime greater than 10 */

/* Use sender-based slots */
#define ORCHESTRA_CONF_UNICAST_SENDER_BASED 1
/* Our "hash" is collision-free */
#define ORCHESTRA_CONF_COLLISION_FREE_HASH 1
/* Max hash value */
#define ORCHESTRA_CONF_MAX_HASH (ORCHESTRA_CONF_UNICAST_PERIOD - 1)

/* CoAP */

#define COAP_SERVER_PORT 5684

#define COAP_OBSERVE_RETURNS_REPRESENTATION 1

/* RPL */
#define UIP_CONF_ROUTER                 1

/* RPL storing mode */
#define RPL_CONF_MOP RPL_MOP_STORING_NO_MULTICAST

/* Default link metric */
#define RPL_CONF_INIT_LINK_METRIC 2 /* default 5 */

#define RPL_CONF_MAX_INSTANCES    1 /* default 1 */
#define RPL_CONF_MAX_DAG_PER_INSTANCE 1 /* default 2 */

/* No RA, No NS */
#define UIP_CONF_TCP             0
#define UIP_CONF_DS6_ADDR_NBU    1
#define UIP_CONF_UDP_CHECKSUMS   1

/* Link-layer security */

#if WITH_TSCH_SECURITY
/* Set security level to the maximum, even if unused, to all crypto code */
#define LLSEC802154_CONF_ENABLED 1
/* Attempt to associate from both secured and non-secured EBs */
#define TSCH_CONF_JOIN_SECURED_ONLY 0
#endif /* WITH_TSCH_SECURITY */

#if MAC_CONF_WITH_CSMA /* Configure Csma with ACK (default MAC) */

#define MICROMAC_CONF_AUTOACK 1

/* increase internal radio buffering */
#define MIRCOMAC_CONF_BUF_NUM 4

#endif /* MAC_CONF_WITH_CSMA */

#include "common-conf-jn516x.h"

#endif /* __COMMON_CONF_H__ */
