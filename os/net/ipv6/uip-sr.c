/*
 * Copyright (c) 2016, Inria.
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
 * \addtogroup uip
 * @{
 *
 * \file
 *         Source routing support
 *
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#include "contiki.h"
#include "net/ipv6/uip-sr.h"
#include "net/ipv6/uiplib.h"
#include "net/routing/routing.h"
#include "lib/list.h"
#include "lib/memb.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "IPv6 SR"
#define LOG_LEVEL LOG_LEVEL_IPV6

/* Total number of nodes */
static int num_nodes;

/* Every known node in the network */
LIST(nodelist);
MEMB(nodememb, uip_sr_node_t, UIP_SR_LINK_NUM);

/*---------------------------------------------------------------------------*/
int
uip_sr_num_nodes(void)
{
  return num_nodes;
}
/*---------------------------------------------------------------------------*/
static int
node_matches_address(const void *graph, const uip_sr_node_t *node,
                     const uip_ipaddr_t *addr)
{
  if(node == NULL || addr == NULL || graph != node->graph) {
    return 0;
  } else {
    uip_ipaddr_t node_ipaddr;
    NETSTACK_ROUTING.get_sr_node_ipaddr(&node_ipaddr, node);
    return uip_ipaddr_cmp(&node_ipaddr, addr);
  }
}
/*---------------------------------------------------------------------------*/
uip_sr_node_t *
uip_sr_get_node(const void *graph, const uip_ipaddr_t *addr)
{
  uip_sr_node_t *l;
  for(l = list_head(nodelist); l != NULL; l = list_item_next(l)) {
    /* Compare prefix and node identifier */
    if(node_matches_address(graph, l, addr)) {
      return l;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
int
uip_sr_is_addr_reachable(const void *graph, const uip_ipaddr_t *addr)
{
  int max_depth = UIP_SR_LINK_NUM;
  uip_ipaddr_t root_ipaddr;
  uip_sr_node_t *node;
  uip_sr_node_t *root_node;

  NETSTACK_ROUTING.get_root_ipaddr(&root_ipaddr);
  node = uip_sr_get_node(graph, addr);
  root_node = uip_sr_get_node(graph, &root_ipaddr);

  while(node != NULL && node != root_node && max_depth > 0) {
    node = node->parent;
    max_depth--;
  }
  return node != NULL && node == root_node;
}
/*---------------------------------------------------------------------------*/
void
uip_sr_expire_parent(const void *graph, const uip_ipaddr_t *child,
                     const uip_ipaddr_t *parent)
{
  uip_sr_node_t *l = uip_sr_get_node(graph, child);
  /* Check if parent matches */
  if(l != NULL && node_matches_address(graph, l->parent, parent)) {
    if(l->lifetime > UIP_SR_REMOVAL_DELAY) {
      l->lifetime = UIP_SR_REMOVAL_DELAY;
    }
  }
}
/*---------------------------------------------------------------------------*/
uip_sr_node_t *
uip_sr_update_node(void *graph, const uip_ipaddr_t *child,
                   const uip_ipaddr_t *parent, uint32_t lifetime)
{
  uip_sr_node_t *child_node = uip_sr_get_node(graph, child);
  uip_sr_node_t *parent_node = uip_sr_get_node(graph, parent);
  uip_sr_node_t *old_parent_node;

  if(parent != NULL) {
    /* No node for the parent, add one with infinite lifetime */
    if(parent_node == NULL) {
      parent_node = uip_sr_update_node(graph, parent, NULL, UIP_SR_INFINITE_LIFETIME);
      if(parent_node == NULL) {
        LOG_ERR("NS: no space left for root node!\n");
        return NULL;
      }
    }
  }

  /* No node for this child, add one */
  if(child_node == NULL) {
    child_node = memb_alloc(&nodememb);
    /* No space left, abort */
    if(child_node == NULL) {
      LOG_ERR("NS: no space left for child ");
      LOG_ERR_6ADDR(child);
      LOG_ERR_("\n");
      return NULL;
    }
    child_node->parent = NULL;
    list_add(nodelist, child_node);
    num_nodes++;
  }

  /* Initialize node */
  child_node->graph = graph;
  child_node->lifetime = lifetime;
  memcpy(child_node->link_identifier, ((const unsigned char *)child) + 8, 8);

  /* Is the node reachable before the update? */
  if(uip_sr_is_addr_reachable(graph, child)) {
    old_parent_node = child_node->parent;
    /* Update node */
    child_node->parent = parent_node;
    /* Has the node become unreachable? May happen if we create a loop. */
    if(!uip_sr_is_addr_reachable(graph, child)) {
      /* The new parent makes the node unreachable, restore old parent.
       * We will take the update next time, with chances we know more of
       * the topology and the loop is gone. */
      child_node->parent = old_parent_node;
    }
  } else {
    child_node->parent = parent_node;
  }

  LOG_INFO("NS: updating link, child ");
  LOG_INFO_6ADDR(child);
  LOG_INFO_(", parent ");
  LOG_INFO_6ADDR(parent);
  LOG_INFO_(", lifetime %u, num_nodes %u\n", (unsigned)lifetime, num_nodes);

  return child_node;
}
/*---------------------------------------------------------------------------*/
void
uip_sr_init(void)
{
  num_nodes = 0;
  memb_init(&nodememb);
  list_init(nodelist);
}
/*---------------------------------------------------------------------------*/
uip_sr_node_t *
uip_sr_node_head(void)
{
  return list_head(nodelist);
}
/*---------------------------------------------------------------------------*/
uip_sr_node_t *
uip_sr_node_next(const uip_sr_node_t *item)
{
  return list_item_next(item);
}
/*---------------------------------------------------------------------------*/
void
uip_sr_periodic(unsigned seconds)
{
  uip_sr_node_t *l;
  uip_sr_node_t *next;

  /* First pass, for all expired nodes, deallocate them iff no child points to them */
  for(l = list_head(nodelist); l != NULL; l = next) {
    next = list_item_next(l);
    if(l->lifetime == 0) {
      uip_sr_node_t *l2;
      int can_be_removed = 1;
      for(l2 = list_head(nodelist); l2 != NULL; l2 = list_item_next(l2)) {
        if(l2->parent == l) {
          can_be_removed = 0;
          break;
        }
      }
      if(can_be_removed) {
        /* No child found, deallocate node */
        if(LOG_INFO_ENABLED) {
          uip_ipaddr_t node_addr;
          NETSTACK_ROUTING.get_sr_node_ipaddr(&node_addr, l);
          LOG_INFO("NS: removing expired node ");
          LOG_INFO_6ADDR(&node_addr);
          LOG_INFO_("\n");
        }
        list_remove(nodelist, l);
        memb_free(&nodememb, l);
        num_nodes--;
      }
    } else if(l->lifetime != UIP_SR_INFINITE_LIFETIME) {
      l->lifetime = l->lifetime > seconds ? l->lifetime - seconds : 0;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
uip_sr_free_all(void)
{
  uip_sr_node_t *l;
  uip_sr_node_t *next;
  for(l = list_head(nodelist); l != NULL; l = next) {
    next = list_item_next(l);
    list_remove(nodelist, l);
    memb_free(&nodememb, l);
    num_nodes--;
  }
}
/*---------------------------------------------------------------------------*/
int
uip_sr_link_snprint(char *buf, int buflen, const uip_sr_node_t *link)
{
  int index = 0;
  uip_ipaddr_t child_ipaddr;
  uip_ipaddr_t parent_ipaddr;

  NETSTACK_ROUTING.get_sr_node_ipaddr(&child_ipaddr, link);
  NETSTACK_ROUTING.get_sr_node_ipaddr(&parent_ipaddr, link->parent);

  if(LOG_WITH_COMPACT_ADDR) {
    index += log_6addr_compact_snprint(buf+index, buflen-index, &child_ipaddr);
  } else {
    index += uiplib_ipaddr_snprint(buf+index, buflen-index, &child_ipaddr);
  }
  if(index >= buflen) {
    return index;
  }

  if(link->parent == NULL) {
    index += snprintf(buf+index, buflen-index, "  (DODAG root)");
    if(index >= buflen) {
      return index;
    }
  } else {
    index += snprintf(buf+index, buflen-index, "  to ");
    if(index >= buflen) {
      return index;
    }
    if(LOG_WITH_COMPACT_ADDR) {
      index += log_6addr_compact_snprint(buf+index, buflen-index, &parent_ipaddr);
    } else {
      index += uiplib_ipaddr_snprint(buf+index, buflen-index, &parent_ipaddr);
    }
    if(index >= buflen) {
      return index;
    }
  }
  if(link->lifetime != UIP_SR_INFINITE_LIFETIME) {
    index += snprintf(buf+index, buflen-index,
              " (lifetime: %lu seconds)", (unsigned long)link->lifetime);
    if(index >= buflen) {
      return index;
    }
  } else {
    index += snprintf(buf+index, buflen-index, " (lifetime: infinite)");
    if(index >= buflen) {
      return index;
    }
  }
  return index;
}
/** @} */
