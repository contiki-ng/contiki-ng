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
 *         Logic for Directed Acyclic Graphs in RPL.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 * Simon Duquennoy <simon.duquennoy@inria.fr>
 * Contributors: George Oikonomou <oikonomou@users.sourceforge.net> (multicast)
 */

/**
 * \addtogroup uip6
 * @{
 */

#include "rpl.h"
#include "net/nbr-table.h"

#define DEBUG DEBUG_NONE
#include "net/ip/uip-debug.h"

/*---------------------------------------------------------------------------*/
extern rpl_of_t rpl_of0, rpl_mrhof;
static rpl_of_t * const objective_functions[] = RPL_SUPPORTED_OFS;
static int init_dag_from_dio(rpl_dio_t *dio);
static void leave_dag(void);

/*---------------------------------------------------------------------------*/
/* Allocate instance table. */
rpl_instance_t curr_instance;

/*---------------------------------------------------------------------------*/
void
rpl_dag_periodic(unsigned seconds)
{
  if(curr_instance.used) {
    if(curr_instance.dag.lifetime != RPL_LIFETIME(RPL_INFINITE_LIFETIME)) {
      curr_instance.dag.lifetime =
        curr_instance.dag.lifetime > seconds ? curr_instance.dag.lifetime - seconds : 0;
      if(curr_instance.dag.lifetime == 0) {
        leave_dag();
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
leave_dag(void)
{
  PRINTF("RPL: leaving DAG \n");
  PRINT6ADDR(&curr_instance.dag.dag_id);
  PRINTF(", instance %u\n", curr_instance.instance_id);

  /* Issue a no-path DAO */
  RPL_LOLLIPOP_INCREMENT(curr_instance.dag.dao_curr_seqno);
  rpl_icmp6_dao_output(0);

  /* Remove all neighbors */
  rpl_neighbor_remove_all();

  rpl_timers_stop_dag_timers();

  /* Remove autoconfigured address */
  if((curr_instance.dag.prefix_info.flags & UIP_ND6_RA_FLAG_AUTONOMOUS)) {
    rpl_reset_prefix(&curr_instance.dag.prefix_info);
  }

  /* Mark instance as unused */
  curr_instance.used = 0;
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
rpl_global_repair(void)
{
  if(rpl_dag_root_is_root()) {
    PRINTF("RPL: initiating global repair (version=%u, rank=%u)\n",
         curr_instance.dag.version, curr_instance.dag.rank);
#if DEBUG
    rpl_neighbor_print_list("Global repair");
#endif

    /* Initiate global repair */
    RPL_LOLLIPOP_INCREMENT(curr_instance.dag.version);  /* New DAG version */
    RPL_LOLLIPOP_INCREMENT(curr_instance.dtsn_out); /* Request new DAOs */
    rpl_local_repair("Global repair");
  }
}
/*---------------------------------------------------------------------------*/
static void
global_repair_non_root(rpl_dio_t *dio)
{
  if(!rpl_dag_root_is_root()) {
    PRINTF("RPL: participating in global repair (version=%u, rank=%u)\n",
         curr_instance.dag.version, curr_instance.dag.rank);
#if DEBUG
    rpl_neighbor_print_list("Global repair");
#endif
    /* Re-initialize configuration from DIO */
    init_dag_from_dio(dio);
    rpl_local_repair("Global repair");
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_local_repair(const char *str)
{
  if(curr_instance.used) { /* Check needed because this is a public function */
    PRINTF("RPL: local repair (%s)\n", str);
    curr_instance.of->reset(); /* Reset OF */
    rpl_neighbor_remove_all(); /* Remove all neighbors */
    rpl_timers_dio_reset("Local repair"); /* Reset Trickle timer */
    rpl_timers_schedule_state_update();
  }
}
/*---------------------------------------------------------------------------*/
/* Updates rank and parent */
void
rpl_dag_update_state(void)
{
  if(curr_instance.used) {
    if(!rpl_dag_root_is_root()) {
      rpl_nbr_t *old_parent = curr_instance.dag.preferred_parent;
      rpl_rank_t old_rank = curr_instance.dag.rank;

      /* Any scheduled state update is no longer needed */
      rpl_timers_unschedule_state_update();

      /* Select and set preferred parent */
      rpl_neighbor_set_preferred(rpl_neighbor_select_best());
      /* Update rank  */
      curr_instance.dag.rank = rpl_neighbor_rank_via_nbr(curr_instance.dag.preferred_parent);

      if(old_parent == NULL || curr_instance.dag.rank < curr_instance.dag.lowest_rank) {
        /* This is a slight departure from RFC6550: if we had no preferred parent before,
         * reset lowest_rank. This helps recovering from temporary bad link conditions. */
        curr_instance.dag.lowest_rank = curr_instance.dag.rank;
      }

      /* if new parent, schedule DAO */
      if(curr_instance.dag.preferred_parent != old_parent) {
        rpl_timers_schedule_dao();
#if DEBUG
        rpl_neighbor_print_list("Parent switch");
#endif
      }

      if(curr_instance.dag.rank != old_rank && curr_instance.dag.rank == RPL_INFINITE_RANK) {
        PRINTF("RPL: intinite rank, trigger local repair\n");
        rpl_local_repair("Infinite rank");
      }
    }

    /* Finally, update metric container */
    curr_instance.of->update_metric_container();
  }
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
      PRINTF("RPL: failed to add neighbor\n");
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
  rpl_nbr_t *p;
  uint8_t last_dtsn;

  /* Does the rank make sense at all? */
  if(dio->rank < ROOT_RANK) {
    return;
  }

  /* If the DIO sender is on an older version of the DAG, ignore it. The node
  will eventually hear the global repair and catch up. */
  if(rpl_lollipop_greater_than(curr_instance.dag.version, dio->version)) {
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
    if(curr_instance.dag.rank == ROOT_RANK) { /* The root should not hear newer versions */
      PRINTF("RPL: inconsistent DIO version (current: %u, received: %u), initiate global repair\n",
          curr_instance.dag.version, dio->version);
      curr_instance.dag.version = dio->version; /* Update version and trigger global repair */
      rpl_global_repair();
    } else {
      PRINTF("RPL: new DIO version (current: %u, received: %u), apply global repair\n",
          curr_instance.dag.version, dio->version);
      global_repair_non_root(dio);
    }
  }

  /* Update IPv6 neighbor cache */
  if(!rpl_icmp6_update_nbr_table(from, NBR_TABLE_REASON_RPL_DIO, dio)) {
    PRINTF("RPL: IPv6 cache full, dropping DIO\n");
    return;
  }

  /* Add neighbor to RPL neighbor table */
  p = rpl_neighbor_get_from_ipaddr(from);
  last_dtsn = p != NULL ? p->dtsn : RPL_LOLLIPOP_INIT;

  if(!update_nbr_from_dio(from, dio)) {
    PRINTF("RPL: neighbor table full, dropping DIO\n");
    return;
  }

  /* Refresh lifetime at every DIO from preferred parent. Use same lifetime as for routes */
  if(p != NULL && p == curr_instance.dag.preferred_parent) {
    curr_instance.dag.lifetime =
      RPL_LIFETIME(dio->default_lifetime);
  }

  /* If the source is our preferred parent and it increased DTSN, we increment
   * our DTSN in turn and schedule a DAO (see RFC6550 section 9.6.) */
  if(curr_instance.mop != RPL_MOP_NO_DOWNWARD_ROUTES) {
    if(p != NULL && p == curr_instance.dag.preferred_parent && rpl_lollipop_greater_than(dio->dtsn, last_dtsn)) {
      RPL_LOLLIPOP_INCREMENT(curr_instance.dtsn_out);
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
    PRINTF("RPL: ignoring DIO with an unsupported OF: %u\n", ocp);
    return 0;
  }

  /* Prefix */
  if(!rpl_set_prefix_from_addr(prefix, prefix_len, prefix_flags)) {
    PRINTF("RPL: failed to set prefix");
    return 0;
  }

  /* Instnace */
  curr_instance.instance_id = instance_id;
  curr_instance.of = of;
  curr_instance.dtsn_out = RPL_LOLLIPOP_INIT;
  curr_instance.used = 1;

  /* DAG */
  curr_instance.dag.rank = RPL_INFINITE_RANK;
  curr_instance.dag.lowest_rank = RPL_INFINITE_RANK;
  curr_instance.dag.dao_last_seqno = RPL_LOLLIPOP_INIT;
  curr_instance.dag.dao_last_acked_seqno = RPL_LOLLIPOP_INIT;
  curr_instance.dag.dao_curr_seqno = RPL_LOLLIPOP_INIT;
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
  curr_instance.dag.preference = dio->preference;
  curr_instance.dag.grounded = dio->grounded;
  curr_instance.dag.version = dio->version;
  curr_instance.dag.dio_intcurrent = dio->dag_intmin;

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
process_dio_join_dag(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  /* Check MOP */
  if(dio->mop != RPL_MOP_NO_DOWNWARD_ROUTES && dio->mop != RPL_MOP_NON_STORING) {
    PRINTF("RPL: ignoring DIO with an unsupported MOP: %d\n", dio->mop);
    return 0;
  }

  /* Initialize instance and DAG data structures */
  if(!init_dag_from_dio(dio)) {
    PRINTF("RPL: failed to initialize DAG\n");
    return 0;
  }

  /* Init OF and timers */
  curr_instance.of->reset();
  rpl_timers_dio_reset("Join");
  #if RPL_WITH_PROBING
    rpl_schedule_probing();
  #endif /* RPL_WITH_PROBING */

  PRINTF("RPL: joined DAG with instance ID %u, DAG ID ",
         curr_instance.instance_id);
  PRINT6ADDR(&curr_instance.dag.dag_id);
  PRINTF(", rank %u\n", curr_instance.dag.rank);

  ANNOTATE("#A join=%u\n", curr_instance.dag.dag_id.u8[sizeof(curr_instance.dag.dag_id) - 1]);

  return 1;
}
/*---------------------------------------------------------------------------*/
void
rpl_process_dio(uip_ipaddr_t *from, rpl_dio_t *dio)
{
  if(!curr_instance.used && !rpl_dag_root_is_root()) {
    /* Attempt to join on this DIO */
    if(!process_dio_join_dag(from, dio)) {
      PRINTF("RPL: failed to join DAG");
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
      PRINTF("RPL: unicast DIS, reply to sender\n");
      rpl_icmp6_dio_output(from);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_process_dao(uip_ipaddr_t *from, rpl_dao_t *dao)
{
  if(dao->lifetime == 0) {
    rpl_ns_expire_parent(from, &dao->parent_addr);
  } else {
    if(!rpl_ns_update_node(from, &dao->parent_addr, RPL_LIFETIME(dao->lifetime))) {
      PRINTF("RPL: failed to add link on incoming DAO\n");
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
    /* stop the retransmit timer when the ACK arrived */
    curr_instance.dag.is_reachable = status < RPL_DAO_ACK_UNABLE_TO_ACCEPT;

    if(status >= RPL_DAO_ACK_UNABLE_TO_ACCEPT) {
      /* We got a NACK, leave the DAG  */
      leave_dag();
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
      PRINTF("RPL: rank error and loop detected, dropping\n");
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
    leave_dag();
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
  curr_instance.dag.dio_intcurrent = RPL_DIO_INTERVAL_MIN;

  rpl_timers_dio_reset("Init root");

  PRINTF("RPL: created DAG with instance ID %u, DAG ID ",
         curr_instance.instance_id);
  PRINT6ADDR(&curr_instance.dag.dag_id);
  PRINTF(", rank %u\n", curr_instance.dag.rank);

  ANNOTATE("#A root=%u\n", curr_instance.dag.dag_id.u8[sizeof(curr_instance.dag.dag_id) - 1]);
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_init(void)
{
  memset(&curr_instance, 0, sizeof(curr_instance));
  rpl_neighbor_init();
}
/*---------------------------------------------------------------------------*/
/** @} */
