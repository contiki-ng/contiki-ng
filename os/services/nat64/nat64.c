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
 *         NAT64 core — dispatches IPv6 packets to protocol-specific
 *         handlers (UDP via kernel sockets, TCP via splice proxy,
 *         ICMPv6 Echo via unprivileged ICMP sockets) and synthesizes
 *         ICMPv6 Destination Unreachable errors back to the IoT node
 *         when forwarding fails.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "nat64.h"
#include "nat64-platform.h"
#include "nat64-dns64.h"
#include "nat64-tcp.h"
#include "ipv6/ip64-addr.h"
#include "net/ipv6/tcpip.h"

#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "NAT64"
#define LOG_LEVEL LOG_LEVEL_INFO

#define IPV6_HDRLEN     40
#define IP_PROTO_TCP    6
#define IP_PROTO_UDP    17
#define IP_PROTO_ICMPV6 58
#define DNS_PORT        53
#define DEFAULT_HOPLIM  64

#define ICMP6_DST_UNREACH 1
#define ICMP6_ECHO_REQUEST 128
#define ICMP6_ECHO_REPLY   129

#define ICMP4_ECHO_REQUEST 8
#define ICMP4_ECHO_REPLY   0

#define ICMP6_HDRLEN 8

#ifndef NAT64_ICMP6_QUEUE_SIZE
#define NAT64_ICMP6_QUEUE_SIZE 8
#endif

/* Maximum DNS query payload that can be rewritten in place (AAAA->A
 * substitution).  Sized to cover EDNS0 messages up to typical 6LoWPAN
 * MTUs; larger queries are forwarded without translation, which means
 * the upstream resolver answers an AAAA query as AAAA and the IoT
 * node sees no A->AAAA synthesis -- harmless for the data path but a
 * miss for DNS64.  Override at compile time if a deployment uses a
 * larger EDNS payload size. */
#ifndef NAT64_DNS_BUF_SIZE
#define NAT64_DNS_BUF_SIZE 1500
#endif

/* Test-only override: when set, 127.0.0.0/8 is treated as a permitted
 * NAT64 destination so the gateway can be exercised against an echo
 * server bound to the BR's loopback interface.  Never enable this in
 * production; doing so lets the IoT mesh reach the BR host's local
 * services. */
#ifndef NAT64_CONF_ALLOW_LOOPBACK
#define NAT64_CONF_ALLOW_LOOPBACK 0
#endif

/* Each queued ICMPv6 message holds an IPv6 header (40) + ICMPv6 header
 * (8) + as much of the invoking packet as fits.  256 bytes is plenty
 * for IPv6+TCP (60) or IPv6+UDP (48) headers and a small slack. */
#define NAT64_ICMP6_BUF_SIZE 256

struct v6hdr {
  uint8_t vtc, tcflow;
  uint16_t flow;
  uint8_t plen[2];
  uint8_t nexthdr, hoplim;
  uip_ip6addr_t src, dst;
};

struct udphdr {
  uint16_t sport, dport, ulen, uchksum;
};

struct icmp6_dst_unreach {
  uint8_t type;
  uint8_t code;
  uint16_t checksum;
  uint32_t unused;
};

struct icmp6_pending {
  uint16_t len;
  uint8_t buf[NAT64_ICMP6_BUF_SIZE];
};

static struct icmp6_pending icmp6_queue[NAT64_ICMP6_QUEUE_SIZE];
static unsigned icmp6_queue_count;

static bool active;
/*---------------------------------------------------------------------------*/
/**
 * \brief Reject IPv4 destinations that must not be reached via NAT64.
 * \param addr Candidate IPv4 destination address.
 * \return true if the destination falls in a special-use range and
 *         must be dropped, false if it is a globally routable address.
 *
 * Covers loopback, private (RFC 1918), CGNAT, link-local, multicast,
 * documentation, benchmarking and other special-use ranges per
 * RFC 5735 / RFC 6890.  Forwarding such addresses across the NAT64
 * gateway would either leak link-local traffic onto the upstream
 * IPv4 network or expose the gateway operator's private IPv4 LAN to
 * the IoT mesh, so the gateway responds with an ICMPv6 Destination
 * Unreachable (Communication Administratively Prohibited) instead.
 */
static bool
ipv4_dst_is_forbidden(const uip_ip4addr_t *addr)
{
  uint8_t a = addr->u8[0];
  uint8_t b = addr->u8[1];

  /* 0.0.0.0/8 — "this" network */
  if(a == 0) {
    return true;
  }
  /* 10.0.0.0/8 — private */
  if(a == 10) {
    return true;
  }
  /* 100.64.0.0/10 — shared address space (RFC 6598) */
  if(a == 100 && (b & 0xc0) == 64) {
    return true;
  }
  /* 127.0.0.0/8 — loopback */
  if(a == 127) {
    return !NAT64_CONF_ALLOW_LOOPBACK;
  }
  /* 169.254.0.0/16 — link-local */
  if(a == 169 && b == 254) {
    return true;
  }
  /* 172.16.0.0/12 — private */
  if(a == 172 && (b & 0xf0) == 16) {
    return true;
  }
  /* 192.0.0.0/24 — IETF protocol assignments */
  if(a == 192 && b == 0 && addr->u8[2] == 0) {
    return true;
  }
  /* 192.0.2.0/24 — TEST-NET-1 (documentation) */
  if(a == 192 && b == 0 && addr->u8[2] == 2) {
    return true;
  }
  /* 192.168.0.0/16 — private */
  if(a == 192 && b == 168) {
    return true;
  }
  /* 198.18.0.0/15 — benchmarking */
  if(a == 198 && (b & 0xfe) == 18) {
    return true;
  }
  /* 198.51.100.0/24 — TEST-NET-2 */
  if(a == 198 && b == 51 && addr->u8[2] == 100) {
    return true;
  }
  /* 203.0.113.0/24 — TEST-NET-3 */
  if(a == 203 && b == 0 && addr->u8[2] == 113) {
    return true;
  }
  /* 224.0.0.0/4 — multicast */
  if((a & 0xf0) == 224) {
    return true;
  }
  /* 240.0.0.0/4 — reserved (includes 255.255.255.255) */
  if((a & 0xf0) == 240) {
    return true;
  }
  return false;
}
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
udp6_checksum(const struct v6hdr *ip6, const struct udphdr *udp,
              const uint8_t *payload, uint16_t payload_len)
{
  uint16_t udp_total = sizeof(struct udphdr) + payload_len;
  uint32_t acc = 0;

  acc = cksum_acc(acc, &ip6->src, sizeof(uip_ip6addr_t));
  acc = cksum_acc(acc, &ip6->dst, sizeof(uip_ip6addr_t));
  acc += udp_total;
  acc += IP_PROTO_UDP;
  acc = cksum_acc(acc, udp, sizeof(struct udphdr));
  acc = cksum_acc(acc, payload, payload_len);

  uint16_t result = cksum_fold(acc);
  return (result == 0) ? 0xffff : result;
}
/*---------------------------------------------------------------------------*/
static uint16_t
icmp6_checksum(const struct v6hdr *ip6, const void *icmp, uint16_t icmp_len)
{
  uint32_t acc = 0;

  acc = cksum_acc(acc, &ip6->src, sizeof(uip_ip6addr_t));
  acc = cksum_acc(acc, &ip6->dst, sizeof(uip_ip6addr_t));
  acc += icmp_len;
  acc += IP_PROTO_ICMPV6;
  acc = cksum_acc(acc, icmp, icmp_len);

  uint16_t result = cksum_fold(acc);
  return (result == 0) ? 0xffff : result;
}
/*---------------------------------------------------------------------------*/
void
nat64_queue_icmp6_unreach(const uint8_t *invoking_pkt, uint16_t invoking_len,
                          uint8_t code)
{
  const struct v6hdr *orig;
  struct icmp6_pending *slot;
  struct v6hdr *ip6;
  struct icmp6_dst_unreach *icmp;
  uint16_t embed_len, icmp_total, total;

  if(invoking_len < IPV6_HDRLEN) {
    return;
  }

  orig = (const struct v6hdr *)invoking_pkt;

  /* RFC 4443 §2.4 (e): do not generate ICMPv6 errors in response to
   * ICMPv6 errors. */
  if(orig->nexthdr == IP_PROTO_ICMPV6) {
    return;
  }

  if(icmp6_queue_count >= NAT64_ICMP6_QUEUE_SIZE) {
    LOG_WARN("icmp6: queue full, dropping Dest Unreach code=%u\n", code);
    return;
  }

  /* Embed as much of the invoking packet as fits, capped by the per-slot
   * buffer.  RFC 4443 §3.1 caps the entire ICMPv6 message at the IPv6
   * minimum MTU (1280); our 256-byte slot is well below that. */
  embed_len = invoking_len;
  if(embed_len > NAT64_ICMP6_BUF_SIZE - IPV6_HDRLEN - sizeof(*icmp)) {
    embed_len = NAT64_ICMP6_BUF_SIZE - IPV6_HDRLEN - sizeof(*icmp);
  }

  icmp_total = sizeof(*icmp) + embed_len;
  total = IPV6_HDRLEN + icmp_total;

  slot = &icmp6_queue[icmp6_queue_count];

  ip6 = (struct v6hdr *)slot->buf;
  ip6->vtc = 0x60;
  ip6->tcflow = 0;
  ip6->flow = 0;
  put16(ip6->plen, icmp_total);
  ip6->nexthdr = IP_PROTO_ICMPV6;
  ip6->hoplim = DEFAULT_HOPLIM;
  /* The error appears to come from the destination the IoT node tried
   * to reach, so its socket layer can match the embedded headers. */
  uip_ip6addr_copy(&ip6->src, &orig->dst);
  uip_ip6addr_copy(&ip6->dst, &orig->src);

  icmp = (struct icmp6_dst_unreach *)(slot->buf + IPV6_HDRLEN);
  icmp->type = ICMP6_DST_UNREACH;
  icmp->code = code;
  icmp->checksum = 0;
  icmp->unused = 0;

  memcpy(slot->buf + IPV6_HDRLEN + sizeof(*icmp), invoking_pkt, embed_len);

  icmp->checksum = uip_htons(icmp6_checksum(ip6, icmp, icmp_total));

  slot->len = total;
  icmp6_queue_count++;

  LOG_DBG("icmp6: queued Dest Unreach code=%u (%u bytes)\n", code, total);
}
/*---------------------------------------------------------------------------*/
void
nat64_queue_icmp6_unreach_tuple(const uip_ip6addr_t *ip6_src, uint16_t src_port,
                                const uip_ip4addr_t *ip4_dst, uint16_t dst_port,
                                uint8_t ipproto, uint8_t code)
{
  uint8_t fake[IPV6_HDRLEN + 8];
  struct v6hdr *ip6 = (struct v6hdr *)fake;
  uint8_t *th = fake + IPV6_HDRLEN;

  ip6->vtc = 0x60;
  ip6->tcflow = 0;
  ip6->flow = 0;
  put16(ip6->plen, 8);
  ip6->nexthdr = ipproto;
  ip6->hoplim = DEFAULT_HOPLIM;
  uip_ip6addr_copy(&ip6->src, ip6_src);
  ip64_addr_4to6(ip4_dst, &ip6->dst);

  /* First 8 bytes of the transport header: source port, destination
   * port, and four zeroed bytes (TCP sequence number or UDP length +
   * checksum).  Enough for the IoT node's socket layer to match. */
  put16(th, src_port);
  put16(th + 2, dst_port);
  th[4] = th[5] = th[6] = th[7] = 0;

  nat64_queue_icmp6_unreach(fake, sizeof(fake), code);
}
/*---------------------------------------------------------------------------*/
void
nat64_flush_icmp6(void)
{
  unsigned i;

  for(i = 0; i < icmp6_queue_count; i++) {
    struct icmp6_pending *slot = &icmp6_queue[i];
    if(slot->len == 0 || slot->len > UIP_BUFSIZE) {
      continue;
    }
    memcpy(uip_buf, slot->buf, slot->len);
    uip_len = slot->len;
    LOG_DBG("icmp6: injecting Dest Unreach (%u bytes)\n", uip_len);
    tcpip_input();
  }
  icmp6_queue_count = 0;
}
/*---------------------------------------------------------------------------*/
bool
nat64_is_ip64_addr(const uip_ip6addr_t *addr)
{
  return active && ip64_addr_is_ip64(addr);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Forward an outbound IPv6/UDP datagram to its IPv4 destination.
 * \param pkt         Original IPv6 packet (header + payload).
 * \param ip6         Parsed IPv6 header (alias for pkt).
 * \param dst4        Pre-extracted IPv4 destination address.
 * \param payload_len Length of the IPv6 payload (UDP header + data).
 * \return 1 if the packet was forwarded, 0 on a parse or send error.
 *
 * If the destination port is 53, a private copy of the payload is
 * passed through ::nat64_dns64_6to4 so the upstream resolver receives
 * an A query rather than the original AAAA query.  All other UDP
 * traffic is forwarded verbatim through the platform layer.
 */
static int
handle_udp_output(const uint8_t *pkt, const struct v6hdr *ip6,
                  const uip_ip4addr_t *dst4, uint16_t payload_len)
{
  const struct udphdr *udp;
  const uint8_t *data;
  uint16_t data_len;
  static uint8_t dns_buf[NAT64_DNS_BUF_SIZE];

  if(payload_len < sizeof(struct udphdr)) {
    LOG_WARN("udp_output: payload too short (%u bytes)\n", payload_len);
    return 0;
  }

  udp = (const struct udphdr *)(pkt + IPV6_HDRLEN);
  data = pkt + IPV6_HDRLEN + sizeof(struct udphdr);
  data_len = payload_len - sizeof(struct udphdr);

  if(udp->dport == UIP_HTONS(DNS_PORT)) {
    if(data_len <= sizeof(dns_buf)) {
      memcpy(dns_buf, data, data_len);
      nat64_dns64_6to4(dns_buf, data_len);
      data = dns_buf;
    } else {
      LOG_WARN("DNS64: query of %u bytes exceeds buffer (%u), forwarding "
               "without AAAA->A rewrite\n",
               data_len, (unsigned)sizeof(dns_buf));
    }
  }

  return nat64_platform_udp_send(dst4, uip_ntohs(udp->dport),
                                 &ip6->src, uip_ntohs(udp->sport),
                                 data, data_len) >= 0 ? 1 : 0;
}
/*---------------------------------------------------------------------------*/
/* ICMPv4 checksum: 16-bit one's complement over the entire ICMP message
 * (no pseudo-header). */
static uint16_t
icmp4_checksum(const void *icmp, uint16_t icmp_len)
{
  return cksum_fold(cksum_acc(0, icmp, icmp_len));
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Translate an outbound ICMPv6 Echo Request to ICMPv4 and send it.
 * \param pkt         Original IPv6 packet (header + payload).
 * \param ip6         Parsed IPv6 header (alias for pkt).
 * \param dst4        Pre-extracted IPv4 destination address.
 * \param payload_len Length of the IPv6 payload (ICMPv6 message).
 * \return 1 on success, 0 on parse or send error.
 *
 * Only Echo Request (type 128) is forwarded — all other ICMPv6 types
 * are link-local control traffic (Neighbor Discovery, MLD, ...) that
 * never traverses NAT64.  The translation rewrites the type byte and
 * recomputes the checksum (ICMPv4 has no pseudo-header) before
 * handing the message to the platform layer for delivery on a Linux
 * unprivileged ICMP socket.
 */
static int
handle_icmp6_output(const uint8_t *pkt, const struct v6hdr *ip6,
                    const uip_ip4addr_t *dst4, uint16_t payload_len)
{
  static uint8_t icmp_buf[256];
  const uint8_t *icmp;
  uint16_t identifier;

  if(payload_len < ICMP6_HDRLEN) {
    LOG_WARN("icmp6_output: payload too short (%u bytes)\n", payload_len);
    return 0;
  }

  icmp = pkt + IPV6_HDRLEN;

  if(icmp[0] != ICMP6_ECHO_REQUEST) {
    LOG_DBG("icmp6_output: ignoring ICMPv6 type %u\n", icmp[0]);
    return 0;
  }

  if(payload_len > sizeof(icmp_buf)) {
    LOG_WARN("icmp6_output: echo request too large (%u bytes)\n",
             payload_len);
    return 0;
  }

  memcpy(icmp_buf, icmp, payload_len);
  icmp_buf[0] = ICMP4_ECHO_REQUEST;
  icmp_buf[2] = 0;
  icmp_buf[3] = 0;
  uint16_t cksum = icmp4_checksum(icmp_buf, payload_len);
  icmp_buf[2] = (uint8_t)(cksum >> 8);
  icmp_buf[3] = (uint8_t)cksum;

  identifier = ((uint16_t)icmp[4] << 8) | icmp[5];

  return nat64_platform_icmp_send(dst4, &ip6->src, identifier,
                                  icmp_buf, payload_len) >= 0 ? 1 : 0;
}
/*---------------------------------------------------------------------------*/
int
nat64_output(const uint8_t *pkt, uint16_t len)
{
  const struct v6hdr *ip6 = (const struct v6hdr *)pkt;
  uint16_t payload_len;
  uip_ip4addr_t dst4;

  if(len < IPV6_HDRLEN) {
    LOG_WARN("output: packet too short (%u bytes)\n", len);
    return 0;
  }

  payload_len = get16(ip6->plen);
  if(payload_len + IPV6_HDRLEN > len) {
    LOG_WARN("output: payload %u exceeds packet %u\n", payload_len, len);
    return 0;
  }

  if(ip64_addr_is_ip64(&ip6->src)) {
    LOG_WARN("output: dropping packet with NAT64 source address\n");
    return 0;
  }

  if(!ip64_addr_6to4(&ip6->dst, &dst4)) {
    LOG_WARN("output: destination is not a NAT64 address\n");
    return 0;
  }

  if(ipv4_dst_is_forbidden(&dst4)) {
    LOG_WARN("output: dropping packet to forbidden IPv4 destination "
             "%u.%u.%u.%u\n",
             dst4.u8[0], dst4.u8[1], dst4.u8[2], dst4.u8[3]);
    nat64_queue_icmp6_unreach(pkt, len, NAT64_ICMP6_ADMIN);
    return 0;
  }

  switch(ip6->nexthdr) {
  case IP_PROTO_UDP:
    return handle_udp_output(pkt, ip6, &dst4, payload_len);
  case IP_PROTO_TCP:
    return nat64_tcp_output(pkt, len);
  case IP_PROTO_ICMPV6:
    return handle_icmp6_output(pkt, ip6, &dst4, payload_len);
  default:
    LOG_WARN("output: unsupported next-header %u\n", ip6->nexthdr);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
void
nat64_udp_input(struct nat64_session *s,
                const uint8_t *payload, uint16_t payload_len)
{
  struct v6hdr *ip6;
  struct udphdr *udp;
  uint16_t udp_total;

  udp_total = sizeof(struct udphdr) + payload_len;

  if(IPV6_HDRLEN + udp_total > UIP_BUFSIZE) {
    LOG_WARN("udp_input: response too large (%u bytes)\n",
             IPV6_HDRLEN + udp_total);
    return;
  }

  ip6 = (struct v6hdr *)uip_buf;
  ip6->vtc = 0x60;
  ip6->tcflow = 0;
  ip6->flow = 0;
  ip6->nexthdr = IP_PROTO_UDP;
  ip6->hoplim = DEFAULT_HOPLIM;

  ip64_addr_4to6(&s->ip4_remote, &ip6->src);
  uip_ip6addr_copy(&ip6->dst, &s->ip6_peer);

  udp = (struct udphdr *)(uip_buf + IPV6_HDRLEN);
  udp->sport = uip_htons(s->ip4_remote_port);
  udp->dport = uip_htons(s->ip6_peer_port);

  memcpy(uip_buf + IPV6_HDRLEN + sizeof(struct udphdr),
         payload, payload_len);

  if(s->ip4_remote_port == DNS_PORT) {
    uint16_t max_payload = UIP_BUFSIZE - IPV6_HDRLEN - sizeof(struct udphdr);
    uint16_t new_len;
    new_len = nat64_dns64_4to6(
      payload, payload_len,
      uip_buf + IPV6_HDRLEN + sizeof(struct udphdr), payload_len,
      max_payload);
    if(new_len > max_payload) {
      LOG_WARN("udp_input: DNS64 response too large (%u bytes)\n", new_len);
      return;
    }
    payload_len = new_len;
    udp_total = sizeof(struct udphdr) + payload_len;
  }

  put16(ip6->plen, udp_total);
  udp->ulen = uip_htons(udp_total);

  udp->uchksum = 0;
  udp->uchksum = uip_htons(udp6_checksum(
    ip6, udp,
    uip_buf + IPV6_HDRLEN + sizeof(struct udphdr),
    payload_len));

  uip_len = IPV6_HDRLEN + udp_total;

  LOG_DBG("udp_input: injecting %u-byte packet\n", uip_len);
  tcpip_input();
}
/*---------------------------------------------------------------------------*/
void
nat64_icmp_input(struct nat64_session *s, const uint8_t *icmp_pkt, uint16_t len)
{
  struct v6hdr *ip6;
  uint8_t *icmp_out;

  if(len < ICMP6_HDRLEN) {
    LOG_WARN("icmp_input: reply too short (%u bytes)\n", len);
    return;
  }
  if(IPV6_HDRLEN + len > UIP_BUFSIZE) {
    LOG_WARN("icmp_input: reply too large (%u bytes)\n", IPV6_HDRLEN + len);
    return;
  }

  /* Only forward Echo Replies; the kernel may surface other ICMP
   * types on the ping socket (e.g., Destination Unreachable from a
   * router on the IPv4 path).  We do not relay those — the kernel
   * already returns them via socket-level errors that the platform
   * layer maps to ICMPv6 errors via nat64_queue_icmp6_unreach_*. */
  if(icmp_pkt[0] != ICMP4_ECHO_REPLY) {
    LOG_DBG("icmp_input: ignoring ICMPv4 type %u\n", icmp_pkt[0]);
    return;
  }

  ip6 = (struct v6hdr *)uip_buf;
  ip6->vtc = 0x60;
  ip6->tcflow = 0;
  ip6->flow = 0;
  put16(ip6->plen, len);
  ip6->nexthdr = IP_PROTO_ICMPV6;
  ip6->hoplim = DEFAULT_HOPLIM;
  ip64_addr_4to6(&s->ip4_remote, &ip6->src);
  uip_ip6addr_copy(&ip6->dst, &s->ip6_peer);

  icmp_out = uip_buf + IPV6_HDRLEN;
  memcpy(icmp_out, icmp_pkt, len);

  /* Translate to ICMPv6 Echo Reply (type 129) and restore the
   * original identifier from the session — the kernel may have
   * rewritten it on the wire. */
  icmp_out[0] = ICMP6_ECHO_REPLY;
  icmp_out[4] = (uint8_t)(s->ip6_peer_port >> 8);
  icmp_out[5] = (uint8_t)s->ip6_peer_port;

  icmp_out[2] = 0;
  icmp_out[3] = 0;
  uint16_t cksum = icmp6_checksum(ip6, icmp_out, len);
  icmp_out[2] = (uint8_t)(cksum >> 8);
  icmp_out[3] = (uint8_t)cksum;

  uip_len = IPV6_HDRLEN + len;

  LOG_DBG("icmp_input: injecting Echo Reply (%u bytes)\n", uip_len);
  tcpip_input();
}
/*---------------------------------------------------------------------------*/
void
nat64_activate(void)
{
  nat64_tcp_init();
  active = true;
}
/*---------------------------------------------------------------------------*/
/** @} */
