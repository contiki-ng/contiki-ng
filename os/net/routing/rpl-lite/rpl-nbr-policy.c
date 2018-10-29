/*
 * Copyright (c) 2014-2015, Yanzi Networks AB.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holders nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *
 * Default RPL NBR policy
 * decides when to add a new discovered node to the nbr table from RPL.
 *
 * \author Joakim Eriksson <joakime@sics.se>
 * Contributors: Niclas Finne <nfi@sics.se>, Oriol Piñol <oriol@yanzi.se>,
 *
 */

#include "net/routing/rpl-lite/rpl.h"
#include "net/nbr-table.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

/*
 * Policy for neighbor addition
 * - one node is locked (default route)
 * - max X "best parents"
 * => at least MAX_NBRS - (X + 1) free slots for other.
 *
 * NOTE: this policy assumes that all neighbors end up being IPv6
 * neighbors and are not only MAC neighbors.
 */

static int num_parents;   /* all nodes that are possible parents */
static int num_free;
static const linkaddr_t *worst_rank_nbr_lladdr; /* lladdr of the the neighbor with the worst rank */
static rpl_rank_t worst_rank;

/*---------------------------------------------------------------------------*/
static void
update_state(void)
{
  uip_ds6_nbr_t *ds6_nbr;
  rpl_nbr_t *rpl_nbr;
  rpl_rank_t nbr_rank;
  int num_used = 0;

  worst_rank = 0;
  worst_rank_nbr_lladdr = NULL;
  num_parents = 0;

  ds6_nbr = uip_ds6_nbr_head();
  while(ds6_nbr != NULL) {

    const linkaddr_t *nbr_lladdr = (const linkaddr_t *)uip_ds6_nbr_get_ll(ds6_nbr);
    rpl_nbr = rpl_neighbor_get_from_lladdr((uip_lladdr_t *)nbr_lladdr);

    if(rpl_nbr != NULL && rpl_neighbor_is_parent(rpl_nbr)) {
      num_parents++;
    }

    nbr_rank = rpl_neighbor_rank_via_nbr(rpl_nbr);
    /* Select worst-rank neighbor */
    if(rpl_nbr != curr_instance.dag.preferred_parent
       && nbr_rank > worst_rank) {
      /* This is the worst-rank neighbor - this is a good candidate for removal */
      worst_rank = nbr_rank;
      worst_rank_nbr_lladdr = nbr_lladdr;
    }

    ds6_nbr = uip_ds6_nbr_next(ds6_nbr);
    num_used++;
  }
  /* how many more IP neighbors can be have? */
  num_free = NBR_TABLE_MAX_NEIGHBORS - num_used;

  LOG_DBG("nbr-policy: free: %d, parents: %d\n", num_free, num_parents);
}
/*---------------------------------------------------------------------------*/
static const linkaddr_t *
find_worst_rank_nbr_lladdr(void)
{
  update_state();
  return worst_rank_nbr_lladdr;
}
/*---------------------------------------------------------------------------*/
static const linkaddr_t *
find_removable_dio(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  update_state();

  if(!curr_instance.used || curr_instance.instance_id != dio->instance_id) {
    LOG_WARN("nbr-policy: did not find instance id: %d\n", dio->instance_id);
    return NULL;
  }

  /* Add the new neighbor only if it is better than the current worst. */
  if(dio->rank + curr_instance.min_hoprankinc < worst_rank - curr_instance.min_hoprankinc / 2) {
    /* Found *great* neighbor - add! */
    LOG_DBG("nbr-policy: DIO rank %u, worst_rank %u -- add to cache\n",
           dio->rank, worst_rank);
    return worst_rank_nbr_lladdr;
  }

  LOG_DBG("nbr-policy: DIO rank %u, worst_rank %u -- do not add to cache\n",
         dio->rank, worst_rank);
  return NULL;
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
rpl_nbr_policy_find_removable(nbr_table_reason_t reason, void *data)
{
  /* When we get the DIO/DAO/DIS we know that UIP contains the
     incoming packet */
  switch(reason) {
    case NBR_TABLE_REASON_RPL_DIO:
      return find_removable_dio(&UIP_IP_BUF->srcipaddr, data);
    case NBR_TABLE_REASON_RPL_DIS:
      return find_worst_rank_nbr_lladdr();
    case NBR_TABLE_REASON_IPV6_ND_AUTOFILL:
      return find_worst_rank_nbr_lladdr();
    default:
      return NULL;
  }
}
/*---------------------------------------------------------------------------*/
/** @}*/
