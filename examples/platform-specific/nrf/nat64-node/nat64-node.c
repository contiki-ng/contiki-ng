/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \file
 *         A mesh node that talks to an IPv4 host through the IP64 border
 *         router's NAT64 translator.
 *
 *         This is the other half of the ip64-router test. The router can
 *         already prove the translator with its own traffic, but only a real
 *         node exercises the parts unique to forwarding: the RPL hop into the
 *         router, and a per-source entry in ip64's address map.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "contiki-net.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/ip64-addr.h"
#include "net/ipv6/uip-debug.h"
#include "net/ipv6/uip-nameserver.h"
#include "resolv.h"
#include "sys/autostart.h"

#include <stdio.h>
/*---------------------------------------------------------------------------*/
/*
 * Optional UDP probe. With NAT64_TEST_ADDR defined as four comma-separated
 * octets, the node sends a datagram every SEND_INTERVAL to that IPv4 host
 * through the border router's NAT64 prefix; run  nc -u -l 7777  there to see
 * it arrive. Off by default so an in-tree build does not talk to anyone's
 * LAN. The DNS64 lookup below runs regardless.
 */
#ifdef NAT64_TEST_ADDR
#ifndef NAT64_TEST_PORT
#define NAT64_TEST_PORT 7777
#endif
#endif

#define SEND_INTERVAL (CLOCK_SECOND * 5)

/*
 * DNS64 test. The node is pointed at a public IPv4 DNS server reached through
 * the NAT64 prefix; ip64 spots UDP to port 53 and rewrites the AAAA query into
 * an A query on the way out, then synthesises an AAAA record from the A answer
 * on the way back (ip64.c 6to4/4to6 -> ip64-dns64.c). A successful lookup
 * therefore returns a 64:ff9b:: address that can be used directly, which is
 * what removes the need to hardcode any IPv4 literal.
 */
#ifndef NAT64_DNS_SERVER
#define NAT64_DNS_SERVER 8, 8, 8, 8
#endif
#ifndef NAT64_LOOKUP_NAME
#define NAT64_LOOKUP_NAME "leshan.eclipseprojects.io"
#endif

/* Expand the octets before uip_nat64addr() counts its arguments. */
#define NAT64_SET_DEST(addr, ...) uip_nat64addr(addr, __VA_ARGS__)
/*---------------------------------------------------------------------------*/
#ifdef NAT64_TEST_ADDR
static struct simple_udp_connection conn;
#endif
/*---------------------------------------------------------------------------*/
PROCESS(nat64_node_process, "NAT64 node");
AUTOSTART_PROCESSES(&nat64_node_process);
/*---------------------------------------------------------------------------*/
#ifdef NAT64_TEST_ADDR
static void
rx(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
   uint16_t sender_port, const uip_ipaddr_t *receiver_addr,
   uint16_t receiver_port, const uint8_t *data, uint16_t datalen)
{
  printf("reply: %u bytes from ", datalen);
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
}
#endif /* NAT64_TEST_ADDR */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nat64_node_process, ev, data)
{
  static struct etimer periodic;
#ifdef NAT64_TEST_ADDR
  static uip_ipaddr_t dest;
  static unsigned long sent;
#endif
  static int joined;
  static uip_ipaddr_t dns;
  static int dns_asked;
  static int dns_done;

  PROCESS_BEGIN();

  printf("\nNAT64 node starting\n");
#ifdef NAT64_TEST_ADDR
  NAT64_SET_DEST(&dest, NAT64_TEST_ADDR);
  printf("  UDP probe target ");
  uip_debug_ipaddr_print(&dest);
  printf(" port %u\n", NAT64_TEST_PORT);

  simple_udp_register(&conn, NAT64_TEST_PORT, NULL, NAT64_TEST_PORT, rx);
  sent = 0;
#endif

  joined = 0;
  while(1) {
    etimer_set(&periodic, SEND_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic));

    if(!NETSTACK_ROUTING.node_is_reachable()) {
      if(joined) {
        joined = 0;
        printf("left the RPL network\n");
      } else {
        printf("waiting to join the RPL network...\n");
      }
      continue;
    }

    if(!joined) {
      joined = 1;
      printf("joined the RPL network -- sending through NAT64 now\n");

      /* Point the resolver at an IPv4 DNS server via the NAT64 prefix. */
      NAT64_SET_DEST(&dns, NAT64_DNS_SERVER);
      uip_nameserver_update(&dns, UIP_NAMESERVER_INFINITE_LIFETIME);
      printf("DNS64: nameserver set to ");
      uip_debug_ipaddr_print(&dns);
      printf("\n");
    }

    if(joined && !dns_asked) {
      dns_asked = 1;
      printf("DNS64: looking up %s\n", NAT64_LOOKUP_NAME);
      resolv_query(NAT64_LOOKUP_NAME);
    }

    if(dns_asked && !dns_done) {
      uip_ipaddr_t *resolved = NULL;
      resolv_status_t st = resolv_lookup(NAT64_LOOKUP_NAME, &resolved);

      if(st == RESOLV_STATUS_CACHED && resolved != NULL) {
        dns_done = 1;
        printf("DNS64 OK: %s -> ", NAT64_LOOKUP_NAME);
        uip_debug_ipaddr_print(resolved);
        printf("\n");
      } else if(st == RESOLV_STATUS_NOT_FOUND || st == RESOLV_STATUS_ERROR) {
        dns_done = 1;
        printf("DNS64 FAILED for %s (status %d)\n", NAT64_LOOKUP_NAME, st);
      } else {
        printf("DNS64: resolving... (status %d)\n", st);
      }
    }

#ifdef NAT64_TEST_ADDR
    {
      char msg[64];
      int len = snprintf(msg, sizeof(msg),
                         "hello from a mesh node via NAT64 #%lu\n", ++sent);
      simple_udp_sendto(&conn, msg, len, &dest);
      printf("sent #%lu (%d bytes)\n", sent, len);
    }
#endif
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
