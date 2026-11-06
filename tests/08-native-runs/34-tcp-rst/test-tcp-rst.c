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
 *         Unit tests for the TCP resets that uIP sends and receives.
 */

#include "contiki.h"
#include "unit-test.h"
#include "net/ipv6/uip.h"

#include <stdio.h>

PROCESS(run_tests, "TCP reset unit tests");
AUTOSTART_PROCESSES(&run_tests);

/* The peer's port for connections that the node opens. */
#define REMOTE_PORT 80

#define TCP_RST 0x04
#define TCP_ACK 0x10

static uip_ipaddr_t peer_addr;
/*---------------------------------------------------------------------------*/
static uint32_t
get32(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}
/*---------------------------------------------------------------------------*/
static struct uip_conn *
find_conn(uint16_t peer_port)
{
  struct uip_conn *conn;

  for(conn = &uip_conns[0]; conn < &uip_conns[UIP_TCP_CONNS]; conn++) {
    if((conn->tcpstateflags & UIP_TS_MASK) != UIP_CLOSED &&
       conn->rport == UIP_HTONS(peer_port)) {
      return conn;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_timeout_rst_seq,
                   "a reset after a timeout carries SND.NXT");
UNIT_TEST(test_timeout_rst_seq)
{
  struct uip_conn *conn;
  uint32_t iss;

  UNIT_TEST_BEGIN();

  conn = tcp_connect(&peer_addr, UIP_HTONS(REMOTE_PORT), NULL);
  UNIT_TEST_ASSERT(conn != NULL);
  iss = get32(conn->snd_nxt);

  /* Let the last SYN retransmission time out. */
  conn->nrtx = UIP_MAXSYNRTX;
  conn->timer = 0;
  uip_periodic_conn(conn);

  UNIT_TEST_ASSERT(uip_len > 0);
  UNIT_TEST_ASSERT(UIP_TCP_BUF->flags == (TCP_RST | TCP_ACK));
  /* The SYN takes one sequence number. */
  UNIT_TEST_ASSERT(get32(UIP_TCP_BUF->seqno) == iss + 1);
  UNIT_TEST_ASSERT(find_conn(REMOTE_PORT) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");

  uip_ip6addr(&peer_addr, 0xfe80, 0, 0, 0, 0, 0, 0, 2);

  UNIT_TEST_RUN(test_timeout_rst_seq);

  if(!UNIT_TEST_PASSED(test_timeout_rst_seq)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
