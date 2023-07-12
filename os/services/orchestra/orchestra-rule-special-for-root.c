/*
 * Copyright (c) 2018, Amber Agriculture
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
 *         Orchestra: a slotframe dedicated to unicast data transmission to the root.
 *         See the paper "TSCH for Long Range Low Data Rate Applications", IEEE Access
 *
 *         Warning: the root rule should not be used together with the NS rule!
 *         The issue is in the prioritization of the different rules - NS rule is
 *         prioritized over the root rule, because it has a lower slotframe handle.
 *         In the NS rule, all slots potentially are active, and all-but-one slots are for Tx.
 *         This leads to a NS-rule slot is always being selected over a root-rule slot.
 *
 * \author Atis Elsts <atis.elsts@gmail.com>
 */

#include "contiki.h"
#include "orchestra.h"

#include "sys/log.h"
#define LOG_MODULE "Orchestra"
#define LOG_LEVEL  LOG_LEVEL_MAC

static struct tsch_slotframe *sf_tx;
static struct tsch_slotframe *sf_rx;
static uint8_t is_root_rule_used;
static uint16_t timeslot_tx;

static void set_self_to_root(uint8_t is_root);
/*---------------------------------------------------------------------------*/
static inline uint8_t
self_is_root(void)
{
  /* If this is changed to look at the RPL status instead of tsch_is_coordinator,
   * multiple roots become possible in a single TSCH network. */
  return tsch_is_coordinator;
}
/*---------------------------------------------------------------------------*/
uint8_t
orchestra_is_root_schedule_active(const linkaddr_t *addr)
{
  return is_root_rule_used && tsch_roots_is_root(addr);
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_node_timeslot(const linkaddr_t *addr)
{
  if(addr != NULL && ORCHESTRA_ROOT_PERIOD > 0) {
    return ORCHESTRA_LINKADDR_HASH(addr) % ORCHESTRA_ROOT_PERIOD;
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
select_packet(uint16_t *slotframe, uint16_t *timeslot, uint16_t *channel_offset)
{
  /* Select data packets to a root node, in case we are not the root ourselves */
  const linkaddr_t *dest = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  if(!self_is_root()
      && sf_tx != NULL
      && packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME
      && dest != NULL
      && orchestra_is_root_schedule_active(dest)) {
    if(slotframe != NULL) {
      *slotframe = sf_tx->handle;
    }
    if(timeslot != NULL) {
      *timeslot = timeslot_tx;
    }
    /* set per-packet channel offset */
    if(channel_offset != NULL) {
      *channel_offset = get_node_channel_offset(dest);
    }
    /* XXX: this should be removed later when the root rule is better tested */
    LOG_INFO("use the root rule for root node ");
    LOG_INFO_LLADDR(dest);
    LOG_INFO_("\n");
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
init(uint16_t sf_handle)
{
  /* signal that the root rule is used */
  is_root_rule_used = 1;

  /* Add a slotframe for unicast transmission to (other) root nodes, initially empty */
  timeslot_tx = get_node_timeslot(&linkaddr_node_addr);
  sf_tx = tsch_schedule_add_slotframe(sf_handle, ORCHESTRA_ROOT_PERIOD);
  if(sf_tx == NULL) {
      LOG_ERR("failed to add a slotframe for transmissions\n");
  }

  if(tsch_is_coordinator) {
    /* perform the deferred addition of the root slotframe */
    set_self_to_root(1);
  }
}
/*---------------------------------------------------------------------------*/
static void
set_self_to_root(uint8_t is_root)
{
  const uint16_t slotframe_rx_handle = (sf_tx == NULL ? (uint16_t)-1 : (sf_tx->handle | 0x8000));

  if(is_root_rule_used == 0) {
    return; /* defer addition until this rule is initialized */
  }

  if(is_root) {
    /* potentially becomes root */
    if(sf_rx == NULL) {
      /* Add a 1-slot long slotframe for unicast reception */
      sf_rx = tsch_schedule_add_slotframe(slotframe_rx_handle, 1);
      if(sf_rx == NULL) {
        LOG_ERR("failed to add a slotframe for reception\n");
        return;
      }
      /* Add a Rx link to this slotframe */
      tsch_schedule_add_link(sf_rx,
          LINK_OPTION_SHARED | LINK_OPTION_RX,
          LINK_TYPE_NORMAL, &tsch_broadcast_address,
          0, get_node_channel_offset(&linkaddr_node_addr), 1);
    }
  } else {
    /* not a root anymore */
    if(sf_rx != NULL) {
      tsch_schedule_remove_slotframe(sf_rx);
      sf_rx = NULL;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
root_node_updated(const linkaddr_t *root, uint8_t is_added)
{
  if(linkaddr_cmp(root, &linkaddr_node_addr)) {
    /* special handling in case the local node becomes a root */
    set_self_to_root(is_added);
    return;
  }

  if(is_added) {
    /* Note in case multiple roots are used:
     * if there are multiple roots in a direct reach, a TSCH cell is added to each of them,
     * at the same timeslot. In each slotframe, the root node will be selected randomly.
     * This means that the cell's efficiency is reduced to 1/N of its full capacity,
     * where N is the number of directly reachable roots.
     */
    tsch_schedule_add_link(sf_tx,
        LINK_OPTION_SHARED | LINK_OPTION_TX,
        LINK_TYPE_NORMAL, root,
        timeslot_tx, get_node_channel_offset(root), 0);
  } else {
    tsch_schedule_remove_link_by_offsets(sf_tx, timeslot_tx, get_node_channel_offset(root));
    tsch_queue_free_packets_to(root);
  }
}
/*---------------------------------------------------------------------------*/
struct orchestra_rule special_for_root = {
  init,
  NULL,
  select_packet,
  NULL,
  NULL,
  NULL,
  root_node_updated,
  "special for root",
  ORCHESTRA_ROOT_PERIOD,
};
