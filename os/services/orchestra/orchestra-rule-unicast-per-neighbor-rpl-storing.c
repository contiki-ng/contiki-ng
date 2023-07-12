/*
 * Copyright (c) 2015, Swedish Institute of Computer Science.
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
 * \file
 *         Orchestra: a slotframe dedicated to unicast data transmission. Designed for
 *         RPL storing mode only, as this is based on the knowledge of the children (and parent).
 *         If receiver-based:
 *           Nodes listen at a timeslot defined as hash(MAC) % ORCHESTRA_SB_UNICAST_PERIOD
 *           Nodes transmit at: for each nbr in RPL children and RPL preferred parent,
 *                                             hash(nbr.MAC) % ORCHESTRA_SB_UNICAST_PERIOD
 *         If sender-based: the opposite
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/ipv6/uip-ds6-route.h"
#include "net/packetbuf.h"
#include "net/routing/routing.h"

/*
 * The body of this rule should be compiled only when "nbr_routes" is available,
 * otherwise a link error causes build failure. "nbr_routes" is compiled if
 * UIP_MAX_ROUTES != 0. See uip-ds6-route.c.
 */
#if UIP_MAX_ROUTES != 0

#if ORCHESTRA_UNICAST_SENDER_BASED && ORCHESTRA_COLLISION_FREE_HASH
#define UNICAST_SLOT_SHARED_FLAG    ((ORCHESTRA_UNICAST_PERIOD < (ORCHESTRA_MAX_HASH + 1)) ? LINK_OPTION_SHARED : 0)
#else
#define UNICAST_SLOT_SHARED_FLAG      LINK_OPTION_SHARED
#endif

static uint16_t slotframe_handle = 0;
static uint16_t local_channel_offset;
static struct tsch_slotframe *sf_unicast;

/*---------------------------------------------------------------------------*/
static uint16_t
get_node_timeslot(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_UNICAST_PERIOD > 0) {
    return ORCHESTRA_LINKADDR_HASH(addr) % ORCHESTRA_UNICAST_PERIOD;
  } else {
    return 0xffff;
  }
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_node_channel_offset(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET >= ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET) {
    return ORCHESTRA_LINKADDR_HASH(addr) % (ORCHESTRA_UNICAST_MAX_CHANNEL_OFFSET - ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET + 1)
        + ORCHESTRA_UNICAST_MIN_CHANNEL_OFFSET;
  } else {
    return 0xffff;
  }
}
/*---------------------------------------------------------------------------*/
static int
neighbor_has_uc_link(const linkaddr_t *linkaddr)
{
  if(linkaddr == NULL || linkaddr_cmp(linkaddr, &linkaddr_null)) {
    return 0;
  }

  if(!ORCHESTRA_UNICAST_SENDER_BASED) {
    /* With the receiver-based Orchestra,
     * all nodes have a link installed at their own timeslot */
    return 1;
  }

  if(linkaddr_cmp(&orchestra_parent_linkaddr, linkaddr)) {
    /* The node is our parent */
    return orchestra_parent_knows_us ? 1 : 0;
  }

  if(nbr_table_get_from_lladdr(nbr_routes, (linkaddr_t *)linkaddr) != NULL) {
    /* We have a route to this node;
     * it should have selected us as its parent and installed a link */
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static void
add_uc_link(const linkaddr_t *linkaddr)
{
  if(linkaddr != NULL) {
    uint16_t timeslot = get_node_timeslot(linkaddr);
    uint8_t link_options = ORCHESTRA_UNICAST_SENDER_BASED ? LINK_OPTION_RX : LINK_OPTION_TX | UNICAST_SLOT_SHARED_FLAG;

    if(timeslot == get_node_timeslot(&linkaddr_node_addr)) {
      /* This is also our timeslot, add necessary flags */
      link_options |= ORCHESTRA_UNICAST_SENDER_BASED ? LINK_OPTION_TX | UNICAST_SLOT_SHARED_FLAG: LINK_OPTION_RX;
    }

    /* Add/update link.
     * Always configure the link with the local node's channel offset.
     * If this is an Rx link, that is what the node needs to use.
     * If this is a Tx link, packet's channel offset will override the link's channel offset.
     */
    tsch_schedule_add_link(sf_unicast, link_options, LINK_TYPE_NORMAL, &tsch_broadcast_address,
          timeslot, local_channel_offset, 1);
  }
}
/*---------------------------------------------------------------------------*/
static void
remove_uc_link(const linkaddr_t *linkaddr)
{
  uint16_t timeslot;
  struct tsch_link *l;

  if(linkaddr == NULL) {
    return;
  }

  timeslot = get_node_timeslot(linkaddr);
  l = tsch_schedule_get_link_by_offsets(sf_unicast, timeslot, local_channel_offset);
  if(l == NULL) {
    return;
  }
  if(!ORCHESTRA_UNICAST_SENDER_BASED) {
    /* Packets to this address were marked with this slotframe and neighbor-specific timeslot;
     * make sure they don't remain stuck in the queues after the link is removed. */
    tsch_queue_free_packets_to(linkaddr);
  }

  /* Does our current parent need this timeslot? */
  if(timeslot == get_node_timeslot(&orchestra_parent_linkaddr)) {
    /* Yes, this timeslot is being used, return */
    return;
  }
  /* Does any other child need this timeslot?
   * (lookup all route next hops) */
  nbr_table_item_t *item = nbr_table_head(nbr_routes);
  while(item != NULL) {
    linkaddr_t *addr = nbr_table_get_lladdr(nbr_routes, item);
    if(timeslot == get_node_timeslot(addr)) {
      /* Yes, this timeslot is being used, return */
      return;
    }
    item = nbr_table_next(nbr_routes, item);
  }

  /* Do we need this timeslot? */
  if(timeslot == get_node_timeslot(&linkaddr_node_addr)) {
    /* This is our link, keep it but update the link options */
    uint8_t link_options = ORCHESTRA_UNICAST_SENDER_BASED ? LINK_OPTION_TX | UNICAST_SLOT_SHARED_FLAG: LINK_OPTION_RX;
    tsch_schedule_add_link(sf_unicast, link_options, LINK_TYPE_NORMAL, &tsch_broadcast_address,
              timeslot, local_channel_offset, 1);
  } else {
    /* Remove link */
    tsch_schedule_remove_link(sf_unicast, l);
  }
}
/*---------------------------------------------------------------------------*/
static void
child_added(const linkaddr_t *linkaddr)
{
  add_uc_link(linkaddr);
}
/*---------------------------------------------------------------------------*/
static void
child_removed(const linkaddr_t *linkaddr)
{
  remove_uc_link(linkaddr);
}
/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset)
{
  /* Select data packets we have a unicast link to */
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
     && !orchestra_is_root_schedule_active(dest)
     && neighbor_has_uc_link(dest)) {
    if(slotframe != NULL) {
      *slotframe = slotframe_handle;
    }
    if(timeslot != NULL) {
      *timeslot = ORCHESTRA_UNICAST_SENDER_BASED ? get_node_timeslot(&linkaddr_node_addr) : get_node_timeslot(dest);
    }
    /* set per-packet channel offset */
    if(channel_offset != NULL) {
      *channel_offset = get_node_channel_offset(dest);
    }
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
  if(new != old) {
    const linkaddr_t *old_addr = tsch_queue_get_nbr_address(old);
    const linkaddr_t *new_addr = tsch_queue_get_nbr_address(new);
    if(new_addr != NULL) {
      linkaddr_copy(&orchestra_parent_linkaddr, new_addr);
    } else {
      linkaddr_copy(&orchestra_parent_linkaddr, &linkaddr_null);
    }
    remove_uc_link(old_addr);
    add_uc_link(new_addr);
  }
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  uint16_t timeslot;
  linkaddr_t *local_addr = &linkaddr_node_addr;

  slotframe_handle = sf_handle;
  local_channel_offset = get_node_channel_offset(local_addr);
  /* Slotframe for unicast transmissions */
  sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_UNICAST_PERIOD);
  timeslot = get_node_timeslot(local_addr);
  tsch_schedule_add_link(sf_unicast,
            ORCHESTRA_UNICAST_SENDER_BASED ? LINK_OPTION_TX | UNICAST_SLOT_SHARED_FLAG: LINK_OPTION_RX,
            LINK_TYPE_NORMAL, &tsch_broadcast_address,
            timeslot, local_channel_offset, 1);
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule unicast_per_neighbor_rpl_storing = {
  init,
  new_time_source,
  select_packet,
  child_added,
  child_removed,
  NULL,
  NULL,
  "unicast per neighbor storing",
  ORCHESTRA_UNICAST_PERIOD,
};

#endif /* UIP_MAX_ROUTES */
