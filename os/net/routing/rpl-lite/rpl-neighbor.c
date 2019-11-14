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
 *         Logic for DAG neighbors in RPL.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 * Simon Duquennoy <simon.duquennoy@inria.fr>
 * Contributors: George Oikonomou <oikonomou@users.sourceforge.net> (multicast)
 */

#include "contiki.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/link-stats.h"
#include "net/nbr-table.h"
#include "net/ipv6/uiplib.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

/* A configurable function called after every RPL parent switch */
#ifdef RPL_CALLBACK_PARENT_SWITCH
void RPL_CALLBACK_PARENT_SWITCH(rpl_nbr_t *old, rpl_nbr_t *new);
#endif /* RPL_CALLBACK_PARENT_SWITCH */

static rpl_nbr_t * best_parent(int fresh_only);

/*---------------------------------------------------------------------------*/
/* Per-neighbor RPL information */
NBR_TABLE_GLOBAL(rpl_nbr_t, rpl_neighbors);

/*---------------------------------------------------------------------------*/
static int
max_acceptable_rank(void)
{
  if(curr_instance.max_rankinc == 0) {
    /* There is no max rank increment */
    return RPL_INFINITE_RANK;
  } else {
    /* Make sure not to exceed RPL_INFINITE_RANK */
    return MIN((uint32_t)curr_instance.dag.lowest_rank + curr_instance.max_rankinc, RPL_INFINITE_RANK);
  }
}
/*---------------------------------------------------------------------------*/
/* As per RFC 6550, section 8.2.2.4 */
static int
acceptable_rank(rpl_rank_t rank)
{
  return rank != RPL_INFINITE_RANK
      && rank >= ROOT_RANK
      && rank <= max_acceptable_rank();
}
/*---------------------------------------------------------------------------*/
int
rpl_neighbor_snprint(char *buf, int buflen, rpl_nbr_t *nbr)
{
  int index = 0;
  rpl_nbr_t *best = best_parent(0);
  const struct link_stats *stats = rpl_neighbor_get_link_stats(nbr);
  clock_time_t clock_now = clock_time();

  if(LOG_WITH_COMPACT_ADDR) {
    index += log_6addr_compact_snprint(buf+index, buflen-index, rpl_neighbor_get_ipaddr(nbr));
  } else {
    index += uiplib_ipaddr_snprint(buf+index, buflen-index, rpl_neighbor_get_ipaddr(nbr));
  }
  if(index >= buflen) {
    return index;
  }
  index += snprintf(buf+index, buflen-index,
      "%5u, %5u => %5u -- %2u %c%c%c%c%c",
      nbr->rank,
      rpl_neighbor_get_link_metric(nbr),
      rpl_neighbor_rank_via_nbr(nbr),
      stats != NULL ? stats->freshness : 0,
      (nbr->rank == ROOT_RANK) ? 'r' : ' ',
      nbr == best ? 'b' : ' ',
      (acceptable_rank(rpl_neighbor_rank_via_nbr(nbr)) && rpl_neighbor_is_acceptable_parent(nbr)) ? 'a' : ' ',
      link_stats_is_fresh(stats) ? 'f' : ' ',
      nbr == curr_instance.dag.preferred_parent ? 'p' : ' '
  );
  if(index >= buflen) {
    return index;
  }
  if(stats != NULL && stats->last_tx_time > 0) {
    index += snprintf(buf+index, buflen-index,
                              " (last tx %u min ago",
                              (unsigned)((clock_now - stats->last_tx_time) / (60 * CLOCK_SECOND)));
  } else {
    index += snprintf(buf+index, buflen-index,
                              " (no tx");
  }
  if(index >= buflen) {
    return index;
  }
  if(nbr->better_parent_since > 0) {
    index += snprintf(buf+index, buflen-index,
                              ", better since %u min)",
                              (unsigned)((clock_now - nbr->better_parent_since) / (60 * CLOCK_SECOND)));
  } else {
    index += snprintf(buf+index, buflen-index,
                              ")");
  }
  return index;
}
/*---------------------------------------------------------------------------*/
void
rpl_neighbor_print_list(const char *str)
{
  if(curr_instance.used) {
    int curr_dio_interval = curr_instance.dag.dio_intcurrent;
    int curr_rank = curr_instance.dag.rank;
    rpl_nbr_t *nbr = nbr_table_head(rpl_neighbors);

    LOG_INFO("nbr: own state, addr ");
    LOG_INFO_6ADDR(rpl_get_global_address());
    LOG_INFO_(", DAG state: %s, MOP %u OCP %u rank %u max-rank %u, dioint %u, nbr count %u (%s)\n",
        rpl_dag_state_to_str(curr_instance.dag.state),
        curr_instance.mop, curr_instance.of->ocp, curr_rank,
        max_acceptable_rank(),
        curr_dio_interval, rpl_neighbor_count(), str);
    while(nbr != NULL) {
      char buf[120];
      rpl_neighbor_snprint(buf, sizeof(buf), nbr);
      LOG_INFO("nbr: %s\n", buf);
      nbr = nbr_table_next(rpl_neighbors, nbr);
    }
    LOG_INFO("nbr: end of list\n");
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_neighbor_count(void)
{
  int count = 0;
  rpl_nbr_t *nbr = nbr_table_head(rpl_neighbors);
  for(nbr = nbr_table_head(rpl_neighbors);
      nbr != NULL;
      nbr = nbr_table_next(rpl_neighbors, nbr)) {
    count++;
  }
  return count;
}
/*---------------------------------------------------------------------------*/
#if UIP_ND6_SEND_NS
static uip_ds6_nbr_t *
rpl_get_ds6_nbr(rpl_nbr_t *nbr)
{
  const uip_lladdr_t *lladdr = (const uip_lladdr_t *)rpl_neighbor_get_lladdr(nbr);
  if(lladdr != NULL) {
    return uip_ds6_nbr_ll_lookup(lladdr);
  } else {
    return NULL;
  }
}
#endif /* UIP_ND6_SEND_NS */
/*---------------------------------------------------------------------------*/
static void
remove_neighbor(rpl_nbr_t *nbr)
{
  /* Make sure we don't point to a removed neighbor. Note that we do not need
  to worry about preferred_parent here, as it is locked in the the table
  and will never be removed by external modules. */
#if RPL_WITH_PROBING
  if(nbr == curr_instance.dag.urgent_probing_target) {
    curr_instance.dag.urgent_probing_target = NULL;
  }
#endif

  if(nbr == curr_instance.dag.unicast_dio_target) {
    curr_instance.dag.unicast_dio_target = NULL;
  }
  nbr_table_remove(rpl_neighbors, nbr);
  rpl_timers_schedule_state_update(); /* Updating from here is unsafe; postpone */
}
/*---------------------------------------------------------------------------*/
rpl_nbr_t *
rpl_neighbor_get_from_lladdr(uip_lladdr_t *addr)
{
  return nbr_table_get_from_lladdr(rpl_neighbors, (linkaddr_t *)addr);
}
/*---------------------------------------------------------------------------*/
int
rpl_neighbor_is_acceptable_parent(rpl_nbr_t *nbr)
{
  if(nbr != NULL && curr_instance.of->nbr_is_acceptable_parent != NULL) {
    return curr_instance.of->nbr_is_acceptable_parent(nbr);
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
uint16_t
rpl_neighbor_get_link_metric(rpl_nbr_t *nbr)
{
  if(nbr != NULL && curr_instance.of->nbr_link_metric != NULL) {
    return curr_instance.of->nbr_link_metric(nbr);
  }
  return 0xffff;
}
/*---------------------------------------------------------------------------*/
rpl_rank_t
rpl_neighbor_rank_via_nbr(rpl_nbr_t *nbr)
{
  if(nbr != NULL && curr_instance.of->rank_via_nbr != NULL) {
    return curr_instance.of->rank_via_nbr(nbr);
  }
  return RPL_INFINITE_RANK;
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
rpl_neighbor_get_lladdr(rpl_nbr_t *nbr)
{
  return nbr_table_get_lladdr(rpl_neighbors, nbr);
}
/*---------------------------------------------------------------------------*/
uip_ipaddr_t *
rpl_neighbor_get_ipaddr(rpl_nbr_t *nbr)
{
  const linkaddr_t *lladdr = rpl_neighbor_get_lladdr(nbr);
  return uip_ds6_nbr_ipaddr_from_lladdr((uip_lladdr_t *)lladdr);
}
/*---------------------------------------------------------------------------*/
const struct link_stats *
rpl_neighbor_get_link_stats(rpl_nbr_t *nbr)
{
  const linkaddr_t *lladdr = rpl_neighbor_get_lladdr(nbr);
  return link_stats_from_lladdr(lladdr);
}
/*---------------------------------------------------------------------------*/
int
rpl_neighbor_is_fresh(rpl_nbr_t *nbr)
{
  const struct link_stats *stats = rpl_neighbor_get_link_stats(nbr);
  return link_stats_is_fresh(stats);
}
/*---------------------------------------------------------------------------*/
int
rpl_neighbor_is_reachable(rpl_nbr_t *nbr) {
  if(nbr == NULL) {
    return 0;
  } else {
#if UIP_ND6_SEND_NS
    uip_ds6_nbr_t *ds6_nbr = rpl_get_ds6_nbr(nbr);
    /* Exclude links to a neighbor that is not reachable at a NUD level */
    if(ds6_nbr == NULL || ds6_nbr->state != NBR_REACHABLE) {
      return 0;
    }
#endif /* UIP_ND6_SEND_NS */
    /* If we don't have fresh link information, assume the nbr is reachable. */
    return !rpl_neighbor_is_fresh(nbr) || curr_instance.of->nbr_has_usable_link(nbr);
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_neighbor_is_parent(rpl_nbr_t *nbr)
{
  return nbr != NULL && nbr->rank < curr_instance.dag.rank;
}
/*---------------------------------------------------------------------------*/
void
rpl_neighbor_set_preferred_parent(rpl_nbr_t *nbr)
{
  if(curr_instance.dag.preferred_parent != nbr) {
    LOG_INFO("parent switch: ");
    LOG_INFO_6ADDR(rpl_neighbor_get_ipaddr(curr_instance.dag.preferred_parent));
    LOG_INFO_(" -> ");
    LOG_INFO_6ADDR(rpl_neighbor_get_ipaddr(nbr));
    LOG_INFO_("\n");

#ifdef RPL_CALLBACK_PARENT_SWITCH
    RPL_CALLBACK_PARENT_SWITCH(curr_instance.dag.preferred_parent, nbr);
#endif /* RPL_CALLBACK_PARENT_SWITCH */

    /* Always keep the preferred parent locked, so it remains in the
     * neighbor table. */
    nbr_table_unlock(rpl_neighbors, curr_instance.dag.preferred_parent);
    nbr_table_lock(rpl_neighbors, nbr);

    /* Update DS6 default route. Use an infinite lifetime */
    uip_ds6_defrt_rm(uip_ds6_defrt_lookup(
      rpl_neighbor_get_ipaddr(curr_instance.dag.preferred_parent)));
    uip_ds6_defrt_add(rpl_neighbor_get_ipaddr(nbr), 0);

    curr_instance.dag.preferred_parent = nbr;
    curr_instance.dag.unprocessed_parent_switch = true;
  }
}
/*---------------------------------------------------------------------------*/
/* Remove all DAG neighbors */
void
rpl_neighbor_remove_all(void)
{
  rpl_nbr_t *nbr;

  LOG_INFO("removing all neighbors\n");

  /* Unset preferred parent before we de-allocate it. This will set
   * unprocessed_parent_switch which will make sure rpl_dag_update_state takes
   * all actions necessary after losing the preferred parent */
  rpl_neighbor_set_preferred_parent(NULL);

  nbr = nbr_table_head(rpl_neighbors);
  while(nbr != NULL) {
    remove_neighbor(nbr);
    nbr = nbr_table_next(rpl_neighbors, nbr);
  }

  /* Update needed immediately. As we have lost the preferred parent this will
   * enter poisoining and set timers accordingly. */
  rpl_dag_update_state();
}
/*---------------------------------------------------------------------------*/
rpl_nbr_t *
rpl_neighbor_get_from_ipaddr(uip_ipaddr_t *addr)
{
  uip_ds6_nbr_t *ds6_nbr = uip_ds6_nbr_lookup(addr);
  const uip_lladdr_t *lladdr = uip_ds6_nbr_get_ll(ds6_nbr);
  return nbr_table_get_from_lladdr(rpl_neighbors, (linkaddr_t *)lladdr);
}
/*---------------------------------------------------------------------------*/
static rpl_nbr_t *
best_parent(int fresh_only)
{
  rpl_nbr_t *nbr;
  rpl_nbr_t *best = NULL;

  if(curr_instance.used == 0) {
    return NULL;
  }

  /* Search for the best parent according to the OF */
  for(nbr = nbr_table_head(rpl_neighbors); nbr != NULL; nbr = nbr_table_next(rpl_neighbors, nbr)) {

    if(!acceptable_rank(rpl_neighbor_rank_via_nbr(nbr))
      || !curr_instance.of->nbr_is_acceptable_parent(nbr)) {
      /* Exclude neighbors with a rank that is not acceptable */
      continue;
    }

    if(fresh_only && !rpl_neighbor_is_fresh(nbr)) {
      /* Filter out non-fresh nerighbors if fresh_only is set */
      continue;
    }

#if UIP_ND6_SEND_NS
    {
    uip_ds6_nbr_t *ds6_nbr = rpl_get_ds6_nbr(nbr);
    /* Exclude links to a neighbor that is not reachable at a NUD level */
    if(ds6_nbr == NULL || ds6_nbr->state != NBR_REACHABLE) {
      continue;
    }
    }
#endif /* UIP_ND6_SEND_NS */

    /* Now we have an acceptable parent, check if it is the new best */
    best = curr_instance.of->best_parent(best, nbr);
  }

  return best;
}
/*---------------------------------------------------------------------------*/
rpl_nbr_t *
rpl_neighbor_select_best(void)
{
  rpl_nbr_t *best;

  if(rpl_dag_root_is_root()) {
    return NULL; /* The root has no parent */
  }

  /* Look for best parent (regardless of freshness) */
  best = best_parent(0);

#if RPL_WITH_PROBING
  if(best != NULL) {
    if(rpl_neighbor_is_fresh(best)) {
      /* Unschedule any already scheduled urgent probing */
      curr_instance.dag.urgent_probing_target = NULL;
      /* Return best if it is fresh */
      return best;
    } else {
      rpl_nbr_t *best_fresh;

      /* The best is not fresh. Probe it (unless there is already an urgent
         probing target). We will be called back after the probing anyway. */
      if(curr_instance.dag.urgent_probing_target == NULL) {
        LOG_INFO("best parent is not fresh, schedule urgent probing to ");
        LOG_INFO_6ADDR(rpl_neighbor_get_ipaddr(best));
        LOG_INFO_("\n");
        curr_instance.dag.urgent_probing_target = best;
        rpl_schedule_probing_now();
      }

      /* The best is our preferred parent. It is not fresh but used to be,
      else we would not have selected it in the first place. Stick to it
      for a little while and rely on urgent probing to make a call. */
      if(best == curr_instance.dag.preferred_parent) {
        return best;
      }

      /* Look for the best fresh parent. */
      best_fresh = best_parent(1);
      if(best_fresh == NULL) {
        if(curr_instance.dag.preferred_parent == NULL) {
          /* We will wait to find a fresh node before selecting our first parent */
          return NULL;
        } else {
          /* We already have a parent, now stick to the best and count on
          urgent probing to get a fresh parent soon */
          return best;
        }
      } else {
        /* Select best fresh */
        return best_fresh;
      }
    }
  } else {
    /* No acceptable parent */
    return NULL;
  }
#else /* RPL_WITH_PROBING */
  return best;
#endif /* RPL_WITH_PROBING */
}
/*---------------------------------------------------------------------------*/
void
rpl_neighbor_init(void)
{
  nbr_table_register(rpl_neighbors, (nbr_table_callback *)remove_neighbor);
}
/** @} */
