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
 *         Logic for Directed Acyclic Graphs in RPL.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 * Simon Duquennoy <simon.duquennoy@inria.fr>
 * Contributors: George Oikonomou <oikonomou@users.sourceforge.net> (multicast)
 */

#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-sr.h"
#include "net/nbr-table.h"
#include "net/link-stats.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

/*---------------------------------------------------------------------------*/
extern rpl_of_t rpl_of0, rpl_mrhof;
static rpl_of_t * const objective_functions[] = RPL_SUPPORTED_OFS;
static int process_dio_init_dag(rpl_dio_t *dio);

/*---------------------------------------------------------------------------*/
/* Allocate instance table. */
rpl_instance_t curr_instance;

/*---------------------------------------------------------------------------*/

#ifdef RPL_VALIDATE_DIO_FUNC
int RPL_VALIDATE_DIO_FUNC(rpl_dio_t *dio);
#endif /* RPL_PROBING_SELECT_FUNC */

/*---------------------------------------------------------------------------*/
const char *
rpl_dag_state_to_str(enum rpl_dag_state state)
{
  switch(state) {
    case DAG_INITIALIZED:
      return "initialized";
    case DAG_JOINED:
      return "joined";
    case DAG_REACHABLE:
      return "reachable";
    case DAG_POISONING:
      return "poisoning";
    default:
      return "unknown";
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_dag_get_root_ipaddr(uip_ipaddr_t *ipaddr)
{
  if(curr_instance.used && ipaddr != NULL) {
    uip_ipaddr_copy(ipaddr, &curr_instance.dag.dag_id);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_leave(void)
{
  LOG_INFO("leaving DAG ");
  LOG_INFO_6ADDR(&curr_instance.dag.dag_id);
  LOG_INFO_(", instance %u\n", curr_instance.instance_id);

  /* Issue a no-path DAO */
  if(!rpl_dag_root_is_root()) {
    RPL_LOLLIPOP_INCREMENT(curr_instance.dag.dao_last_seqno);
    rpl_icmp6_dao_output(0);
  }

  /* Forget past link statistics */
  link_stats_reset();

  /* Remove all neighbors, links and default route */
  rpl_neighbor_remove_all();
  uip_sr_free_all();

  /* Stop all timers */
  rpl_timers_stop_dag_timers();

  /* Remove autoconfigured address */
  if((curr_instance.dag.prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS)) {
    rpl_reset_prefix(&curr_instance.dag.prefix_info);
  }

  /* Mark instance as unused */
  curr_instance.used = 0;
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_poison_and_leave(void)
{
  curr_instance.dag.state = DAG_POISONING;
  rpl_timers_schedule_state_update();
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_periodic(unsigned seconds)
{
  if(curr_instance.used) {
    if(curr_instance.dag.lifetime != RPL_LIFETIME(RPL_INFINITE_LIFETIME)) {
      curr_instance.dag.lifetime =
        curr_instance.dag.lifetime > seconds ? curr_instance.dag.lifetime - seconds : 0;
      if(curr_instance.dag.lifetime == 0) {
        LOG_WARN("DAG expired, poison and leave\n");
        curr_instance.dag.state = DAG_POISONING;
        rpl_timers_schedule_state_update();
      } else if(curr_instance.dag.lifetime < 300 && curr_instance.dag.preferred_parent != NULL) {
        /* Five minutes before expiring, start sending unicast DIS to get an update */
        LOG_WARN("DAG expiring in %u seconds, send DIS to preferred parent\n", (unsigned)curr_instance.dag.lifetime);
        rpl_icmp6_dis_output(rpl_neighbor_get_ipaddr(curr_instance.dag.preferred_parent));
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_is_addr_in_our_dag(const uip_ipaddr_t *addr)
{
  return curr_instance.used
    && uip_ipaddr_prefixcmp(&curr_instance.dag.dag_id, addr, curr_instance.dag.prefix_info.length);
}
/*---------------------------------------------------------------------------*/
rpl_instance_t *
rpl_get_default_instance(void)
{
  return curr_instance.used ? &curr_instance : NULL;
}
/*---------------------------------------------------------------------------*/
rpl_dag_t *
rpl_get_any_dag(void)
{
  return curr_instance.used ? &curr_instance.dag : NULL;
}
/*---------------------------------------------------------------------------*/
static rpl_of_t *
find_objective_function(rpl_ocp_t ocp)
{
  unsigned int i;
  for(i = 0; i < sizeof(objective_functions) / sizeof(objective_functions[0]); i++) {
    if(objective_functions[i]->ocp == ocp) {
      return objective_functions[i];
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
rpl_refresh_routes(const char *str)
{
  if(rpl_dag_root_is_root()) {
    /* Increment DTSN */
    RPL_LOLLIPOP_INCREMENT(curr_instance.dtsn_out);

    LOG_WARN("incremented DTSN (%s), current %u\n",
         str, curr_instance.dtsn_out);
    if(LOG_INFO_ENABLED) {
      rpl_neighbor_print_list("Refresh routes (before)");
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_global_repair(const char *str)
{
  if(rpl_dag_root_is_root()) {
    RPL_LOLLIPOP_INCREMENT(curr_instance.dag.version);  /* New DAG version */
    curr_instance.dtsn_out = RPL_LOLLIPOP_INIT;  /* Re-initialize DTSN */

    LOG_WARN("initiating global repair (%s), version %u, rank %u\n",
         str, curr_instance.dag.version, curr_instance.dag.rank);
    if(LOG_INFO_ENABLED) {
      rpl_neighbor_print_list("Global repair (before)");
    }

    /* Now do a local repair to disseminate the new version */
    rpl_local_repair("Global repair");
  }
}
/*---------------------------------------------------------------------------*/
static void
global_repair_non_root(rpl_dio_t *dio)
{
  if(!rpl_dag_root_is_root()) {
    LOG_WARN("participating in global repair, version %u, rank %u\n",
         dio->version, curr_instance.dag.rank);
    if(LOG_INFO_ENABLED) {
      rpl_neighbor_print_list("Global repair (before)");
    }
    /* Re-initialize configuration from DIO */
    rpl_timers_stop_dag_timers();
    rpl_neighbor_set_preferred_parent(NULL);
    /* This will both re-init the DAG and schedule required timers */
    process_dio_init_dag(dio);
    rpl_local_repair("Global repair");
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_local_repair(const char *str)
{
  if(curr_instance.used) { /* Check needed because this is a public function */
    LOG_WARN("local repair (%s)\n", str);
    if(!rpl_dag_root_is_root()) {
      curr_instance.dag.state = DAG_INITIALIZED; /* Reset DAG state */
    }
    curr_instance.of->reset(); /* Reset OF */
    rpl_neighbor_remove_all(); /* Remove all neighbors */
    rpl_timers_dio_reset("Local repair"); /* Reset Trickle timer */
    rpl_timers_schedule_state_update();
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_dag_ready_to_advertise(void)
{
  if(curr_instance.mop == RPL_MOP_NO_DOWNWARD_ROUTES) {
    return curr_instance.used && curr_instance.dag.state >= DAG_INITIALIZED;
  } else {
    return curr_instance.used && curr_instance.dag.state >= DAG_REACHABLE;
  }
}
/*---------------------------------------------------------------------------*/
/* Updates rank and parent */
void
rpl_dag_update_state(void)
{
  rpl_rank_t old_rank;

  if(!curr_instance.used) {
    return;
  }

  old_rank = curr_instance.dag.rank;
  /* Any scheduled state update is no longer needed */
  rpl_timers_unschedule_state_update();

  if(curr_instance.dag.state == DAG_POISONING) {
    rpl_neighbor_set_preferred_parent(NULL);
    curr_instance.dag.rank = RPL_INFINITE_RANK;
    if(old_rank != RPL_INFINITE_RANK) {
      /* Advertise that we are leaving, and leave after a delay */
      LOG_WARN("poisoning and leaving after a delay\n");
      rpl_timers_dio_reset("Poison routes");
      rpl_timers_schedule_leaving();
    }
  } else if(!rpl_dag_root_is_root()) {
    rpl_nbr_t *old_parent = curr_instance.dag.preferred_parent;
    rpl_nbr_t *nbr;

    /* Select and set preferred parent */
    rpl_neighbor_set_preferred_parent(rpl_neighbor_select_best());
    /* Update rank  */
    curr_instance.dag.rank = rpl_neighbor_rank_via_nbr(curr_instance.dag.preferred_parent);

    /* Update better_parent_since flag for each neighbor */
    nbr = nbr_table_head(rpl_neighbors);
    while(nbr != NULL) {
      if(rpl_neighbor_rank_via_nbr(nbr) < curr_instance.dag.rank) {
        /* This neighbor would be a better parent than our current.
        Set 'better_parent_since' if not already set. */
        if(nbr->better_parent_since == 0) {
          nbr->better_parent_since = clock_time(); /* Initialize */
        }
      } else {
        nbr->better_parent_since = 0; /* Not a better parent */
      }
      nbr = nbr_table_next(rpl_neighbors, nbr);
    }

    if(old_parent == NULL || curr_instance.dag.rank < curr_instance.dag.lowest_rank) {
      /* This is a slight departure from RFC6550: if we had no preferred parent before,
       * reset lowest_rank. This helps recovering from temporary bad link conditions. */
      curr_instance.dag.lowest_rank = curr_instance.dag.rank;
    }

    /* Reset DIO timer in case of significant rank update */
    if(curr_instance.dag.last_advertised_rank != RPL_INFINITE_RANK
        && curr_instance.dag.rank != RPL_INFINITE_RANK
        && ABS((int32_t)curr_instance.dag.rank - curr_instance.dag.last_advertised_rank) > RPL_SIGNIFICANT_CHANGE_THRESHOLD) {
      LOG_WARN("significant rank update %u->%u\n",
          curr_instance.dag.last_advertised_rank, curr_instance.dag.rank);
      /* Update already here to avoid multiple resets in a row */
      curr_instance.dag.last_advertised_rank = curr_instance.dag.rank;
      rpl_timers_dio_reset("Significant rank update");
    }

    /* Parent switch */
    if(curr_instance.dag.unprocessed_parent_switch) {

      if(curr_instance.dag.preferred_parent != NULL) {
        /* We just got a parent (was NULL), reset trickle timer to advertise this */
        if(old_parent == NULL) {
          curr_instance.dag.state = DAG_JOINED;
          rpl_timers_dio_reset("Got parent");
          LOG_WARN("found parent: ");
          LOG_WARN_6ADDR(rpl_neighbor_get_ipaddr(curr_instance.dag.preferred_parent));
          LOG_WARN_(", staying in DAG\n");
          rpl_timers_unschedule_leaving();
        }
        /* Schedule a DAO */
        rpl_timers_schedule_dao();
      } else {
        /* We have no more parent, schedule DIS to get a chance to hear updated state */
        curr_instance.dag.state = DAG_INITIALIZED;
        LOG_WARN("no parent, scheduling periodic DIS, will leave if no parent is found\n");
        rpl_timers_dio_reset("Poison routes");
        rpl_timers_schedule_periodic_dis();
        rpl_timers_schedule_leaving();
      }

      if(LOG_INFO_ENABLED) {
        rpl_neighbor_print_list("Parent switch");
      }

      /* Clear unprocessed_parent_switch now that we have processed it */
      curr_instance.dag.unprocessed_parent_switch = false;
    }
  }

  /* Finally, update metric container */
  curr_instance.of->update_metric_container();
}
/*---------------------------------------------------------------------------*/
static rpl_nbr_t *
update_nbr_from_dio(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  rpl_nbr_t *nbr = NULL;
  const uip_lladdr_t *lladdr;

  nbr = rpl_neighbor_get_from_ipaddr(from);
  /* Neighbor not in RPL neighbor table, add it */
  if(nbr == NULL) {
    /* Is the neighbor known by ds6? Drop this request if not.
     * Typically, the neighbor is added upon receiving a DIO. */
    lladdr = uip_ds6_nbr_lladdr_from_ipaddr(from);
    if(lladdr == NULL) {
      return NULL;
    }

    /* Add neighbor to RPL table */
    nbr = nbr_table_add_lladdr(rpl_neighbors, (linkaddr_t *)lladdr,
                             NBR_TABLE_REASON_RPL_DIO, dio);
    if(nbr == NULL) {
      LOG_ERR("failed to add neighbor\n");
      return NULL;
    }
  }

  /* Update neighbor info from DIO */
  nbr->rank = dio->rank;
  nbr->dtsn = dio->dtsn;
#if RPL_WITH_MC
  memcpy(&nbr->mc, &dio->mc, sizeof(nbr->mc));
#endif /* RPL_WITH_MC */

  return nbr;
}
/*---------------------------------------------------------------------------*/
static void
process_dio_from_current_dag(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  rpl_nbr_t *nbr;
  uint8_t last_dtsn;

  /* Does the rank make sense at all? */
  if(dio->rank < ROOT_RANK) {
    return;
  }

  /* If the DIO sender is on an older version of the DAG, do not process it
   * further. The sender will eventually hear the global repair and catch up. */
  if(rpl_lollipop_greater_than(curr_instance.dag.version, dio->version)) {
    if(dio->rank == ROOT_RANK) {
      /* Before returning, if the DIO was from the root, an old DAG versions
       * likely incidates a root reboot. Reset our DIO timer to make sure the
       * root hears our version ASAP, and in turn triggers a global repair. */
      rpl_timers_dio_reset("Heard old version from root");
    }
    return;
  }

  /* The DIO is valid, proceed further */

  /* Update DIO counter for redundancy mngt */
  if(dio->rank != RPL_INFINITE_RANK) {
    curr_instance.dag.dio_counter++;
  }

  /* The DIO has a newer version: global repair.
   * Must come first, as it might remove all neighbors, and we then need
   * to re-add this source of the DIO to the neighbor table */
  if(rpl_lollipop_greater_than(dio->version, curr_instance.dag.version)) {
    if(curr_instance.dag.rank == ROOT_RANK) {
      /* The root should not hear newer versions unless it just rebooted */
      LOG_ERR("inconsistent DIO version (current: %u, received: %u), initiate global repair\n",
          curr_instance.dag.version, dio->version);
      /* Update version and trigger global repair */
      curr_instance.dag.version = dio->version;
      rpl_global_repair("Inconsistent DIO version");
    } else {
      LOG_WARN("new DIO version (current: %u, received: %u), apply global repair\n",
          curr_instance.dag.version, dio->version);
      global_repair_non_root(dio);
    }
  }

  /* Update IPv6 neighbor cache */
  if(!rpl_icmp6_update_nbr_table(from, NBR_TABLE_REASON_RPL_DIO, dio)) {
    LOG_ERR("IPv6 cache full, dropping DIO\n");
    return;
  }

  nbr = rpl_neighbor_get_from_ipaddr(from);
  last_dtsn = nbr != NULL ? nbr->dtsn : RPL_LOLLIPOP_INIT;

  /* Add neighbor to RPL neighbor table */
  if(!update_nbr_from_dio(from, dio)) {
    LOG_ERR("neighbor table full, dropping DIO\n");
    return;
  }

  /* Init lifetime if not set yet. Refresh it at every DIO from preferred parent. */
  if(curr_instance.dag.lifetime == 0 ||
    (nbr != NULL && nbr == curr_instance.dag.preferred_parent)) {
    LOG_INFO("refreshing lifetime\n");
    curr_instance.dag.lifetime = RPL_LIFETIME(RPL_DAG_LIFETIME);
  }

  /* If the source is our preferred parent and it increased DTSN, we increment
   * our DTSN in turn and schedule a DAO (see RFC6550 section 9.6.) */
  if(curr_instance.mop != RPL_MOP_NO_DOWNWARD_ROUTES) {
    if(nbr != NULL && nbr == curr_instance.dag.preferred_parent && rpl_lollipop_greater_than(dio->dtsn, last_dtsn)) {
      RPL_LOLLIPOP_INCREMENT(curr_instance.dtsn_out);
      LOG_WARN("DTSN increment %u->%u, schedule new DAO with DTSN %u\n",
        last_dtsn, dio->dtsn, curr_instance.dtsn_out);
      rpl_timers_schedule_dao();
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
init_dag(uint8_t instance_id, uip_ipaddr_t *dag_id, rpl_ocp_t ocp,
        uip_ipaddr_t *prefix, unsigned prefix_len, uint8_t prefix_flags)
{
  rpl_of_t *of;

  memset(&curr_instance, 0, sizeof(curr_instance));

  /* OF */
  of = find_objective_function(ocp);
  if(of == NULL) {
    LOG_ERR("ignoring DIO with an unsupported OF: %u\n", ocp);
    return 0;
  }

  /* Prefix */
  if(!rpl_set_prefix_from_addr(prefix, prefix_len, prefix_flags)) {
    LOG_ERR("failed to set prefix\n");
    return 0;
  }

  /* Instnace */
  curr_instance.instance_id = instance_id;
  curr_instance.of = of;
  curr_instance.dtsn_out = RPL_LOLLIPOP_INIT;
  curr_instance.used = 1;

  /* DAG */
  curr_instance.dag.rank = RPL_INFINITE_RANK;
  curr_instance.dag.last_advertised_rank = RPL_INFINITE_RANK;
  curr_instance.dag.lowest_rank = RPL_INFINITE_RANK;
  curr_instance.dag.dao_last_seqno = RPL_LOLLIPOP_INIT;
  curr_instance.dag.dao_last_acked_seqno = RPL_LOLLIPOP_INIT;
  curr_instance.dag.dao_last_seqno = RPL_LOLLIPOP_INIT;
  memcpy(&curr_instance.dag.dag_id, dag_id, sizeof(curr_instance.dag.dag_id));

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
init_dag_from_dio(rpl_dio_t *dio)
{
  if(!init_dag(dio->instance_id, &dio->dag_id, dio->ocp,
        &dio->prefix_info.prefix, dio->prefix_info.length, dio->prefix_info.flags)) {
    return 0;
  }

  /* Instnace */
  curr_instance.mop = dio->mop;
  curr_instance.mc.type = dio->mc.type;
  curr_instance.mc.flags = dio->mc.flags;
  curr_instance.mc.aggr = dio->mc.aggr;
  curr_instance.mc.prec = dio->mc.prec;
  curr_instance.max_rankinc = dio->dag_max_rankinc;
  curr_instance.min_hoprankinc = dio->dag_min_hoprankinc;
  curr_instance.dio_intdoubl = dio->dag_intdoubl;
  curr_instance.dio_intmin = dio->dag_intmin;
  curr_instance.dio_redundancy = dio->dag_redund;
  curr_instance.default_lifetime = dio->default_lifetime;
  curr_instance.lifetime_unit = dio->lifetime_unit;

  /* DAG */
  curr_instance.dag.state = DAG_INITIALIZED;
  curr_instance.dag.preference = dio->preference;
  curr_instance.dag.grounded = dio->grounded;
  curr_instance.dag.version = dio->version;
  /* dio_intcurrent will be reset by rpl_timers_dio_reset() */
  curr_instance.dag.dio_intcurrent = 0;

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
process_dio_init_dag(rpl_dio_t *dio)
{
#ifdef RPL_VALIDATE_DIO_FUNC
  if(!RPL_VALIDATE_DIO_FUNC(dio)) {
    LOG_WARN("DIO validation failed\n");
    return 0;
  }
#endif

  /* Check MOP */
  if(dio->mop != RPL_MOP_NO_DOWNWARD_ROUTES && dio->mop != RPL_MOP_NON_STORING) {
    LOG_WARN("ignoring DIO with an unsupported MOP: %d\n", dio->mop);
    return 0;
  }

  /* Initialize instance and DAG data structures */
  if(!init_dag_from_dio(dio)) {
    LOG_WARN("failed to initialize DAG\n");
    return 0;
  }

  /* Init OF and timers */
  curr_instance.of->reset();
  rpl_timers_dio_reset("Join");
#if RPL_WITH_PROBING
  rpl_schedule_probing();
#endif /* RPL_WITH_PROBING */
  /* Leave the network after RPL_DELAY_BEFORE_LEAVING in case we do not
  find a parent */
  LOG_INFO("initialized DAG with instance ID %u, DAG ID ",
         curr_instance.instance_id);
  LOG_INFO_6ADDR(&curr_instance.dag.dag_id);
  LOG_INFO_(", prexix ");
  LOG_INFO_6ADDR(&dio->prefix_info.prefix);
  LOG_INFO_("/%u, rank %u\n", dio->prefix_info.length, curr_instance.dag.rank);

  LOG_ANNOTATE("#A init=%u\n", curr_instance.dag.dag_id.u8[sizeof(curr_instance.dag.dag_id) - 1]);

  LOG_WARN("just joined, no parent yet, setting timer for leaving\n");
  rpl_timers_schedule_leaving();

  return 1;
}
/*---------------------------------------------------------------------------*/
void
rpl_process_dio(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  if(!curr_instance.used && !rpl_dag_root_is_root()) {
    /* Attempt to init our DAG from this DIO */
    if(!process_dio_init_dag(dio)) {
      LOG_WARN("failed to init DAG\n");
      return;
    }
  }

  if(curr_instance.used
      && curr_instance.instance_id == dio->instance_id
      && uip_ipaddr_cmp(&curr_instance.dag.dag_id, &dio->dag_id)) {
    process_dio_from_current_dag(from, dio);
    rpl_dag_update_state();
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_process_dis(uip_ipaddr_t *from, int is_multicast)
{
  if(is_multicast) {
    rpl_timers_dio_reset("Multicast DIS");
  } else {
    /* Add neighbor to cache and reply to the unicast DIS with a unicast DIO*/
    if(rpl_icmp6_update_nbr_table(from, NBR_TABLE_REASON_RPL_DIS, NULL) != NULL) {
      LOG_INFO("unicast DIS, reply to sender\n");
      rpl_icmp6_dio_output(from);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_process_dao(uip_ipaddr_t *from, rpl_dao_t *dao)
{
  if(dao->lifetime == 0) {
    uip_sr_expire_parent(NULL, from, &dao->parent_addr);
  } else {
    if(!uip_sr_update_node(NULL, from, &dao->parent_addr, RPL_LIFETIME(dao->lifetime))) {
      LOG_ERR("failed to add link on incoming DAO\n");
      return;
    }
  }

#if RPL_WITH_DAO_ACK
  if(dao->flags & RPL_DAO_K_FLAG) {
    rpl_timers_schedule_dao_ack(from, dao->sequence);
  }
#endif /* RPL_WITH_DAO_ACK */
}
/*---------------------------------------------------------------------------*/
#if RPL_WITH_DAO_ACK
void
rpl_process_dao_ack(uint8_t sequence, uint8_t status)
{
  /* Update dao_last_acked_seqno */
  if(rpl_lollipop_greater_than(sequence, curr_instance.dag.dao_last_acked_seqno)) {
    curr_instance.dag.dao_last_acked_seqno = sequence;
  }
  /* Is this an ACK for our last DAO? */
  if(sequence == curr_instance.dag.dao_last_seqno) {
    int status_ok = status < RPL_DAO_ACK_UNABLE_TO_ACCEPT;
    if(curr_instance.dag.state == DAG_JOINED && status_ok) {
      curr_instance.dag.state = DAG_REACHABLE;
      rpl_timers_dio_reset("Reachable");
    }
    /* Let the rpl-timers module know that we got an ACK for the last DAO */
    rpl_timers_notify_dao_ack();

    if(!status_ok) {
      /* We got a NACK, start poisoning and leave */
      LOG_WARN("DAO-NACK received with seqno %u, status %u, poison and leave\n",
              sequence, status);
      curr_instance.dag.state = DAG_POISONING;
    }
  }
}
#endif /* RPL_WITH_DAO_ACK */
/*---------------------------------------------------------------------------*/
int
rpl_process_hbh(rpl_nbr_t *sender, uint16_t sender_rank, int loop_detected, int rank_error_signaled)
{
  int drop = 0;

  if(loop_detected) {
    if(rank_error_signaled) {
#if RPL_LOOP_ERROR_DROP
      /* Drop packet and reset trickle timer, as per  RFC6550 - 11.2.2.2 */
      rpl_timers_dio_reset("HBH error");
      LOG_WARN("rank error and loop detected, dropping\n");
      drop = 1;
#endif /* RPL_LOOP_ERROR_DROP */
    }
    /* Attempt to repair the loop by sending a unicast DIO back to the sender
     * so that it gets a fresh update of our rank. */
    rpl_timers_schedule_unicast_dio(sender);
  }

  if(rank_error_signaled) {
    /* A rank error was signalled, attempt to repair it by updating
     * the sender's rank from ext header */
    if(sender != NULL) {
      sender->rank = sender_rank;
      /* Select DAG and preferred parent. In case of a parent switch,
      the new parent will be used to forward the current packet. */
      rpl_dag_update_state();
    }
  }

  return !drop;
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_init_root(uint8_t instance_id, uip_ipaddr_t *dag_id,
            uip_ipaddr_t *prefix, unsigned prefix_len, uint8_t prefix_flags)
{
  uint8_t version = RPL_LOLLIPOP_INIT;

  /* If we're in an instance, first leave it */
  if(curr_instance.used) {
    /* We were already root. Increment version */
    if(uip_ipaddr_cmp(&curr_instance.dag.dag_id, dag_id)) {
      version = curr_instance.dag.version;
      RPL_LOLLIPOP_INCREMENT(version);
    }
    rpl_dag_leave();
  }

  /* Init DAG and instance */
  init_dag(instance_id, dag_id, RPL_OF_OCP, prefix, prefix_len, prefix_flags);

  /* Instance */
  curr_instance.mop = RPL_MOP_DEFAULT;
  curr_instance.max_rankinc = RPL_MAX_RANKINC;
  curr_instance.min_hoprankinc = RPL_MIN_HOPRANKINC;
  curr_instance.dio_intdoubl = RPL_DIO_INTERVAL_DOUBLINGS;
  curr_instance.dio_intmin = RPL_DIO_INTERVAL_MIN;
  curr_instance.dio_redundancy = RPL_DIO_REDUNDANCY;
  curr_instance.default_lifetime = RPL_DEFAULT_LIFETIME;
  curr_instance.lifetime_unit = RPL_DEFAULT_LIFETIME_UNIT;

  /* DAG */
  curr_instance.dag.preference = RPL_PREFERENCE;
  curr_instance.dag.grounded = RPL_GROUNDED;
  curr_instance.dag.version = version;
  curr_instance.dag.rank = ROOT_RANK;
  curr_instance.dag.lifetime = RPL_LIFETIME(RPL_INFINITE_LIFETIME);
  /* dio_intcurrent will be reset by rpl_timers_dio_reset() */
  curr_instance.dag.dio_intcurrent = 0;
  curr_instance.dag.state = DAG_REACHABLE;

  rpl_timers_dio_reset("Init root");

  LOG_INFO("created DAG with instance ID %u, DAG ID ",
         curr_instance.instance_id);
  LOG_INFO_6ADDR(&curr_instance.dag.dag_id);
  LOG_INFO_(", rank %u\n", curr_instance.dag.rank);

  LOG_ANNOTATE("#A root=%u\n", curr_instance.dag.dag_id.u8[sizeof(curr_instance.dag.dag_id) - 1]);
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_init(void)
{
  memset(&curr_instance, 0, sizeof(curr_instance));
}
/*---------------------------------------------------------------------------*/
/** @} */
