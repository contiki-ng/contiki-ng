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
*
* \file
     Per-deployment MAC <-> nodeid mapping
* \author Simon Duquennoy <simon.duquennoy@ri.se>
*
*/

#ifndef DEPLOYMENT_H_
#define DEPLOYMENT_H_

#include "contiki-conf.h"
#include "sys/node-id.h"
#include "net/ipv6/uip.h"
#include "net/linkaddr.h"

/**
 * \brief ID<->MAC address mapping structure
 */
struct id_mac {
  uint16_t id;
  linkaddr_t mac;
};

/**
 * DEPLOYMENT_MAPPING:
 * A table of struct id_mac that provides ID-MAC mapping for a deployment.
 * Example with four nodes:
 * In configuration file:
 *  \#define DEPLOYMENT_MAPPING custom_array
 * In a .c file:
 *  const struct id_mac custom_array[] = {
      { 1, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb6,0x14}}},
      { 2, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0xe7}}},
      { 3, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb4,0x35}}},
      { 4, {{0x00,0x12,0x4b,0x00,0x06,0x0d,0xb1,0xcf}}},
      { 0, {{0}}}
    };
 */

/**
 * Initialize the deployment module
 */
void deployment_init(void);

/**
 * Get the number of nodes for the deployment (length of mapping table)
 *
 * \return The number of nodes in the deployment
 */
int deployment_node_count(void);

/**
 * Get node ID from a link-layer address, from the deployment mapping table
 *
 * \param lladdr The link-layer address to look up for
 * \return Node ID from a corresponding link-layer address
 */
uint16_t deployment_id_from_lladdr(const linkaddr_t *lladdr);

/**
 * Get node link-layer address from a node ID, from the deployment mapping table
 *
 * \param lladdr A pointer where to write the link-layer address
 * \param id The node ID to look up for
 */
void deployment_lladdr_from_id(linkaddr_t *lladdr, uint16_t id);

/**
 * Get node ID from the IID of an IPv6 address
 *
 * \param ipaddr The IPv6 (global or link-local) address that contains the IID
 * \return Node ID from a corresponding IID
 */
uint16_t deployment_id_from_iid(const uip_ipaddr_t *ipaddr);

/**
 * Get IPv6 IID from node IDs
 *
 * \param ipaddr The IPv6 where to write the IID
 * \param id The node ID
 */
void deployment_iid_from_id(uip_ipaddr_t *ipaddr, uint16_t id);

/**
 * Get node ID from index in mapping table
 *
 * \param index The index in the deployment mapping table
 * \return Node ID at the corresponding index
 */
uint16_t deployment_id_from_index(uint16_t index);

#endif /* DEPLOYMENT_H_ */
/** @} */
