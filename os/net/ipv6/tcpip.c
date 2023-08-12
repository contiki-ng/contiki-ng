/*
 * Copyright (c) 2004, Swedish Institute of Computer Science.
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
 * \file
 *         Core of the TCP/IP stack, handles input/output/routing
 *
 * \author  Adam Dunkels <adam@sics.se>\author
 * \author  Mathilde Durvy <mdurvy@cisco.com> (IPv6 related code)
 * \author  Julien Abeille <jabeille@cisco.com> (IPv6 related code)
 */

#include "contiki.h"
#include "contiki-net.h"
#include "net/ipv6/uip-packetqueue.h"

#include "net/ipv6/uip-nd6.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-ds6-nbr.h"
#include "net/linkaddr.h"
#include "net/routing/routing.h"

#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TCP/IP"
#define LOG_LEVEL LOG_LEVEL_TCPIP

#ifdef UIP_FALLBACK_INTERFACE
extern struct uip_fallback_interface UIP_FALLBACK_INTERFACE;
#endif

process_event_t tcpip_event;
#if UIP_CONF_ICMP6
process_event_t tcpip_icmp6_event;
#endif /* UIP_CONF_ICMP6 */

/* Periodic check of active connections. */
static struct etimer periodic;

#if UIP_CONF_IPV6_REASSEMBLY
/* Timer for reassembly. */
extern struct etimer uip_reass_timer;
#endif

#if UIP_TCP
/**
 * \internal Structure for holding a TCP port and a process ID.
 */
struct listenport {
  uint16_t port;
  struct process *p;
};

static struct internal_state {
  struct listenport listenports[UIP_LISTENPORTS];
  struct process *p;
} s;
#endif

enum {
  TCP_POLL,
  UDP_POLL,
  PACKET_INPUT
};

/*---------------------------------------------------------------------------*/
#if UIP_TCP || UIP_UDP
static void
init_appstate(uip_tcp_appstate_t *as, void *state)
{
  as->p = PROCESS_CURRENT();
  as->state = state;
}
#endif /* UIP_TCP || UIP_UDP */
/*---------------------------------------------------------------------------*/

uint8_t
tcpip_output(const uip_lladdr_t *a)
{
  int ret;

  /* Tag Traffic Class if we are using TC for variable retrans */
#if UIP_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS
  if(uipbuf_get_attr(UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS) !=
     UIP_MAX_MAC_TRANSMISSIONS_UNDEFINED) {
    LOG_INFO("Tagging TC with retrans: %d\n", uipbuf_get_attr(UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS));
    /* Encapsulate the MAC transmission limit in the Traffic Class field */
    UIP_IP_BUF->vtc = 0x60 | (UIP_TC_MAC_TRANSMISSION_COUNTER_BIT >> 4);
    UIP_IP_BUF->tcflow =
      uipbuf_get_attr(UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS) << 4;
  }
#endif

  if(netstack_process_ip_callback(NETSTACK_IP_OUTPUT, (const linkaddr_t *)a) ==
     NETSTACK_IP_PROCESS) {
    ret = NETSTACK_NETWORK.output((const linkaddr_t *) a);
    return ret;
  } else {
    /* Ok, ignore and drop... */
    uipbuf_clear();
    return 0;
  }
}

PROCESS(tcpip_process, "TCP/IP stack");

/*---------------------------------------------------------------------------*/
#if UIP_TCP
static void
start_periodic_tcp_timer(void)
{
  if(etimer_expired(&periodic)) {
    etimer_restart(&periodic);
  }
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
static void
check_for_tcp_syn(void)
{
#if UIP_TCP
  /* This is a hack that is needed to start the periodic TCP timer if
     an incoming packet contains a SYN: since uIP does not inform the
     application if a SYN arrives, we have no other way of starting
     this timer.  This function is called for every incoming IP packet
     to check for such SYNs. */
#define TCP_SYN 0x02
  if(uip_len >= UIP_IPTCPH_LEN + uip_ext_len &&
     UIP_IP_BUF->proto == UIP_PROTO_TCP &&
     (UIP_TCP_BUF->flags & TCP_SYN) == TCP_SYN) {
    start_periodic_tcp_timer();
  }
#endif /* UIP_TCP */
}
/*---------------------------------------------------------------------------*/
static void
packet_input(void)
{
  if(uip_len > 0) {
    LOG_INFO("input: received %u bytes\n", uip_len);

    check_for_tcp_syn();

#if UIP_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS
    if(uip_len >= UIP_IPH_LEN) {
      uint8_t traffic_class = (UIP_IP_BUF->vtc << 4) | (UIP_IP_BUF->tcflow >> 4);
      if(traffic_class & UIP_TC_MAC_TRANSMISSION_COUNTER_BIT) {
        uint8_t max_mac_transmissions = traffic_class & UIP_TC_MAC_TRANSMISSION_COUNTER_MASK;
        uipbuf_set_attr(UIPBUF_ATTR_MAX_MAC_TRANSMISSIONS, max_mac_transmissions);
        LOG_INFO("Received packet tagged with TC retrans: %d (%x)",
                 max_mac_transmissions, traffic_class);
      }
    }
#endif /* UIP_TAG_TC_WITH_VARIABLE_RETRANSMISSIONS */

    uip_input();
    if(uip_len > 0) {
      tcpip_ipv6_output();
    }
  }
}
/*---------------------------------------------------------------------------*/
#if UIP_TCP
#if UIP_ACTIVE_OPEN
struct uip_conn *
tcp_connect(const uip_ipaddr_t *ripaddr, uint16_t port, void *appstate)
{
  struct uip_conn *c;

  c = uip_connect(ripaddr, port);
  if(c == NULL) {
    return NULL;
  }

  init_appstate(&c->appstate, appstate);

  tcpip_poll_tcp(c);

  return c;
}
#endif /* UIP_ACTIVE_OPEN */
/*---------------------------------------------------------------------------*/
void
tcp_unlisten(uint16_t port)
{
  unsigned char i;
  struct listenport *l;

  l = s.listenports;
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    if(l->port == port &&
       l->p == PROCESS_CURRENT()) {
      l->port = 0;
      uip_unlisten(port);
      break;
    }
    ++l;
  }
}
/*---------------------------------------------------------------------------*/
void
tcp_listen(uint16_t port)
{
  unsigned char i;
  struct listenport *l;

  l = s.listenports;
  for(i = 0; i < UIP_LISTENPORTS; ++i) {
    if(l->port == 0) {
      l->port = port;
      l->p = PROCESS_CURRENT();
      uip_listen(port);
      break;
    }
    ++l;
  }
}
/*---------------------------------------------------------------------------*/
void
tcp_attach(struct uip_conn *conn, void *appstate)
{
  init_appstate(&conn->appstate, appstate);
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
#if UIP_UDP
void
udp_attach(struct uip_udp_conn *conn, void *appstate)
{
  init_appstate(&conn->appstate, appstate);
}
/*---------------------------------------------------------------------------*/
struct uip_udp_conn *
udp_new(const uip_ipaddr_t *ripaddr, uint16_t port, void *appstate)
{
  struct uip_udp_conn *c = uip_udp_new(ripaddr, port);

  if(c == NULL) {
    return NULL;
  }

  init_appstate(&c->appstate, appstate);

  return c;
}
/*---------------------------------------------------------------------------*/
struct uip_udp_conn *
udp_broadcast_new(uint16_t port, void *appstate)
{
  uip_ipaddr_t addr;
  struct uip_udp_conn *conn;

  uip_create_linklocal_allnodes_mcast(&addr);

  conn = udp_new(&addr, port, appstate);
  if(conn != NULL) {
    udp_bind(conn, port);
  }
  return conn;
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
#if UIP_CONF_ICMP6
uint8_t
icmp6_new(void *appstate) {
  if(uip_icmp6_conns.appstate.p == PROCESS_NONE) {
    init_appstate(&uip_icmp6_conns.appstate, appstate);
    return 0;
  }
  return 1;
}

void
tcpip_icmp6_call(uint8_t type)
{
  if(uip_icmp6_conns.appstate.p != PROCESS_NONE) {
    /* XXX: This is a hack that needs to be updated. Passing a pointer (&type)
       like this only works with process_post_synch. */
    process_post_synch(uip_icmp6_conns.appstate.p, tcpip_icmp6_event, &type);
  }
  return;
}
#endif /* UIP_CONF_ICMP6 */
/*---------------------------------------------------------------------------*/
static void
eventhandler(process_event_t ev, process_data_t data)
{
  switch(ev) {
#if UIP_TCP || UIP_UDP
  case PROCESS_EVENT_EXITED:
    /* This is the event we get if a process has exited. We go through
         the TCP/IP tables to see if this process had any open
         connections or listening TCP ports. If so, we'll close those
         connections. */
    {
      struct process *p = (struct process *)data;
#if UIP_TCP
      struct listenport *l = s.listenports;
      for(uint8_t i = 0; i < UIP_LISTENPORTS; ++i) {
        if(l->p == p) {
          uip_unlisten(l->port);
          l->port = 0;
          l->p = PROCESS_NONE;
        }
        ++l;
      }

      for(struct uip_conn *cptr = &uip_conns[0];
          cptr < &uip_conns[UIP_TCP_CONNS]; ++cptr) {
        if(cptr->appstate.p == p) {
          cptr->appstate.p = PROCESS_NONE;
          cptr->tcpstateflags = UIP_CLOSED;
        }
      }
#endif /* UIP_TCP */
#if UIP_UDP
      for(struct uip_udp_conn *cptr = &uip_udp_conns[0];
          cptr < &uip_udp_conns[UIP_UDP_CONNS]; ++cptr) {
        if(cptr->appstate.p == p) {
          cptr->lport = 0;
        }
      }
#endif /* UIP_UDP */
    }
    break;
#endif /* UIP_TCP || UIP_UDP */

  case PROCESS_EVENT_TIMER:
    /* We get this event if one of our timers have expired. */
  {
    /* Check the clock so see if we should call the periodic uIP
           processing. */
    if(data == &periodic &&
        etimer_expired(&periodic)) {
#if UIP_TCP
      for(uint8_t i = 0; i < UIP_TCP_CONNS; ++i) {
        if(uip_conn_active(i)) {
          /* Only restart the timer if there are active
                 connections. */
          etimer_restart(&periodic);
          uip_periodic(i);
          tcpip_ipv6_output();
        }
      }
#endif /* UIP_TCP */
    }

#if UIP_CONF_IPV6_REASSEMBLY
    /*
     * check the timer for reassembly
     */
    if(data == &uip_reass_timer &&
        etimer_expired(&uip_reass_timer)) {
      uip_reass_over();
      tcpip_ipv6_output();
    }
#endif /* UIP_CONF_IPV6_REASSEMBLY */
    /*
     * check the different timers for neighbor discovery and
     * stateless autoconfiguration
     */
    /*if(data == &uip_ds6_timer_periodic &&
           etimer_expired(&uip_ds6_timer_periodic)) {
          uip_ds6_periodic();
          tcpip_ipv6_output();
        }*/
#if !UIP_CONF_ROUTER
    if(data == &uip_ds6_timer_rs &&
        etimer_expired(&uip_ds6_timer_rs)) {
      uip_ds6_send_rs();
      tcpip_ipv6_output();
    }
#endif /* !UIP_CONF_ROUTER */
    if(data == &uip_ds6_timer_periodic &&
        etimer_expired(&uip_ds6_timer_periodic)) {
      uip_ds6_periodic();
      tcpip_ipv6_output();
    }
  }
  break;

#if UIP_TCP
  case TCP_POLL:
    if(data != NULL) {
      uip_poll_conn(data);
      tcpip_ipv6_output();
      /* Start the periodic polling, if it isn't already active. */
      start_periodic_tcp_timer();
    }
    break;
#endif /* UIP_TCP */
#if UIP_UDP
  case UDP_POLL:
    if(data != NULL) {
      uip_udp_periodic_conn(data);
      tcpip_ipv6_output();
    }
    break;
#endif /* UIP_UDP */

  case PACKET_INPUT:
    packet_input();
    break;
  };
}
/*---------------------------------------------------------------------------*/
void
tcpip_input(void)
{
  if(netstack_process_ip_callback(NETSTACK_IP_INPUT, NULL) ==
     NETSTACK_IP_PROCESS) {
    process_post_synch(&tcpip_process, PACKET_INPUT, NULL);
  } /* else - do nothing and drop */
  uipbuf_clear();
}
/*---------------------------------------------------------------------------*/
static void
output_fallback(void)
{
#ifdef UIP_FALLBACK_INTERFACE
  uip_last_proto = *((uint8_t *)UIP_IP_BUF + 40);
  LOG_INFO("fallback: removing ext hdrs & setting proto %d %d\n",
         uip_ext_len, uip_last_proto);
  uip_remove_ext_hdr();
  /* Inform the other end that the destination is not reachable. If it's
   * not informed routes might get lost unexpectedly until there's a need
   * to send a new packet to the peer */
  if(UIP_FALLBACK_INTERFACE.output() < 0) {
    LOG_ERR("fallback: output error. Reporting DST UNREACH\n");
    uip_icmp6_error_output(ICMP6_DST_UNREACH, ICMP6_DST_UNREACH_ADDR, 0);
    uip_flags = 0;
    tcpip_ipv6_output();
    return;
  }
#else
  LOG_ERR("output: destination off-link and no default route\n");
#endif /* !UIP_FALLBACK_INTERFACE */
}
/*---------------------------------------------------------------------------*/
static void
annotate_transmission(const uip_ipaddr_t *nexthop)
{
#if TCPIP_CONF_ANNOTATE_TRANSMISSIONS
  static uint8_t annotate_last;
  static uint8_t annotate_has_last = 0;

  if(annotate_has_last) {
    printf("#L %u 0; red\n", annotate_last);
  }
  printf("#L %u 1; red\n", nexthop->u8[sizeof(uip_ipaddr_t) - 1]);
  annotate_last = nexthop->u8[sizeof(uip_ipaddr_t) - 1];
  annotate_has_last = 1;
#endif /* TCPIP_CONF_ANNOTATE_TRANSMISSIONS */
}
/*---------------------------------------------------------------------------*/
static const uip_ipaddr_t*
get_nexthop(uip_ipaddr_t *addr)
{
  const uip_ipaddr_t *nexthop;
  uip_ds6_route_t *route;

  LOG_INFO("output: processing %u bytes packet from ", uip_len);
  LOG_INFO_6ADDR(&UIP_IP_BUF->srcipaddr);
  LOG_INFO_(" to ");
  LOG_INFO_6ADDR(&UIP_IP_BUF->destipaddr);
  LOG_INFO_("\n");

  if(NETSTACK_ROUTING.ext_header_srh_get_next_hop(addr)) {
    LOG_INFO("output: selected next hop from SRH: ");
    LOG_INFO_6ADDR(addr);
    LOG_INFO_("\n");
    return addr;
  }

  /* We first check if the destination address is on our immediate
     link. If so, we simply use the destination address as our
     nexthop address. */
  if(uip_ds6_is_addr_onlink(&UIP_IP_BUF->destipaddr)) {
    LOG_INFO("output: destination is on link\n");
    return &UIP_IP_BUF->destipaddr;
  }

  /* Check if we have a route to the destination address. */
  route = uip_ds6_route_lookup(&UIP_IP_BUF->destipaddr);

  /* No route was found - we send to the default route instead. */
  if(route == NULL) {
    nexthop = uip_ds6_defrt_choose();
    if(nexthop == NULL) {
      output_fallback();
    } else {
      LOG_INFO("output: no route found, using default route: ");
      LOG_INFO_6ADDR(nexthop);
      LOG_INFO_("\n");
    }

  } else {
    /* A route was found, so we look up the nexthop neighbor for
       the route. */
    nexthop = uip_ds6_route_nexthop(route);

    /* If the nexthop is dead, for example because the neighbor
       never responded to link-layer acks, we drop its route. */
    if(nexthop == NULL) {
      LOG_ERR("output: found dead route\n");
      /* Notifiy the routing protocol that we are about to remove the route */
      NETSTACK_ROUTING.drop_route(route);
      /* Remove the route */
      uip_ds6_route_rm(route);
      /* We don't have a nexthop to send the packet to, so we drop it. */
    } else {
      LOG_INFO("output: found next hop from routing table: ");
      LOG_INFO_6ADDR(nexthop);
      LOG_INFO_("\n");
    }
  }

  return nexthop;
}
/*---------------------------------------------------------------------------*/
#if UIP_ND6_SEND_NS
static int
queue_packet(uip_ds6_nbr_t *nbr)
{
  /* Copy outgoing pkt in the queuing buffer for later transmit. */
#if UIP_CONF_IPV6_QUEUE_PKT
  if(uip_packetqueue_alloc(&nbr->packethandle, UIP_DS6_NBR_PACKET_LIFETIME) != NULL) {
    memcpy(uip_packetqueue_buf(&nbr->packethandle), UIP_IP_BUF, uip_len);
    uip_packetqueue_set_buflen(&nbr->packethandle, uip_len);
    return 0;
  }
#endif

  return 1;
}
#endif
/*---------------------------------------------------------------------------*/
static void
send_queued(uip_ds6_nbr_t *nbr)
{
#if UIP_CONF_IPV6_QUEUE_PKT
  /*
   * Send the queued packets from here, may not be 100% perfect though.
   * This happens in a few cases, for example when instead of receiving a
   * NA after sendiong a NS, you receive a NS with SLLAO: the entry moves
   * to STALE, and you must both send a NA and the queued packet.
   */
  if(uip_packetqueue_buflen(&nbr->packethandle) != 0) {
    uip_len = uip_packetqueue_buflen(&nbr->packethandle);
    memcpy(UIP_IP_BUF, uip_packetqueue_buf(&nbr->packethandle), uip_len);
    uip_packetqueue_free(&nbr->packethandle);
    tcpip_output(uip_ds6_nbr_get_ll(nbr));
  }
#endif /*UIP_CONF_IPV6_QUEUE_PKT*/
}
/*---------------------------------------------------------------------------*/
static int
send_nd6_ns(const uip_ipaddr_t *nexthop)
{
  int err = 1;

#if UIP_ND6_SEND_NS
   uip_ds6_nbr_t *nbr = NULL;
  if((nbr = uip_ds6_nbr_add(nexthop, NULL, 0, NBR_INCOMPLETE, NBR_TABLE_REASON_IPV6_ND, NULL)) != NULL) {
    err = 0;

    queue_packet(nbr);
  /* RFC4861, 7.2.2:
   * "If the source address of the packet prompting the solicitation is the
   * same as one of the addresses assigned to the outgoing interface, that
   * address SHOULD be placed in the IP Source Address of the outgoing
   * solicitation.  Otherwise, any one of the addresses assigned to the
   * interface should be used."*/
   if(uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr)){
      uip_nd6_ns_output(&UIP_IP_BUF->srcipaddr, NULL, &nbr->ipaddr);
    } else {
      uip_nd6_ns_output(NULL, NULL, &nbr->ipaddr);
    }

    stimer_set(&nbr->sendns, uip_ds6_if.retrans_timer / 1000);
    nbr->nscount = 1;
    /* Send the first NS try from here (multicast destination IP address). */
  }
#else
  LOG_ERR("output: neighbor not in cache: ");
  LOG_ERR_6ADDR(nexthop);
  LOG_ERR_("\n");
#endif

  return err;
}
/*---------------------------------------------------------------------------*/
void
tcpip_ipv6_output(void)
{
  uip_ipaddr_t ipaddr;
  uip_ds6_nbr_t *nbr = NULL;
  const uip_lladdr_t *linkaddr;
  const uip_ipaddr_t *nexthop;

  if(uip_len == 0) {
    return;
  }

  if(uip_len > UIP_LINK_MTU) {
    LOG_ERR("output: Packet too big");
    goto exit;
  }

  if(uip_is_addr_unspecified(&UIP_IP_BUF->destipaddr)){
    LOG_ERR("output: Destination address unspecified");
    goto exit;
  }


  if(!NETSTACK_ROUTING.ext_header_update()) {
    /* Packet can not be forwarded */
    LOG_ERR("output: routing protocol extension header update error\n");
    uipbuf_clear();
    return;
  }

  if(uip_is_addr_mcast(&UIP_IP_BUF->destipaddr)) {
    linkaddr = NULL;
    goto send_packet;
  }

  /* We first check if the destination address is one of ours. There is no
   * loopback interface -- instead, process this directly as incoming. */
  if(uip_ds6_is_my_addr(&UIP_IP_BUF->destipaddr)) {
    LOG_INFO("output: sending to ourself\n");
    packet_input();
    return;
  }

  /* Look for a next hop */
  if((nexthop = get_nexthop(&ipaddr)) == NULL) {
    LOG_WARN("output: No next-hop found, dropping packet\n");
    goto exit;
  }
  annotate_transmission(nexthop);

  nbr = uip_ds6_nbr_lookup(nexthop);

#if UIP_ND6_AUTOFILL_NBR_CACHE
  if(nbr == NULL) {
    /* Neighbor not found in cache? Derive its link-layer address from it's
    link-local IPv6, assuming it used autoconfiguration. This is not
    standard-compliant but this is a convenient way to keep the
    neighbor cache out of the way in cases ND is not used */
    uip_lladdr_t lladdr;
    uip_ds6_set_lladdr_from_iid(&lladdr, nexthop);
    if((nbr = uip_ds6_nbr_add(nexthop, &lladdr,
        0, NBR_REACHABLE, NBR_TABLE_REASON_IPV6_ND_AUTOFILL, NULL)) == NULL) {
      LOG_ERR("output: failed to autofill neighbor cache for host ");
      LOG_ERR_6ADDR(nexthop);
      LOG_ERR_(", link-layer addr ");
      LOG_ERR_LLADDR((linkaddr_t*)&lladdr);
      LOG_ERR_("\n");
      goto exit;
    }
   }
#endif /* UIP_ND6_AUTOFILL_NBR_CACHE */

  if(nbr == NULL) {
    if(send_nd6_ns(nexthop)) {
      LOG_ERR("output: failed to add neighbor to cache\n");
      goto exit;
    } else {
      /* We're sending NS here instead of original packet */
      goto send_packet;
    }
  }

#if UIP_ND6_SEND_NS
  if(nbr->state == NBR_INCOMPLETE) {
    LOG_ERR("output: nbr cache entry incomplete\n");
    queue_packet(nbr);
    goto exit;
  }
  /* Send in parallel if we are running NUD (nbc state is either STALE,
     DELAY, or PROBE). See RFC 4861, section 7.3.3 on node behavior. */
  if(nbr->state == NBR_STALE) {
    nbr->state = NBR_DELAY;
    stimer_set(&nbr->reachable, UIP_ND6_DELAY_FIRST_PROBE_TIME);
    nbr->nscount = 0;
    LOG_INFO("output: nbr cache entry stale moving to delay\n");
  }
#endif /* UIP_ND6_SEND_NS */

send_packet:
  if(nbr) {
    linkaddr = uip_ds6_nbr_get_ll(nbr);
  } else {
    linkaddr = NULL;
  }

  LOG_INFO("output: sending to ");
  LOG_INFO_LLADDR((linkaddr_t *)linkaddr);
  LOG_INFO_("\n");
  tcpip_output(linkaddr);

  if(nbr) {
    send_queued(nbr);
  }

exit:
  uipbuf_clear();
  return;
}
/*---------------------------------------------------------------------------*/
#if UIP_UDP
void
tcpip_poll_udp(struct uip_udp_conn *conn)
{
  process_post(&tcpip_process, UDP_POLL, conn);
}
#endif /* UIP_UDP */
/*---------------------------------------------------------------------------*/
#if UIP_TCP
void
tcpip_poll_tcp(struct uip_conn *conn)
{
  process_post(&tcpip_process, TCP_POLL, conn);
}
#endif /* UIP_TCP */
/*---------------------------------------------------------------------------*/
void
tcpip_uipcall(void)
{
  uip_udp_appstate_t *ts;

#if UIP_UDP
  if(uip_conn != NULL) {
    ts = &uip_conn->appstate;
  } else {
    ts = &uip_udp_conn->appstate;
  }
#else /* UIP_UDP */
  ts = &uip_conn->appstate;
#endif /* UIP_UDP */

#if UIP_TCP
  {
    unsigned char i;
    struct listenport *l;

    /* If this is a connection request for a listening port, we must
      mark the connection with the right process ID. */
    if(uip_connected()) {
      l = &s.listenports[0];
      for(i = 0; i < UIP_LISTENPORTS; ++i) {
        if(l->port == uip_conn->lport &&
            l->p != PROCESS_NONE) {
          ts->p = l->p;
          ts->state = NULL;
          break;
        }
        ++l;
      }

      /* Start the periodic polling, if it isn't already active. */
      start_periodic_tcp_timer();
    }
  }
#endif /* UIP_TCP */

  if(ts->p != NULL) {
    process_post_synch(ts->p, tcpip_event, ts->state);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(tcpip_process, ev, data)
{
  PROCESS_BEGIN();

#if UIP_TCP
  memset(s.listenports, 0, UIP_LISTENPORTS*sizeof(*(s.listenports)));
  s.p = PROCESS_CURRENT();
#endif

  tcpip_event = process_alloc_event();
#if UIP_CONF_ICMP6
  tcpip_icmp6_event = process_alloc_event();
#endif /* UIP_CONF_ICMP6 */
  etimer_set(&periodic, CLOCK_SECOND / 2);

  uip_init();
#ifdef UIP_FALLBACK_INTERFACE
  UIP_FALLBACK_INTERFACE.init();
#endif
  /* Initialize routing protocol */
  NETSTACK_ROUTING.init();

  while(1) {
    PROCESS_YIELD();
    eventhandler(ev, data);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
