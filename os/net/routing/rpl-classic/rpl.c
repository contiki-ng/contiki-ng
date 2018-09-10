/*
 * Copyright (c) 2009, Swedish Institute of Computer Science.
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
 * \file
 *         ContikiRPL, an implementation of RPL: IPv6 Routing Protocol
 *         for Low-Power and Lossy Networks (IETF RFC 6550)
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup uip
 * @{
 */

#include "net/ipv6/uip.h"
#include "net/ipv6/tcpip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-sr.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/routing/routing.h"
#include "net/routing/rpl-classic/rpl-private.h"
#include "net/routing/rpl-classic/rpl-dag-root.h"
#include "net/ipv6/multicast/uip-mcast6.h"

#include "sys/log.h"

#include <limits.h>
#include <string.h>

#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

#if RPL_CONF_STATS
rpl_stats_t rpl_stats;
#endif

static enum rpl_mode mode = RPL_MODE_MESH;
/*---------------------------------------------------------------------------*/
enum rpl_mode
rpl_get_mode(void)
{
  return mode;
}
/*---------------------------------------------------------------------------*/
enum rpl_mode
rpl_set_mode(enum rpl_mode m)
{
  enum rpl_mode oldmode = mode;

  /* We need to do different things depending on what mode we are
     switching to. */
  if(m == RPL_MODE_MESH) {

    /* If we switch to mesh mode, we should send out a DAO message to
       inform our parent that we now are reachable. Before we do this,
       we must set the mode variable, since DAOs will not be sent if
       we are in feather mode. */
    LOG_DBG("rpl_set_mode: switching to mesh mode\n");
    mode = m;

    if(default_instance != NULL) {
      rpl_schedule_dao_immediately(default_instance);
    }
  } else if(m == RPL_MODE_FEATHER) {

    LOG_INFO("rpl_set_mode: switching to feather mode\n");
    if(default_instance != NULL) {
      LOG_INFO("rpl_set_mode: RPL sending DAO with zero lifetime\n");
      if(default_instance->current_dag != NULL) {
        dao_output(default_instance->current_dag->preferred_parent, RPL_ZERO_LIFETIME);
      }
      rpl_cancel_dao(default_instance);
    } else {
      LOG_INFO("rpl_set_mode: no default instance\n");
    }

    mode = m;
  } else {
    mode = m;
  }

  return oldmode;
}
/*---------------------------------------------------------------------------*/
void
rpl_purge_routes(void)
{
  uip_ds6_route_t *r;
  uip_ipaddr_t prefix;
  rpl_dag_t *dag;
#if RPL_WITH_MULTICAST
  uip_mcast6_route_t *mcast_route;
#endif

  /* First pass, decrement lifetime */
  r = uip_ds6_route_head();

  while(r != NULL) {
    if(r->state.lifetime >= 1 && r->state.lifetime != RPL_ROUTE_INFINITE_LIFETIME) {
      /*
       * If a route is at lifetime == 1, set it to 0, scheduling it for
       * immediate removal below. This achieves the same as the original code,
       * which would delete lifetime <= 1
       */
      r->state.lifetime--;
    }
    r = uip_ds6_route_next(r);
  }

  /* Second pass, remove dead routes */
  r = uip_ds6_route_head();

  while(r != NULL) {
    if(r->state.lifetime < 1) {
      /* Routes with lifetime == 1 have only just been decremented from 2 to 1,
       * thus we want to keep them. Hence < and not <= */
      uip_ipaddr_copy(&prefix, &r->ipaddr);
      uip_ds6_route_rm(r);
      r = uip_ds6_route_head();
      LOG_INFO("No more routes to ");
      LOG_INFO_6ADDR(&prefix);
      dag = default_instance->current_dag;
      /* Propagate this information with a No-Path DAO to preferred parent if we are not a RPL Root */
      if(dag->rank != ROOT_RANK(default_instance)) {
        LOG_INFO_(" -> generate No-Path DAO\n");
        dao_output_target(dag->preferred_parent, &prefix, RPL_ZERO_LIFETIME);
        /* Don't schedule more than 1 No-Path DAO, let next iteration handle that */
        return;
      }
      LOG_INFO_("\n");
    } else {
      r = uip_ds6_route_next(r);
    }
  }

#if RPL_WITH_MULTICAST
  mcast_route = uip_mcast6_route_list_head();

  while(mcast_route != NULL) {
    if(mcast_route->lifetime <= 1) {
      uip_mcast6_route_rm(mcast_route);
      mcast_route = uip_mcast6_route_list_head();
    } else {
      mcast_route->lifetime--;
      mcast_route = list_item_next(mcast_route);
    }
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
rpl_remove_routes(rpl_dag_t *dag)
{
  uip_ds6_route_t *r;
#if RPL_WITH_MULTICAST
  uip_mcast6_route_t *mcast_route;
#endif

  r = uip_ds6_route_head();

  while(r != NULL) {
    if(r->state.dag == dag) {
      uip_ds6_route_rm(r);
      r = uip_ds6_route_head();
    } else {
      r = uip_ds6_route_next(r);
    }
  }

#if RPL_WITH_MULTICAST
  mcast_route = uip_mcast6_route_list_head();

  while(mcast_route != NULL) {
    if(mcast_route->dag == dag) {
      uip_mcast6_route_rm(mcast_route);
      mcast_route = uip_mcast6_route_list_head();
    } else {
      mcast_route = list_item_next(mcast_route);
    }
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
rpl_remove_routes_by_nexthop(uip_ipaddr_t *nexthop, rpl_dag_t *dag)
{
  uip_ds6_route_t *r;

  r = uip_ds6_route_head();

  while(r != NULL) {
    if(uip_ipaddr_cmp(uip_ds6_route_nexthop(r), nexthop) &&
        r->state.dag == dag) {
      r->state.lifetime = 0;
    }
    r = uip_ds6_route_next(r);
  }
  LOG_ANNOTATE("#L %u 0\n", nexthop->u8[sizeof(uip_ipaddr_t) - 1]);
}
/*---------------------------------------------------------------------------*/
uip_ds6_route_t *
rpl_add_route(rpl_dag_t *dag, uip_ipaddr_t *prefix, int prefix_len,
              uip_ipaddr_t *next_hop)
{
  uip_ds6_route_t *rep;

  if((rep = uip_ds6_route_add(prefix, prefix_len, next_hop)) == NULL) {
    LOG_ERR("No space for more route entries\n");
    return NULL;
  }

  rep->state.dag = dag;
  rep->state.lifetime = RPL_LIFETIME(dag->instance, dag->instance->default_lifetime);
  /* always clear state flags for the no-path received when adding/refreshing */
  RPL_ROUTE_CLEAR_NOPATH_RECEIVED(rep);

  LOG_INFO("Added a route to ");
  LOG_INFO_6ADDR(prefix);
  LOG_INFO_("/%d via ", prefix_len);
  LOG_INFO_6ADDR(next_hop);
  LOG_INFO_("\n");

  return rep;
}
/*---------------------------------------------------------------------------*/
void
rpl_link_callback(const linkaddr_t *addr, int status, int numtx)
{
  uip_ipaddr_t ipaddr;
  rpl_parent_t *parent;
  rpl_instance_t *instance;
  rpl_instance_t *end;

  uip_ip6addr(&ipaddr, 0xfe80, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, (uip_lladdr_t *)addr);

  for(instance = &instance_table[0], end = instance + RPL_MAX_INSTANCES; instance < end; ++instance) {
    if(instance->used == 1 ) {
      parent = rpl_find_parent_any_dag(instance, &ipaddr);
      if(parent != NULL) {
        /* If this is the neighbor we were probing urgently, mark urgent
        probing as done */
#if RPL_WITH_PROBING
        if(instance->urgent_probing_target == parent) {
          instance->urgent_probing_target = NULL;
        }
#endif /* RPL_WITH_PROBING */
        /* Trigger DAG rank recalculation. */
        LOG_DBG("rpl_link_callback triggering update\n");
        parent->flags |= RPL_PARENT_FLAG_UPDATED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_ipv6_neighbor_callback(uip_ds6_nbr_t *nbr)
{
  rpl_parent_t *p;
  rpl_instance_t *instance;
  rpl_instance_t *end;

  LOG_DBG("Neighbor state changed for ");
  LOG_DBG_6ADDR(&nbr->ipaddr);
#if UIP_ND6_SEND_NS || UIP_ND6_SEND_RA
  LOG_DBG_(", nscount=%u, state=%u\n", nbr->nscount, nbr->state);
#else /* UIP_ND6_SEND_NS || UIP_ND6_SEND_RA */
  LOG_DBG_(", state=%u\n", nbr->state);
#endif /* UIP_ND6_SEND_NS || UIP_ND6_SEND_RA */
  for(instance = &instance_table[0], end = instance + RPL_MAX_INSTANCES; instance < end; ++instance) {
    if(instance->used == 1 ) {
      p = rpl_find_parent_any_dag(instance, &nbr->ipaddr);
      if(p != NULL) {
        p->rank = RPL_INFINITE_RANK;
        /* Trigger DAG rank recalculation. */
        LOG_DBG("rpl_ipv6_neighbor_callback infinite rank\n");
        p->flags |= RPL_PARENT_FLAG_UPDATED;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_purge_dags(void)
{
  rpl_instance_t *instance;
  rpl_instance_t *end;
  int i;

  for(instance = &instance_table[0], end = instance + RPL_MAX_INSTANCES;
      instance < end; ++instance) {
    if(instance->used) {
      for(i = 0; i < RPL_MAX_DAG_PER_INSTANCE; i++) {
        if(instance->dag_table[i].used) {
          if(instance->dag_table[i].lifetime == 0) {
            if(!instance->dag_table[i].joined) {
              LOG_INFO("Removing dag ");
              LOG_INFO_6ADDR(&instance->dag_table[i].dag_id);
              LOG_INFO_("\n");
              rpl_free_dag(&instance->dag_table[i]);
            }
          } else {
            instance->dag_table[i].lifetime--;
          }
        }
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  uip_ipaddr_t rplmaddr;
  LOG_INFO("rpl-classic started\n");
  default_instance = NULL;

  rpl_dag_init();
  rpl_reset_periodic_timer();
  rpl_icmp6_register_handlers();

  /* add rpl multicast address */
  uip_create_linklocal_rplnodes_mcast(&rplmaddr);
  uip_ds6_maddr_add(&rplmaddr);

#if RPL_CONF_STATS
  memset(&rpl_stats, 0, sizeof(rpl_stats));
#endif

#if RPL_WITH_NON_STORING
  uip_sr_init();
#endif /* RPL_WITH_NON_STORING */
}
/*---------------------------------------------------------------------------*/
static int
get_sr_node_ipaddr(uip_ipaddr_t *addr, const uip_sr_node_t *node)
{
  if(addr != NULL && node != NULL) {
    memcpy(addr, &((rpl_dag_t *)node->graph)->dag_id, 8);
    memcpy(((unsigned char *)addr) + 8, &node->link_identifier, 8);
    return 1;
  } else {
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static void
global_repair(const char *str)
{
  rpl_dag_t *dag = rpl_get_any_dag();
  if(dag != NULL && dag->instance != NULL) {
    rpl_repair_root(dag->instance->instance_id);
  }
}
/*---------------------------------------------------------------------------*/
static void
local_repair(const char *str)
{
  rpl_dag_t *dag = rpl_get_any_dag();
  if(dag != NULL) {
    rpl_local_repair(dag->instance);
  }
}
/*---------------------------------------------------------------------------*/
static void
drop_route(uip_ds6_route_t *route)
{
  /* If we are the root of the network, trigger a global repair before
  the route gets removed */
  rpl_dag_t *dag;
  dag = (rpl_dag_t *)route->state.dag;
  if(dag != NULL && dag->instance != NULL) {
    rpl_repair_root(dag->instance->instance_id);
  }
}
/*---------------------------------------------------------------------------*/
static void
leave_network(void)
{
  LOG_ERR("leave_network not supported in RPL Classic\n");
}
/*---------------------------------------------------------------------------*/
static int
get_root_ipaddr(uip_ipaddr_t *ipaddr)
{
  rpl_dag_t *dag;
  /* Use the DAG id as server address if no other has been specified */
  dag = rpl_get_any_dag();
  if(dag != NULL && ipaddr != NULL) {
    uip_ipaddr_copy(ipaddr, &dag->dag_id);
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
const struct routing_driver rpl_classic_driver = {
  "RPL Classic",
  init,
  rpl_dag_root_set_prefix,
  rpl_dag_root_start,
  rpl_dag_root_is_root,
  get_root_ipaddr,
  get_sr_node_ipaddr,
  leave_network,
  rpl_has_joined,
  rpl_has_downward_route,
  global_repair,
  local_repair,
  rpl_ext_header_remove,
  rpl_ext_header_update,
  rpl_ext_header_hbh_update,
  rpl_ext_header_srh_update,
  rpl_ext_header_srh_get_next_hop,
  rpl_link_callback,
  rpl_ipv6_neighbor_callback,
  drop_route,
};
/*---------------------------------------------------------------------------*/

/** @}*/
