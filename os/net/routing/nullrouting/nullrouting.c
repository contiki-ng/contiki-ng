/*
 * Copyright (c) 2017, RISE SICS.
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
 * \addtogroup null-routing
 * @{
 *
 * \file
 *         A routing protocol that does nothing
 *
 * \author Simon Duquennoy <simon.duquennoy@ri.se>
 */

#include "net/routing/routing.h"

/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static void
root_set_prefix(uip_ipaddr_t *prefix, uip_ipaddr_t *iid)
{
}
/*---------------------------------------------------------------------------*/
static int
root_start(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
node_is_root(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
get_root_ipaddr(uip_ipaddr_t *ipaddr)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
get_sr_node_ipaddr(uip_ipaddr_t *addr, const uip_sr_node_t *node)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
leave_network(void)
{
}
/*---------------------------------------------------------------------------*/
static int
node_has_joined(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
node_is_reachable(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
global_repair(const char *str)
{
}
/*---------------------------------------------------------------------------*/
static void
local_repair(const char *str)
{
}
/*---------------------------------------------------------------------------*/
static void
ext_header_remove(void)
{
#if NETSTACK_CONF_WITH_IPV6
  uip_ext_len = 0;
#endif /* NETSTACK_CONF_WITH_IPV6 */
}
/*---------------------------------------------------------------------------*/
static int
ext_header_update(void)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ext_header_hbh_update(int uip_ext_opt_offset)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ext_header_srh_update(void)
{
  return 0; /* Means SRH not found */
}
/*---------------------------------------------------------------------------*/
static int
ext_header_srh_get_next_hop(uip_ipaddr_t *ipaddr)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
link_callback(const linkaddr_t *addr, int status, int numtx)
{
}
/*---------------------------------------------------------------------------*/
static void
neighbor_state_changed(uip_ds6_nbr_t *nbr)
{
}
/*---------------------------------------------------------------------------*/
static void
drop_route(uip_ds6_route_t *route)
{
}
/*---------------------------------------------------------------------------*/
const struct routing_driver nullrouting_driver = {
  "nullrouting",
  init,
  root_set_prefix,
  root_start,
  node_is_root,
  get_root_ipaddr,
  get_sr_node_ipaddr,
  leave_network,
  node_has_joined,
  node_is_reachable,
  global_repair,
  local_repair,
  ext_header_remove,
  ext_header_update,
  ext_header_hbh_update,
  ext_header_srh_update,
  ext_header_srh_get_next_hop,
  link_callback,
  neighbor_state_changed,
  drop_route,
};
/*---------------------------------------------------------------------------*/

/** @}*/
