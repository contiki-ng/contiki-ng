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

#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include "lib/list.h"
#include "net/link-stats.h"
#include "net/linkaddr.h"
#include "net/packetbuf.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/ipv6/uip-nd6.h"
#include "net/routing/routing.h"

#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
#include "lib/memb.h"
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "IPv6 Nbr"
#define LOG_LEVEL LOG_LEVEL_IPV6

#if BUILD_WITH_ORCHESTRA

/* A configurable function called after adding a new neighbor, or removing one */
#ifndef NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK
#define NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK orchestra_callback_neighbor_updated
#endif /* NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK */
void NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK(const linkaddr_t *, uint8_t is_added);

#endif /* BUILD_WITH_ORCHESTRA */

#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
/**
 * Add nbr to the list in nbr_entry. In other words, this function associates an
 * IPv6 address in nbr with a link-layer address in nbr_entry.
 * \param nbr the neighbor cache entry for an IPv6 address
 * \param nbr_entry the nbr_table entry for an link-layer address
 */
static void add_uip_ds6_nbr_to_nbr_entry(uip_ds6_nbr_t *nbr,
                                         uip_ds6_nbr_entry_t *nbr_entry);

/**
 * Remove nbr from the list of the corresponding nbr_entry
 * \param nbr a neighbor cache entry (nbr) to be removed
 */
static void remove_uip_ds6_nbr_from_nbr_entry(uip_ds6_nbr_t *nbr);

/**
 * Remove nbr_etnry from nbr_table
 * \param nbr_entry a nbr_table entry (nbr_entry) to be removed
 */
static void remove_nbr_entry(uip_ds6_nbr_entry_t *nbr_entry);

/**
 * Free memory for a specified neighbor cache entry
 * \param nbr a neighbor cache entry to be freed
 */
static void free_uip_ds6_nbr(uip_ds6_nbr_t *nbr);

/**
 * Callback function called when a nbr_table entry is removed
 * \param nbr_entry a nbr_entry to be removed
 */
static void callback_nbr_entry_removal(uip_ds6_nbr_entry_t *nbr_entry);

NBR_TABLE(uip_ds6_nbr_entry_t, uip_ds6_nbr_entries);
MEMB(uip_ds6_nbr_memb, uip_ds6_nbr_t, UIP_DS6_NBR_MAX_NEIGHBOR_CACHES);
#else
NBR_TABLE(uip_ds6_nbr_t, ds6_neighbors);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

/*---------------------------------------------------------------------------*/
void
uip_ds6_neighbors_init(void)
{
  link_stats_init();
#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  memb_init(&uip_ds6_nbr_memb);
  nbr_table_register(uip_ds6_nbr_entries,
                     (nbr_table_callback *)callback_nbr_entry_removal);
#else
  nbr_table_register(ds6_neighbors, (nbr_table_callback *)uip_ds6_nbr_rm);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
}
/*---------------------------------------------------------------------------*/
uip_ds6_nbr_t *
uip_ds6_nbr_add(const uip_ipaddr_t *ipaddr, const uip_lladdr_t *lladdr,
                uint8_t isrouter, uint8_t state, nbr_table_reason_t reason,
                void *data)
{
  uip_ds6_nbr_t *nbr;

#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  uip_ds6_nbr_entry_t *nbr_entry;

  assert(uip_ds6_nbr_lookup(ipaddr) == NULL);
  if(uip_ds6_nbr_lookup(ipaddr)) {
    LOG_ERR("%s: uip_ds6_nbr for ", __func__);
    LOG_ERR_6ADDR(ipaddr);
    LOG_ERR_("has already existed\n");
    return NULL;
  }

  /* firstly, allocate memory for a new nbr cache entry */
  if((nbr = (uip_ds6_nbr_t *)memb_alloc(&uip_ds6_nbr_memb)) == NULL) {
    LOG_ERR("%s: cannot allocate a new uip_ds6_nbr\n", __func__);
    return NULL;
  }

  /* secondly, get or allocate nbr_entry for the link-layer address */
  nbr_entry = nbr_table_get_from_lladdr(uip_ds6_nbr_entries,
                                        (const linkaddr_t *)lladdr);
  if(nbr_entry == NULL) {
    if((nbr_entry =
        nbr_table_add_lladdr(uip_ds6_nbr_entries,
                             (linkaddr_t*)lladdr, reason, data)) == NULL) {
      LOG_ERR("%s: cannot allocate a new uip_ds6_nbr_entry\n", __func__);
      /* return from this function later */
    } else {
      LIST_STRUCT_INIT(nbr_entry, uip_ds6_nbrs);
    }
  }

  /* free nbr and return if nbr_entry is not available */
  if((nbr_entry == NULL) ||
     (list_length(nbr_entry->uip_ds6_nbrs) == UIP_DS6_NBR_MAX_6ADDRS_PER_NBR)) {
    if(list_length(nbr_entry->uip_ds6_nbrs) == UIP_DS6_NBR_MAX_6ADDRS_PER_NBR) {
      /*
       * it's already had the maximum number of IPv6 addresses; cannot
       * add another.
       */
      LOG_ERR("%s: no room in nbr_entry for ", __func__);
      LOG_ERR_LLADDR((const linkaddr_t *)lladdr);
      LOG_ERR_("\n");
    }
    /* free the newly allocated memory in this function call */
    memb_free(&uip_ds6_nbr_memb, nbr);
    return NULL;
  } else {
    /* everything is fine; nbr is ready to be used */
    /* it has room to add another IPv6 address */
    add_uip_ds6_nbr_to_nbr_entry(nbr, nbr_entry);
  }
#else
  nbr = nbr_table_add_lladdr(ds6_neighbors, (linkaddr_t*)lladdr, reason, data);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

  if(nbr) {
#ifdef NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK
    NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK((const linkaddr_t *)lladdr, 1);
#endif /* NETSTACK_CONF_DS6_NEIGHBOR_ADDED_CALLBACK */
    uip_ipaddr_copy(&nbr->ipaddr, ipaddr);
#if UIP_ND6_SEND_RA || !UIP_CONF_ROUTER
    nbr->isrouter = isrouter;
#endif /* UIP_ND6_SEND_RA || !UIP_CONF_ROUTER */
    nbr->state = state;
#if UIP_CONF_IPV6_QUEUE_PKT
    uip_packetqueue_new(&nbr->packethandle);
#endif /* UIP_CONF_IPV6_QUEUE_PKT */
#if UIP_ND6_SEND_NS
    if(nbr->state == NBR_REACHABLE) {
      stimer_set(&nbr->reachable, UIP_ND6_REACHABLE_TIME / 1000);
    } else {
      /* We set the timer in expired state */
      stimer_set(&nbr->reachable, 0);
    }
    stimer_set(&nbr->sendns, 0);
    nbr->nscount = 0;
#endif /* UIP_ND6_SEND_NS */
    LOG_INFO("Adding neighbor with ip addr ");
    LOG_INFO_6ADDR(ipaddr);
    LOG_INFO_(" link addr ");
    LOG_INFO_LLADDR((linkaddr_t*)lladdr);
    LOG_INFO_(" state %u\n", state);
    NETSTACK_ROUTING.neighbor_state_changed(nbr);
    return nbr;
  } else {
    LOG_INFO("Add drop ip addr ");
    LOG_INFO_6ADDR(ipaddr);
    LOG_INFO_(" link addr (%p) ", lladdr);
    LOG_INFO_LLADDR((linkaddr_t*)lladdr);
    LOG_INFO_(" state %u\n", state);
    return NULL;
  }
}

#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
/*---------------------------------------------------------------------------*/
static void
add_uip_ds6_nbr_to_nbr_entry(uip_ds6_nbr_t *nbr,
                               uip_ds6_nbr_entry_t *nbr_entry)
{
  LOG_DBG("%s: add nbr(%p) to nbr_entry (%p)\n",
          __func__, nbr, nbr_entry);
  nbr->nbr_entry = nbr_entry;
  list_add(nbr_entry->uip_ds6_nbrs, nbr);
}
/*---------------------------------------------------------------------------*/
static void
remove_uip_ds6_nbr_from_nbr_entry(uip_ds6_nbr_t *nbr)
{
  if(nbr == NULL) {
    return;
  }
  LOG_DBG("%s: remove nbr(%p) from nbr_entry (%p)\n",
          __func__, nbr, nbr->nbr_entry);
  list_remove(nbr->nbr_entry->uip_ds6_nbrs, nbr);
}
/*---------------------------------------------------------------------------*/
static void
remove_nbr_entry(uip_ds6_nbr_entry_t *nbr_entry)
{
  if(nbr_entry == NULL) {
    return;
  }
  LOG_DBG("%s: remove nbr_entry (%p) from nbr_table\n",
          __func__, nbr_entry);
  (void)nbr_table_remove(uip_ds6_nbr_entries, nbr_entry);
}
/*---------------------------------------------------------------------------*/
static void
free_uip_ds6_nbr(uip_ds6_nbr_t *nbr)
{
  if(nbr == NULL) {
    return;
  }
#if UIP_CONF_IPV6_QUEUE_PKT
  uip_packetqueue_free(&nbr->packethandle);
#endif /* UIP_CONF_IPV6_QUEUE_PKT */
  NETSTACK_ROUTING.neighbor_state_changed(nbr);
  assert(nbr->nbr_entry != NULL);
  if(nbr->nbr_entry == NULL) {
    LOG_ERR("%s: unexpected error nbr->nbr_entry is NULL\n", __func__);
  } else {
    remove_uip_ds6_nbr_from_nbr_entry(nbr);
    if(list_length(nbr->nbr_entry->uip_ds6_nbrs) == 0) {
      remove_nbr_entry(nbr->nbr_entry);
    }
  }
  LOG_DBG("%s: free memory for nbr(%p)\n", __func__, nbr);
  memb_free(&uip_ds6_nbr_memb, nbr);
}
/*---------------------------------------------------------------------------*/
static void
callback_nbr_entry_removal(uip_ds6_nbr_entry_t *nbr_entry)
{
  uip_ds6_nbr_t *nbr;
  uip_ds6_nbr_t *next_nbr;
  if(nbr_entry == NULL) {
    return;
  }
  for(nbr = (uip_ds6_nbr_t *)list_head(nbr_entry->uip_ds6_nbrs);
      nbr != NULL;
      nbr = next_nbr) {
    next_nbr = (uip_ds6_nbr_t *)list_item_next(nbr);
    free_uip_ds6_nbr(nbr);
  }
}
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
/*---------------------------------------------------------------------------*/
int
uip_ds6_nbr_rm(uip_ds6_nbr_t *nbr)
{
  int ret;
  if(nbr == NULL) {
    return 0;
  }

#ifdef NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK
  linkaddr_t lladdr = {0};

  const uip_lladdr_t *plladdr = uip_ds6_nbr_get_ll(nbr);
  if(plladdr != NULL) {
    memcpy(&lladdr, plladdr, sizeof(lladdr));
  }
#endif /* NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK */

#if UIP_DS6_NBR_MULTI_IPV6_ADDRS

  free_uip_ds6_nbr(nbr);
  ret = 1;

#else /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

#if UIP_CONF_IPV6_QUEUE_PKT
  uip_packetqueue_free(&nbr->packethandle);
#endif /* UIP_CONF_IPV6_QUEUE_PKT */

  NETSTACK_ROUTING.neighbor_state_changed(nbr);
  ret = nbr_table_remove(ds6_neighbors, nbr);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

#ifdef NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK
  NETSTACK_CONF_DS6_NEIGHBOR_UPDATED_CALLBACK(&lladdr, 0);
#endif /* NETSTACK_CONF_DS6_NEIGHBOR_ADDED_CALLBACK */

  return ret;
}

/*---------------------------------------------------------------------------*/
int
uip_ds6_nbr_update_ll(uip_ds6_nbr_t **nbr_pp, const uip_lladdr_t *new_ll_addr)
{
#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  uip_ds6_nbr_entry_t *nbr_entry;
  uip_ds6_nbr_t *nbr;
#else
  uip_ds6_nbr_t nbr_backup;
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

  if(nbr_pp == NULL || new_ll_addr == NULL) {
    LOG_ERR("%s: invalid argument\n", __func__);
    return -1;
  }

#if UIP_DS6_NBR_MULTI_IPV6_ADDRS

  if((nbr_entry =
      nbr_table_get_from_lladdr(uip_ds6_nbr_entries,
                                (const linkaddr_t *)new_ll_addr)) == NULL) {
    if((nbr_entry =
        nbr_table_add_lladdr(uip_ds6_nbr_entries,
                             (const linkaddr_t*)new_ll_addr,
                             NBR_TABLE_REASON_IPV6_ND, NULL)) == NULL) {
      LOG_ERR("%s: cannot allocate a nbr_entry for", __func__);
      LOG_ERR_LLADDR((const linkaddr_t *)new_ll_addr);
      return -1;
    } else {
      LIST_STRUCT_INIT(nbr_entry, uip_ds6_nbrs);
    }
  }

  nbr = *nbr_pp;

  remove_uip_ds6_nbr_from_nbr_entry(nbr);
  if(list_length(nbr->nbr_entry->uip_ds6_nbrs) == 0) {
    remove_nbr_entry(nbr->nbr_entry);
  }
  add_uip_ds6_nbr_to_nbr_entry(nbr, nbr_entry);

#else /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

  /* make sure new_ll_addr is not used in some other nbr */
  if(uip_ds6_nbr_ll_lookup(new_ll_addr) != NULL) {
    LOG_ERR("%s: new_ll_addr, ", __func__);
    LOG_ERR_LLADDR((const linkaddr_t *)new_ll_addr);
    LOG_ERR_(", is already used in another nbr\n");
    return -1;
  }

  memcpy(&nbr_backup, *nbr_pp, sizeof(uip_ds6_nbr_t));
  if(uip_ds6_nbr_rm(*nbr_pp) == 0) {
    LOG_ERR("%s: input nbr cannot be removed\n", __func__);
    return -1;
  }

  if((*nbr_pp = uip_ds6_nbr_add(&nbr_backup.ipaddr, new_ll_addr,
                                nbr_backup.isrouter, nbr_backup.state,
                                NBR_TABLE_REASON_IPV6_ND, NULL)) == NULL) {
    LOG_ERR("%s: cannot allocate a new nbr for new_ll_addr\n", __func__);
    return -1;
  }
  memcpy(*nbr_pp, &nbr_backup, sizeof(uip_ds6_nbr_t));
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */

  return 0;
}
/*---------------------------------------------------------------------------*/
const uip_ipaddr_t *
uip_ds6_nbr_get_ipaddr(const uip_ds6_nbr_t *nbr)
{
  return (nbr != NULL) ? &nbr->ipaddr : NULL;
}

/*---------------------------------------------------------------------------*/
const uip_lladdr_t *
uip_ds6_nbr_get_ll(const uip_ds6_nbr_t *nbr)
{
#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  if(nbr == NULL) {
    return NULL;
  }
  return (const uip_lladdr_t *)nbr_table_get_lladdr(uip_ds6_nbr_entries,
                                                    nbr->nbr_entry);
#else
  return (const uip_lladdr_t *)nbr_table_get_lladdr(ds6_neighbors, nbr);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
}
/*---------------------------------------------------------------------------*/
int
uip_ds6_nbr_num(void)
{
  int num = 0;

#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  uip_ds6_nbr_entry_t *nbr_entry;
  for(nbr_entry = nbr_table_head(uip_ds6_nbr_entries);
      nbr_entry != NULL;
      nbr_entry = nbr_table_next(uip_ds6_nbr_entries, nbr_entry)) {
    num += list_length(nbr_entry->uip_ds6_nbrs);
  }
#else
  uip_ds6_nbr_t *nbr;
  for(nbr = nbr_table_head(ds6_neighbors);
      nbr != NULL;
      nbr = nbr_table_next(ds6_neighbors, nbr)) {
    num++;
  }
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
  return num;
}
/*---------------------------------------------------------------------------*/
uip_ds6_nbr_t *
uip_ds6_nbr_head(void)
{
#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  uip_ds6_nbr_entry_t *nbr_entry;
  if((nbr_entry = nbr_table_head(uip_ds6_nbr_entries)) == NULL) {
    return NULL;
  }
  assert(list_head(nbr_entry->uip_ds6_nbrs) != NULL);
  return (uip_ds6_nbr_t *)list_head(nbr_entry->uip_ds6_nbrs);
#else
  return nbr_table_head(ds6_neighbors);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
}
/*---------------------------------------------------------------------------*/
uip_ds6_nbr_t *
uip_ds6_nbr_next(uip_ds6_nbr_t *nbr)
{
#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  uip_ds6_nbr_entry_t *nbr_entry;
  if(nbr == NULL) {
    return NULL;
  }
  if(list_item_next(nbr) != NULL) {
    return list_item_next(nbr);
  }
  nbr_entry = nbr_table_next(uip_ds6_nbr_entries, nbr->nbr_entry);
  if(nbr_entry == NULL) {
    return NULL;
  } else {
    assert(list_head(nbr_entry->uip_ds6_nbrs) != NULL);
    return (uip_ds6_nbr_t *)list_head(nbr_entry->uip_ds6_nbrs);
  }
#else
  return nbr_table_next(ds6_neighbors, nbr);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
}
/*---------------------------------------------------------------------------*/
uip_ds6_nbr_t *
uip_ds6_nbr_lookup(const uip_ipaddr_t *ipaddr)
{
  uip_ds6_nbr_t *nbr;
  if(ipaddr == NULL) {
    return NULL;
  }
  for(nbr = uip_ds6_nbr_head(); nbr != NULL; nbr = uip_ds6_nbr_next(nbr)) {
    if(uip_ipaddr_cmp(&nbr->ipaddr, ipaddr)) {
      return nbr;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
uip_ds6_nbr_t *
uip_ds6_nbr_ll_lookup(const uip_lladdr_t *lladdr)
{
#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
  uip_ds6_nbr_entry_t *nbr_entry;
  /*
   * we cannot determine which entry should return by lladdr alone;
   * return the first entry associated with lladdr.
   */
  nbr_entry =
    (uip_ds6_nbr_entry_t *)nbr_table_get_from_lladdr(uip_ds6_nbr_entries,
                                                     (linkaddr_t*)lladdr);
  if(nbr_entry == NULL) {
    return NULL;
  }
  assert(list_head(nbr_entry->uip_ds6_nbrs) != NULL);
  return (uip_ds6_nbr_t *)list_head(nbr_entry->uip_ds6_nbrs);
#else
  return nbr_table_get_from_lladdr(ds6_neighbors, (linkaddr_t*)lladdr);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
}

/*---------------------------------------------------------------------------*/
uip_ipaddr_t *
uip_ds6_nbr_ipaddr_from_lladdr(const uip_lladdr_t *lladdr)
{
  uip_ds6_nbr_t *nbr = uip_ds6_nbr_ll_lookup(lladdr);
  return nbr ? &nbr->ipaddr : NULL;
}

/*---------------------------------------------------------------------------*/
const uip_lladdr_t *
uip_ds6_nbr_lladdr_from_ipaddr(const uip_ipaddr_t *ipaddr)
{
  uip_ds6_nbr_t *nbr = uip_ds6_nbr_lookup(ipaddr);
  return nbr ? uip_ds6_nbr_get_ll(nbr) : NULL;
}
#if UIP_DS6_LL_NUD
/*---------------------------------------------------------------------------*/
static void
update_nbr_reachable_state_by_ack(uip_ds6_nbr_t *nbr, const linkaddr_t *lladdr)
{
  if(nbr != NULL && nbr->state != NBR_INCOMPLETE) {
    nbr->state = NBR_REACHABLE;
    stimer_set(&nbr->reachable, UIP_ND6_REACHABLE_TIME / 1000);
    LOG_INFO("received a link layer ACK : ");
    LOG_INFO_LLADDR(lladdr);
    LOG_INFO_(" is reachable.\n");
  }
}
#endif /* UIP_DS6_LL_NUD */
/*---------------------------------------------------------------------------*/
void
uip_ds6_link_callback(int status, int numtx)
{
#if UIP_DS6_LL_NUD
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(linkaddr_cmp(dest, &linkaddr_null)) {
    return;
  }

  /* From RFC4861, page 72, last paragraph of section 7.3.3:
   *
   *         "In some cases, link-specific information may indicate that a path to
   *         a neighbor has failed (e.g., the resetting of a virtual circuit). In
   *         such cases, link-specific information may be used to purge Neighbor
   *         Cache entries before the Neighbor Unreachability Detection would do
   *         so. However, link-specific information MUST NOT be used to confirm
   *         the reachability of a neighbor; such information does not provide
   *         end-to-end confirmation between neighboring IP layers."
   *
   * However, we assume that receiving a link layer ack ensures the delivery
   * of the transmitted packed to the IP stack of the neighbour. This is a
   * fair assumption and allows battery powered nodes save some battery by
   * not re-testing the state of a neighbour periodically if it
   * acknowledges link packets. */
  if(status == MAC_TX_OK) {
    uip_ds6_nbr_t *nbr;
#if UIP_DS6_NBR_MULTI_IPV6_ADDRS
    uip_ds6_nbr_entry_t *nbr_entry;
    if((nbr_entry =
        (uip_ds6_nbr_entry_t *)nbr_table_get_from_lladdr(uip_ds6_nbr_entries,
                                                         dest)) == NULL) {
      return;
    }
    for(nbr = (uip_ds6_nbr_t *)list_head(nbr_entry->uip_ds6_nbrs);
        nbr != NULL;
        nbr = (uip_ds6_nbr_t *)list_item_next(nbr)) {
      update_nbr_reachable_state_by_ack(nbr, dest);
    }
#else /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
    nbr = uip_ds6_nbr_ll_lookup((uip_lladdr_t *)dest);
    update_nbr_reachable_state_by_ack(nbr, dest);
#endif /* UIP_DS6_NBR_MULTI_IPV6_ADDRS */
  }
#endif /* UIP_DS6_LL_NUD */
}
#if UIP_ND6_SEND_NS
/*---------------------------------------------------------------------------*/
/** Periodic processing on neighbors */
void
uip_ds6_neighbor_periodic(void)
{
  uip_ds6_nbr_t *nbr = uip_ds6_nbr_head();
  while(nbr != NULL) {
    switch(nbr->state) {
    case NBR_REACHABLE:
      if(stimer_expired(&nbr->reachable)) {
#if UIP_CONF_ROUTER
        /* when a neighbor leave its REACHABLE state and is a default router,
           instead of going to STALE state it enters DELAY state in order to
           force a NUD on it. Otherwise, if there is no upward traffic, the
           node never knows if the default router is still reachable. This
           mimics the 6LoWPAN-ND behavior.
         */
        if(uip_ds6_defrt_lookup(&nbr->ipaddr) != NULL) {
          LOG_INFO("REACHABLE: defrt moving to DELAY (");
          LOG_INFO_6ADDR(&nbr->ipaddr);
          LOG_INFO_(")\n");
          nbr->state = NBR_DELAY;
          stimer_set(&nbr->reachable, UIP_ND6_DELAY_FIRST_PROBE_TIME);
          nbr->nscount = 0;
        } else {
          LOG_INFO("REACHABLE: moving to STALE (");
          LOG_INFO_6ADDR(&nbr->ipaddr);
          LOG_INFO_(")\n");
          nbr->state = NBR_STALE;
        }
#else /* UIP_CONF_ROUTER */
        LOG_INFO("REACHABLE: moving to STALE (");
        LOG_INFO_6ADDR(&nbr->ipaddr);
        LOG_INFO_(")\n");
        nbr->state = NBR_STALE;
#endif /* UIP_CONF_ROUTER */
      }
      break;
    case NBR_INCOMPLETE:
      if(nbr->nscount >= UIP_ND6_MAX_MULTICAST_SOLICIT) {
        uip_ds6_nbr_rm(nbr);
      } else if(stimer_expired(&nbr->sendns) && (uip_len == 0)) {
        nbr->nscount++;
        LOG_INFO("NBR_INCOMPLETE: NS %u\n", nbr->nscount);
        uip_nd6_ns_output(NULL, NULL, &nbr->ipaddr);
        stimer_set(&nbr->sendns, uip_ds6_if.retrans_timer / 1000);
      }
      break;
    case NBR_DELAY:
      if(stimer_expired(&nbr->reachable)) {
        nbr->state = NBR_PROBE;
        nbr->nscount = 0;
        LOG_INFO("DELAY: moving to PROBE\n");
        stimer_set(&nbr->sendns, 0);
      }
      break;
    case NBR_PROBE:
      if(nbr->nscount >= UIP_ND6_MAX_UNICAST_SOLICIT) {
        uip_ds6_defrt_t *locdefrt;
        LOG_INFO("PROBE END\n");
        if((locdefrt = uip_ds6_defrt_lookup(&nbr->ipaddr)) != NULL) {
          if (!locdefrt->isinfinite) {
            uip_ds6_defrt_rm(locdefrt);
          }
        }
        uip_ds6_nbr_rm(nbr);
      } else if(stimer_expired(&nbr->sendns) && (uip_len == 0)) {
        nbr->nscount++;
        LOG_INFO("PROBE: NS %u\n", nbr->nscount);
        uip_nd6_ns_output(NULL, &nbr->ipaddr, &nbr->ipaddr);
        stimer_set(&nbr->sendns, uip_ds6_if.retrans_timer / 1000);
      }
      break;
    default:
      break;
    }
    nbr = uip_ds6_nbr_next(nbr);
  }
}
/*---------------------------------------------------------------------------*/
void
uip_ds6_nbr_refresh_reachable_state(const uip_ipaddr_t *ipaddr)
{
  uip_ds6_nbr_t *nbr;
  nbr = uip_ds6_nbr_lookup(ipaddr);
  if(nbr != NULL) {
    nbr->state = NBR_REACHABLE;
    nbr->nscount = 0;
    stimer_set(&nbr->reachable, UIP_ND6_REACHABLE_TIME / 1000);
  }
}
#endif /* UIP_ND6_SEND_NS */
/*---------------------------------------------------------------------------*/
/** @} */
