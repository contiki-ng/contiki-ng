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
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *         RPL non-storing mode specific functions. Includes support for
 *         source routing.
 *
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 */


#ifndef RPL_NS_H
#define RPL_NS_H

/********** Includes  **********/

#include "net/ipv6/uip.h"
#include "net/rpl-lite/rpl.h"

/********** Data Structures  **********/

/* A node in a RPL Non-storing graph, stored at the root and representing
 * all child-parent relationship. Used to build source routes */
typedef struct rpl_ns_node {
  struct rpl_ns_node *next;
  uint32_t lifetime;
  rpl_dag_t *dag;
  /* Store only IPv6 link identifiers as all nodes in the DAG share the same prefix */
  unsigned char link_identifier[8];
  struct rpl_ns_node *parent;
} rpl_ns_node_t;

/********** Public functions **********/

/**
 * Tells how many nodes are currently stored in the graph
 *
 * \return The number of nodes
*/
int rpl_ns_num_nodes(void);

/**
 * Expires a given child-parent link
 *
 * \param child The IPv6 address of the child
 * \param parent The IPv6 address of the parent
*/
void rpl_ns_expire_parent(const uip_ipaddr_t *child, const uip_ipaddr_t *parent);

/**
 * Updates a child-parent link
 *
 * \param child The IPv6 address of the child
 * \param parent The IPv6 address of the parent
 * \param lifetime The link lifetime in seconds
*/
rpl_ns_node_t *rpl_ns_update_node(const uip_ipaddr_t *child, const uip_ipaddr_t *parent, uint32_t lifetime);

/**
 * Returns the head of the non-storing node list
 *
 * \return The head of the list
*/
rpl_ns_node_t *rpl_ns_node_head(void);

/**
 * Returns the next element of the non-storing node list
 *
 * \param item The current element in the list
 * \return The next element of the list
*/
rpl_ns_node_t *rpl_ns_node_next(rpl_ns_node_t *item);

/**
 * Looks up for a RPL NS node from its IPv6 global address
 *
 * \param addr The target address
 * \return A pointer to the node
*/
rpl_ns_node_t *rpl_ns_get_node(const uip_ipaddr_t *addr);

/**
 * Telle whether an address is reachable, i.e. if there exists a path from
 * the root to the node in the current RPL NS graph
 *
 * \param addr The target IPv6 global address
 * \return 1 if the node is reachable, 0 otherwise
*/
int rpl_ns_is_addr_reachable(const uip_ipaddr_t *addr);

/**
 * Finds the global address of a given node
 *
 * \param addr A pointer to the address to be written
 * \param node The target node
 * \return 1 if success, 0 otherwise
*/
int rpl_ns_get_node_global_addr(uip_ipaddr_t *addr, rpl_ns_node_t *node);

/**
 * A function called periodically. Used to age the links (decrease lifetime
 * and expire links accordingly)
 *
 * \param seconds The number of seconds elapsted since last call
*/
void rpl_ns_periodic(unsigned seconds);

/**
 * Initialize rpl-ns module
*/
void rpl_ns_init(void);

/**
 * Deallocate all neighbors
*/
void rpl_ns_free_all(void);

 /** @} */

#endif /* RPL_NS_H */
