/*
 * Copyright (c) 2020, Institute of Electronics and Computer Science (EDI)
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
 *         This is the link-based mode:
 *         For each nbr in RPL children and RPL preferred parent,
 *             nodes listen at: hash(nbr.MAC, local.MAC) % ORCHESTRA_SB_UNICAST_PERIOD
 *             nodes transmit at: hash(local.MAC, nbr.MAC) % ORCHESTRA_SB_UNICAST_PERIOD
 *         For receiver-based and sender-based modes, see orchestra-rule-unicast-per-neighbor-rpl-storing.c
 *         The Orchestra link-based rule has been designed based on the insights from:
 *         1) An Empirical Survey of Autonomous Scheduling Methods for TSCH,
 *            Atis Elsts, Seohyang Kim, Hyung-Sin Kim, Chongkwon Kim, IEEE Access, 2020
 *         2) ALICE: autonomous link-based cell scheduling for TSCH,
 *            Seohyang Kim, Hyung-Sin Kim, Chongkwon Kim, IEEE Access, ACM/IEEE IPSN, 2019.
 *
 * \author Atis Elsts <atis.elsts@edi.lv>
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"

/*
 * The body of this rule should be compiled only when "nbr_routes" is available,
 * otherwise a link error causes build failure. "nbr_routes" is compiled if
 * UIP_MAX_ROUTES != 0. See uip-ds6-route.c.
 */
#if UIP_MAX_ROUTES != 0

static uint16_t slotframe_handle = 0;
static uint16_t local_channel_offset;
static struct tsch_slotframe *sf_unicast;

/*---------------------------------------------------------------------------*/
static uint16_t
get_node_pair_timeslot(const linkaddr_t *from, const linkaddr_t *to)
{
  if(from != NULL && to != NULL && ORCHESTRA_UNICAST_PERIOD > 0) {
    return ORCHESTRA_LINKADDR_HASH2(from, to) % ORCHESTRA_UNICAST_PERIOD;
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
  if(linkaddr != NULL && !linkaddr_cmp(linkaddr, &linkaddr_null)) {
    if(orchestra_parent_knows_us && linkaddr_cmp(&orchestra_parent_linkaddr, linkaddr)) {
      return 1;
    }
    if(nbr_table_get_from_lladdr(nbr_routes, (linkaddr_t *)linkaddr) != NULL) {
      return 1;
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
add_uc_links(const linkaddr_t *linkaddr)
{
  if(linkaddr != NULL) {
    uint16_t timeslot_rx = get_node_pair_timeslot(linkaddr, &linkaddr_node_addr);
    uint16_t timeslot_tx = get_node_pair_timeslot(&linkaddr_node_addr, linkaddr);

    /* Add Tx link */
    tsch_schedule_add_link(sf_unicast, LINK_OPTION_TX | LINK_OPTION_SHARED, LINK_TYPE_NORMAL, &tsch_broadcast_address,
                           timeslot_tx, local_channel_offset, 0);
    /* Add Rx link */
    tsch_schedule_add_link(sf_unicast, LINK_OPTION_RX, LINK_TYPE_NORMAL, &tsch_broadcast_address,
                           timeslot_rx, local_channel_offset, 0);

  }
}
/*---------------------------------------------------------------------------*/
static void
remove_unicast_link(uint16_t timeslot, uint16_t options)
{
  struct tsch_link *l = list_head(sf_unicast->links_list);
  while(l != NULL) {
    if(l->timeslot == timeslot
        && l->channel_offset == local_channel_offset
        && l->link_options == options) {
      tsch_schedule_remove_link(sf_unicast, l);
      break;
    }
    l = list_item_next(l);
  }
}
/*---------------------------------------------------------------------------*/
static void
remove_uc_links(const linkaddr_t *linkaddr)
{
  if(linkaddr != NULL) {
    uint16_t timeslot_rx = get_node_pair_timeslot(linkaddr, &linkaddr_node_addr);
    uint16_t timeslot_tx = get_node_pair_timeslot(&linkaddr_node_addr, linkaddr);

    remove_unicast_link(timeslot_rx, LINK_OPTION_RX);
    remove_unicast_link(timeslot_tx, LINK_OPTION_TX | LINK_OPTION_SHARED);
  }
}
/*---------------------------------------------------------------------------*/
static void
child_added(const linkaddr_t *linkaddr)
{
  add_uc_links(linkaddr);
}
/*---------------------------------------------------------------------------*/
static void
child_removed(const linkaddr_t *linkaddr)
{
  remove_uc_links(linkaddr);
}
/*---------------------------------------------------------------------------*/
static int
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset)
{
  /* Select data packets we have a unicast link to */
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
     && neighbor_has_uc_link(dest)) {
    if(slotframe != NULL) {
      *slotframe = slotframe_handle;
    }
    if(timeslot != NULL) {
      *timeslot = get_node_pair_timeslot(&linkaddr_node_addr, dest);
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
    remove_uc_links(old_addr);
    add_uc_links(new_addr);
  }
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  slotframe_handle = sf_handle;
  local_channel_offset = get_node_channel_offset(&linkaddr_node_addr);
  /* Slotframe for unicast transmissions */
  sf_unicast = tsch_schedule_add_slotframe(slotframe_handle, ORCHESTRA_UNICAST_PERIOD);
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule unicast_per_neighbor_link_based = {
  init,
  new_time_source,
  select_packet,
  child_added,
  child_removed,
  "unicast per neighbor link based",
};

#endif /* UIP_MAX_ROUTES */
