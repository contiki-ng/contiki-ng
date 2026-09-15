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
 *         An IP64 border router: RPL root on the 802.15.4 side, NAT64 and
 *         an IPv4 uplink over an ENC28J60 on the Ethernet side.
 *
 *         Functionally this is the stock examples/ip64-router, with a status
 *         report added. The stock example prints nothing at all, which during
 *         bring-up is indistinguishable from a hang -- and the one thing you
 *         most want to see is whether DHCP ever returned an address.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "contiki-net.h"
#include "ip64/ip64.h"
#include "net/netstack.h"
#include "net/routing/routing.h"
#include "sys/autostart.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/ip64-addr.h"
#include "net/ipv6/uip-debug.h"

#include <stdio.h>
/*---------------------------------------------------------------------------*/
#define REPORT_INTERVAL (CLOCK_SECOND * 5)
/*---------------------------------------------------------------------------*/
/*
 * Optional self-test of the translator. The router sends a UDP datagram to an
 * IPv4 host through its own NAT64 prefix, which exercises ip64_6to4() without
 * needing a second node on the mesh: a locally generated packet to an
 * off-link, unroutable destination takes the same uIP fallback path
 * (tcpip.c output_fallback) that a forwarded one does.
 *
 * Set NAT64_TEST_ADDR to an IPv4 host running a UDP listener, e.g.
 *   nc -u -l 7777
 */
#ifdef NAT64_TEST_ADDR
#ifndef NAT64_TEST_PORT
#define NAT64_TEST_PORT 7777
#endif
/*
 * Indirection so NAT64_TEST_ADDR's four octets are macro-expanded before
 * uip_nat64addr() counts its arguments.
 */
#define NAT64_SET_DEST(addr, ...) uip_nat64addr(addr, __VA_ARGS__)

static struct simple_udp_connection nat64_conn;
#endif
/*---------------------------------------------------------------------------*/
PROCESS(router_node_process, "IP64 router");
AUTOSTART_PROCESSES(&router_node_process);
/*---------------------------------------------------------------------------*/
static void
print_ip4(const char *label, const uip_ip4addr_t *a)
{
  printf("  %-9s %u.%u.%u.%u\n", label,
         a->u8[0], a->u8[1], a->u8[2], a->u8[3]);
}
/*---------------------------------------------------------------------------*/
#ifdef NAT64_TEST_ADDR
static void
nat64_reply(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
            uint16_t sender_port, const uip_ipaddr_t *receiver_addr,
            uint16_t receiver_port, const uint8_t *data, uint16_t datalen)
{
  printf("NAT64 reply: %u bytes from ", datalen);
  uip_debug_ipaddr_print(sender_addr);
  printf("\n");
}
#endif
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(router_node_process, ev, data)
{
  static struct etimer periodic;
  static int was_configured;
#ifdef NAT64_TEST_ADDR
  static uip_ipaddr_t nat64_dest;
  static unsigned long sent;
#endif

  PROCESS_BEGIN();

  printf("\nIP64 router starting\n");

  /* Set us up as a RPL root node. */
  NETSTACK_ROUTING.root_start();
  printf("  RPL root started\n");

  /* Initialize the IP64 module so we'll start translating packets. */
  ip64_init();
  printf("  IP64 initialised, waiting for DHCP on the Ethernet side\n");

  was_configured = 0;
  while(1) {
    etimer_set(&periodic, REPORT_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic));

    if(ip64_hostaddr_is_configured()) {
      if(!was_configured) {
        was_configured = 1;
        printf("IPv4 address acquired:\n");
        print_ip4("address", ip64_get_hostaddr());
        print_ip4("netmask", ip64_get_netmask());
        print_ip4("gateway", ip64_get_draddr());
        printf("router is up -- IPv6 nodes can now reach IPv4 hosts\n");
#ifdef NAT64_TEST_ADDR
        NAT64_SET_DEST(&nat64_dest, NAT64_TEST_ADDR);
        simple_udp_register(&nat64_conn, NAT64_TEST_PORT, NULL,
                            NAT64_TEST_PORT, nat64_reply);
        printf("NAT64 self-test: sending UDP to ");
        uip_debug_ipaddr_print(&nat64_dest);
        printf(" port %u\n", NAT64_TEST_PORT);
#endif
      }
#ifdef NAT64_TEST_ADDR
      {
        char msg[64];
        int len = snprintf(msg, sizeof(msg),
                           "hello from contiki-ng via NAT64 #%lu\n", ++sent);
        simple_udp_sendto(&nat64_conn, msg, len, &nat64_dest);
        printf("NAT64 self-test: sent #%lu (%d bytes)\n", sent, len);
      }
#endif
    } else {
      if(was_configured) {
        was_configured = 0;
        printf("IPv4 address lost, waiting for DHCP again\n");
      } else {
        printf("waiting for DHCP (no IPv4 address yet)\n");
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
