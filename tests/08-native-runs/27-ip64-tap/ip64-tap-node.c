/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
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
 */


/**
 * \file
 *   Node for the ip64 TAP test. It runs the ip64 service with its IPv4 side
 *   on a host TAP device, and sends UDP to an IPv4 echo server on the host
 *   through the NAT64 prefix until the reply comes back. The host can reach
 *   the node in the other direction at the IPv4 address configured here,
 *   which is what the accompanying script uses ping for.
 *
 *   Built with IP64_CONF_DHCP=1 the node takes its IPv4 address from a DHCP
 *   server on the host instead of the fixed one below. That exchange travels
 *   through ip64 as well: the client sends to 64:ff9b::255.255.255.255, which
 *   ip64 translates into an IPv4 broadcast, and the reply comes back as an
 *   IPv4 broadcast that ip64 turns into an all-nodes multicast.
 *
 *   The node also resolves a name through a DNS server on the host. Because
 *   the node is IPv6-only it asks for a AAAA record, which the IPv4 server
 *   can only answer if ip64 rewrites the question, and the A record that
 *   comes back is of no use unless ip64 turns it into an AAAA record.
 */

#include "contiki.h"
#include "contiki-net.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/ip64-addr.h"

#include "ip64/ip64.h"
#include "ip64/ip64-eth.h"
#include "net/ipv6/uip-nameserver.h"
#include "net/ipv6/uiplib.h"
#include "resolv.h"

#include <stdio.h>
#include <string.h>

#define LOCAL_PORT   3000
#define ECHO_PORT    5557

#define REQUEST      "PING-IP64-TAP"


#define SEND_INTERVAL (CLOCK_SECOND * 2)

/* The name the host's DNS server answers, and the address it answers with.
   The address differs from the server's own so that a resolved address can
   only have come from the answer. */
#define LOOKUP_NAME "ip64-test.example"

static bool name_resolved;

static struct simple_udp_connection conn;

PROCESS(ip64_tap_node_process, "ip64 TAP node");
AUTOSTART_PROCESSES(&ip64_tap_node_process);

/*---------------------------------------------------------------------------*/
static void
echo_received(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
              uint16_t sender_port, const uip_ipaddr_t *receiver_addr,
              uint16_t receiver_port, const uint8_t *data, uint16_t datalen)
{
  if(datalen == strlen(REQUEST) && memcmp(data, REQUEST, datalen) == 0) {
    /* The echo server on the host received IPv4 and its reply was translated
       back, so both legs work. */
    printf("IP64_TAP_ECHO_OK\n");
  }
}
/*---------------------------------------------------------------------------*/
/* Report the address the name resolved to, once. The address the host's DNS
   server put in its A record must come back as a NAT64 address. */
static void
attempt_lookup(void)
{
  uip_ipaddr_t *resolved = NULL;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];

  switch(resolv_lookup(LOOKUP_NAME, &resolved)) {
  case RESOLV_STATUS_CACHED:
    if(!name_resolved && resolved != NULL) {
      name_resolved = true;
      uiplib_ipaddr_snprint(buf, sizeof(buf), resolved);
      printf("IP64_TAP_DNS_OK %s %s\n", LOOKUP_NAME, buf);
    }
    break;
  case RESOLV_STATUS_UNCACHED:
  case RESOLV_STATUS_EXPIRED:
    resolv_query(LOOKUP_NAME);
    break;
  case RESOLV_STATUS_NOT_FOUND:
    printf("IP64_TAP_DNS_NOT_FOUND %s\n", LOOKUP_NAME);
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ip64_tap_node_process, ev, data)
{
  static struct etimer timer;
  static uip_ipaddr_t echoaddr;
#if IP64_DHCP
  const uip_ip4addr_t *configured;
#else
  static uip_ip4addr_t hostaddr, netmask, draddr;
#endif
  static struct ip64_eth_addr eth_addr = {
    { 0x02, 0x11, 0x22, 0x33, 0x44, 0x55 }
  };

  PROCESS_BEGIN();

  /* The test script builds this node both ways and reads here which one it
     is running, so that it knows whether to expect a lease. */
#if IP64_DHCP
  printf("IP64_TAP_MODE dhcp\n");
#else
  printf("IP64_TAP_MODE static\n");
#endif

  ip64_eth_addr_set(&eth_addr);

  /* Creates the TAP device, and starts the DHCP client if it is enabled. */
  ip64_init();
  printf("IP64_TAP_DEVICE_UP\n");

#if IP64_DHCP
  /* Wait for the lease. The server on the host may not be listening yet, but
     the DHCP client retransmits. */
  etimer_set(&timer, CLOCK_SECOND / 2);
  while(!ip64_hostaddr_is_configured()) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  configured = ip64_get_hostaddr();
  printf("IP64_TAP_LEASE %d.%d.%d.%d", uip_ipaddr_to_quad(configured));
  configured = ip64_get_draddr();
  printf(" router %d.%d.%d.%d\n", uip_ipaddr_to_quad(configured));
#else
  /* The address the node answers to on the TAP network, and the host end of
     it, which doubles as the node's default router. */
  uip_ipaddr(&hostaddr, 192, 0, 2, 50);
  uip_ipaddr(&netmask, 255, 255, 255, 0);
  uip_ipaddr(&draddr, 192, 0, 2, 1);

  ip64_set_hostaddr(&hostaddr);
  ip64_set_netmask(&netmask);
  ip64_set_draddr(&draddr);
#endif /* IP64_DHCP */

  simple_udp_register(&conn, LOCAL_PORT, NULL, ECHO_PORT, echo_received);

  /* The echo server listens on the host end of the TAP network, which is
     also the default router, reached through the NAT64 prefix. Taking the
     address from ip64 covers both ways of configuring it. */
  ip64_addr_4to6(ip64_get_draddr(), &echoaddr);

  /* The DNS server runs on the host too, reached through the NAT64 prefix
     like any other IPv4 address. */
  uip_nameserver_update(&echoaddr, UIP_NAMESERVER_INFINITE_LIFETIME);

  printf("IP64_TAP_READY\n");

  etimer_set(&timer, SEND_INTERVAL);
  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == resolv_event_found) {
      attempt_lookup();
    }

    if(etimer_expired(&timer)) {
      simple_udp_sendto(&conn, REQUEST, strlen(REQUEST), &echoaddr);
      if(!name_resolved) {
        attempt_lookup();
      }
      etimer_reset(&timer);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
