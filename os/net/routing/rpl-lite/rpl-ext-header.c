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
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *         Management of extension headers for ContikiRPL.
 *
 * \author Vincent Brillault <vincent.brillault@imag.fr>,
 *         Joakim Eriksson <joakime@sics.se>,
 *         Niclas Finne <nfi@sics.se>,
 *         Nicolas Tsiftes <nvt@sics.se>,
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#include "net/routing/routing.h"
#include "net/routing/rpl-lite/rpl.h"
#include "net/ipv6/uip-sr.h"
#include "net/packetbuf.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

/*---------------------------------------------------------------------------*/
#define UIP_IP_BUF                ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])
#define UIP_EXT_BUF               ((struct uip_ext_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_HBHO_BUF              ((struct uip_hbho_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_HBHO_NEXT_BUF         ((struct uip_ext_hdr *)&uip_buf[uip_l2_l3_hdr_len + RPL_HOP_BY_HOP_LEN])
#define UIP_RH_BUF                ((struct uip_routing_hdr *)&uip_buf[uip_l2_l3_hdr_len])
#define UIP_RPL_SRH_BUF           ((struct uip_rpl_srh_hdr *)&uip_buf[uip_l2_l3_hdr_len + RPL_RH_LEN])
#define UIP_EXT_HDR_OPT_BUF       ((struct uip_ext_hdr_opt *)&uip_buf[uip_l2_l3_hdr_len + uip_ext_opt_offset])
#define UIP_EXT_HDR_OPT_PADN_BUF  ((struct uip_ext_hdr_opt_padn *)&uip_buf[uip_l2_l3_hdr_len + uip_ext_opt_offset])
#define UIP_EXT_HDR_OPT_RPL_BUF   ((struct uip_ext_hdr_opt_rpl *)&uip_buf[uip_l2_l3_hdr_len + uip_ext_opt_offset])

/*---------------------------------------------------------------------------*/
int
rpl_ext_header_srh_get_next_hop(uip_ipaddr_t *ipaddr)
{
  uint8_t *uip_next_hdr;
  int last_uip_ext_len = uip_ext_len;
  uip_sr_node_t *dest_node;
  uip_sr_node_t *root_node;

  uip_ext_len = 0;
  uip_next_hdr = &UIP_IP_BUF->proto;

  /* Look for routing header */
  while(uip_next_hdr != NULL && *uip_next_hdr != UIP_PROTO_ROUTING) {
    switch(*uip_next_hdr) {
      case UIP_PROTO_HBHO:
      case UIP_PROTO_DESTO:
        /*
         * As per RFC 2460, only the Hop-by-Hop Options header and
         * Destination Options header can appear before the Routing header.
         */
        uip_next_hdr = &UIP_EXT_BUF->next;
        uip_ext_len += (UIP_EXT_BUF->len << 3) + 8;
        break;
      default:
        uip_next_hdr = NULL;
        break;
    }
  }

  if(!rpl_is_addr_in_our_dag(&UIP_IP_BUF->destipaddr)) {
    return 0;
  }

  root_node = uip_sr_get_node(NULL, &curr_instance.dag.dag_id);
  dest_node = uip_sr_get_node(NULL, &UIP_IP_BUF->destipaddr);

  if((uip_next_hdr != NULL && *uip_next_hdr == UIP_PROTO_ROUTING
      && UIP_RH_BUF->routing_type == RPL_RH_TYPE_SRH) ||
     (dest_node != NULL && root_node != NULL &&
      dest_node->parent == root_node)) {
    /* Routing header found or the packet destined for a direct child of the root.
     * The next hop should be already copied as the IPv6 destination
     * address, via rpl_ext_header_srh_update. We turn this address into a link-local to enable
     * forwarding to next hop */
    uip_ipaddr_copy(ipaddr, &UIP_IP_BUF->destipaddr);
    uip_create_linklocal_prefix(ipaddr);
    uip_ext_len = last_uip_ext_len;
    return 1;
  }

  LOG_DBG("no SRH found\n");
  uip_ext_len = last_uip_ext_len;
  return 0;
}
/*---------------------------------------------------------------------------*/
int
rpl_ext_header_srh_update(void)
{
  uint8_t *uip_next_hdr;
  int last_uip_ext_len = uip_ext_len;
  uint8_t cmpri, cmpre;
  uint8_t ext_len;
  uint8_t padding;
  uint8_t path_len;
  uint8_t segments_left;
  uip_ipaddr_t current_dest_addr;

  uip_ext_len = 0;
  uip_next_hdr = &UIP_IP_BUF->proto;

  /* Look for routing header */
  while(uip_next_hdr != NULL && *uip_next_hdr != UIP_PROTO_ROUTING) {
    switch(*uip_next_hdr) {
      case UIP_PROTO_HBHO:
      case UIP_PROTO_DESTO:
        /*
         * As per RFC 2460, only the Hop-by-Hop Options header and
         * Destination Options header can appear before the Routing header.
         */
        uip_next_hdr = &UIP_EXT_BUF->next;
        uip_ext_len += (UIP_EXT_BUF->len << 3) + 8;
        break;
      default:
        uip_next_hdr = NULL;
        break;
    }
  }

  if(uip_next_hdr == NULL || *uip_next_hdr != UIP_PROTO_ROUTING
      || UIP_RH_BUF->routing_type != RPL_RH_TYPE_SRH) {
    LOG_INFO("SRH not found\n");
    uip_ext_len = last_uip_ext_len;
    return 0;
  }

  /* Parse SRH */
  segments_left = UIP_RH_BUF->seg_left;
  ext_len = (UIP_RH_BUF->len * 8) + 8;
  cmpri = UIP_RPL_SRH_BUF->cmpr >> 4;
  cmpre = UIP_RPL_SRH_BUF->cmpr & 0x0f;
  padding = UIP_RPL_SRH_BUF->pad >> 4;
  path_len = ((ext_len - padding - RPL_RH_LEN - RPL_SRH_LEN - (16 - cmpre)) / (16 - cmpri)) + 1;
  (void)path_len;

  LOG_INFO("read SRH, path len %u, segments left %u, Cmpri %u, Cmpre %u, ext len %u (padding %u)\n",
      path_len, segments_left, cmpri, cmpre, ext_len, padding);

  /* Update SRH in-place */
  if(segments_left == 0) {
    /* We are the final destination, do nothing */
  } else {
    uint8_t i = path_len - segments_left; /* The index of the next address to be visited */
    uint8_t *addr_ptr = ((uint8_t *)UIP_RH_BUF) + RPL_RH_LEN + RPL_SRH_LEN + (i * (16 - cmpri));
    uint8_t cmpr = segments_left == 1 ? cmpre : cmpri;

    /* As per RFC6554: swap the IPv6 destination address with address[i] */

    /* First, copy the current IPv6 destination address */
    uip_ipaddr_copy(&current_dest_addr, &UIP_IP_BUF->destipaddr);
    /* Second, update the IPv6 destination address with addresses[i] */
    memcpy(((uint8_t *)&UIP_IP_BUF->destipaddr) + cmpr, addr_ptr, 16 - cmpr);
    /* Third, write current_dest_addr to addresses[i] */
    memcpy(addr_ptr, ((uint8_t *)&current_dest_addr) + cmpr, 16 - cmpr);

    /* Update segments left field */
    UIP_RH_BUF->seg_left--;

    LOG_INFO("SRH next hop ");
    LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
    LOG_INFO_("\n");
  }

  uip_ext_len = last_uip_ext_len;
  return 1;
}
/*---------------------------------------------------------------------------*/
/* Utility function for SRH. Counts the number of bytes in common between
 * two addresses at p1 and p2. */
static int
count_matching_bytes(const void *p1, const void *p2, size_t n)
{
  int i = 0;
  for(i = 0; i < n; i++) {
    if(((uint8_t *)p1)[i] != ((uint8_t *)p2)[i]) {
      return i;
    }
  }
  return n;
}
/*---------------------------------------------------------------------------*/
/* Used by rpl_ext_header_update to insert a RPL SRH extension header. This
 * is used at the root, to initiate downward routing. Returns 1 on success,
 * 0 on failure.
*/
static int
insert_srh_header(void)
{
  /* Implementation of RFC6554 */
  uint8_t temp_len;
  uint8_t path_len;
  uint8_t ext_len;
  uint8_t cmpri, cmpre; /* ComprI and ComprE fields of the RPL Source Routing Header */
  uint8_t *hop_ptr;
  uint8_t padding;
  uip_sr_node_t *dest_node;
  uip_sr_node_t *root_node;
  uip_sr_node_t *node;
  uip_ipaddr_t node_addr;

  LOG_INFO("SRH creating source routing header with destination ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" \n");

  /* Construct source route. We do not do this recursively to keep the runtime stack usage constant. */

  /* Get link of the destination and root */

  if(!rpl_is_addr_in_our_dag(&UIP_IP_BUF->destipaddr)) {
    /* The destination is not in our DAG, skip SRH insertion */
    LOG_INFO("SRH destination not in our DAG, skip SRH insertion\n");
    return 1;
  }

  dest_node = uip_sr_get_node(NULL, &UIP_IP_BUF->destipaddr);
  if(dest_node == NULL) {
    /* The destination is not found, skip SRH insertion */
    LOG_INFO("SRH node not found, skip SRH insertion\n");
    return 1;
  }

  root_node = uip_sr_get_node(NULL, &curr_instance.dag.dag_id);
  if(root_node == NULL) {
    LOG_ERR("SRH root node not found\n");
    return 0;
  }

  if(!uip_sr_is_addr_reachable(NULL, &UIP_IP_BUF->destipaddr)) {
    LOG_ERR("SRH no path found to destination\n");
    return 0;
  }

  /* Compute path length and compression factors (we use cmpri == cmpre) */
  path_len = 0;
  node = dest_node->parent;
  /* For simplicity, we use cmpri = cmpre */
  cmpri = 15;
  cmpre = 15;

  /* Note that in case of a direct child (node == root_node), we insert
  SRH anyway, as RFC 6553 mandates that routed datagrams must include
  SRH or the RPL option (or both) */

  while(node != NULL && node != root_node) {

    NETSTACK_ROUTING.get_sr_node_ipaddr(&node_addr, node);

    /* How many bytes in common between all nodes in the path? */
    cmpri = MIN(cmpri, count_matching_bytes(&node_addr, &UIP_IP_BUF->destipaddr, 16));
    cmpre = cmpri;

    LOG_INFO("SRH Hop ");
    LOG_INFO_6ADDR(&node_addr);
    LOG_INFO_("\n");
    node = node->parent;
    path_len++;
  }

  /* Extension header length: fixed headers + (n-1) * (16-ComprI) + (16-ComprE)*/
  ext_len = RPL_RH_LEN + RPL_SRH_LEN
      + (path_len - 1) * (16 - cmpre)
      + (16 - cmpri);

  padding = ext_len % 8 == 0 ? 0 : (8 - (ext_len % 8));
  ext_len += padding;

  LOG_INFO("SRH path len: %u, ComprI %u, ComprE %u, ext len %u (padding %u)\n",
      path_len, cmpri, cmpre, ext_len, padding);

  /* Check if there is enough space to store the extension header */
  if(uip_len + ext_len > UIP_BUFSIZE - UIP_LLH_LEN) {
    LOG_ERR("packet too long: impossible to add source routing header (%u bytes)\n", ext_len);
    return 0;
  }

  /* Move existing ext headers and payload uip_ext_len further */
  memmove(uip_buf + uip_l2_l3_hdr_len + ext_len,
      uip_buf + uip_l2_l3_hdr_len, uip_len - UIP_IPH_LEN);
  memset(uip_buf + uip_l2_l3_hdr_len, 0, ext_len);

  /* Insert source routing header */
  UIP_RH_BUF->next = UIP_IP_BUF->proto;
  UIP_IP_BUF->proto = UIP_PROTO_ROUTING;

  /* Initialize IPv6 Routing Header */
  UIP_RH_BUF->len = (ext_len - 8) / 8;
  UIP_RH_BUF->routing_type = RPL_RH_TYPE_SRH;
  UIP_RH_BUF->seg_left = path_len;

  /* Initialize RPL Source Routing Header */
  UIP_RPL_SRH_BUF->cmpr = (cmpri << 4) + cmpre;
  UIP_RPL_SRH_BUF->pad = padding << 4;

  /* Initialize addresses field (the actual source route).
   * From last to first. */
  node = dest_node;
  hop_ptr = ((uint8_t *)UIP_RH_BUF) + ext_len - padding; /* Pointer where to write the next hop compressed address */

  while(node != NULL && node->parent != root_node) {
    NETSTACK_ROUTING.get_sr_node_ipaddr(&node_addr, node);

    hop_ptr -= (16 - cmpri);
    memcpy(hop_ptr, ((uint8_t*)&node_addr) + cmpri, 16 - cmpri);

    node = node->parent;
  }

  /* The next hop (i.e. node whose parent is the root) is placed as the current IPv6 destination */
  NETSTACK_ROUTING.get_sr_node_ipaddr(&node_addr, node);
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &node_addr);

  /* In-place update of IPv6 length field */
  temp_len = UIP_IP_BUF->len[1];
  UIP_IP_BUF->len[1] += ext_len;
  if(UIP_IP_BUF->len[1] < temp_len) {
    UIP_IP_BUF->len[0]++;
  }

  uip_ext_len += ext_len;
  uip_len += ext_len;

  return 1;
}
/*---------------------------------------------------------------------------*/
int
rpl_ext_header_hbh_update(int uip_ext_opt_offset)
{
  int down;
  int rank_error_signaled;
  int loop_detected;
  uint16_t sender_rank;
  uint8_t sender_closer;
  rpl_nbr_t *sender;
  uint8_t opt_type = UIP_EXT_HDR_OPT_RPL_BUF->opt_type;
  uint8_t opt_len = UIP_EXT_HDR_OPT_RPL_BUF->opt_len;

  if(UIP_HBHO_BUF->len != ((RPL_HOP_BY_HOP_LEN - 8) / 8)
      || opt_type != UIP_EXT_HDR_OPT_RPL
      || opt_len != RPL_HDR_OPT_LEN) {
    LOG_ERR("hop-by-hop extension header has wrong size or type (%u %u %u)\n",
        UIP_HBHO_BUF->len, opt_type, opt_len);
    return 0; /* Drop */
  }

  if(!curr_instance.used || curr_instance.instance_id != UIP_EXT_HDR_OPT_RPL_BUF->instance) {
    LOG_ERR("unknown instance: %u\n",
           UIP_EXT_HDR_OPT_RPL_BUF->instance);
    return 0; /* Drop */
  }

  if(UIP_EXT_HDR_OPT_RPL_BUF->flags & RPL_HDR_OPT_FWD_ERR) {
    LOG_ERR("forward error!\n");
    return 0; /* Drop */
  }

  down = (UIP_EXT_HDR_OPT_RPL_BUF->flags & RPL_HDR_OPT_DOWN) ? 1 : 0;
  sender_rank = UIP_HTONS(UIP_EXT_HDR_OPT_RPL_BUF->senderrank);
  sender = nbr_table_get_from_lladdr(rpl_neighbors, packetbuf_addr(PACKETBUF_ADDR_SENDER));
  rank_error_signaled = (UIP_EXT_HDR_OPT_RPL_BUF->flags & RPL_HDR_OPT_RANK_ERR) ? 1 : 0;
  sender_closer = sender_rank < curr_instance.dag.rank;
  loop_detected = (down && !sender_closer) || (!down && sender_closer);

  LOG_INFO("ext hdr: packet from ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_(" going %s, sender closer %d (%d < %d), rank error %u, loop detected %u\n",
      down == 1 ? "down" : "up", sender_closer, sender_rank,
      curr_instance.dag.rank, rank_error_signaled, loop_detected);

  if(loop_detected) {
    /* Set forward error flag */
    UIP_EXT_HDR_OPT_RPL_BUF->flags |= RPL_HDR_OPT_RANK_ERR;
  }

  return rpl_process_hbh(sender, sender_rank, loop_detected, rank_error_signaled);
}
/*---------------------------------------------------------------------------*/
/* In-place update of the RPL HBH extension header, when already present
 * in the uIP packet. Used by insert_hbh_header and rpl_ext_header_update.
 * Returns 1 on success, 0 on failure. */
static int
update_hbh_header(void)
{
  int uip_ext_opt_offset;
  int last_uip_ext_len;

  last_uip_ext_len = uip_ext_len;
  uip_ext_len = 0;
  uip_ext_opt_offset = 2;

  if(UIP_IP_BUF->proto == UIP_PROTO_HBHO && UIP_EXT_HDR_OPT_RPL_BUF->opt_type == UIP_EXT_HDR_OPT_RPL) {
    if(UIP_HBHO_BUF->len != ((RPL_HOP_BY_HOP_LEN - 8) / 8)
        || UIP_EXT_HDR_OPT_RPL_BUF->opt_len != RPL_HDR_OPT_LEN) {

      LOG_ERR("hop-by-hop extension header has wrong size (%u %u)\n",
          UIP_EXT_HDR_OPT_RPL_BUF->opt_len, uip_ext_len);
      return 0; /* Drop */
    }

    if(!curr_instance.used || curr_instance.instance_id != UIP_EXT_HDR_OPT_RPL_BUF->instance) {
      LOG_ERR("unable to add/update hop-by-hop extension header: incorrect instance\n");
      uip_ext_len = last_uip_ext_len;
      return 0; /* Drop */
    }

    /* Update sender rank and instance, will update flags next */
    UIP_EXT_HDR_OPT_RPL_BUF->senderrank = UIP_HTONS(curr_instance.dag.rank);
    UIP_EXT_HDR_OPT_RPL_BUF->instance = curr_instance.instance_id;
  }

  uip_ext_len = last_uip_ext_len;
  return 1;
}
/*---------------------------------------------------------------------------*/
/* Used by rpl_ext_header_update on packets without an HBH extension header,
 * for packets initated by non-root nodes.
 * Inserts and initalizes (via update_hbh_header) a RPL HBH ext header.
 * Returns 1 on success, 0 on failure. */
static int
insert_hbh_header(void)
{
  int uip_ext_opt_offset;
  int last_uip_ext_len;
  uint8_t temp_len;

  last_uip_ext_len = uip_ext_len;
  uip_ext_len = 0;
  uip_ext_opt_offset = 2;

  /* Insert hop-by-hop header */
  LOG_INFO("creating hop-by-hop option\n");
  if(uip_len + RPL_HOP_BY_HOP_LEN > UIP_BUFSIZE - UIP_LLH_LEN) {
    LOG_ERR("packet too long: impossible to add hop-by-hop option\n");
    uip_ext_len = last_uip_ext_len;
    return 0;
  }

  /* Move existing ext headers and payload UIP_EXT_BUF further */
  memmove(UIP_HBHO_NEXT_BUF, UIP_EXT_BUF, uip_len - UIP_IPH_LEN);
  memset(UIP_HBHO_BUF, 0, RPL_HOP_BY_HOP_LEN);

  /* Update IP and HBH protocol and fields */
  UIP_HBHO_BUF->next = UIP_IP_BUF->proto;
  UIP_IP_BUF->proto = UIP_PROTO_HBHO;

  /* Initialize HBH option */
  UIP_HBHO_BUF->len = (RPL_HOP_BY_HOP_LEN - 8) / 8;
  UIP_EXT_HDR_OPT_RPL_BUF->opt_type = UIP_EXT_HDR_OPT_RPL;
  UIP_EXT_HDR_OPT_RPL_BUF->opt_len = RPL_HDR_OPT_LEN;
  UIP_EXT_HDR_OPT_RPL_BUF->flags = 0;
  UIP_EXT_HDR_OPT_RPL_BUF->senderrank = UIP_HTONS(curr_instance.dag.rank);
  UIP_EXT_HDR_OPT_RPL_BUF->instance = curr_instance.instance_id;
  uip_len += RPL_HOP_BY_HOP_LEN;
  temp_len = UIP_IP_BUF->len[1];
  UIP_IP_BUF->len[1] += RPL_HOP_BY_HOP_LEN;
  if(UIP_IP_BUF->len[1] < temp_len) {
    UIP_IP_BUF->len[0]++;
  }

  uip_ext_len = last_uip_ext_len + RPL_HOP_BY_HOP_LEN;

  /* Update header before returning */
  return update_hbh_header();
}
/*---------------------------------------------------------------------------*/
int
rpl_ext_header_update(void)
{
  if(!curr_instance.used
      || uip_is_addr_linklocal(&UIP_IP_BUF->destipaddr)
      || uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
    return 1;
  }

  if(rpl_dag_root_is_root()) {
    /* At the root, remove headers if any, and insert SRH or HBH
    * (SRH is inserted only if the destination is down the DODAG) */
    rpl_ext_header_remove();
    /* Insert SRH (if needed) */
    return insert_srh_header();
  } else {
    if(uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)
        && UIP_IP_BUF->ttl == uip_ds6_if.cur_hop_limit) {
      /* Insert HBH option at source. Checking the address is not sufficient because
       * in non-storing mode, a packet may go up and then down the same path again */
      return insert_hbh_header();
    } else {
      /* Update HBH option at forwarders */
      return update_hbh_header();
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rpl_ext_header_remove(void)
{
  uint8_t temp_len;
  uint8_t rpl_ext_hdr_len;
  int uip_ext_opt_offset;
  uint8_t *uip_next_hdr;

  uip_ext_len = 0;
  uip_ext_opt_offset = 2;
  uip_next_hdr = &UIP_IP_BUF->proto;

  /* Look for hop-by-hop and routing headers */
  while(uip_next_hdr != NULL) {
    switch(*uip_next_hdr) {
      case UIP_PROTO_HBHO:
      case UIP_PROTO_ROUTING:
        if((*uip_next_hdr != UIP_PROTO_HBHO || UIP_EXT_HDR_OPT_RPL_BUF->opt_type == UIP_EXT_HDR_OPT_RPL)) {
          /* Remove hop-by-hop and routing headers */
          *uip_next_hdr = UIP_EXT_BUF->next;
          rpl_ext_hdr_len = (UIP_EXT_BUF->len * 8) + 8;
          temp_len = UIP_IP_BUF->len[1];
          uip_len -= rpl_ext_hdr_len;
          UIP_IP_BUF->len[1] -= rpl_ext_hdr_len;
          if(UIP_IP_BUF->len[1] > temp_len) {
            UIP_IP_BUF->len[0]--;
          }
          LOG_INFO("removing RPL extension header (type %u, len %u)\n", *uip_next_hdr, rpl_ext_hdr_len);
          memmove(UIP_EXT_BUF, ((uint8_t *)UIP_EXT_BUF) + rpl_ext_hdr_len, uip_len - UIP_IPH_LEN);
        } else {
          uip_next_hdr = &UIP_EXT_BUF->next;
          uip_ext_len += (UIP_EXT_BUF->len << 3) + 8;
        }
        break;
      case UIP_PROTO_DESTO:
        /*
         * As per RFC 2460, any header other than the Destination
         * Options header does not appear between the Hop-by-Hop
         * Options header and the Routing header.
         *
         * We're moving to the next header only if uip_next_hdr has
         * UIP_PROTO_DESTO. Otherwise, we'll return.
         */
        /* Move to next header */
        uip_next_hdr = &UIP_EXT_BUF->next;
        uip_ext_len += (UIP_EXT_BUF->len << 3) + 8;
        break;
    default:
      return;
    }
  }
}

/** @}*/
