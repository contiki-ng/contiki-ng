/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 * \addtogroup link-layer
 * @{
 *
 * \defgroup csma Implementation of the 802.15.4 standard CSMA protocol
 * @{
 */

/**
 * \file
 *         The 802.15.4 standard CSMA protocol (nonbeacon-enabled)
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#ifndef CSMA_H_
#define CSMA_H_

#include "contiki.h"
#include "net/mac/mac.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "dev/radio.h"

#ifdef CSMA_CONF_SEND_SOFT_ACK
#define CSMA_SEND_SOFT_ACK CSMA_CONF_SEND_SOFT_ACK
#else /* CSMA_CONF_SEND_SOFT_ACK */
#define CSMA_SEND_SOFT_ACK 0
#endif /* CSMA_CONF_SEND_SOFT_ACK */

#ifdef CSMA_CONF_ACK_WAIT_TIME
#define CSMA_ACK_WAIT_TIME CSMA_CONF_ACK_WAIT_TIME
#else /* CSMA_CONF_ACK_WAIT_TIME */
#define CSMA_ACK_WAIT_TIME                      RTIMER_SECOND / 2500
#endif /* CSMA_CONF_ACK_WAIT_TIME */

#ifdef CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME
#define CSMA_AFTER_ACK_DETECTED_WAIT_TIME CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME
#else /* CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME */
#define CSMA_AFTER_ACK_DETECTED_WAIT_TIME       RTIMER_SECOND / 1500
#endif /* CSMA_CONF_AFTER_ACK_DETECTED_WAIT_TIME */

#define CSMA_ACK_LEN 3

/* just a default - with LLSEC, etc */
#define CSMA_MAC_MAX_HEADER 21

extern const struct mac_driver csma_driver;

/* CSMA security framer functions */
int csma_security_create_frame(void);
int csma_security_parse_frame(void);

/* key management for CSMA */
int csma_security_set_key(uint8_t index, const uint8_t *key);

/**
 * Determines if statistics support should be compiled in.
 */
#ifndef CSMA_CONF_STATISTICS
#define CSMA_STATISTICS  0
#else /* CSMA_CONF_STATISTICS */
#define CSMA_STATISTICS (CSMA_CONF_STATISTICS)
#endif /* CSMA_CONF_STATISTICS */

/**
 * The CSMA statistics.
 *
 * This is the variable in which the uIP TCP/IP statistics are gathered.
 */
#if CSMA_STATISTICS == 1
extern struct csma_stats csma_stat;
#define CSMA_STAT(s) s
#else
#define CSMA_STAT(s)
#endif /* CSMA_STATISTICS == 1 */

/**
 * The structure holding the CSMA-CA statistics that are gathered if
 * CSMA_CONF_STATISTICS is set to 1.
 *
 */
struct csma_stats {
  struct {
    uint32_t sent;
    uint32_t retry;
    uint32_t collisions;
    uint32_t acked;
    uint32_t not_acked;
    uint32_t err;
  } tx;
  struct {
    uint32_t recv;
    uint32_t acked;
    uint32_t duplicate;
    uint32_t not_for_us;
    uint32_t ignore_ack;
    uint32_t err;
  } rx;
};

#endif /* CSMA_H_ */
/**
 * @}
 * @}
 */
