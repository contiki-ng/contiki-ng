/*
 * Copyright (c) 2016, Hasso-Plattner-Institut.
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
 *         This file autoconfigures CSL if included from a project-conf.h
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/* common settings */
#define LINKADDR_CONF_SIZE 2
#define PACKETBUF_CONF_WITH_BURST_INDEX 1
#define PACKETBUF_CONF_WITH_PENDING 1
#define SICSLOWPAN_CONF_MAC_MAX_PAYLOAD 127
#define QUEUEBUF_CONF_NUM 20

#ifndef CSL_CONF_COMPLIANT
#define CSL_CONF_COMPLIANT 1
#endif /* CSL_CONF_COMPLIANT */

/* branch on whether CSL shall operate compliantly or securely */
#if CSL_CONF_COMPLIANT
#define FRAME802154_CONF_VERSION 0x02

#ifndef AKES_MAC_CONF_ENABLED
#define AKES_MAC_CONF_ENABLED 0
#endif /* AKES_MAC_CONF_ENABLED */

/* branch on whether IEEE 802.15.4 security is enabled or not */
#if AKES_MAC_CONF_ENABLED
/* configure Contiki */
#define LLSEC802154_CONF_USES_FRAME_COUNTER 1
#define LLSEC802154_CONF_USES_AUX_HEADER 1
#define AES_128_CONF_WITH_LOCKING 1
#define NBR_TABLE_CONF_WITH_LOCKING 1
#define AKES_NBR_CONF_WITH_LOCKING 1
#define NBR_TABLE_CONF_WITH_FIND_REMOVABLE 0
#define CSPRNG_CONF_ENABLED 1

/* configure AKES */
#define AKES_CONF_MAX_RETRANSMISSIONS_OF_HELLOACKS_AND_ACKS 2
#define NETSTACK_CONF_MAC akes_mac_driver
#define AKES_MAC_CONF_DECORATED_MAC csl_driver
#define NETSTACK_CONF_FRAMER crc16_framer
#define CRC16_FRAMER_CONF_DECORATED_FRAMER akes_mac_framer
#define AKES_MAC_CONF_DECORATED_FRAMER csl_framer_compliant_framer
#include "os/services/akes/akes-strategy-autoconf.h"
#else /* AKES_MAC_CONF_ENABLED */
#define NETSTACK_CONF_FRAMER crc16_framer
#define CRC16_FRAMER_CONF_DECORATED_FRAMER csl_framer_compliant_framer
#define NETSTACK_CONF_MAC csl_driver
#endif /* AKES_MAC_CONF_ENABLED */

#else /* CSL_CONF_COMPLIANT */
/* configure Contiki */
#define AES_128_CONF_WITH_LOCKING 1
#define NBR_TABLE_CONF_WITH_LOCKING 1
#define AKES_NBR_CONF_WITH_LOCKING 1
#define NBR_TABLE_CONF_WITH_FIND_REMOVABLE 0
#define CSPRNG_CONF_ENABLED 1
#define AKES_NBR_CONF_WITH_SEQNOS 1

/* configure AKES */
#define AKES_MAC_CONF_ENABLED 1
#define AKES_MAC_CONF_UNSECURE_UNICASTS 0
#define NETSTACK_CONF_MAC akes_mac_driver
#define AKES_MAC_CONF_DECORATED_MAC csl_driver
#define AKES_MAC_CONF_STRATEGY csl_strategy
#define AKES_DELETE_CONF_STRATEGY csl_strategy_delete
#define NETSTACK_CONF_FRAMER akes_mac_framer
#define AKES_MAC_CONF_DECORATED_FRAMER csl_framer_potr_framer
#define AKES_NBR_CONF_WITH_INDICES 1
#define AKES_NBR_CONF_WITH_PAIRWISE_KEYS 1
#define AKES_NBR_CONF_WITH_GROUP_KEYS 0
#define AKES_NBR_CONF_WITH_EXPIRATION_TIME 0
#define AKES_CONF_MAX_RETRANSMISSIONS_OF_HELLOACKS_AND_ACKS 2
#define AKES_MAC_CONF_BROADCAST_SEC_LVL 1
#endif /* CSL_CONF_COMPLIANT */

/** @} */
