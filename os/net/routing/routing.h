/*
 * Copyright (c) 2017, RISE SICS.
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
 *         Routing driver header file
 * \author
 *         Simon Duquennoy <simon.duquennoy@ri.se>
 */

#ifndef ROUTING_H_
#define ROUTING_H_

#include "contiki.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/linkaddr.h"

/**
 * The structure of a routing protocol driver.
 */
struct routing_driver {
  char *name;
  /** Initialize the routing protocol */
  void (* init)(void);
  /**
   * Set the prefix, for nodes that will operate as root
   *
   * \param prefix The prefix. If NULL, UIP_DS6_DEFAULT_PREFIX is used instead
   * \param iid The IID. If NULL, it will be built from uip_ds6_set_addr_iid.
  */
  void (* root_set_prefix)(uip_ipaddr_t *prefix, uip_ipaddr_t *iid);
  /**
   * Set the node as root and start a network
   *
   * \return 0 in case of success, -1 otherwise
  */
  int (* root_start)(void);
  /**
   * Triggers a global topology repair
   *
   * \param str A textual description of the cause for triggering a repair
  */
  void (* global_repair)(const char *str);
  /**
   * Triggers a RPL local topology repair
   *
   * \param str A textual description of the cause for triggering a repair
  */
  void (* local_repair)(const char *str);
  /**
   * Removes all extension headers that pertain to the routing protocol.
  */
  void (* ext_header_remove)(void);
  /**
   * Adds/updates routing protocol extension headers to current uIP packet.
   *
   * \return 1 in case of success, 0 otherwise
  */
  int (* ext_header_update)(void);
  /**
  * Process and update the routing protocol hob-by-hop
  * extention headers of the current uIP packet.
  *
  * \param uip_ext_opt_offset The offset within the uIP packet where
  * extension headers start
  * \return 1 in case the packet is valid and to be processed further,
  * 0 in case the packet must be dropped.
  */
  int (* ext_header_hbh_update)(int uip_ext_opt_offset);
  /**
   * Look for next hop from SRH of current uIP packet.
   *
   * \param ipaddr A pointer to the address where to store the next hop.
   * \return 1 if a next hop was found, 0 otherwise
  */
  int (* ext_header_srh_get_next_hop)(uip_ipaddr_t *ipaddr);
};

#endif /* ROUTING_H_ */
