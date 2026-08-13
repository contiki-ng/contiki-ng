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
 *         Probe for the standalone native NAT64 translator
 *         (os/services/nat64/native/standalone).
 *
 *         A plain native uIP application (no RVM, no RPL, no border router),
 *         built with the standalone module so the node translates its own
 *         NAT64 traffic over host sockets. It sends a UDP datagram to an IPv4
 *         address written in the well-known NAT64 prefix (64:ff9b::/96); the
 *         translator forwards it to a host IPv4 socket where a loopback echo
 *         server reflects it. When the reply returns (translated back to
 *         IPv6), the probe logs NAT64_ECHO_OK.
 *
 *         The destination is 127.0.0.1, which NAT64 rejects by default, so the
 *         test build sets NAT64_CONF_ALLOW_LOOPBACK=1 (project-conf.h) to keep
 *         the exchange on the loopback interface.
 */

#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/ip64-addr.h"

#include <stdio.h>
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "NAT64Test"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Loopback echo server port; matches test-nat64-standalone.sh. */
#ifndef NAT64_TEST_PORT
#define NAT64_TEST_PORT 5557
#endif

#define PROBE_INTERVAL (CLOCK_SECOND)
#define PROBE_MAX 15

static const char payload[] = "PING-NAT64";
#define PAYLOAD_LEN (sizeof(payload) - 1)

static struct simple_udp_connection udp_conn;
static bool echo_received;
/*---------------------------------------------------------------------------*/
PROCESS(nat64_standalone_test_process, "standalone NAT64 probe");
AUTOSTART_PROCESSES(&nat64_standalone_test_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr, uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
                const uint8_t *data, uint16_t datalen)
{
  if(datalen == PAYLOAD_LEN && memcmp(data, payload, PAYLOAD_LEN) == 0) {
    if(!echo_received) {
      echo_received = true;
      LOG_INFO("NAT64_ECHO_OK\n");
    }
  } else {
    LOG_INFO("NAT64_RX_UNEXPECTED bytes=%u\n", datalen);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nat64_standalone_test_process, ev, data)
{
  static struct etimer probe_timer;
  static uip_ipaddr_t target;
  static unsigned probes;

  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, NAT64_TEST_PORT, NULL, NAT64_TEST_PORT,
                      udp_rx_callback);

  /* 64:ff9b::7f00:1 maps 127.0.0.1 via the well-known NAT64 prefix. */
  uip_nat64addr(&target, 127, 0, 0, 1);

  /* Let the interface settle before the first probe. */
  etimer_set(&probe_timer, PROBE_INTERVAL);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&probe_timer));

  for(probes = 0; probes < PROBE_MAX && !echo_received; probes++) {
    LOG_INFO("NAT64_PROBE_SEND probe=%u\n", probes);
    simple_udp_sendto(&udp_conn, payload, PAYLOAD_LEN, &target);
    etimer_set(&probe_timer, PROBE_INTERVAL);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&probe_timer));
  }

  if(echo_received) {
    LOG_INFO("NAT64_TEST_DONE result=ok\n");
  } else {
    LOG_INFO("NAT64_TEST_DONE result=timeout\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
