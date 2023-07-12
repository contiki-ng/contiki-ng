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
 *         Orchestra: an autonomous scheduler for TSCH exploiting RPL state.
 *         See "Orchestra: Robust Mesh Networks Through Autonomously Scheduled TSCH", ACM SenSys'15
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"
#include "orchestra.h"
#include "net/packetbuf.h"
#include "net/ipv6/uip-icmp6.h"
#include "net/routing/routing.h"
#if ROUTING_CONF_RPL_LITE
#include "net/routing/rpl-lite/rpl.h"
#elif ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"
#endif

#include "sys/log.h"
#define LOG_MODULE "Orchestra"
#define LOG_LEVEL  LOG_LEVEL_MAC

/* A net-layer sniffer for packets sent and received */
static void orchestra_packet_received(void);
static void orchestra_packet_sent(int mac_status);
NETSTACK_SNIFFER(orchestra_sniffer, orchestra_packet_received, orchestra_packet_sent);

/* The current RPL preferred parent's link-layer address */
linkaddr_t orchestra_parent_linkaddr;
/* Set to one only after getting an ACK for a DAO sent to our preferred parent */
int orchestra_parent_knows_us = 0;

/* The set of Orchestra rules in use */
const struct orchestra_rule *all_rules[] = ORCHESTRA_RULES;
#define NUM_RULES (sizeof(all_rules) / sizeof(struct orchestra_rule *))

/*---------------------------------------------------------------------------*/
static void
orchestra_packet_received(void)
{
}
/*---------------------------------------------------------------------------*/
static void
orchestra_packet_sent(int mac_status)
{
  /* Check if our parent just ACKed a DAO */
  if(orchestra_parent_knows_us == 0
     && mac_status == MAC_TX_OK
     && packetbuf_attr(PACKETBUF_ATTR_NETWORK_ID) == UIP_PROTO_ICMP6
     && packetbuf_attr(PACKETBUF_ATTR_CHANNEL) == (ICMP6_RPL << 8 | RPL_CODE_DAO)) {
    if(!linkaddr_cmp(&orchestra_parent_linkaddr, &linkaddr_null)
       && linkaddr_cmp(&orchestra_parent_linkaddr, packetbuf_addr(PACKETBUF_ADDR_RECEIVER))) {
      orchestra_parent_knows_us = 1;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_child_added(const linkaddr_t *addr)
{
  /* Notify all Orchestra rules that a child was added */
  int i;
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->child_added != NULL) {
      all_rules[i]->child_added(addr);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_child_removed(const linkaddr_t *addr)
{
  /* Notify all Orchestra rules that a child was removed */
  int i;
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->child_removed != NULL) {
      all_rules[i]->child_removed(addr);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_neighbor_updated(const linkaddr_t *addr, uint8_t is_added)
{
  /* Notify all Orchestra rules that a neighbor was added or removed */
  int i;
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->neighbor_updated != NULL) {
      all_rules[i]->neighbor_updated(addr, is_added);
    }
  }
}
/*---------------------------------------------------------------------------*/
int
orchestra_callback_packet_ready(void)
{
  int i;
  /* By default, use any slotframe, any timeslot */
  uint16_t slotframe = 0xffff;
  uint16_t timeslot = 0xffff;
  /* The default channel offset 0xffff means that the channel offset in the scheduled
   * tsch_link structure is used instead. Any other value specified in the packetbuf
   * overrides per-link value, allowing to implement multi-channel Orchestra. */
  uint16_t channel_offset = 0xffff;
  int matched_rule = -1;

  /* Loop over all rules until finding one able to handle the packet */
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->select_packet != NULL) {
      if(all_rules[i]->select_packet(&slotframe, &timeslot, &channel_offset)) {
        matched_rule = i;
        break;
      }
    }
  }

#if TSCH_WITH_LINK_SELECTOR
  packetbuf_set_attr(PACKETBUF_ATTR_TSCH_SLOTFRAME, slotframe);
  packetbuf_set_attr(PACKETBUF_ATTR_TSCH_TIMESLOT, timeslot);
  packetbuf_set_attr(PACKETBUF_ATTR_TSCH_CHANNEL_OFFSET, channel_offset);
#endif

  return matched_rule;
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_new_time_source(const struct tsch_neighbor *old, const struct tsch_neighbor *new)
{
  /* Orchestra assumes that the time source is also the RPL parent.
   * This is the case if the following is set:
   * #define RPL_CALLBACK_PARENT_SWITCH tsch_rpl_callback_parent_switch
   * */

  int i;
  if(new != old) {
    orchestra_parent_knows_us = 0;
  }
  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->new_time_source != NULL) {
      all_rules[i]->new_time_source(old, new);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_callback_root_node_updated(const linkaddr_t *root, uint8_t is_added)
{
  int i;

  for(i = 0; i < NUM_RULES; i++) {
    if(all_rules[i]->root_node_updated != NULL) {
      all_rules[i]->root_node_updated(root, is_added);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
orchestra_init(void)
{
  int i;
  /* Snoop on packet transmission to know if our parent knows about us
   * (i.e. has ACKed at one of our DAOs since we decided to use it as a parent) */
  netstack_sniffer_add(&orchestra_sniffer);
  linkaddr_copy(&orchestra_parent_linkaddr, &linkaddr_null);
  /* Initialize all Orchestra rules */
  for(i = 0; i < NUM_RULES; i++) {
    LOG_INFO("Initializing rule %s (%u), size %d\n", all_rules[i]->name, i, all_rules[i]->slotframe_size);
    if(all_rules[i]->init != NULL) {
      all_rules[i]->init(i);
    }
  }
  LOG_INFO("Initialization done\n");
}
