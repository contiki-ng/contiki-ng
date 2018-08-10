/*
 * Copyright (c) 2018, RISE SICS.
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
 */

/**
* \addtogroup deployment
* @{
*/

/**
 * \file
 *         Code managing id<->mac address<->IPv6 address mapping, and doing this
 *         for different deployment scenarios: Cooja, Nodes, Indriya or Twist testbeds
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"
#include "contiki-net.h"
#include "deployment.h"
#include "sys/node-id.h"
#include <string.h>
#include <stdio.h>

/**
 * \brief List of ID<->MAC mapping used for different deployments
 */
extern const struct id_mac DEPLOYMENT_MAPPING[];
/**
 * \brief The number of nodes in the deployment
 */
static int node_count = 0;

/*---------------------------------------------------------------------------*/
void
deployment_init(void)
{
  const struct id_mac *curr = DEPLOYMENT_MAPPING;
  /* Initialize node_id */
  node_id = deployment_id_from_lladdr((const linkaddr_t *)&linkaddr_node_addr);
  /* Count nodes */
  node_count = 0;
  while(curr->id != 0) {
    node_count++;
    curr++;
  }
}
/*---------------------------------------------------------------------------*/
int
deployment_node_count(void)
{
  return node_count;
}
/*---------------------------------------------------------------------------*/
uint16_t
deployment_id_from_lladdr(const linkaddr_t *lladdr)
{
  const struct id_mac *curr = DEPLOYMENT_MAPPING;
  if(lladdr == NULL) {
    return 0;
  }
  while(curr->id != 0) {
    /* Assume network-wide unique 16-bit MAC addresses */
    if(linkaddr_cmp(lladdr, &curr->mac)) {
      return curr->id;
    }
    curr++;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
deployment_lladdr_from_id(linkaddr_t *lladdr, uint16_t id)
{
  const struct id_mac *curr = DEPLOYMENT_MAPPING;
  if(id == 0 || lladdr == NULL) {
    return;
  }
  while(curr->id != 0) {
    if(curr->id == id) {
      linkaddr_copy(lladdr, &curr->mac);
      return;
    }
    curr++;
  }
}
/*---------------------------------------------------------------------------*/
uint16_t
deployment_id_from_iid(const uip_ipaddr_t *ipaddr)
{
  const linkaddr_t lladdr;
  uip_ds6_set_lladdr_from_iid((uip_lladdr_t *)&lladdr, ipaddr);
  return deployment_id_from_lladdr(&lladdr);
}
/*---------------------------------------------------------------------------*/
void
deployment_iid_from_id(uip_ipaddr_t *ipaddr, uint16_t id)
{
  linkaddr_t lladdr;
  deployment_lladdr_from_id(&lladdr, id);
  uip_ds6_set_addr_iid(ipaddr, (uip_lladdr_t *)&lladdr);
}
/*---------------------------------------------------------------------------*/
uint16_t
deployment_id_from_index(uint16_t index)
{
  if(index < deployment_node_count()) {
    return DEPLOYMENT_MAPPING[index].id;
  } else {
    return 0;
  }
}
/*---------------------------------------------------------------------------*/

/** @} */
