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
 *
 * \ingroup routing
 * \addtogroup rpl-lite
 RPL-lite is a lightweight implementation of RPL tailored for reliability.
 Supports only non-storing mode, one instance and one DAG.
 * @{
 *
 * \file
 *	Public API declarations for RPL.
 * \author
 *	Joakim Eriksson <joakime@sics.se> & Nicolas Tsiftes <nvt@sics.se>
 *  Simon Duquennoy <simon.duquennoy@inria.fr>
 *
 */

#ifndef RPL_H
#define RPL_H

/********** Includes **********/

#include "net/ipv6/uip.h"
#include "net/routing/rpl-lite/rpl-const.h"
#include "net/routing/rpl-lite/rpl-conf.h"
#include "net/routing/rpl-lite/rpl-types.h"
#include "net/routing/rpl-lite/rpl-icmp6.h"
#include "net/routing/rpl-lite/rpl-dag.h"
#include "net/routing/rpl-lite/rpl-dag-root.h"
#include "net/routing/rpl-lite/rpl-neighbor.h"
#include "net/routing/rpl-lite/rpl-ext-header.h"
#include "net/routing/rpl-lite/rpl-timers.h"

/********** Public symbols **********/

/* The only instance */
extern rpl_instance_t curr_instance;
/* The RPL multicast address (used for DIS and DIO) */
extern uip_ipaddr_t rpl_multicast_addr;

/********** Public functions **********/

/**
 * Called by lower layers after every packet transmission
 *
 * \param addr The link-layer addrress of the packet destination
 * \param status The transmission status (see os/net/mac/mac.h)
 * \param numtx The total number of transmission attempts
 */
void rpl_link_callback(const linkaddr_t *addr, int status, int numtx);

/**
 * Set prefix from an prefix data structure (from DIO)
 *
 * \param prefix The prefix
 * \return 1 if success, 0 otherwise
 */
int rpl_set_prefix(rpl_prefix_t *prefix);
/**
* Set prefix from an IPv6 address
*
* \param addr The prefix
* \param len The prefix length
* \param flags The DIO prefix flags
* \return 1 if success, 0 otherwise
*/

int rpl_set_prefix_from_addr(uip_ipaddr_t *addr, unsigned len, uint8_t flags);

/**
 * Removes current prefx
 *
 * \param last_prefix The last prefix (which is to be removed)
 */
void rpl_reset_prefix(rpl_prefix_t *last_prefix);

/**
 * Get one of the node's global addresses
 *
 * \return A (constant) pointer to the global IPv6 address found.
 * The pointer directs to the internals of DS6, should only be used
 * in the current function's local scope
 */
const uip_ipaddr_t *rpl_get_global_address(void);

/**
 * Get the RPL's best guess on if we are reachable via have downward route or not.
 *
 * \return 1 if we are reachable, 0 otherwise.
 */
int rpl_is_reachable(void);

/**
 * Greater-than function for a lollipop counter
 *
 * \param a The first counter
 * \param b The second counter
 * \return 1 is a>b else 0
 */
int rpl_lollipop_greater_than(int a, int b);

/**
 * Triggers a route fresh via DTSN increment
 *
 * \param str a textual description of the cause for refresh
 */
void rpl_refresh_routes(const char *str);

/**
 * Changes the value of the rpl_leaf_only flag, which determines if a node acts
 * only as a leaf in the network
 *
 * \param value the value to set: 0-disable, 1-enable
 */
void rpl_set_leaf_only(uint8_t value);

/**
 * Get the value of the rpl_leaf_only flag
 *
 * \return The value of the rpl_leaf_only flag
 */
uint8_t rpl_get_leaf_only(void);
 /** @} */

#endif /* RPL_H */
