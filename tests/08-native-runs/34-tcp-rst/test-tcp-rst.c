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
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uipbuf.h"

#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "TCP reset unit tests");
AUTOSTART_PROCESSES(&run_tests);

#define LOCAL_PORT  8080
/* The peer's ports for connections that it opens to LOCAL_PORT. */
#define PEER_PORT_A 40000
#define PEER_PORT_B 40001
#define PEER_PORT_C 40002
#define PEER_PORT_D 40003
/* The peer's port for connections that the node opens. */
#define REMOTE_PORT 80
#define PEER_ISS    1000

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_ACK 0x10

static uip_ipaddr_t peer_addr;
static uip_ipaddr_t node_addr;

/* Sequence numbers of the connection under test, as the peer sees them. */
static uint32_t peer_seq;
static uint32_t node_seq;
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
static uint32_t
get32(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}
/*---------------------------------------------------------------------------*/
/*
 * Pass a segment from the peer to uIP. Returns true if uIP answered,
 * in which case the answer is in uip_buf.
 */
static bool
input_segment(uint16_t sport, uint16_t dport, uint32_t seq, uint32_t ack,
              uint8_t flags)
{
  memset(uip_buf, 0, UIP_IPTCPH_LEN);
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->proto = UIP_PROTO_TCP;
  UIP_IP_BUF->ttl = 64;
  uipbuf_set_len_field(UIP_IP_BUF, UIP_TCPH_LEN);
  uip_ipaddr_copy(&UIP_IP_BUF->srcipaddr, &peer_addr);
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, &node_addr);

  UIP_TCP_BUF->srcport = UIP_HTONS(sport);
  UIP_TCP_BUF->destport = UIP_HTONS(dport);
  put32(UIP_TCP_BUF->seqno, seq);
  put32(UIP_TCP_BUF->ackno, ack);
  UIP_TCP_BUF->tcpoffset = (UIP_TCPH_LEN / 4) << 4;
  UIP_TCP_BUF->flags = flags;
  UIP_TCP_BUF->wnd[0] = 0x10;
  UIP_TCP_BUF->wnd[1] = 0x00;

  uip_len = UIP_IPTCPH_LEN;
  uip_ext_len = 0;
  UIP_TCP_BUF->tcpchksum = ~uip_tcpchksum();

  uip_input();
  return uip_len > 0;
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
static bool
is_established(uint16_t peer_port)
{
  struct uip_conn *conn = find_conn(peer_port);

  return conn != NULL &&
         (conn->tcpstateflags & UIP_TS_MASK) == UIP_ESTABLISHED;
}
/*---------------------------------------------------------------------------*/
/* Open a connection from the peer to the listening port. */
static bool
handshake(uint16_t peer_port)
{
  if(!input_segment(peer_port, LOCAL_PORT, PEER_ISS, 0, TCP_SYN)) {
    return false;
  }
  if(UIP_TCP_BUF->flags != (TCP_SYN | TCP_ACK) ||
     get32(UIP_TCP_BUF->ackno) != PEER_ISS + 1) {
    return false;
  }
  peer_seq = PEER_ISS + 1;
  node_seq = get32(UIP_TCP_BUF->seqno) + 1;

  input_segment(peer_port, LOCAL_PORT, peer_seq, node_seq, TCP_ACK);
  return is_established(peer_port);
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_rst_out_of_window,
                   "a reset outside the window is ignored");
UNIT_TEST(test_rst_out_of_window)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(handshake(PEER_PORT_A));

  /* The reset from the report: sequence and acknowledgement 0. */
  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_A, LOCAL_PORT, 0, 0, TCP_RST));
  UNIT_TEST_ASSERT(is_established(PEER_PORT_A));

  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_A, LOCAL_PORT,
                                  peer_seq + UIP_RECEIVE_WINDOW, 0,
                                  TCP_RST));
  UNIT_TEST_ASSERT(is_established(PEER_PORT_A));

  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_A, LOCAL_PORT, peer_seq - 1,
                                  node_seq, TCP_RST | TCP_ACK));
  UNIT_TEST_ASSERT(is_established(PEER_PORT_A));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_rst_in_window,
                   "a reset in the window but not at RCV.NXT is challenged");
UNIT_TEST(test_rst_in_window)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(input_segment(PEER_PORT_A, LOCAL_PORT, peer_seq + 10, 0,
                                 TCP_RST));
  UNIT_TEST_ASSERT(UIP_TCP_BUF->flags == TCP_ACK);
  UNIT_TEST_ASSERT(get32(UIP_TCP_BUF->seqno) == node_seq);
  UNIT_TEST_ASSERT(get32(UIP_TCP_BUF->ackno) == peer_seq);
  UNIT_TEST_ASSERT(is_established(PEER_PORT_A));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_rst_exact, "a reset at RCV.NXT closes the connection");
UNIT_TEST(test_rst_exact)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_A, LOCAL_PORT, peer_seq, 0,
                                  TCP_RST));
  UNIT_TEST_ASSERT(find_conn(PEER_PORT_A) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_rst_syn_rcvd,
                   "in SYN-RECEIVED, only a reset at RCV.NXT counts");
UNIT_TEST(test_rst_syn_rcvd)
{
  struct uip_conn *conn;

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(input_segment(PEER_PORT_C, LOCAL_PORT, PEER_ISS, 0,
                                 TCP_SYN));
  conn = find_conn(PEER_PORT_C);
  UNIT_TEST_ASSERT(conn != NULL);
  UNIT_TEST_ASSERT((conn->tcpstateflags & UIP_TS_MASK) == UIP_SYN_RCVD);

  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_C, LOCAL_PORT, 0, 0, TCP_RST));
  UNIT_TEST_ASSERT(find_conn(PEER_PORT_C) == conn);

  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_C, LOCAL_PORT, PEER_ISS + 1, 0,
                                  TCP_RST));
  UNIT_TEST_ASSERT(find_conn(PEER_PORT_C) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_rst_zero_window,
                   "with a zero window, only a reset at RCV.NXT counts");
UNIT_TEST(test_rst_zero_window)
{
  struct uip_conn *conn;

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(handshake(PEER_PORT_D));
  conn = find_conn(PEER_PORT_D);

  /* As uip_stop() does, which makes uIP advertise a zero window. */
  conn->tcpstateflags |= UIP_STOPPED;

  /* In the window that would otherwise be open, but no challenge ACK. */
  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_D, LOCAL_PORT, peer_seq + 10, 0,
                                  TCP_RST));
  UNIT_TEST_ASSERT(find_conn(PEER_PORT_D) == conn);

  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_D, LOCAL_PORT, peer_seq, 0,
                                  TCP_RST));
  UNIT_TEST_ASSERT(find_conn(PEER_PORT_D) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_rst_syn_sent,
                   "in SYN-SENT, only a reset that acknowledges the SYN counts");
UNIT_TEST(test_rst_syn_sent)
{
  struct uip_conn *conn;
  uint32_t iss;

  UNIT_TEST_BEGIN();

  conn = tcp_connect(&peer_addr, UIP_HTONS(REMOTE_PORT), NULL);
  UNIT_TEST_ASSERT(conn != NULL);
  UNIT_TEST_ASSERT((conn->tcpstateflags & UIP_TS_MASK) == UIP_SYN_SENT);
  iss = get32(conn->snd_nxt);

  /* No ACK, and an ACK that does not cover the SYN. */
  UNIT_TEST_ASSERT(!input_segment(REMOTE_PORT, UIP_HTONS(conn->lport), 0, 0,
                                  TCP_RST));
  UNIT_TEST_ASSERT(find_conn(REMOTE_PORT) == conn);
  UNIT_TEST_ASSERT(!input_segment(REMOTE_PORT, UIP_HTONS(conn->lport), 0, iss,
                                  TCP_RST | TCP_ACK));
  UNIT_TEST_ASSERT(find_conn(REMOTE_PORT) == conn);

  UNIT_TEST_ASSERT(!input_segment(REMOTE_PORT, UIP_HTONS(conn->lport), 0,
                                  iss + 1, TCP_RST | TCP_ACK));
  UNIT_TEST_ASSERT(find_conn(REMOTE_PORT) == NULL);

  UNIT_TEST_END();
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
UNIT_TEST_REGISTER(test_challenge_ack_limit,
                   "challenge ACKs are rate-limited");
UNIT_TEST(test_challenge_ack_limit)
{
  unsigned i;
  unsigned acks = 0;

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(handshake(PEER_PORT_B));

  for(i = 0; i < UIP_TCP_CHALLENGE_ACK_LIMIT + 5; i++) {
    if(input_segment(PEER_PORT_B, LOCAL_PORT, peer_seq + 1 + i, 0,
                     TCP_RST)) {
      acks++;
    }
  }
  UNIT_TEST_ASSERT(acks == UIP_TCP_CHALLENGE_ACK_LIMIT);
  UNIT_TEST_ASSERT(is_established(PEER_PORT_B));

  UNIT_TEST_ASSERT(!input_segment(PEER_PORT_B, LOCAL_PORT, peer_seq, 0,
                                  TCP_RST));
  UNIT_TEST_ASSERT(find_conn(PEER_PORT_B) == NULL);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  static struct etimer et;
  uip_ds6_addr_t *lladdr;

  PROCESS_BEGIN();

  printf("Run unit-test\n");

  lladdr = uip_ds6_get_link_local(-1);
  if(lladdr == NULL) {
    printf("=check-me= FAILED: no link-local address\n");
    PROCESS_EXIT();
  }
  uip_ipaddr_copy(&node_addr, &lladdr->ipaddr);
  uip_ip6addr(&peer_addr, 0xfe80, 0, 0, 0, 0, 0, 0, 2);

  tcp_listen(UIP_HTONS(LOCAL_PORT));

  UNIT_TEST_RUN(test_rst_out_of_window);
  UNIT_TEST_RUN(test_rst_in_window);
  UNIT_TEST_RUN(test_rst_exact);
  UNIT_TEST_RUN(test_rst_syn_rcvd);
  UNIT_TEST_RUN(test_rst_zero_window);
  UNIT_TEST_RUN(test_rst_syn_sent);
  UNIT_TEST_RUN(test_timeout_rst_seq);

  /* Start the rate-limit test in a fresh one-second period. */
  etimer_set(&et, CLOCK_SECOND + CLOCK_SECOND / 4);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

  UNIT_TEST_RUN(test_challenge_ack_limit);

  if(!UNIT_TEST_PASSED(test_rst_out_of_window)
     || !UNIT_TEST_PASSED(test_rst_in_window)
     || !UNIT_TEST_PASSED(test_rst_exact)
     || !UNIT_TEST_PASSED(test_rst_syn_rcvd)
     || !UNIT_TEST_PASSED(test_rst_zero_window)
     || !UNIT_TEST_PASSED(test_rst_syn_sent)
     || !UNIT_TEST_PASSED(test_timeout_rst_seq)
     || !UNIT_TEST_PASSED(test_challenge_ack_limit)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
