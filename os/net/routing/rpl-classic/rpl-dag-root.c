/*
 * Copyright (c) 2012-2014, Thingsquare, http://www.thingsquare.com/.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "contiki.h"
#include "contiki-net.h"

#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"
#include "net/ipv6/uip-ds6-route.h"

#include "sys/log.h"

#include <string.h>

#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

/*---------------------------------------------------------------------------*/
static void
set_global_address(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
  static uip_ipaddr_t root_ipaddr;
  int i;

  /* Assign a unique local address (RFC4193,
     http://tools.ietf.org/html/rfc4193). */
  if(prefix == NULL) {
    uip_ip6addr(&root_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  } else {
    memcpy(&root_ipaddr, prefix, 8);
  }
  if(iid == NULL) {
    uip_ds6_set_addr_iid(&root_ipaddr, &uip_lladdr);
  } else {
    memcpy(((uint8_t*)&root_ipaddr) + 8, ((uint8_t*)iid) + 8, 8);
  }

  uip_ds6_addr_add(&root_ipaddr, 0, ADDR_AUTOCONF);

  if(LOG_DBG_ENABLED) {
    uint8_t state;

    LOG_DBG("IPv6 addresses: ");
    for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
      state = uip_ds6_if.addr_list[i].state;
      if(uip_ds6_if.addr_list[i].isused &&
         (state == ADDR_TENTATIVE || state == ADDR_PREFERRED)) {
        LOG_DBG_6ADDR(&uip_ds6_if.addr_list[i].ipaddr);
        LOG_DBG_("\n");
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_dag_root_set_prefix(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
  static uint8_t initialized = 0;

  if(!initialized) {
    set_global_address(prefix, iid);
    initialized = 1;
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_dag_root_start(void)
{
  struct uip_ds6_addr *root_if;
  int i;
  uint8_t state;
  uip_ipaddr_t *ipaddr = NULL;

  rpl_dag_root_set_prefix(NULL, NULL);

  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       state == ADDR_PREFERRED &&
       !uip_is_addr_linklocal(&uip_ds6_if.addr_list[i].ipaddr)) {
      ipaddr = &uip_ds6_if.addr_list[i].ipaddr;
    }
  }

  if(ipaddr != NULL) {
    root_if = uip_ds6_addr_lookup(ipaddr);
    if(root_if != NULL) {
      rpl_dag_t *dag;
      uip_ipaddr_t prefix;

      rpl_set_root(RPL_DEFAULT_INSTANCE, ipaddr);
      dag = rpl_get_any_dag();

      /* If there are routes in this dag, we remove them all as we are
         from now on the new dag root and the old routes are wrong */
      if(RPL_IS_STORING(dag->instance)) {
        rpl_remove_routes(dag);
      }
      if(dag->instance != NULL && dag->instance->def_route != NULL) {
	      uip_ds6_defrt_rm(dag->instance->def_route);
        dag->instance->def_route = NULL;
      }

      uip_ip6addr(&prefix, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
      rpl_set_prefix(dag, &prefix, 64);
      LOG_INFO("root_set_prefix: created a new RPL dag\n");
      return 0;
    } else {
      LOG_ERR("root_set_prefix: failed to create a new RPL DAG\n");
      return -1;
    }
  } else {
    LOG_ERR("root_set_prefix_dag: failed to create a new RPL DAG, no preferred IP address found\n");
    return -2;
  }
}
/*---------------------------------------------------------------------------*/
int
rpl_dag_root_is_root(void)
{
  rpl_instance_t *instance;

  instance = rpl_get_default_instance();

  if(instance == NULL) {
    return 0;
  }

  if(instance->current_dag &&
     instance->current_dag->rank == ROOT_RANK(instance)) {
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
