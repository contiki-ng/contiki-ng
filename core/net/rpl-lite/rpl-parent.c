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
 * \file
 *         Logic for DAG parents in RPL.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 * Simon Duquennoy <simon.duquennoy@inria.fr>
 * Contributors: George Oikonomou <oikonomou@users.sourceforge.net> (multicast)
 */

/**
 * \addtogroup uip6
 * @{
 */

#include "contiki.h"
#include "rpl.h"
#include "net/link-stats.h"
#include "net/nbr-table.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

/* A configurable function called after every RPL parent switch */
#ifdef RPL_CALLBACK_PARENT_SWITCH
void RPL_CALLBACK_PARENT_SWITCH(rpl_parent_t *old, rpl_parent_t *new);
#endif /* RPL_CALLBACK_PARENT_SWITCH */

/*---------------------------------------------------------------------------*/
/* Per-parent RPL information */
NBR_TABLE_GLOBAL(rpl_parent_t, rpl_parents);

/*---------------------------------------------------------------------------*/
static int
acceptable_rank(rpl_rank_t rank)
{
  return rank != RPL_INFINITE_RANK
      && rank >= ROOT_RANK
      && ((curr_instance.max_rankinc == 0) ||
          DAG_RANK(rank) <=
          DAG_RANK(curr_instance.dag.lowest_rank + curr_instance.max_rankinc));
}
/*---------------------------------------------------------------------------*/
void
rpl_parent_print_list(const char *str)
{
  if(curr_instance.used) {
    int curr_dio_interval = curr_instance.dag.dio_intcurrent;
    int curr_rank = curr_instance.dag.rank;
    rpl_parent_t *p = nbr_table_head(rpl_parents);
    clock_time_t clock_now = clock_time();

    printf("RPL nbr: MOP %u OCP %u rank %u dioint %u, DS6 nbr count %u (%s)\n",
        curr_instance.mop, curr_instance.of->ocp, curr_rank,
        curr_dio_interval, uip_ds6_nbr_num(), str);
    while(p != NULL) {
      const struct link_stats *stats = rpl_parent_get_link_stats(p);
      printf("RPL nbr: %3u %5u, %5u => %5u -- %2u %c%c%c (last tx %u min ago)\n",
          rpl_parent_get_ipaddr(p)->u8[15],
          p->rank,
          rpl_parent_get_link_metric(p),
          rpl_parent_rank_via_parent(p),
          stats != NULL ? stats->freshness : 0,
          (acceptable_rank(rpl_parent_rank_via_parent(p)) && curr_instance.of->parent_is_acceptable(p)) ? 'a' : ' ',
          link_stats_is_fresh(stats) ? 'f' : ' ',
          p == curr_instance.dag.preferred_parent ? 'p' : ' ',
          (unsigned)((clock_now - stats->last_tx_time) / (60 * CLOCK_SECOND))
      );
      p = nbr_table_next(rpl_parents, p);
    }
    printf("RPL nbr: end of list\n");
  }
}
/*---------------------------------------------------------------------------*/
#if UIP_ND6_SEND_NS
static uip_ds6_nbr_t *
rpl_get_ds6_nbr(rpl_parent_t *parent)
{
  const linkaddr_t *lladdr = rpl_parent_get_lladdr(parent);
  if(lladdr != NULL) {
    return nbr_table_get_from_lladdr(ds6_neighbors, lladdr);
  } else {
    return NULL;
  }
}
#endif /* UIP_ND6_SEND_NS */
/*---------------------------------------------------------------------------*/
static void
remove_parent(rpl_parent_t *parent)
{
  /* Make sure we don't point to a removed parent. Note that we do not need
  to worry about preferred_parent here, as it is locked in the the table
  and will never be removed by external modules. */
  if(parent == curr_instance.dag.urgent_probing_target) {
    curr_instance.dag.urgent_probing_target = NULL;
  }
  if(parent == curr_instance.dag.unicast_dio_target) {
    curr_instance.dag.unicast_dio_target = NULL;
  }
  nbr_table_remove(rpl_parents, parent);
  rpl_timers_schedule_state_update(); /* Updating from here is unsafe; postpone */
}
/*---------------------------------------------------------------------------*/
rpl_parent_t *
rpl_parent_get_from_lladdr(uip_lladdr_t *addr)
{
  return nbr_table_get_from_lladdr(rpl_parents, (linkaddr_t *)addr);
}
/*---------------------------------------------------------------------------*/
uint16_t
rpl_parent_get_link_metric(rpl_parent_t *p)
{
  if(p != NULL && curr_instance.of->parent_link_metric != NULL) {
    return curr_instance.of->parent_link_metric(p);
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
rpl_rank_t
rpl_parent_rank_via_parent(rpl_parent_t *p)
{
  if(p != NULL && curr_instance.of->rank_via_parent != NULL) {
    return curr_instance.of->rank_via_parent(p);
  }
  return RPL_INFINITE_RANK;
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
rpl_parent_get_lladdr(rpl_parent_t *p)
{
  return nbr_table_get_lladdr(rpl_parents, p);
}
/*---------------------------------------------------------------------------*/
uip_ipaddr_t *
rpl_parent_get_ipaddr(rpl_parent_t *p)
{
  const linkaddr_t *lladdr = rpl_parent_get_lladdr(p);
  return uip_ds6_nbr_ipaddr_from_lladdr((uip_lladdr_t *)lladdr);
}
/*---------------------------------------------------------------------------*/
const struct link_stats *
rpl_parent_get_link_stats(rpl_parent_t *p)
{
  const linkaddr_t *lladdr = rpl_parent_get_lladdr(p);
  return link_stats_from_lladdr(lladdr);
}
/*---------------------------------------------------------------------------*/
int
rpl_parent_is_fresh(rpl_parent_t *p)
{
  const struct link_stats *stats = rpl_parent_get_link_stats(p);
  return link_stats_is_fresh(stats);
}
/*---------------------------------------------------------------------------*/
int
rpl_parent_is_reachable(rpl_parent_t *p) {
  if(p == NULL) {
    return 0;
  } else {
#if UIP_ND6_SEND_NS
    uip_ds6_nbr_t *nbr = rpl_get_ds6_nbr(p);
    /* Exclude links to a neighbor that is not reachable at a NUD level */
    if(nbr == NULL || nbr->state != NBR_REACHABLE) {
      return 0;
    }
#endif /* UIP_ND6_SEND_NS */
    /* If we don't have fresh link information, assume the parent is reachable. */
    return !rpl_parent_is_fresh(p) || curr_instance.of->parent_has_usable_link(p);
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_parent_set_preferred(rpl_parent_t *p)
{
  if(curr_instance.dag.preferred_parent != p) {
    PRINTF("RPL: parent switch ");
    if(p != NULL) {
      PRINT6ADDR(rpl_parent_get_ipaddr(p));
    } else {
      PRINTF("NULL");
    }
    PRINTF(" used to be ");
    if(curr_instance.dag.preferred_parent != NULL) {
      PRINT6ADDR(rpl_parent_get_ipaddr(curr_instance.dag.preferred_parent));
    } else {
      PRINTF("NULL");
    }
    PRINTF("\n");

#ifdef RPL_CALLBACK_PARENT_SWITCH
    RPL_CALLBACK_PARENT_SWITCH(curr_instance.dag.preferred_parent, p);
#endif /* RPL_CALLBACK_PARENT_SWITCH */

    /* Always keep the preferred parent locked, so it remains in the
     * neighbor table. */
    nbr_table_unlock(rpl_parents, curr_instance.dag.preferred_parent);
    nbr_table_lock(rpl_parents, p);

    /* Update DS6 default route. Use an infinite lifetime */
    uip_ds6_defrt_rm(uip_ds6_defrt_lookup(
      rpl_parent_get_ipaddr(curr_instance.dag.preferred_parent)));
    uip_ds6_defrt_add(rpl_parent_get_ipaddr(p), 0);

    curr_instance.dag.preferred_parent = p;
  }
}
/*---------------------------------------------------------------------------*/
/* Remove DAG parents with a rank that is at least the same as minimum_rank. */
void
rpl_parent_remove_all(void)
{
  rpl_parent_t *p;

  PRINTF("RPL: removing all parents\n");

  p = nbr_table_head(rpl_parents);
  while(p != NULL) {
    remove_parent(p);
    p = nbr_table_next(rpl_parents, p);
  }

  /* Update needed immediately so as to ensure preferred_parent becomes NULL,
   * and no longer points to a de-allocated parent. */
  rpl_dag_update_state();
}
/*---------------------------------------------------------------------------*/
rpl_parent_t *
rpl_parent_get_from_ipaddr(uip_ipaddr_t *addr)
{
  uip_ds6_nbr_t *ds6_nbr = uip_ds6_nbr_lookup(addr);
  const uip_lladdr_t *lladdr = uip_ds6_nbr_get_ll(ds6_nbr);
  return nbr_table_get_from_lladdr(rpl_parents, (linkaddr_t *)lladdr);
}
/*---------------------------------------------------------------------------*/
static rpl_parent_t *
best_parent(int fresh_only)
{
  rpl_parent_t *p;
  rpl_parent_t *best = NULL;

  if(curr_instance.used == 0) {
    return NULL;
  }

  /* Search for the best parent according to the OF */
  for(p = nbr_table_head(rpl_parents); p != NULL; p = nbr_table_next(rpl_parents, p)) {

    if(!acceptable_rank(p->rank) || !curr_instance.of->parent_is_acceptable(p)) {
      /* Exclude parents with a rank that is not acceptable) */
      continue;
    }

    if(fresh_only && !rpl_parent_is_fresh(p)) {
      /* Filter out non-fresh parents if fresh_only is set */
      continue;
    }

#if UIP_ND6_SEND_NS
    {
    uip_ds6_nbr_t *nbr = rpl_get_ds6_nbr(p);
    /* Exclude links to a neighbor that is not reachable at a NUD level */
    if(nbr == NULL || nbr->state != NBR_REACHABLE) {
      continue;
    }
    }
#endif /* UIP_ND6_SEND_NS */

    /* Now we have an acceptable parent, check if it is the new best */
    best = curr_instance.of->best_parent(best, p);
  }

  return best;
}
/*---------------------------------------------------------------------------*/
rpl_parent_t *
rpl_parent_select_best(void)
{
  rpl_parent_t *best;

  if(rpl_dag_root_is_root()) {
    return NULL; /* The root has no parent */
  }

  /* Look for best parent (regardless of freshness) */
  best = best_parent(0);

#if RPL_WITH_PROBING
  if(best != NULL) {
    if(rpl_parent_is_fresh(best)) {
      return best;
    } else {
      /* The best is not fresh. Look for the best fresh now. */
      rpl_parent_t *best_fresh = best_parent(1);
      if(best_fresh == NULL) {
        /* No fresh parent around, select best (non-fresh) */
        return best;
      } else {
        /* Select best fresh */
        return best_fresh;
      }
      /* Probe the best parent shortly in order to get a fresh estimate */
      curr_instance.dag.urgent_probing_target = best;
      rpl_schedule_probing();
      /* Stick to current preferred parent until a better one is fresh */
      return curr_instance.dag.preferred_parent;
    }
  } else {
    return NULL;
  }
#else /* RPL_WITH_PROBING */
  return best;
#endif /* RPL_WITH_PROBING */
}
/*---------------------------------------------------------------------------*/
void
rpl_parent_init(void)
{
  nbr_table_register(rpl_parents, (nbr_table_callback *)remove_parent);
}
/** @} */
