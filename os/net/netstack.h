/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 */

/**
 * \file
 *         Include file for the Contiki low-layer network stack (NETSTACK)
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef NETSTACK_H
#define NETSTACK_H

#include "contiki.h"

/* Routing protocol configuration. The Routing protocol is configured through the Makefile,
   via the flag MAKE_ROUTING */
#ifdef NETSTACK_CONF_ROUTING
#define NETSTACK_ROUTING NETSTACK_CONF_ROUTING
#else /* NETSTACK_CONF_ROUTING */
#if ROUTING_CONF_RPL_LITE
#define NETSTACK_ROUTING rpl_lite_driver
#elif ROUTING_CONF_RPL_CLASSIC
#define NETSTACK_ROUTING rpl_classic_driver
#elif ROUTING_CONF_NULLROUTING
#define NETSTACK_ROUTING nullrouting_driver
#else
#error Unknown ROUTING configuration
#endif
#endif /* NETSTACK_CONF_ROUTING */

/* Network layer configuration. The NET layer is configured through the Makefile,
   via the flag MAKE_NET */
#ifdef NETSTACK_CONF_NETWORK
#define NETSTACK_NETWORK NETSTACK_CONF_NETWORK
#else /* NETSTACK_CONF_NETWORK */
#if NETSTACK_CONF_WITH_IPV6
#define NETSTACK_NETWORK sicslowpan_driver
#elif NETSTACK_CONF_WITH_NULLNET
#define NETSTACK_NETWORK nullnet_driver
#else
#error Unknown NET configuration
#endif
#endif /* NETSTACK_CONF_NETWORK */

/* MAC layer configuration. The MAC layer is configured through the Makefile,
   via the flag MAKE_MAC */
#ifdef NETSTACK_CONF_MAC
#define NETSTACK_MAC NETSTACK_CONF_MAC
#else /* NETSTACK_CONF_MAC */
#if MAC_CONF_WITH_NULLMAC
#define NETSTACK_MAC     nullmac_driver
#elif MAC_CONF_WITH_CSMA
#define NETSTACK_MAC     csma_driver
#elif MAC_CONF_WITH_TSCH
#define NETSTACK_MAC     tschmac_driver
#elif MAC_CONF_WITH_BLE
#define NETSTACK_MAC   ble_l2cap_driver
#else
#error Unknown MAC configuration
#endif
#endif /* NETSTACK_CONF_MAC */

/* Radio driver configuration. Most often set by the platform. */
#ifdef NETSTACK_CONF_RADIO
#define NETSTACK_RADIO NETSTACK_CONF_RADIO
#else /* NETSTACK_CONF_RADIO */
#define NETSTACK_RADIO   nullradio_driver
#endif /* NETSTACK_CONF_RADIO */

/* Framer selection. The framer is used by the MAC implementation
   to build and parse frames. */
#ifdef NETSTACK_CONF_FRAMER
#define NETSTACK_FRAMER NETSTACK_CONF_FRAMER
#else /* NETSTACK_CONF_FRAMER */
#define NETSTACK_FRAMER   framer_802154
#endif /* NETSTACK_CONF_FRAMER */

#include "net/mac/mac.h"
#include "net/mac/framer/framer.h"
#include "dev/radio.h"
#include "net/linkaddr.h"

/**
 * The structure of a network driver in Contiki.
 */
struct network_driver {
  char *name;

  /** Initialize the network driver */
  void (*init)(void);

  /** Callback for getting notified of incoming packet in packetbuf. */
  void (*input)(void);

  /** Output funtion, sends from uipbuf. */
  uint8_t (*output)(const linkaddr_t *localdest);
};

extern const struct routing_driver NETSTACK_ROUTING;
extern const struct network_driver NETSTACK_NETWORK;
extern const struct mac_driver NETSTACK_MAC;
extern const struct radio_driver NETSTACK_RADIO;
extern const struct framer NETSTACK_FRAMER;

static inline void
netstack_init(void)
{
  NETSTACK_RADIO.init();
  NETSTACK_MAC.init();
  NETSTACK_NETWORK.init();
}

/* Netstack ip_packet_processor - for implementing packet filters, firewalls,
   debuggin info, etc */

enum netstack_ip_action {
  NETSTACK_IP_PROCESS = 0, /* Default behaviour - nothing else */
  NETSTACK_IP_DROP = 1, /* Drop this packet before processing/sending anymore */
};

enum netstack_ip_callback_type {
  NETSTACK_IP_INPUT = 0,
  NETSTACK_IP_OUTPUT = 1,
};

struct netstack_ip_packet_processor {
  struct netstack_ip_packet_processor *next;
  enum netstack_ip_action (*process_input)(void);
  enum netstack_ip_action (*process_output)(const linkaddr_t * localdest);
};

/* This function is intended for the IP stack to call whenever input/output
   callback needs to be called */
enum netstack_ip_action netstack_process_ip_callback(uint8_t type, const linkaddr_t *localdest);

void netstack_ip_packet_processor_add(struct netstack_ip_packet_processor *p);
void netstack_ip_packet_processor_remove(struct netstack_ip_packet_processor *p);

/* Netstack sniffer - this will soon be deprecated... */

struct netstack_sniffer {
  struct netstack_sniffer *next;
  void (*input_callback)(void);
  void (*output_callback)(int mac_status);
};

#define NETSTACK_SNIFFER(name, input_callback, output_callback) \
  static struct netstack_sniffer name = { NULL, input_callback, output_callback }

void netstack_sniffer_add(struct netstack_sniffer *s);
void netstack_sniffer_remove(struct netstack_sniffer *s);

#endif /* NETSTACK_H */
