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
 */

 /**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *	Header file for rpl-neighbor module
 * \author
 *	Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 *  Simon DUquennoy <simon.duquennoy@inria.fr>
 *
 */

#ifndef RPL_NEIGHBOR_H
#define RPL_NEIGHBOR_H

/********** Includes **********/

#include "net/routing/rpl-lite/rpl.h"
#include "lib/list.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "sys/ctimer.h"

/********** Public symbols **********/

/* Per-neighbor RPL information. According to RFC 6550, there exist three
 * types of neighbors:
 * - Candidate neighbor set: any neighbor, selected in an implementation
 * and OF-specific way. The nodes in rpl_neighbors constitute the candidate neighbor set.
 * - Parent set: the subset of the candidate neighbor set with rank below our rank
 * - Preferred parent: one node of the parent set
 */
NBR_TABLE_DECLARE(rpl_neighbors);

/********** Public functions **********/

/**
 * Initialize rpl-dag-neighbor module
*/
void rpl_neighbor_init(void);

/**
 * Tells whether a neighbor is in the parent set.
 *
 * \param nbr The neighbor to be tested
 * \return 1 if nbr is in the parent set, 0 otherwise
 */
int rpl_neighbor_is_parent(rpl_nbr_t *nbr);

/**
 * Set current RPL preferred parent and update DS6 default route accordingly
 *
 * \param nbr The new preferred parent
*/
void rpl_neighbor_set_preferred_parent(rpl_nbr_t *nbr);

/**
 * Tells wether we have fresh link information towards a given neighbor
 *
 * \param nbr The neighbor
 * \return 1 if we have fresh link information, 0 otherwise
*/
int rpl_neighbor_is_fresh(rpl_nbr_t *nbr);

/**
 * Tells wether we a given neighbor is reachable
 *
 * \param nbr The neighbor
 * \return 1 if the parent is reachable, 0 otherwise
*/
int rpl_neighbor_is_reachable(rpl_nbr_t *nbr);

/**
 * Tells whether a nbr is acceptable as per the OF's definition
 *
 * \param nbr The neighbor
 * \return 1 if acceptable, 0 otherwise
*/
int rpl_neighbor_is_acceptable_parent(rpl_nbr_t *nbr);

/**
 * Returns a neighbor's link metric
 *
 * \param nbr The neighbor
 * \return The link metric if any, 0xffff otherwise
*/
uint16_t rpl_neighbor_get_link_metric(rpl_nbr_t *nbr);

/**
 * Returns our rank if selecting a given parent as preferred parent
 *
 * \param nbr The neighbor
 * \return The resulting rank if any, RPL_INFINITE_RANK otherwise
*/
rpl_rank_t rpl_neighbor_rank_via_nbr(rpl_nbr_t *nbr);

/**
 * Returns a neighbors's link-layer address
 *
 * \param nbr The neighbor
 * \return The link-layer address if any, NULL otherwise
*/
const linkaddr_t *rpl_neighbor_get_lladdr(rpl_nbr_t *nbr);

/**
 * Returns a neighbor's link statistics
 *
 * \param nbr The neighbor
 * \return The link_stats structure address if any, NULL otherwise
*/
const struct link_stats *rpl_neighbor_get_link_stats(rpl_nbr_t *nbr);

/**
 * Returns a neighbor's (link-local) IPv6 address
 *
 * \param nbr The neighbor
 * \return The link-local IPv6 address if any, NULL otherwise
*/
uip_ipaddr_t *rpl_neighbor_get_ipaddr(rpl_nbr_t *nbr);

/**
 * Returns a neighbor from its link-layer address
 *
 * \param addr The link-layer address
 * \return The neighbor if found, NULL otherwise
*/
rpl_nbr_t *rpl_neighbor_get_from_lladdr(uip_lladdr_t *addr);

/**
 * Returns a neighbor from its link-local IPv6 address
 *
 * \param addr The link-local IPv6 address
 * \return The neighbor if found, NULL otherwise
*/
rpl_nbr_t *rpl_neighbor_get_from_ipaddr(uip_ipaddr_t *addr);

/**
 * Returns the number of nodes in the RPL neighbor table
 *
 * \return the neighbor count
*/
int rpl_neighbor_count(void);

/**
 * Prints a summary of all RPL neighbors and their properties
 *
 * \param str A descriptive text on the caller
*/
void rpl_neighbor_print_list(const char *str);

/**
 * Empty the RPL neighbor table
*/
void rpl_neighbor_remove_all(void);

/**
 * Returns the best candidate for preferred parent
 *
 * \return The best candidate, NULL if no usable parent is found
*/
rpl_nbr_t *rpl_neighbor_select_best(void);

/**
* Print a textual description of RPL neighbor into a string
*
* \param buf The buffer where to write content
* \param buflen The buffer len
* \param nbr A pointer to a RPL neighbor that will be written to the buffer
* \return Identical to snprintf: number of bytes written excluding ending null
* byte. A value >= buflen if the buffer was too small.
*/
int rpl_neighbor_snprint(char *buf, int buflen, rpl_nbr_t *nbr);

typedef rpl_nbr_t rpl_parent_t;
#define rpl_parent_get_from_ipaddr(addr) rpl_neighbor_get_from_ipaddr(addr)
#define rpl_parent_get_ipaddr(nbr) rpl_neighbor_get_ipaddr(nbr)
 /** @} */

#endif /* RPL_NEIGHBOR_H */
