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
 * \addtogroup nat64
 * @{
 *
 * \file
 *         NAT64 TCP splice proxy.
 *
 *         Implements per-session sequence-number state, RFC 6528
 *         ISN generation, fabrication of IPv6/TCP segments back to
 *         the IoT node, ACK-paced delivery of server data and
 *         half-close handling.  See nat64-tcp.h for the public API
 *         and `os/services/nat64/README.md` for the high-level
 *         design rationale.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "nat64-tcp.h"
#include "nat64.h"
#include "nat64-platform.h"
#include "ipv6/ip64-addr.h"
#include "net/ipv6/tcpip.h"
#include "lib/sha-256.h"

#include <string.h>
#include <sys/time.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "NAT64"
#define LOG_LEVEL LOG_LEVEL_INFO

#define IPV6_HDRLEN 40
#define TCP_HDRLEN  20
#define IP_PROTO_TCP 6
#define DEFAULT_HOPLIM 64

#define TCP_FIN 0x01
#define TCP_SYN 0x02
#define TCP_RST 0x04
#define TCP_PSH 0x08
#define TCP_ACK 0x10


#ifndef NAT64_MAX_TCP_SESSIONS
#define NAT64_MAX_TCP_SESSIONS 16
#endif

/* Maximum TCP payload per injected segment.  Sized to fit a single
 * IEEE 802.15.4 frame after 6LoWPAN IPHC compression + TCP header,
 * avoiding 6LoWPAN fragmentation on constrained links. */
#ifndef NAT64_TCP_SEGMENT_SIZE
#define NAT64_TCP_SEGMENT_SIZE 76
#endif

/* The native socket backend reads up to 1500 bytes at a time.  The
 * splice proxy does not have a second staging buffer, so this buffer
 * must be large enough to retain every byte already consumed from the
 * IPv4 socket. */
#ifndef NAT64_TCP_RXBUF_SIZE
#define NAT64_TCP_RXBUF_SIZE 1500
#endif

#if NAT64_TCP_RXBUF_SIZE < 1500
#error "NAT64_TCP_RXBUF_SIZE must be at least 1500 bytes"
#endif

/* Retransmit timeout for an injected paced segment that has not been
 * ACKed by the IoT node.  Sized for typical 6LoWPAN RTTs (100 ms - 1 s)
 * with margin; the IoT-facing TCP layer does not generate dup-ACKs for
 * never-received data, so we rely on this timer to recover from radio
 * losses. */
#ifndef NAT64_TCP_RTX_TIMEOUT
#define NAT64_TCP_RTX_TIMEOUT (3 * CLOCK_SECOND)
#endif

/* Number of retransmits attempted before declaring the IoT-side TCP
 * peer unreachable and tearing down the session. */
#ifndef NAT64_TCP_MAX_RETRIES
#define NAT64_TCP_MAX_RETRIES 5
#endif

struct v6hdr {
  uint8_t vtc, tcflow;
  uint16_t flow;
  uint8_t plen[2];
  uint8_t nexthdr, hoplim;
  uip_ip6addr_t src, dst;
};

struct tcphdr {
  uint16_t sport, dport;
  uint8_t seqno[4];
  uint8_t ackno[4];
  uint8_t offset;
  uint8_t flags;
  uint8_t wnd[2];
  uint16_t tchksum;
  uint8_t urgp[2];
};

/*---------------------------------------------------------------------------*/
static inline uint16_t
get16(const uint8_t *p)
{
  return ((uint16_t)p[0] << 8) | p[1];
}
/*---------------------------------------------------------------------------*/
static inline void
put16(uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v >> 8);
  p[1] = (uint8_t)v;
}
/*---------------------------------------------------------------------------*/
static inline uint32_t
get32(const uint8_t *p)
{
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
         ((uint32_t)p[2] << 8) | p[3];
}
/*---------------------------------------------------------------------------*/
static inline void
put32(uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v >> 24);
  p[1] = (uint8_t)(v >> 16);
  p[2] = (uint8_t)(v >> 8);
  p[3] = (uint8_t)v;
}
/*---------------------------------------------------------------------------*/
static uint32_t
cksum_acc(uint32_t acc, const void *buf, uint16_t nbytes)
{
  const uint8_t *p = buf;
  while(nbytes > 1) {
    acc += ((uint16_t)p[0] << 8) | p[1];
    p += 2;
    nbytes -= 2;
  }
  if(nbytes == 1) {
    acc += (uint16_t)p[0] << 8;
  }
  return acc;
}
/*---------------------------------------------------------------------------*/
static uint16_t
cksum_fold(uint32_t acc)
{
  while(acc >> 16) {
    acc = (acc & 0xffff) + (acc >> 16);
  }
  return ~((uint16_t)acc);
}
/*---------------------------------------------------------------------------*/
static uint16_t
tcp6_checksum(const struct v6hdr *ip6, const void *tcp, uint16_t tcp_len)
{
  uint32_t acc = 0;

  acc = cksum_acc(acc, &ip6->src, sizeof(uip_ip6addr_t));
  acc = cksum_acc(acc, &ip6->dst, sizeof(uip_ip6addr_t));
  acc += tcp_len;
  acc += IP_PROTO_TCP;
  acc = cksum_acc(acc, tcp, tcp_len);

  uint16_t result = cksum_fold(acc);
  return (result == 0) ? 0xffff : result;
}

/*---------------------------------------------------------------------------*/
/* Per-session TCP sequence number state.
 *
 * The proxy terminates TCP on both sides, so the numbers here describe
 * only the synthetic IoT-facing stream.  our_seq is the next sequence
 * number to send to the IoT node; peer_next is the next sequence number
 * expected from it.  Buffered server data is stop-and-wait: while
 * in_flight is non-zero, rxbuf_offset and our_seq still point at the
 * retransmittable bytes and advance only after the IoT ACK covers them.
 * initial_our_seq is kept separately so a retransmitted SYN can get the
 * same SYN-ACK even after our_seq has advanced past the SYN.
 */
/*---------------------------------------------------------------------------*/

struct tcp_seqstate {
  bool in_use;
  bool pending_ack;
  bool peer_fin_received;
  bool server_fin_pending;
  struct nat64_session *session;
  uint32_t initial_our_seq;
  uint32_t our_seq;
  uint32_t peer_next;
  /* Paced delivery buffer for server-to-IoT data. */
  uint8_t rxbuf[NAT64_TCP_RXBUF_SIZE];
  uint16_t rxbuf_len;
  uint16_t rxbuf_offset;
  /* Retransmit state for the in-flight injected segment.  in_flight
   * is the size of the most recently injected segment that has not
   * yet been ACKed by the IoT node; while non-zero, rxbuf_offset and
   * our_seq are NOT advanced, so a retransmit replays the same bytes
   * with the same sequence number. */
  uint16_t in_flight;
  uint8_t rtx_count;
  struct timer rtx_timer;
};

static struct tcp_seqstate tcp_seq[NAT64_MAX_TCP_SESSIONS];
static uint8_t isn_key[16];

static void nat64_tcp_send_pending(struct tcp_seqstate *ts);
static void nat64_tcp_ack_confirmed(struct tcp_seqstate *ts);
/*---------------------------------------------------------------------------*/
static struct tcp_seqstate *
find_seqstate(const struct nat64_session *s)
{
  unsigned i;
  for(i = 0; i < NAT64_MAX_TCP_SESSIONS; i++) {
    if(tcp_seq[i].in_use && tcp_seq[i].session == s) {
      return &tcp_seq[i];
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
/*
 * Generate an ISN per RFC 6528:
 *   ISN = M + F(localip, localport, remoteip, remoteport, secretkey)
 * where M is a monotonic timer (~4 µs granularity) and F is HMAC-SHA-256.
 *
 * Note: the practical threat from predictable ISNs is low here — the
 * IoT-facing TCP runs over a 6LoWPAN mesh where an attacker would need
 * radio access to inject segments, at which point ISN prediction is
 * the least concern.  We follow RFC 6528 anyway since the cost is
 * negligible (one HMAC per connection) and it is good practice.
 */
static uint32_t
generate_isn(const struct nat64_session *s)
{
  struct {
    uip_ip6addr_t ip6_peer;
    uint16_t ip6_peer_port;
    uip_ip4addr_t ip4_remote;
    uint16_t ip4_remote_port;
  } tuple;
  struct timeval tv;
  uint8_t digest[SHA_256_DIGEST_LENGTH];
  uint32_t f;

  memcpy(&tuple.ip6_peer, &s->ip6_peer, sizeof(uip_ip6addr_t));
  tuple.ip6_peer_port = s->ip6_peer_port;
  memcpy(&tuple.ip4_remote, &s->ip4_remote, sizeof(uip_ip4addr_t));
  tuple.ip4_remote_port = s->ip4_remote_port;

  sha_256_hmac(isn_key, sizeof(isn_key),
               (const uint8_t *)&tuple, sizeof(tuple), digest);
  memcpy(&f, digest, sizeof(f));

  gettimeofday(&tv, NULL);
  uint32_t m = (uint32_t)((uint64_t)tv.tv_sec * 250000 + tv.tv_usec / 4);

  return m + f;
}
/*---------------------------------------------------------------------------*/
static struct tcp_seqstate *
alloc_seqstate(struct nat64_session *s, uint32_t peer_isn)
{
  unsigned i;
  for(i = 0; i < NAT64_MAX_TCP_SESSIONS; i++) {
    if(!tcp_seq[i].in_use) {
      tcp_seq[i].in_use = true;
      tcp_seq[i].pending_ack = false;
      tcp_seq[i].peer_fin_received = false;
      tcp_seq[i].server_fin_pending = false;
      tcp_seq[i].session = s;
      tcp_seq[i].our_seq = generate_isn(s);
      tcp_seq[i].initial_our_seq = tcp_seq[i].our_seq;
      tcp_seq[i].peer_next = peer_isn + 1;
      tcp_seq[i].rxbuf_len = 0;
      tcp_seq[i].rxbuf_offset = 0;
      tcp_seq[i].in_flight = 0;
      tcp_seq[i].rtx_count = 0;
      return &tcp_seq[i];
    }
  }
  LOG_WARN("TCP sequence state table full\n");
  return NULL;
}
/*---------------------------------------------------------------------------*/
static struct tcp_seqstate *
find_seqstate_by_addrs(const uip_ip6addr_t *ip6_peer, uint16_t peer_port,
                       const uip_ip4addr_t *ip4_remote, uint16_t remote_port)
{
  unsigned i;
  for(i = 0; i < NAT64_MAX_TCP_SESSIONS; i++) {
    struct tcp_seqstate *ts = &tcp_seq[i];
    if(!ts->in_use || ts->session == NULL) {
      continue;
    }
    struct nat64_session *s = ts->session;
    if(s->ip6_peer_port == peer_port &&
       s->ip4_remote_port == remote_port &&
       uip_ip6addr_cmp(&s->ip6_peer, ip6_peer) &&
       uip_ip4addr_cmp(&s->ip4_remote, ip4_remote)) {
      return ts;
    }
  }
  return NULL;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief Fabricate and inject an IPv6+TCP segment toward the IoT node.
 * \param s           The NAT64 session this segment belongs to.
 * \param ts          Per-session sequence state (provides seq/ack numbers).
 * \param flags       TCP flag byte (combination of TCP_SYN/ACK/FIN/PSH/RST).
 * \param payload     Optional segment payload, or NULL for header-only.
 * \param payload_len Payload length in bytes, or 0.
 *
 * Builds the IPv6 and TCP headers in `uip_buf`, copies the payload,
 * computes the TCP checksum (with IPv6 pseudo-header) and hands the
 * resulting packet to ::tcpip_input for delivery up the uIP stack.
 * The seqstate's sequence/ack counters are NOT advanced here — the
 * caller is responsible for updating them after the segment is sent.
 */
static void
inject_tcp(const struct nat64_session *s, struct tcp_seqstate *ts,
           uint8_t flags, const uint8_t *payload, uint16_t payload_len)
{
  struct v6hdr *ip6;
  struct tcphdr *tcp;
  uint16_t tcp_total;

  tcp_total = TCP_HDRLEN + payload_len;
  if(IPV6_HDRLEN + tcp_total > UIP_BUFSIZE) {
    LOG_WARN("inject_tcp: packet too large\n");
    return;
  }

  ip6 = (struct v6hdr *)uip_buf;
  ip6->vtc = 0x60;
  ip6->tcflow = 0;
  ip6->flow = 0;
  put16(ip6->plen, tcp_total);
  ip6->nexthdr = IP_PROTO_TCP;
  ip6->hoplim = DEFAULT_HOPLIM;

  ip64_addr_4to6(&s->ip4_remote, &ip6->src);
  uip_ip6addr_copy(&ip6->dst, &s->ip6_peer);

  tcp = (struct tcphdr *)(uip_buf + IPV6_HDRLEN);
  tcp->sport = uip_htons(s->ip4_remote_port);
  tcp->dport = uip_htons(s->ip6_peer_port);
  put32(tcp->seqno, ts->our_seq);
  put32(tcp->ackno, ts->peer_next);
  tcp->offset = (TCP_HDRLEN / 4) << 4;
  tcp->flags = flags;
  put16(tcp->wnd, 4096);
  tcp->tchksum = 0;
  put16(tcp->urgp, 0);

  if(payload_len > 0) {
    memcpy(uip_buf + IPV6_HDRLEN + TCP_HDRLEN, payload, payload_len);
  }

  tcp->tchksum = uip_htons(tcp6_checksum(ip6, tcp, tcp_total));

  uip_len = IPV6_HDRLEN + tcp_total;

  LOG_INFO("inject_tcp: %u bytes, flags=0x%02x seq=%lu ack=%lu\n",
           uip_len, flags, (unsigned long)ts->our_seq,
           (unsigned long)ts->peer_next);
  tcpip_input();
}

/*---------------------------------------------------------------------------*/
/* Process outgoing TCP from the IoT node (received at fallback interface).  */
/*---------------------------------------------------------------------------*/

int
nat64_tcp_output(const uint8_t *pkt, uint16_t len)
{
  const struct v6hdr *ip6 = (const struct v6hdr *)pkt;
  uint16_t payload_len = get16(ip6->plen);
  const struct tcphdr *tcp;
  uint16_t data_offset, data_len;
  uint32_t seq;
  uip_ip4addr_t dst4;

  if(payload_len < TCP_HDRLEN) {
    LOG_WARN("tcp_output: payload too short (%u bytes)\n", payload_len);
    return 0;
  }

  if(!ip64_addr_6to4(&ip6->dst, &dst4)) {
    LOG_WARN("tcp_output: destination is not a NAT64 address\n");
    return 0;
  }

  tcp = (const struct tcphdr *)(pkt + IPV6_HDRLEN);
  data_offset = ((tcp->offset >> 4) & 0x0f) * 4;
  if(data_offset < TCP_HDRLEN || data_offset > payload_len) {
    LOG_WARN("tcp_output: invalid data offset %u for payload %u\n",
             data_offset, payload_len);
    return 0;
  }
  data_len = payload_len - data_offset;
  seq = get32(tcp->seqno);

  LOG_INFO("tcp_output: flags=0x%02x data=%u seq=%lu\n",
           tcp->flags, data_len, (unsigned long)seq);

  if(tcp->flags & TCP_SYN) {
    struct tcp_seqstate *ts = find_seqstate_by_addrs(
      &ip6->src, uip_ntohs(tcp->sport),
      &dst4, uip_ntohs(tcp->dport));
    if(ts != NULL) {
      uint32_t saved_seq = ts->our_seq;

      LOG_INFO("TCP duplicate SYN: retransmitting SYN-ACK\n");
      ts->our_seq = ts->initial_our_seq;
      inject_tcp(ts->session, ts, TCP_SYN | TCP_ACK, NULL, 0);
      ts->our_seq = saved_seq;
      return 1;
    }

    LOG_INFO("TCP SYN: port %u -> %u.%u.%u.%u:%u\n",
             uip_ntohs(tcp->sport),
             dst4.u8[0], dst4.u8[1], dst4.u8[2], dst4.u8[3],
             uip_ntohs(tcp->dport));

    struct nat64_session *s = nat64_platform_tcp_connect(
      &dst4, uip_ntohs(tcp->dport),
      &ip6->src, uip_ntohs(tcp->sport), seq);
    return (s != NULL) ? 1 : 0;
  }

  struct tcp_seqstate *ts = find_seqstate_by_addrs(
    &ip6->src, uip_ntohs(tcp->sport),
    &dst4, uip_ntohs(tcp->dport));

  if(ts == NULL) {
    LOG_WARN("TCP packet for unknown session (flags=0x%02x)\n", tcp->flags);
    return 0;
  }

  struct nat64_session *s = ts->session;

  if(tcp->flags & TCP_RST) {
    LOG_INFO("TCP RST from IoT, aborting session\n");
    /* Full teardown: the IPv4 socket is closed with SO_LINGER=0 so
     * the upstream server sees an equivalent RST instead of a
     * delayed graceful FIN. */
    nat64_platform_tcp_abort(s);
    return 1;
  }

  /* ACK from the IoT node: confirm the in-flight paced segment if the
   * ackno covers its end.  Otherwise the segment was lost in flight;
   * we leave the retransmit timer to recover rather than guessing
   * from dup-ACK heuristics, since uIP-side TCP does not consistently
   * dup-ACK in the way classic TCP stacks do. */
  if(tcp->flags & TCP_ACK) {
    if(ts->in_flight > 0) {
      uint32_t ackno = get32(tcp->ackno);
      uint32_t end_of_inflight = ts->our_seq + ts->in_flight;
      if((int32_t)(ackno - end_of_inflight) >= 0) {
        nat64_tcp_ack_confirmed(ts);
      }
    } else if(ts->rxbuf_len > ts->rxbuf_offset) {
      /* Make progress if the previous sender stopped after clearing
       * in_flight but before queuing the next buffered segment. */
      nat64_tcp_send_pending(ts);
    }
  }

  if(data_len > 0) {
    const uint8_t *data = pkt + IPV6_HDRLEN + data_offset;
    uint32_t seq_end = seq + (uint32_t)data_len;
    int32_t gap = (int32_t)(seq - ts->peer_next);

    if(gap > 0) {
      /* The IoT node skipped ahead in the sequence space — we never
       * saw the bytes between peer_next and seq.  Drop the segment
       * (including any FIN) so it retransmits from peer_next. */
      LOG_WARN("TCP out-of-order seq=%lu peer_next=%lu, dropping\n",
               (unsigned long)seq, (unsigned long)ts->peer_next);
      ts->pending_ack = true;
      return 1;
    }

    if((int32_t)(seq_end - ts->peer_next) <= 0) {
      /* Pure retransmit: every byte was already forwarded to the IPv4
       * server.  Re-ACK so the IoT node stops resending, but do not
       * forward the duplicate payload — that would corrupt the
       * server-side stream. */
      LOG_DBG("TCP retransmit seq=%lu len=%u (already forwarded)\n",
              (unsigned long)seq, data_len);
      ts->pending_ack = true;
    } else {
      /* Partial overlap: skip the prefix that was already forwarded
       * and send only the new tail. */
      uint32_t skip = ts->peer_next - seq;
      const uint8_t *new_data = data + skip;
      uint16_t new_len = data_len - (uint16_t)skip;

      LOG_INFO("TCP forwarding %u bytes to IPv4 server%s\n",
               new_len, skip > 0 ? " (skipped retransmitted prefix)" : "");
      int sent = nat64_platform_tcp_send(s, new_data, new_len);
      if(sent < 0) {
        LOG_ERR("TCP send failed, aborting session\n");
        nat64_platform_tcp_abort(s);
        return 1;
      }
      ts->peer_next += (uint32_t)sent;
      ts->pending_ack = true;
      if((uint32_t)sent < new_len) {
        /* Short write — only ACK what was forwarded.  Don't process
         * FIN yet; the IoT node will retransmit the remaining data. */
        return 1;
      }
    }
  }

  if(tcp->flags & TCP_FIN) {
    if(!ts->peer_fin_received) {
      LOG_INFO("TCP FIN from IoT node (half-close)\n");
      ts->peer_next++;
      ts->peer_fin_received = true;
      /* Forward the half-close to the IPv4 server (SHUT_WR), but
       * keep the read side open: server->IoT data can still arrive
       * and must be delivered.  Our own FIN toward the IoT node is
       * deferred until nat64_tcp_closed() fires when the IPv4 server
       * eventually closes its end. */
      nat64_platform_tcp_close(s);
      ts->pending_ack = true;

      if(s->tcp_state == NAT64_TCP_CLOSING) {
        /* Server already closed and we already injected our FIN;
         * receiving the IoT-side FIN means both halves are done.
         * Tear down the session now rather than waiting for the
         * idle timer. */
        LOG_INFO("TCP both sides FIN'd, destroying session\n");
        nat64_platform_tcp_destroy(s);
        return 1;
      }
    } else {
      LOG_DBG("TCP duplicate FIN from IoT node (already half-closed)\n");
    }
  }

  return 1;
}

/*---------------------------------------------------------------------------*/
/**
 * \brief Inject the next paced chunk from a session's receive buffer.
 * \param ts The sequence state whose rxbuf has data to deliver.
 *
 * Sends up to ::NAT64_TCP_SEGMENT_SIZE bytes per call, fitting one
 * 802.15.4 frame after 6LoWPAN compression.  Stop-and-wait: only one
 * segment is in flight at a time, with the retransmit timer armed for
 * recovery if the IoT node never ACKs (e.g., radio loss).  The
 * sequence number and rxbuf offset are NOT advanced here — that
 * happens in ::nat64_tcp_ack_confirmed once the ACK arrives.
 */
static void
nat64_tcp_send_pending(struct tcp_seqstate *ts)
{
  uint16_t remaining;
  uint16_t chunk;

  if(ts->in_flight > 0) {
    /* A previous segment is still awaiting ACK or retransmit. */
    return;
  }

  remaining = ts->rxbuf_len - ts->rxbuf_offset;
  if(remaining == 0) {
    return;
  }

  chunk = remaining > NAT64_TCP_SEGMENT_SIZE
          ? NAT64_TCP_SEGMENT_SIZE : remaining;

  LOG_INFO("TCP paced: %u/%u bytes -> IoT node\n", chunk, remaining);
  inject_tcp(ts->session, ts, TCP_PSH | TCP_ACK,
             ts->rxbuf + ts->rxbuf_offset, chunk);
  ts->in_flight = chunk;
  ts->rtx_count = 0;
  timer_set(&ts->rtx_timer, NAT64_TCP_RTX_TIMEOUT);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Promote the in-flight segment to acknowledged and queue what's next.
 * \param ts The sequence state whose latest segment has been ACKed.
 *
 * Advances rxbuf_offset and our_seq past the now-acknowledged bytes,
 * clears the retransmit state, and either sends the next chunk, emits
 * a previously-deferred server FIN, or leaves the session idle.
 */
static void
nat64_tcp_ack_confirmed(struct tcp_seqstate *ts)
{
  ts->our_seq += ts->in_flight;
  ts->rxbuf_offset += ts->in_flight;
  ts->in_flight = 0;
  ts->rtx_count = 0;

  if(ts->rxbuf_offset >= ts->rxbuf_len) {
    ts->rxbuf_len = 0;
    ts->rxbuf_offset = 0;
    if(ts->server_fin_pending) {
      ts->server_fin_pending = false;
      LOG_INFO("TCP deferred FIN: sending now\n");
      inject_tcp(ts->session, ts, TCP_FIN | TCP_ACK, NULL, 0);
      ts->our_seq++;
    }
    return;
  }

  nat64_tcp_send_pending(ts);
}

/*---------------------------------------------------------------------------*/
/* Flush deferred ACKs and send paced data.  Called from the select loop.    */
/*---------------------------------------------------------------------------*/

void
nat64_tcp_flush_acks(void)
{
  unsigned i;
  for(i = 0; i < NAT64_MAX_TCP_SESSIONS; i++) {
    struct tcp_seqstate *ts = &tcp_seq[i];
    if(!ts->in_use || ts->session == NULL) {
      continue;
    }

    /* Retransmit a paced segment that the IoT node never ACKed.  The
     * IoT-facing TCP layer does not dup-ACK for never-received data,
     * so we recover from radio losses purely on this timer. */
    if(ts->in_flight > 0 && timer_expired(&ts->rtx_timer)) {
      if(++ts->rtx_count > NAT64_TCP_MAX_RETRIES) {
        LOG_ERR("TCP retransmit limit reached, aborting session\n");
        nat64_platform_tcp_abort(ts->session);
        continue;
      }
      LOG_WARN("TCP retransmit %u/%u (%u bytes)\n",
               ts->rtx_count, NAT64_TCP_MAX_RETRIES, ts->in_flight);
      inject_tcp(ts->session, ts, TCP_PSH | TCP_ACK,
                 ts->rxbuf + ts->rxbuf_offset, ts->in_flight);
      timer_reset(&ts->rtx_timer);
    }

    if(ts->pending_ack) {
      ts->pending_ack = false;
      /* Pure ACK: never bundle our own FIN here, even after a peer
       * FIN.  Our FIN toward the IoT node is emitted by
       * nat64_tcp_closed() when the IPv4 server closes its end.
       * Bundling FIN with this ACK would break TCP half-close
       * semantics by actively closing the IoT-facing side as part
       * of ACK processing. */
      inject_tcp(ts->session, ts, TCP_ACK, NULL, 0);
    }
  }
}

/*---------------------------------------------------------------------------*/
/* Callbacks from the platform layer.                                        */
/*---------------------------------------------------------------------------*/

void
nat64_tcp_established(struct nat64_session *s)
{
  struct tcp_seqstate *ts = alloc_seqstate(s, s->peer_isn);
  if(ts == NULL) {
    LOG_ERR("TCP seqstate table full, aborting connection\n");
    nat64_platform_tcp_abort(s);
    return;
  }

  LOG_INFO("TCP established: sending SYN-ACK\n");
  inject_tcp(s, ts, TCP_SYN | TCP_ACK, NULL, 0);
  ts->our_seq++;
}
/*---------------------------------------------------------------------------*/
void
nat64_tcp_data_in(struct nat64_session *s,
                  const uint8_t *data, uint16_t len)
{
  struct tcp_seqstate *ts = find_seqstate(s);
  if(ts == NULL) {
    LOG_WARN("tcp_data_in: no sequence state\n");
    return;
  }

  if(ts->rxbuf_len > 0) {
    LOG_WARN("tcp_data_in: buffer busy, dropping %u bytes\n", len);
    return;
  }

  if(len > NAT64_TCP_RXBUF_SIZE) {
    len = NAT64_TCP_RXBUF_SIZE;
  }

  memcpy(ts->rxbuf, data, len);
  ts->rxbuf_len = len;
  ts->rxbuf_offset = 0;

  /* ACK pacing starts immediately; later chunks are sent only after
   * the IoT node confirms the previous one. */
  nat64_tcp_send_pending(ts);
}
/*---------------------------------------------------------------------------*/
void
nat64_tcp_closed(struct nat64_session *s)
{
  struct tcp_seqstate *ts = find_seqstate(s);
  if(ts == NULL) {
    return;
  }

  if(ts->rxbuf_len > ts->rxbuf_offset) {
    /* Data still buffered — defer FIN until the buffer drains. */
    LOG_INFO("TCP remote closed: deferring FIN (%u bytes pending)\n",
             ts->rxbuf_len - ts->rxbuf_offset);
    ts->server_fin_pending = true;
    return;
  }

  LOG_INFO("TCP remote closed: sending FIN to IoT node\n");
  inject_tcp(s, ts, TCP_FIN | TCP_ACK, NULL, 0);
  ts->our_seq++;
  /* Keep ts->in_use = true so we can handle the FIN-ACK from the IoT
   * node.  The seqstate is freed when we see the peer's FIN-ACK or RST
   * in nat64_tcp_output(), or by nat64_tcp_free_seqstate() when the
   * platform layer closes the session. */
}
/*---------------------------------------------------------------------------*/
void
nat64_tcp_init(void)
{
  memset(tcp_seq, 0, sizeof(tcp_seq));
}
/*---------------------------------------------------------------------------*/
void
nat64_tcp_set_isn_secret(const uint8_t key[16])
{
  memcpy(isn_key, key, 16);
}
/*---------------------------------------------------------------------------*/
bool
nat64_tcp_has_pending_data(const struct nat64_session *s)
{
  struct tcp_seqstate *ts = find_seqstate(s);
  return ts != NULL && ts->rxbuf_len > ts->rxbuf_offset;
}
/*---------------------------------------------------------------------------*/
bool
nat64_tcp_peer_fin_received(const struct nat64_session *s)
{
  struct tcp_seqstate *ts = find_seqstate(s);
  return ts != NULL && ts->peer_fin_received;
}
/*---------------------------------------------------------------------------*/
void
nat64_tcp_free_seqstate(const struct nat64_session *s)
{
  struct tcp_seqstate *ts = find_seqstate(s);
  if(ts != NULL) {
    ts->rxbuf_len = 0;
    ts->rxbuf_offset = 0;
    ts->in_flight = 0;
    ts->rtx_count = 0;
    ts->in_use = false;
    ts->session = NULL;
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
