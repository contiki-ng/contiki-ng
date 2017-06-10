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
 * \file
 *	Header file for rpl-parent module
 * \author
 *	Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 *  Simon DUquennoy <simon.duquennoy@inria.fr>
 *
 */

#ifndef RPL_PARENT_H
#define RPL_PARENT_H

/********** Includes **********/

#include "rpl.h"
#include "lib/list.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "sys/ctimer.h"

/**
 * \addtogroup uip6
 * @{
 */

/********** Public symbols **********/

/* Per-parent RPL information */
NBR_TABLE_DECLARE(rpl_parents);

/********** Public functions **********/

/**
 * Initialize rpl-dag-parent module
*/
void rpl_parent_init(void);

/**
 * Set current RPL preferred parent and update DS6 default route accordingly
 *
 * \param p The new preferred parent
*/
void rpl_parent_set_preferred(rpl_parent_t *p);

/**
 * Tells wether we have fresh link information towards a given parent
 *
 * \param p The parent
 * \return 1 if we have fresh link information, 0 otherwise
*/
int rpl_parent_is_fresh(rpl_parent_t *p);

/**
 * Tells wether we a given parent is reachable
 *
 * \param p The parent
 * \return 1 if the parent is reachable, 0 otherwise
*/
int rpl_parent_is_reachable(rpl_parent_t *p);

/**
 * Returns a parent's link metric
 *
 * \param p The parent
 * \return The link metric if any, 0xffff otherwise
*/
uint16_t rpl_parent_get_link_metric(rpl_parent_t *p);

/**
 * Returns our rank if selecting a given parent as preferred parent
 *
 * \param p The parent
 * \return The resulting rank if any, RPL_INFINITE_RANK otherwise
*/
rpl_rank_t rpl_parent_rank_via_parent(rpl_parent_t *p);

/**
 * Returns a parent's link-layer address
 *
 * \param p The parent
 * \return The link-layer address if any, NULL otherwise
*/
const linkaddr_t *rpl_parent_get_lladdr(rpl_parent_t *p);

/**
 * Returns a parent's link statistics
 *
 * \param p The parent
 * \return The link_stats structure address if any, NULL otherwise
*/
const struct link_stats *rpl_parent_get_link_stats(rpl_parent_t *p);

/**
 * Returns a parent's (link-local) IPv6 address
 *
 * \param p The parent
 * \return The link-local IPv6 address if any, NULL otherwise
*/
uip_ipaddr_t *rpl_parent_get_ipaddr(rpl_parent_t *p);

/**
 * Returns a parent from its link-layer address
 *
 * \param addr The link-layer address
 * \return The parent if found, NULL otherwise
*/
rpl_parent_t *rpl_parent_get_from_lladdr(uip_lladdr_t *addr);

/**
 * Returns a parent from its link-local IPv6 address
 *
 * \param addr The link-local IPv6 address
 * \return The parent if found, NULL otherwise
*/
rpl_parent_t *rpl_parent_get_from_ipaddr(uip_ipaddr_t *addr);

/**
 * Prints a summary of all RPL neighbors and their properties
 *
 * \param str A descriptive text on the caller
*/
void rpl_parent_print_list(const char *str);

/**
 * Empty the RPL parent table
*/
void rpl_parent_remove_all(void);

/**
 * Returns the best candidate for preferred parent
 *
 * \return The best candidate, NULL if no usable parent is found
*/
rpl_parent_t *rpl_parent_select_best(void);

 /** @} */

#endif /* RPL_PARENT_H */
