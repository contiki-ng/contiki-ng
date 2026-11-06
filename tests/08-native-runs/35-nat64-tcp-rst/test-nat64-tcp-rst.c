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
 *         Unit tests for the handling of RSTs from the IoT node in the
 *         NAT64 TCP proxy. The platform layer is stubbed out.
 */

#include "contiki.h"
#include "unit-test.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/ip64-addr.h"
#include "nat64.h"
#include "nat64-platform.h"
#include "nat64-tcp.h"

#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "NAT64 TCP reset unit tests");
AUTOSTART_PROCESSES(&run_tests);

#define IOT_PORT    1025
#define SERVER_PORT 80
#define IOT_ISS     5000

#define TCP_RST 0x04

static struct nat64_session session;
static uip_ip6addr_t server_ip6;
static unsigned aborts;

static uint8_t pkt[UIP_IPTCPH_LEN];
/*---------------------------------------------------------------------------*/
/* Platform layer stubs. */
bool
nat64_platform_init(void)
{
  return true;
}
bool
nat64_is_enabled(void)
{
  return true;
}
int
nat64_platform_udp_send(const uip_ip4addr_t *dst, uint16_t dstport,
                        const uip_ip6addr_t *ip6_src, uint16_t srcport,
                        const uint8_t *payload, uint16_t len)
{
  return -1;
}
struct nat64_session *
nat64_platform_tcp_connect(const uip_ip4addr_t *dst, uint16_t dstport,
                           const uip_ip6addr_t *ip6_src, uint16_t srcport,
                           uint32_t peer_isn)
{
  return NULL;
}
int
nat64_platform_tcp_send(struct nat64_session *s, const uint8_t *data,
                        uint16_t len)
{
  return len;
}
void
nat64_platform_tcp_close(struct nat64_session *s)
{
}
void
nat64_platform_tcp_destroy(struct nat64_session *s)
{
}
void
nat64_platform_tcp_abort(struct nat64_session *s)
{
  aborts++;
}
int
nat64_platform_icmp_send(const uip_ip4addr_t *dst,
                         const uip_ip6addr_t *ip6_src, uint16_t identifier,
                         const uint8_t *icmp_pkt, uint16_t icmp_len)
{
  return -1;
}
/*---------------------------------------------------------------------------*/
static void
put32(uint8_t *p, uint32_t v)
{
  p[0] = v >> 24;
  p[1] = v >> 16;
  p[2] = v >> 8;
  p[3] = v;
}
/*---------------------------------------------------------------------------*/
/* Pass an RST from the IoT node to the proxy. */
static void
output_rst(uint32_t seq)
{
  struct uip_ip_hdr *ip = (struct uip_ip_hdr *)pkt;
  struct uip_tcp_hdr *tcp = (struct uip_tcp_hdr *)&pkt[UIP_IPH_LEN];

  memset(pkt, 0, sizeof(pkt));
  ip->vtc = 0x60;
  ip->proto = UIP_PROTO_TCP;
  ip->ttl = 64;
  ip->len[1] = UIP_TCPH_LEN;
  uip_ipaddr_copy(&ip->srcipaddr, &session.ip6_peer);
  uip_ipaddr_copy(&ip->destipaddr, &server_ip6);

  tcp->srcport = UIP_HTONS(IOT_PORT);
  tcp->destport = UIP_HTONS(SERVER_PORT);
  put32(tcp->seqno, seq);
  tcp->tcpoffset = (UIP_TCPH_LEN / 4) << 4;
  tcp->flags = TCP_RST;

  nat64_tcp_output(pkt, sizeof(pkt));
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_rst_not_next,
                   "an RST that is not at the next sequence number is ignored");
UNIT_TEST(test_rst_not_next)
{
  UNIT_TEST_BEGIN();

  /* The reset from the report against uIP: sequence number 0. */
  output_rst(0);
  UNIT_TEST_ASSERT(aborts == 0);

  /* In the window, which only gets a challenge ACK. */
  output_rst(IOT_ISS + 1 + 100);
  UNIT_TEST_ASSERT(aborts == 0);

  /* Just below the expected number. */
  output_rst(IOT_ISS);
  UNIT_TEST_ASSERT(aborts == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_rst_next, "an RST at the next sequence number aborts");
UNIT_TEST(test_rst_next)
{
  UNIT_TEST_BEGIN();

  output_rst(IOT_ISS + 1);
  UNIT_TEST_ASSERT(aborts == 1);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  uip_ip4addr_t server_ip4;

  PROCESS_BEGIN();

  printf("Run unit-test\n");

  nat64_tcp_init();

  uip_ipaddr(&server_ip4, 192, 0, 2, 1);
  ip64_addr_4to6(&server_ip4, &server_ip6);

  session.active = true;
  session.proto = NAT64_PROTO_TCP;
  session.fd = -1;
  uip_ip6addr(&session.ip6_peer, 0xfd00, 0, 0, 0, 0, 0, 0, 2);
  session.ip6_peer_port = IOT_PORT;
  uip_ipaddr_copy(&session.ip4_remote, &server_ip4);
  session.ip4_remote_port = SERVER_PORT;
  session.peer_isn = IOT_ISS;
  session.tcp_state = NAT64_TCP_ESTABLISHED;
  nat64_tcp_established(&session);

  UNIT_TEST_RUN(test_rst_not_next);
  UNIT_TEST_RUN(test_rst_next);

  if(!UNIT_TEST_PASSED(test_rst_not_next)
     || !UNIT_TEST_PASSED(test_rst_next)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
