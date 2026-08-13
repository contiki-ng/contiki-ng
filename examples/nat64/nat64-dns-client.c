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
 *         NAT64 DNS lookup + HTTP GET example.
 *
 *         Demonstrates how an IPv6-only Contiki-NG node can use the
 *         border-router-side NAT64 gateway to reach the IPv4 internet:
 *
 *           1. Configures Google Public DNS (8.8.8.8) as the upstream
 *              resolver, addressed via the well-known NAT64 prefix
 *              `64:ff9b::808:808`.
 *           2. Periodically resolves a small list of public hostnames
 *              through the standard Contiki-NG `resolv` module — the
 *              border router's DNS64 translator transparently rewrites
 *              AAAA queries to A and the responses back into AAAA with
 *              the NAT64 prefix.
 *           3. For each successful lookup, sends a UDP probe to the
 *              resolved address as a connectivity check.
 *           4. For the dedicated `HTTP_TARGET_HOST`, performs an HTTP
 *              GET via the `http-socket` module to exercise the TCP
 *              splice proxy end-to-end.
 *
 *         The example is intentionally minimal: there is no per-host
 *         retry logic and the periodic timer simply rotates through
 *         the hostname list.  See `examples/nat64/README.md` for
 *         build/run instructions in both Cooja and on hardware.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip-nameserver.h"
#include "net/ipv6/ip64-addr.h"
#include "net/app-layer/http-socket/http-socket.h"
#include "resolv.h"

#include <stdio.h>
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_PROBE_PORT 1234
#define LOOKUP_INTERVAL (60 * CLOCK_SECOND)

#define HTTP_TARGET_HOST "www.contiki-ng.org"

static const char *hostnames[] = {
  HTTP_TARGET_HOST,
  "www.example.com",
  "www.google.com",
};
#define NUM_HOSTNAMES (sizeof(hostnames) / sizeof(hostnames[0]))

static struct simple_udp_connection udp_conn;
static unsigned current_host;

static struct http_socket http_sock;
static bool http_in_progress;
static uint32_t http_body_bytes;
/*---------------------------------------------------------------------------*/
PROCESS(nat64_dns_process, "NAT64 DNS client");
AUTOSTART_PROCESSES(&nat64_dns_process);
/*---------------------------------------------------------------------------*/
static void
configure_nameserver(void)
{
  uip_ipaddr_t dns_server;

  /* Google Public DNS (8.8.8.8) via the NAT64 well-known prefix. */
  uip_nat64addr(&dns_server, 8, 8, 8, 8);

  uip_nameserver_update(&dns_server, UIP_NAMESERVER_INFINITE_LIFETIME);

  LOG_INFO("DNS server set to ");
  LOG_INFO_6ADDR(&dns_server);
  LOG_INFO_(" (8.8.8.8 via NAT64)\n");
}
/*---------------------------------------------------------------------------*/
static void
send_udp_probe(const uip_ipaddr_t *addr, const char *hostname)
{
  char msg[64];
  int len;
  size_t payload_len;

  len = snprintf(msg, sizeof(msg), "Hello from Contiki-NG to %s", hostname);
  if(len < 0) {
    payload_len = 0;
  } else if((size_t)len >= sizeof(msg)) {
    payload_len = sizeof(msg) - 1;
  } else {
    payload_len = (size_t)len;
  }

  simple_udp_sendto(&udp_conn, msg, payload_len, addr);

  LOG_INFO("Sent UDP probe to ");
  LOG_INFO_6ADDR(addr);
  LOG_INFO_(" (%s)\n", hostname);
}
/*---------------------------------------------------------------------------*/
static void
http_callback(struct http_socket *s, void *ptr,
              http_socket_event_t event,
              const uint8_t *data, uint16_t datalen)
{
  switch(event) {
  case HTTP_SOCKET_HEADER:
    LOG_INFO("HTTP %u, Content-Length: %ld\n",
             s->header.status_code,
             (long)s->header.content_length);
    break;
  case HTTP_SOCKET_DATA:
    http_body_bytes += datalen;
    break;
  case HTTP_SOCKET_CLOSED:
    LOG_INFO("HTTP complete: received %lu body bytes\n",
             (unsigned long)http_body_bytes);
    http_in_progress = false;
    break;
  case HTTP_SOCKET_TIMEDOUT:
    LOG_WARN("HTTP timed out after %lu bytes\n",
             (unsigned long)http_body_bytes);
    http_in_progress = false;
    break;
  case HTTP_SOCKET_ABORTED:
    LOG_WARN("HTTP aborted\n");
    http_in_progress = false;
    break;
  case HTTP_SOCKET_HOSTNAME_NOT_FOUND:
    LOG_WARN("HTTP hostname not found\n");
    http_in_progress = false;
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
start_http_get(void)
{
  if(http_in_progress) {
    return;
  }

  LOG_INFO("Starting HTTP GET http://" HTTP_TARGET_HOST "/\n");
  http_in_progress = true;
  http_body_bytes = 0;

  http_socket_init(&http_sock);
  http_socket_get(&http_sock,
                  "http://" HTTP_TARGET_HOST "/",
                  0, 0,
                  http_callback, NULL);
}
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{
  LOG_INFO("Received %u bytes from ", datalen);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_(":%u\n", sender_port);
}
/*---------------------------------------------------------------------------*/
static void
attempt_lookup(const char *hostname)
{
  uip_ipaddr_t *resolved_addr = NULL;
  resolv_status_t status;

  status = resolv_lookup(hostname, &resolved_addr);

  switch(status) {
  case RESOLV_STATUS_CACHED:
    LOG_INFO("Resolved \"%s\" -> ", hostname);
    LOG_INFO_6ADDR(resolved_addr);
    LOG_INFO_("\n");
    if(strcmp(hostname, HTTP_TARGET_HOST) == 0) {
      start_http_get();
    } else {
      send_udp_probe(resolved_addr, hostname);
    }
    break;
  case RESOLV_STATUS_UNCACHED:
  case RESOLV_STATUS_EXPIRED:
    LOG_INFO("Querying DNS for \"%s\"...\n", hostname);
    resolv_query(hostname);
    break;
  case RESOLV_STATUS_RESOLVING:
    LOG_DBG("Still resolving \"%s\"...\n", hostname);
    break;
  case RESOLV_STATUS_NOT_FOUND:
    LOG_WARN("DNS lookup failed for \"%s\": not found\n", hostname);
    break;
  case RESOLV_STATUS_ERROR:
    LOG_ERR("DNS lookup error for \"%s\"\n", hostname);
    break;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nat64_dns_process, ev, data)
{
  static struct etimer periodic_timer;
  static struct etimer startup_timer;

  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, UDP_PROBE_PORT, NULL,
                      UDP_PROBE_PORT, udp_rx_callback);

  LOG_INFO("NAT64 DNS client starting, waiting for network...\n");
  etimer_set(&startup_timer, 5 * CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&startup_timer));

  configure_nameserver();

  current_host = 0;
  etimer_set(&periodic_timer, 2 * CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == resolv_event_found && data != NULL) {
      attempt_lookup((const char *)data);
    }

    if(etimer_expired(&periodic_timer)) {
      if(NETSTACK_ROUTING.node_is_reachable()) {
        attempt_lookup(hostnames[current_host]);
        current_host = (current_host + 1) % NUM_HOSTNAMES;
      } else {
        LOG_INFO("Network not reachable yet\n");
      }
      etimer_set(&periodic_timer, LOOKUP_INTERVAL);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
