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
 *         Mote-side driver for the NAT64 end-to-end test.
 *
 *         Each mote tags its UDP and TCP payload with its node_id
 *         ("PING-<id>") and sends them to a fixed IPv4 destination
 *         (127.0.0.1) addressed via the NAT64 well-known prefix
 *         (64:ff9b::/96).  The native border router translates the
 *         traffic to plain IPv4 sockets, where a Python echo server
 *         reflects each datagram and connection.  When the mote
 *         receives the reflected payload back, it logs a distinctive
 *         marker (UDP_ECHO_OK / TCP_ECHO_OK) including its node_id,
 *         so the shell driver can verify that every mote in the
 *         multi-hop network completed both legs.
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/tcp-socket.h"
#include "net/ipv6/ip64-addr.h"
#include "sys/node-id.h"

#include <stdio.h>
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "NAT64Test"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_LOCAL_PORT  5557
#define UDP_REMOTE_PORT 5557
#define TCP_REMOTE_PORT 5558

#define UDP_PROBE_INTERVAL (5 * CLOCK_SECOND)
#define UDP_PROBE_MAX 24

#define PAYLOAD_BUF_SIZE 16
#define TCP_BUF_SIZE     128

static struct simple_udp_connection udp_conn;
static struct tcp_socket tcp_sock;
static uint8_t tcp_input_buf[TCP_BUF_SIZE];
static uint8_t tcp_output_buf[TCP_BUF_SIZE];

static char payload[PAYLOAD_BUF_SIZE];
static size_t payload_len;

static bool udp_echo_received;
static bool tcp_echo_received;
/*---------------------------------------------------------------------------*/
PROCESS(nat64_test_process, "NAT64 end-to-end test");
AUTOSTART_PROCESSES(&nat64_test_process);
/*---------------------------------------------------------------------------*/
static bool
payload_matches(const uint8_t *data, uint16_t datalen)
{
  return datalen >= payload_len &&
         memcmp(data, payload, payload_len) == 0;
}
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr, uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
                const uint8_t *data, uint16_t datalen)
{
  if(payload_matches(data, datalen)) {
    if(!udp_echo_received) {
      udp_echo_received = true;
      LOG_INFO("UDP_ECHO_OK node=%u\n", node_id);
    }
  } else {
    LOG_INFO("UDP_RX_UNEXPECTED node=%u bytes=%u\n", node_id, datalen);
  }
}
/*---------------------------------------------------------------------------*/
static int
tcp_data_callback(struct tcp_socket *s, void *ptr,
                  const uint8_t *data, int datalen)
{
  if(payload_matches(data, datalen)) {
    if(!tcp_echo_received) {
      tcp_echo_received = true;
      LOG_INFO("TCP_ECHO_OK node=%u\n", node_id);
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
tcp_event_callback(struct tcp_socket *s, void *ptr,
                   tcp_socket_event_t event)
{
  switch(event) {
  case TCP_SOCKET_CONNECTED:
    LOG_INFO("TCP_CONNECTED node=%u\n", node_id);
    tcp_socket_send(s, (const uint8_t *)payload, payload_len);
    break;
  case TCP_SOCKET_CLOSED:
    LOG_INFO("TCP_CLOSED node=%u\n", node_id);
    break;
  case TCP_SOCKET_TIMEDOUT:
    LOG_INFO("TCP_TIMEDOUT node=%u\n", node_id);
    break;
  case TCP_SOCKET_ABORTED:
    LOG_INFO("TCP_ABORTED node=%u\n", node_id);
    break;
  case TCP_SOCKET_DATA_SENT:
    break;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nat64_test_process, ev, data)
{
  static struct etimer wait_timer;
  static uip_ipaddr_t target;
  static unsigned probes;
  int n;

  PROCESS_BEGIN();

  /* Stagger startup across motes so they don't all probe simultaneously. */
  etimer_set(&wait_timer, (clock_time_t)node_id * CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));

  n = snprintf(payload, sizeof(payload), "PING-%u", node_id);
  payload_len = (n > 0 && (size_t)n < sizeof(payload)) ? (size_t)n : 0;

  simple_udp_register(&udp_conn, UDP_LOCAL_PORT, NULL, UDP_REMOTE_PORT,
                      udp_rx_callback);

  tcp_socket_register(&tcp_sock, NULL,
                      tcp_input_buf, sizeof(tcp_input_buf),
                      tcp_output_buf, sizeof(tcp_output_buf),
                      tcp_data_callback, tcp_event_callback);

  /* 64:ff9b::7f00:1 maps 127.0.0.1 via the well-known NAT64 prefix. */
  uip_nat64addr(&target, 127, 0, 0, 1);

  LOG_INFO("Starting node=%u, waiting for routing...\n", node_id);
  etimer_set(&wait_timer, CLOCK_SECOND);
  while(!NETSTACK_ROUTING.node_is_reachable()) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
    etimer_reset(&wait_timer);
  }
  LOG_INFO("Network reachable node=%u, starting probes\n", node_id);

  for(probes = 0; probes < UDP_PROBE_MAX && !udp_echo_received; probes++) {
    LOG_INFO("UDP_PROBE_SEND node=%u probe=%u\n", node_id, probes);
    simple_udp_sendto(&udp_conn, payload, payload_len, &target);
    etimer_set(&wait_timer, UDP_PROBE_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
  }

  if(!udp_echo_received) {
    LOG_INFO("UDP_GAVE_UP node=%u\n", node_id);
  }

  LOG_INFO("TCP_CONNECT node=%u to ", node_id);
  LOG_INFO_6ADDR(&target);
  LOG_INFO_(":%u\n", TCP_REMOTE_PORT);
  tcp_socket_connect(&tcp_sock, &target, TCP_REMOTE_PORT);

  while(1) {
    etimer_set(&wait_timer, 30 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&wait_timer));
    LOG_INFO("STATUS node=%u udp=%d tcp=%d\n",
             node_id, udp_echo_received, tcp_echo_received);
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
