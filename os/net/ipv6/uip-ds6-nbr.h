/*
 * Copyright (c) 2013, Swedish Institute of Computer Science.
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
 *
 */

/**
 * \addtogroup uip
 * @{
 */

/**
 * \file
 *    IPv6 Neighbor cache (link-layer/IPv6 address mapping)
 * \author Mathilde Durvy <mdurvy@cisco.com>
 * \author Julien Abeille <jabeille@cisco.com>
 * \author Simon Duquennoy <simonduq@sics.se>
 *
 */

#ifndef UIP_DS6_NEIGHBOR_H_
#define UIP_DS6_NEIGHBOR_H_

#include "contiki.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-nd6.h"
#include "net/nbr-table.h"
#include "sys/stimer.h"
#if UIP_CONF_IPV6_QUEUE_PKT
#include "net/ipv6/uip-packetqueue.h"
#endif                          /*UIP_CONF_QUEUE_PKT */
#if UIP_DS6_NBR_CONF_MULTI_IPV6_ADDRS
#include "lib/assert.h"
#include "lib/list.h"
#endif

/*--------------------------------------------------*/
/** \brief Possible states for the nbr cache entries */
#define  NBR_INCOMPLETE 0
#define  NBR_REACHABLE 1
#define  NBR_STALE 2
#define  NBR_DELAY 3
#define  NBR_PROBE 4

/** \brief Set non-zero (1) to enable multiple IPv6 addresses to be
 * associated with a link-layer address */
#ifdef UIP_DS6_NBR_CONF_MULTI_IPV6_ADDRS
#define UIP_DS6_NBR_MULTI_IPV6_ADDRS UIP_DS6_NBR_CONF_MULTI_IPV6_ADDRS
#else
#define UIP_DS6_NBR_MULTI_IPV6_ADDRS 0
#endif /* UIP_DS6_NBR_CONF_MULTI_IPV6_ADDRS */

/** \brief Set the maximum number of IPv6 addresses per link-layer
 * address */
#ifdef UIP_DS6_NBR_CONF_MAX_6ADDRS_PER_NBR
#define UIP_DS6_NBR_MAX_6ADDRS_PER_NBR UIP_DS6_NBR_CONF_MAX_6ADDRS_PER_NBR
#else
#define UIP_DS6_NBR_MAX_6ADDRS_PER_NBR 2
#endif /* UIP_DS6_NBR_CONF_MAX_6ADDRS_PER_NBR */

/** \brief Set the maximum number of neighbor cache entries */
#ifdef UIP_DS6_NBR_CONF_MAX_NEIGHBOR_CACHES
#define UIP_DS6_NBR_MAX_NEIGHBOR_CACHES UIP_DS6_NBR_CONF_MAX_NEIGHBOR_CACHES
#else
#define UIP_DS6_NBR_MAX_NEIGHBOR_CACHES \
  (NBR_TABLE_MAX_NEIGHBORS * UIP_DS6_NBR_MAX_6ADDRS_PER_NBR)
#endif /* UIP_DS6_NBR_CONF_MAX_NEIGHBOR_CACHES */

#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
/** \brief nbr_table entry when UIP_DS6_NBR_MULTI_IPV6_ADDRS is
 * enabled. uip_ds6_nbrs is a list of uip_ds6_nbr_t objects */
typedef struct {
  LIST_STRUCT(uip_ds6_nbrs);
} uip_ds6_nbr_entry_t;
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

/** \brief The default nbr_table entry (when
 * UIP_DS6_NBR_MULTI_IPV6_ADDRS is disabled), that implements nbr
 * cache */
typedef struct uip_ds6_nbr {
#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  struct uip_ds6_nbr *next;
  uip_ds6_nbr_entry_t *nbr_entry;
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
  uip_ipaddr_t ipaddr;
  uint8_t isrouter;
  uint8_t state;
#if UIP_ND6_SEND_NS || UIP_ND6_SEND_RA
  struct stimer reachable;
  struct stimer sendns;
  uint8_t nscount;
#endif /* UIP_ND6_SEND_NS || UIP_ND6_SEND_RA */
#if UIP_CONF_IPV6_QUEUE_PKT
  struct uip_packetqueue_handle packethandle;
#define UIP_DS6_NBR_PACKET_LIFETIME CLOCK_SECOND * 4
#endif                          /*UIP_CONF_QUEUE_PKT */
} uip_ds6_nbr_t;

void uip_ds6_neighbors_init(void);

/**
 * Add a neighbor cache for a specified IPv6 address, which is
 *  associated with a specified link-layer address
 * \param ipaddr IPv6 address of a neighbor to add
 * \param lladdr Link-layer address to associate with ipaddr
 * \param isrouter Set 1 if the neighbor is a router
 * \param state Set the initial neighbor cache state (e.g.,
 * NBR_INCOMPLETE)
 * \param reason Set a reason of the addition (e.g.,
 * NBR_TABLE_REASON_RPL_DIO)
 * \param data Set data associated with the nbr cache
 * \return the address of a newly added nbr cache on success, NULL on
 * failure
 */
uip_ds6_nbr_t *uip_ds6_nbr_add(const uip_ipaddr_t *ipaddr,
                               const uip_lladdr_t *lladdr,
                               uint8_t isrouter, uint8_t state,
                               nbr_table_reason_t reason, void *data);

/**
 * Remove a neighbor cache
 * \param nbr the address of a neighbor cache to remove
 * \return 1 on success, 0 on failure (nothing was removed)
 */
int uip_ds6_nbr_rm(uip_ds6_nbr_t *nbr);

/**
 * Get the link-layer address associated with a specified nbr cache
 * \param nbr the address of a neighbor cache
 * \return pointer to the link-layer address on success, NULL on failure
 */
const uip_lladdr_t *uip_ds6_nbr_get_ll(const uip_ds6_nbr_t *nbr);

/**
 * Get the link-layer address associated with a specified IPv6 address
 * \param ipaddr an IPv6 address used as a search key
 * \return the pointer to the link-layer address on success, NULL on failure
 */
const uip_lladdr_t *uip_ds6_nbr_lladdr_from_ipaddr(const uip_ipaddr_t *ipaddr);

/**
 * Update the link-layer address associated with an  IPv6 address
 * \param nbr the double pointer to a neighbor cache which has the
 * target IPv6 address
 * \param new_ll_addr the new link-layer address of the IPv6 address
 * return 0 on success, -1 on failure
 */
int uip_ds6_nbr_update_ll(uip_ds6_nbr_t **nbr, const uip_lladdr_t *new_ll_addr);

/**
 * Get an IPv6 address of a neighbor cache
 * \param nbr the pointer to a neighbor cache
 * \return the pointer to an IPv6 address associated with the neighbor cache
 * \note This returns the first IPv6 address found in the neighbor
 * cache when UIP_DS6_NBR_MULTI_IPV6_ADDRS is enabled
 */
const uip_ipaddr_t *uip_ds6_nbr_get_ipaddr(const uip_ds6_nbr_t *nbr);

/**
 * Get an IPv6 address associated with a specified link-layer address
 * \param lladdr a link-layer address used as a search key
 * \return the pointer to an IPv6 address associated with the neighbor cache
 * \note This returns the first IPv6 address found in the neighbor
 * cache when UIP_DS6_NBR_MULTI_IPV6_ADDRS is enabled
 */
uip_ipaddr_t *uip_ds6_nbr_ipaddr_from_lladdr(const uip_lladdr_t *lladdr);

/**
 * Get the neighbor cache associated with a specified IPv6 address
 * \param ipaddr an IPv6 address used as a search key
 * \return the pointer to a neighbor cache on success, NULL on failure
 */
uip_ds6_nbr_t *uip_ds6_nbr_lookup(const uip_ipaddr_t *ipaddr);

/**
 * Get the neighbor cache associated with a specified link-layer address
 * \param lladdr a link-layer address used as a search key
 * \return the pointer to a neighbor cache on success, NULL on failure
 */
uip_ds6_nbr_t *uip_ds6_nbr_ll_lookup(const uip_lladdr_t *lladdr);

/**
 * Return the number of neighbor caches
 * \return the number of neighbor caches in use
 */
int uip_ds6_nbr_num(void);

/**
 * Get the first neighbor cache in nbr_table
 * \return the pointer to the first neighbor cache entry
 */
uip_ds6_nbr_t *uip_ds6_nbr_head(void);

/**
 * Get the next neighbor cache of a specified one
 * \param nbr the pointer to a neighbor cache
 * \return the pointer to the next one on success, NULL on failure
 */
uip_ds6_nbr_t *uip_ds6_nbr_next(uip_ds6_nbr_t *nbr);

/**
 * The callback function to update link-layer stats in a neighbor
 * cache
 * \param status MAC return value defined in mac.h
 * \param numtx the number of transmissions happened for a packet
 */
void uip_ds6_link_callback(int status, int numtx);

/**
 * The housekeeping function called periodically
 */
void uip_ds6_neighbor_periodic(void);

#if UIP_ND6_SEND_NS
/**
 * \brief Refresh the reachable state of a neighbor. This function
 * may be called when a node receives an IPv6 message that confirms the
 * reachability of a neighbor.
 * \param ipaddr pointer to the IPv6 address whose neighbor reachability state
 * should be refreshed.
 */
void uip_ds6_nbr_refresh_reachable_state(const uip_ipaddr_t *ipaddr);
#endif /* UIP_ND6_SEND_NS */

#endif /* UIP_DS6_NEIGHBOR_H_ */
/** @} */
