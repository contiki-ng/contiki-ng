/*
 * Copyright (c) 2016, Inria.
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
 * \addtogroup uip
 * @{
 *
 * \file
 *         Source routing support
 *
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 */


#ifndef UIP_SR_H
#define UIP_SR_H

/********** Includes  **********/

#include "contiki.h"
#include "net/ipv6/uip.h"

/********** Configuration  **********/

/* The number of source routing nodes, i.e. the maximum netwrok size at the root */
#ifdef UIP_SR_CONF_LINK_NUM

#define UIP_SR_LINK_NUM UIP_SR_CONF_LINK_NUM

#else /* UIP_SR_CONF_LINK_NUM */

#if ROUTING_CONF_RPL_LITE
#define UIP_SR_LINK_NUM NETSTACK_MAX_ROUTE_ENTRIES
#elif ROUTING_CONF_RPL_CLASSIC

#include "net/routing/rpl-classic/rpl-conf.h"
#if RPL_WITH_NON_STORING
#define UIP_SR_LINK_NUM NETSTACK_MAX_ROUTE_ENTRIES
#else /* RPL_WITH_NON_STORING */
#define UIP_SR_LINK_NUM 0
#endif /* RPL_WITH_NON_STORING */

#else

#define UIP_SR_LINK_NUM 0

#endif

#endif /* UIP_SR_CONF_LINK_NUM */

/* Delay between between expiration order and actual node removal */
#ifdef UIP_SR_CONF_REMOVAL_DELAY
#define UIP_SR_REMOVAL_DELAY          UIP_SR_CONF_REMOVAL_DELAY
#else /* UIP_SR_CONF_REMOVAL_DELAY */
#define UIP_SR_REMOVAL_DELAY          60
#endif /* UIP_SR_CONF_REMOVAL_DELAY */

#define UIP_SR_INFINITE_LIFETIME           0xFFFFFFFF

/********** Data Structures  **********/

/** \brief A node in a source routing graph, stored at the root and representing
 * all child-parent relationship. Used to build source routes */
typedef struct uip_sr_node {
  struct uip_sr_node *next;
  uint32_t lifetime;
  /* Protocol-specific graph structure */
  void *graph;
  /* Store only IPv6 link identifiers, the routing protocol will provide
  us with the prefix */
  unsigned char link_identifier[8];
  struct uip_sr_node *parent;
} uip_sr_node_t;

/********** Public functions **********/

/**
 * Tells how many nodes are currently stored in the graph
 *
 * \return The number of nodes
*/
int uip_sr_num_nodes(void);

/**
 * Expires a given child-parent link
 *
 * \param graph The graph the link belongs to
 * \param child The IPv6 address of the child
 * \param parent The IPv6 address of the parent
*/
void uip_sr_expire_parent(void *graph, const uip_ipaddr_t *child, const uip_ipaddr_t *parent);

/**
 * Updates a child-parent link
 *
 * \param graph The graph the link belongs to
 * \param child The IPv6 address of the child
 * \param parent The IPv6 address of the parent
 * \param lifetime The link lifetime in seconds
*/
uip_sr_node_t *uip_sr_update_node(void *graph, const uip_ipaddr_t *child, const uip_ipaddr_t *parent, uint32_t lifetime);

/**
 * Returns the head of the non-storing node list
 *
 * \return The head of the list
*/
uip_sr_node_t *uip_sr_node_head(void);

/**
 * Returns the next element of the non-storing node list
 *
 * \param item The current element in the list
 * \return The next element of the list
*/
uip_sr_node_t *uip_sr_node_next(uip_sr_node_t *item);

/**
 * Looks up for a source routing node from its IPv6 global address
 *
 * \param graph The graph where to look up for the node
 * \param addr The target address
 * \return A pointer to the node
*/
uip_sr_node_t *uip_sr_get_node(void *graph, const uip_ipaddr_t *addr);

/**
 * Telle whether an address is reachable, i.e. if there exists a path from
 * the root to the node in the current source routing graph
 *
 * \param graph The graph where to look up for the node
 * \param addr The target IPv6 global address
 * \return 1 if the node is reachable, 0 otherwise
*/
int uip_sr_is_addr_reachable(void *graph, const uip_ipaddr_t *addr);

/**
 * A function called periodically. Used to age the links (decrease lifetime
 * and expire links accordingly)
 *
 * \param seconds The number of seconds elapsted since last call
*/
void uip_sr_periodic(unsigned seconds);

/**
 * Initialize this module
*/
void uip_sr_init(void);

/**
 * Deallocate all neighbors
*/
void uip_sr_free_all(void);

/**
* Print a textual description of a source routing link
*
* \param buf The buffer where to write content
* \param buflen The buffer len
* \param link A pointer to the source routing link
* \return Identical to snprintf: number of bytes written excluding ending null
* byte. A value >= buflen if the buffer was too small.
*/
int uip_sr_link_snprint(char *buf, int buflen, uip_sr_node_t *link);

 /** @} */

#endif /* UIP_SR_H */
