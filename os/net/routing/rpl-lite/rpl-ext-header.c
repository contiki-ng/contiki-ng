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
int
rpl_ext_header_srh_get_next_hop(uip_ipaddr_t *ipaddr)
{
  struct uip_routing_hdr *rh_header;
  uip_sr_node_t *dest_node;
  uip_sr_node_t *root_node;

  /* Look for routing ext header */
  rh_header = (struct uip_routing_hdr *)uipbuf_search_header(uip_buf, uip_len, UIP_PROTO_ROUTING);

  if(!rpl_is_addr_in_our_dag(&UIP_IP_BUF->destipaddr)) {
    return 0;
  }

  root_node = uip_sr_get_node(NULL, &curr_instance.dag.dag_id);
  dest_node = uip_sr_get_node(NULL, &UIP_IP_BUF->destipaddr);

  if((rh_header != NULL && rh_header->routing_type == RPL_RH_TYPE_SRH) ||
     (dest_node != NULL && root_node != NULL &&
      dest_node->parent == root_node)) {
    /* Routing header found or the packet destined for a direct child of the root.
     * The next hop should be already copied as the IPv6 destination
     * address, via rpl_ext_header_srh_update. We turn this address into a link-local to enable
     * forwarding to next hop */
    uip_ipaddr_copy(ipaddr, &UIP_IP_BUF->destipaddr);
    uip_create_linklocal_prefix(ipaddr);
    return 1;
  }

  LOG_DBG("no SRH found\n");
  return 0;
}
/*---------------------------------------------------------------------------*/
int
rpl_ext_header_srh_update(void)
{
  struct uip_routing_hdr *rh_header;
  struct uip_rpl_srh_hdr *srh_header;
  uint8_t cmpri, cmpre;
  uint8_t ext_len;
  uint8_t padding;
  uint8_t path_len;
  uint8_t segments_left;
  uip_ipaddr_t current_dest_addr;

  /* Look for routing ext header */
  rh_header = (struct uip_routing_hdr *)uipbuf_search_header(uip_buf, uip_len, UIP_PROTO_ROUTING);

  if(rh_header == NULL || rh_header->routing_type != RPL_RH_TYPE_SRH) {
    LOG_INFO("SRH not found\n");
    return 0;
  }

  /* Parse SRH */
  srh_header = (struct uip_rpl_srh_hdr *)(((uint8_t *)rh_header) + RPL_RH_LEN);
  segments_left = rh_header->seg_left;
  ext_len = rh_header->len * 8 + 8;
  cmpri = srh_header->cmpr >> 4;
  cmpre = srh_header->cmpr & 0x0f;
  padding = srh_header->pad >> 4;
  path_len = ((ext_len - padding - RPL_RH_LEN - RPL_SRH_LEN - (16 - cmpre)) / (16 - cmpri)) + 1;
  (void)path_len;

  LOG_INFO("read SRH, path len %u, segments left %u, Cmpri %u, Cmpre %u, ext len %u (padding %u)\n",
      path_len, segments_left, cmpri, cmpre, ext_len, padding);

  /* Update SRH in-place */
  if(segments_left == 0) {
    /* We are the final destination, do nothing */
  } else if(segments_left > path_len) {
    /* Discard the packet because of a parameter problem. */
    LOG_ERR("SRH with too many segments left (%u > %u)\n",
            segments_left, path_len);
    return 0;
  } else {
    uint8_t i = path_len - segments_left; /* The index of the next address to be visited */
    uint8_t *addr_ptr = ((uint8_t *)rh_header) + RPL_RH_LEN + RPL_SRH_LEN + (i * (16 - cmpri));
    uint8_t cmpr = segments_left == 1 ? cmpre : cmpri;

    /* As per RFC6554: swap the IPv6 destination address with address[i] */

    /* First, copy the current IPv6 destination address */
    uip_ipaddr_copy(&current_dest_addr, &UIP_IP_BUF->destipaddr);
    /* Second, update the IPv6 destination address with addresses[i] */
    memcpy(((uint8_t *)&UIP_IP_BUF->destipaddr) + cmpr, addr_ptr, 16 - cmpr);
    /* Third, write current_dest_addr to addresses[i] */
    memcpy(addr_ptr, ((uint8_t *)&current_dest_addr) + cmpr, 16 - cmpr);

    /* Update segments left field */
    rh_header->seg_left--;

    LOG_INFO("SRH next hop ");
    LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
    LOG_INFO_("\n");
  }

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
  uint8_t path_len;
  uint8_t ext_len;
  uint8_t cmpri, cmpre; /* ComprI and ComprE fields of the RPL Source Routing Header */
  uint8_t *hop_ptr;
  uint8_t padding;
  uip_sr_node_t *dest_node;
  uip_sr_node_t *root_node;
  uip_sr_node_t *node;
  uip_ipaddr_t node_addr;

  /* Always insest SRH as first extension header */
  struct uip_routing_hdr *rh_hdr = (struct uip_routing_hdr *)UIP_IP_PAYLOAD(0);
  struct uip_rpl_srh_hdr *srh_hdr = (struct uip_rpl_srh_hdr *)(UIP_IP_PAYLOAD(0) + RPL_RH_LEN);

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
  if(uip_len + ext_len > UIP_LINK_MTU) {
    LOG_ERR("packet too long: impossible to add source routing header (%u bytes)\n", ext_len);
    return 0;
  }

  /* Move existing ext headers and payload ext_len further */
  memmove(uip_buf + UIP_IPH_LEN + uip_ext_len + ext_len,
      uip_buf + UIP_IPH_LEN + uip_ext_len, uip_len - UIP_IPH_LEN);
  memset(uip_buf + UIP_IPH_LEN + uip_ext_len, 0, ext_len);

  /* Insert source routing header (as first ext header) */
  rh_hdr->next = UIP_IP_BUF->proto;
  UIP_IP_BUF->proto = UIP_PROTO_ROUTING;

  /* Initialize IPv6 Routing Header */
  rh_hdr->len = (ext_len - 8) / 8;
  rh_hdr->routing_type = RPL_RH_TYPE_SRH;
  rh_hdr->seg_left = path_len;

  /* Initialize RPL Source Routing Header */
  srh_hdr->cmpr = (cmpri << 4) + cmpre;
  srh_hdr->pad = padding << 4;

  /* Initialize addresses field (the actual source route).
   * From last to first. */
  node = dest_node;
  hop_ptr = ((uint8_t *)rh_hdr) + ext_len - padding; /* Pointer where to write the next hop compressed address */

  while(node != NULL && node->parent != root_node) {
    NETSTACK_ROUTING.get_sr_node_ipaddr(&node_addr, node);

    hop_ptr -= (16 - cmpri);
    memcpy(hop_ptr, ((uint8_t*)&node_addr) + cmpri, 16 - cmpri);

    node = node->parent;
  }

  /* The next hop (i.e. node whose parent is the root) is placed as the current IPv6 destination */
  NETSTACK_ROUTING.get_sr_node_ipaddr(&node_addr, node);
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &node_addr);

  /* Update the IPv6 length field */
  uipbuf_add_ext_hdr(ext_len);
  uipbuf_set_len_field(UIP_IP_BUF, uip_len - UIP_IPH_LEN);

  return 1;
}
/*---------------------------------------------------------------------------*/
int
rpl_ext_header_hbh_update(uint8_t *ext_buf, int opt_offset)
{
  int down;
  int rank_error_signaled;
  int loop_detected;
  uint16_t sender_rank;
  uint8_t sender_closer;
  rpl_nbr_t *sender;
  struct uip_hbho_hdr *hbh_hdr = (struct uip_hbho_hdr *)ext_buf;
  struct uip_ext_hdr_opt_rpl *rpl_opt = (struct uip_ext_hdr_opt_rpl *)(ext_buf + opt_offset);

  if(hbh_hdr->len != ((RPL_HOP_BY_HOP_LEN - 8) / 8)
      || rpl_opt->opt_type != UIP_EXT_HDR_OPT_RPL
      || rpl_opt->opt_len != RPL_HDR_OPT_LEN) {
    LOG_ERR("hop-by-hop extension header has wrong size or type (%u %u %u)\n",
        hbh_hdr->len, rpl_opt->opt_type, rpl_opt->opt_len);
    return 0; /* Drop */
  }

  if(!curr_instance.used || curr_instance.instance_id != rpl_opt->instance) {
    LOG_ERR("unknown instance: %u\n", rpl_opt->instance);
    return 0; /* Drop */
  }

  if(rpl_opt->flags & RPL_HDR_OPT_FWD_ERR) {
    LOG_ERR("forward error!\n");
    return 0; /* Drop */
  }

  down = (rpl_opt->flags & RPL_HDR_OPT_DOWN) ? 1 : 0;
  sender_rank = UIP_HTONS(rpl_opt->senderrank);
  sender = nbr_table_get_from_lladdr(rpl_neighbors, packetbuf_addr(PACKETBUF_ADDR_SENDER));
  rank_error_signaled = (rpl_opt->flags & RPL_HDR_OPT_RANK_ERR) ? 1 : 0;
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
    rpl_opt->flags |= RPL_HDR_OPT_RANK_ERR;
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
  struct uip_hbho_hdr *hbh_hdr = (struct uip_hbho_hdr *)UIP_IP_PAYLOAD(0);
  struct uip_ext_hdr_opt_rpl *rpl_opt = (struct uip_ext_hdr_opt_rpl *)(UIP_IP_PAYLOAD(2));

  if(UIP_IP_BUF->proto == UIP_PROTO_HBHO && rpl_opt->opt_type == UIP_EXT_HDR_OPT_RPL) {
    if(hbh_hdr->len != ((RPL_HOP_BY_HOP_LEN - 8) / 8)
        || rpl_opt->opt_len != RPL_HDR_OPT_LEN) {

      LOG_ERR("hop-by-hop extension header has wrong size (%u)\n", rpl_opt->opt_len);
      return 0; /* Drop */
    }

    if(!curr_instance.used || curr_instance.instance_id != rpl_opt->instance) {
      LOG_ERR("unable to add/update hop-by-hop extension header: incorrect instance\n");
      return 0; /* Drop */
    }

    /* Update sender rank and instance, will update flags next */
    rpl_opt->senderrank = UIP_HTONS(curr_instance.dag.rank);
    rpl_opt->instance = curr_instance.instance_id;
  }

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
  struct uip_hbho_hdr *hbh_hdr = (struct uip_hbho_hdr *)UIP_IP_PAYLOAD(0);
  struct uip_ext_hdr_opt_rpl *rpl_opt = (struct uip_ext_hdr_opt_rpl *)(UIP_IP_PAYLOAD(2));

  /* Insert hop-by-hop header */
  LOG_INFO("creating hop-by-hop option\n");
  if(uip_len + RPL_HOP_BY_HOP_LEN > UIP_LINK_MTU) {
    LOG_ERR("packet too long: impossible to add hop-by-hop option\n");
    return 0;
  }

  /* Move existing ext headers and payload RPL_HOP_BY_HOP_LEN further */
  memmove(UIP_IP_PAYLOAD(RPL_HOP_BY_HOP_LEN), UIP_IP_PAYLOAD(0), uip_len - UIP_IPH_LEN);
  memset(UIP_IP_PAYLOAD(0), 0, RPL_HOP_BY_HOP_LEN);

  /* Insert HBH header (as first ext header) */
  hbh_hdr->next = UIP_IP_BUF->proto;
  UIP_IP_BUF->proto = UIP_PROTO_HBHO;

  /* Initialize HBH option */
  hbh_hdr->len = (RPL_HOP_BY_HOP_LEN - 8) / 8;
  rpl_opt->opt_type = UIP_EXT_HDR_OPT_RPL;
  rpl_opt->opt_len = RPL_HDR_OPT_LEN;
  rpl_opt->flags = 0;
  rpl_opt->senderrank = UIP_HTONS(curr_instance.dag.rank);
  rpl_opt->instance = curr_instance.instance_id;

  uipbuf_add_ext_hdr(RPL_HOP_BY_HOP_LEN);
  uipbuf_set_len_field(UIP_IP_BUF, uip_len - UIP_IPH_LEN);

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
bool
rpl_ext_header_remove(void)
{
  uint8_t *prev_proto_ptr;
  uint8_t protocol;
  uint16_t ext_len;
  uint8_t *next_header;
  struct uip_ext_hdr *ext_ptr;
  struct uip_ext_hdr_opt *opt_ptr;

  next_header = uipbuf_get_next_header(uip_buf, uip_len, &protocol, true);
  if(next_header == NULL) {
    return true;
  }
  ext_ptr = (struct uip_ext_hdr *)next_header;
  prev_proto_ptr = &UIP_IP_BUF->proto;

  while(uip_is_proto_ext_hdr(protocol)) {
    opt_ptr = (struct uip_ext_hdr_opt *)(next_header + 2);
    if(protocol == UIP_PROTO_ROUTING ||
       (protocol == UIP_PROTO_HBHO && opt_ptr->type == UIP_EXT_HDR_OPT_RPL)) {
      /* Remove ext header */
      *prev_proto_ptr = ext_ptr->next;
      ext_len = ext_ptr->len * 8 + 8;
      if(uipbuf_add_ext_hdr(-ext_len) == false) {
        return false;
      }

      /* Update length field and move rest of packet to the "left" */
      uipbuf_set_len_field(UIP_IP_BUF, uip_len - UIP_IPH_LEN);
      if(uip_len <= next_header - uip_buf) {
        /* No more data to move. */
        return false;
      }
      memmove(next_header, next_header + ext_len,
              uip_len - (next_header - uip_buf));

      /* Update loop variables */
      protocol = *prev_proto_ptr;
    } else {
      /* move to the ext hdr */
      next_header = uipbuf_get_next_header(next_header,
                                           uip_len - (next_header - uip_buf),
                                           &protocol, false);
      if(next_header == NULL) {
        /* Processing finished. */
        break;
      }
      ext_ptr = (struct uip_ext_hdr *)next_header;
      prev_proto_ptr = &ext_ptr->next;
    }
  }

  return true;
}
/** @}*/
