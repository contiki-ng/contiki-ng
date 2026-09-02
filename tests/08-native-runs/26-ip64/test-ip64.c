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
 *   End-to-end test of the ip64 NAT64 translation service.
 *
 *   The module is exercised through its real entry points, with the ENC28J60
 *   driver replaced by a capture driver, so no hardware, tun device, or
 *   superuser privileges are needed. Outbound packets are produced by a
 *   regular UDP socket and picked up through the uIP fallback interface;
 *   inbound packets are handed to ip64_eth_interface_input() as Ethernet
 *   frames, exactly as an Ethernet driver would deliver them.
 *
 *   Covered: uIP fallback dispatch, ARP resolution, IPv6-to-IPv4 and
 *   IPv4-to-IPv6 header translation, forwarding of a flow from a node behind
 *   the router, address-mapping allocation and reverse lookup, DNS64
 *   rewriting in both directions, ICMP echo translation, and the inbound
 *   port handling. Not covered: the ENC28J60 driver itself, the
 *   DHCPv4 client, and TCP.
 *
 */

#include "contiki.h"
#include "contiki-net.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/ip64-addr.h"

#include "ip64/ip64.h"
#include "ip64/ip64-addrmap.h"
#include "ip64/ip64-eth.h"
#include "ip64/ip64-eth-interface.h"

#include "ip64-test-driver.h"
#include "unit-test.h"

#include <stdio.h>
#include <string.h>

#define LOCAL_PORT      3000
#define SERVER_PORT     5000
#define DNS_LOCAL_PORT  3053
#define DNS_PORT        53
/* Below EPHEMERAL_PORTRANGE in ip64.c: reachable without a mapping. */
#define SERVICE_PORT    780
/* Source port of the flow that arrives from the simulated mote. */
#define MOTE_PORT       4711

#define REQUEST         "PING-IP64"
#define REPLY           "PONG-IP64"

#define IPV6_HDRLEN     40
#define IPV4_HDRLEN     20
#define UDP_HDRLEN      8
#define ICMP_ECHO_HDRLEN 8

/* uIP has no IPv4 protocol numbers, so the ICMPv4 ones are defined here. */
#define IP_PROTO_ICMPV4 1
#define ICMP_ECHO_REPLY 0
#define ICMP_ECHO       8

#define ECHO_ID         0xbeef
#define ECHO_SEQNO      7

#define ARP_REQUEST     1
#define ARP_REPLY       2

#define DNS_TYPE_A      1
#define DNS_TYPE_AAAA  28

/* Offsets into the DNS test messages below: a 12-byte header, a 13-byte
   name ("example.com"), then the 4-byte type and class fields. */
#define DNS_QUESTION_TYPE_OFFSET (12 + 13)
#define DNS_ANSWER_OFFSET        (12 + 13 + 4 + 2)

/* Upper bound on the time the stack may take to produce a result. Every path
   the test drives is synchronous, so the waits below normally return without
   sleeping; the timer only keeps a packet that never arrives from hanging the
   test, leaving the assertion that follows to report the failure. */
#define SETTLE_TIME (CLOCK_SECOND / 4)

/* IPv4 configuration, standing in for a DHCP lease. */
static uip_ip4addr_t hostaddr;
static uip_ip4addr_t netmask;
static uip_ip4addr_t draddr;
/* The IPv4 server, reached over IPv6 as 64:ff9b::c000:20a. */
static uip_ip4addr_t serveraddr;
/* A node in the IPv6 network behind the router, standing in for a mote. */
static uip_ip6addr_t moteaddr;

static struct simple_udp_connection conn;
static struct simple_udp_connection dnsconn;
static struct simple_udp_connection serviceconn;

static bool reply_received;
static bool dns_rewritten;
static bool service_delivered;

/* MAC address the test answers ARP requests with. */
static const uint8_t router_mac[6] = { 0x02, 0xaa, 0xbb, 0xcc, 0xdd, 0x01 };

/* A query for "example.com" of type AAAA. */
static const uint8_t dns_query[] = {
  0x12, 0x34, 0x01, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  7, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3, 'c', 'o', 'm', 0,
  0x00, 0x1c, 0x00, 0x01
};

/* The matching response, carrying one A record for 93.184.216.34. The
   answer names the question through a compression pointer. */
static const uint8_t dns_response[] = {
  0x12, 0x34, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
  7, 'e', 'x', 'a', 'm', 'p', 'l', 'e', 3, 'c', 'o', 'm', 0,
  0x00, 0x01, 0x00, 0x01,
  0xc0, 0x0c,
  0x00, 0x01, 0x00, 0x01,
  0x00, 0x00, 0x0e, 0x10,
  0x00, 0x04,
  93, 184, 216, 34
};

struct eth_hdr {
  uint8_t dest[6];
  uint8_t src[6];
  uint16_t type;
};

struct arp_hdr {
  struct eth_hdr ethhdr;
  uint16_t hwtype;
  uint16_t protocol;
  uint8_t hwlen;
  uint8_t protolen;
  uint16_t opcode;
  uint8_t shwaddr[6];
  uip_ip4addr_t sipaddr;
  uint8_t dhwaddr[6];
  uip_ip4addr_t dipaddr;
};

struct ipv6_hdr {
  uint8_t vtc;
  uint8_t tcflow;
  uint16_t flow;
  uint8_t len[2];
  uint8_t nxthdr;
  uint8_t hoplim;
  uip_ip6addr_t srcipaddr;
  uip_ip6addr_t destipaddr;
};

struct ipv4_hdr {
  uint8_t vhl;
  uint8_t tos;
  uint8_t len[2];
  uint8_t ipid[2];
  uint8_t ipoffset[2];
  uint8_t ttl;
  uint8_t proto;
  uint16_t ipchksum;
  uip_ip4addr_t srcipaddr;
  uip_ip4addr_t destipaddr;
};

struct udp_hdr {
  uint16_t srcport;
  uint16_t destport;
  uint16_t udplen;
  uint16_t udpchksum;
};

struct icmp_echo_hdr {
  uint8_t type;
  uint8_t icode;
  uint16_t icmpchksum;
  uint16_t id;
  uint16_t seqno;
};

static uint8_t frame[512];
static struct etimer settle_timer;

PROCESS(test_ip64_process, "ip64 end-to-end test");
AUTOSTART_PROCESSES(&test_ip64_process);

UNIT_TEST_REGISTER(arp_resolution,
                   "ARP resolution precedes the first translated packet");
UNIT_TEST_REGISTER(udp_round_trip,
                   "UDP is translated out to IPv4 and back to the socket");
UNIT_TEST_REGISTER(forwarded_flow,
                   "A flow forwarded from a mote is mapped to the mote");
UNIT_TEST_REGISTER(dns64_rewrite,
                   "DNS64 rewrites AAAA to A and the A answer back to AAAA");
UNIT_TEST_REGISTER(icmp_echo,
                   "An IPv4 ping is answered by the local IPv6 host");
UNIT_TEST_REGISTER(inbound_ports,
                   "Inbound packets reach the local host only below the "
                   "ephemeral port range");

/*---------------------------------------------------------------------------*/
/* One's complement sum, as used for IPv4 and UDP checksums. */
static uint16_t
chksum(uint16_t sum, const uint8_t *data, uint16_t len)
{
  const uint8_t *last_byte = data + len - 1;
  uint16_t t;

  while(data < last_byte) {
    t = (data[0] << 8) + data[1];
    sum += t;
    if(sum < t) {
      sum++;
    }
    data += 2;
  }

  if(data == last_byte) {
    t = data[0] << 8;
    sum += t;
    if(sum < t) {
      sum++;
    }
  }

  return sum;
}
/*---------------------------------------------------------------------------*/
static void
udp_received(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
             uint16_t sender_port, const uip_ipaddr_t *receiver_addr,
             uint16_t receiver_port, const uint8_t *data, uint16_t datalen)
{
  if(datalen == strlen(REPLY) && memcmp(data, REPLY, datalen) == 0) {
    reply_received = true;
  }
}
/*---------------------------------------------------------------------------*/
/* Records the packets that ip64 sent to the local host without a mapping. */
static void
service_received(struct simple_udp_connection *c,
                 const uip_ipaddr_t *sender_addr, uint16_t sender_port,
                 const uip_ipaddr_t *receiver_addr, uint16_t receiver_port,
                 const uint8_t *data, uint16_t datalen)
{
  uip_ip6addr_t expected_sender;

  ip64_addr_4to6(&serveraddr, &expected_sender);

  if(datalen == strlen(REQUEST) && memcmp(data, REQUEST, datalen) == 0 &&
     uip_ipaddr_cmp(sender_addr, &expected_sender)) {
    service_delivered = true;
  }
}
/*---------------------------------------------------------------------------*/
static void
dns_received(struct simple_udp_connection *c, const uip_ipaddr_t *sender_addr,
             uint16_t sender_port, const uip_ipaddr_t *receiver_addr,
             uint16_t receiver_port, const uint8_t *data, uint16_t datalen)
{
  const uint8_t *answer;
  uip_ip4addr_t recorded_addr;
  uip_ip6addr_t expected_addr;

  /* The A record must have become an AAAA record, which is 12 bytes
     longer because the address grows from 4 to 16 bytes. */
  if(datalen != sizeof(dns_response) + 12) {
    printf("DNS response has length %u, expected %u\n",
           datalen, (unsigned)sizeof(dns_response) + 12);
    return;
  }

  answer = &data[DNS_ANSWER_OFFSET];
  uip_ipaddr(&recorded_addr, 93, 184, 216, 34);
  ip64_addr_4to6(&recorded_addr, &expected_addr);

  if((answer[0] << 8) + answer[1] == DNS_TYPE_AAAA &&
     (answer[8] << 8) + answer[9] == sizeof(uip_ip6addr_t) &&
     memcmp(&answer[10], &expected_addr, sizeof(uip_ip6addr_t)) == 0) {
    dns_rewritten = true;
  }
}
/*---------------------------------------------------------------------------*/
/* Fill in the Ethernet header of a frame sent by the IPv4 router. */
static void
fill_eth_hdr(struct eth_hdr *eth, uint16_t type)
{
  memcpy(eth->dest, ip64_eth_addr.addr, sizeof(eth->dest));
  memcpy(eth->src, router_mac, sizeof(eth->src));
  eth->type = uip_htons(type);
}
/*---------------------------------------------------------------------------*/
/* Fill in an IPv4 header, including its checksum. */
static void
fill_ipv4_hdr(struct ipv4_hdr *ip, const uip_ip4addr_t *src,
              const uip_ip4addr_t *dst, uint8_t proto, uint16_t ip_len)
{
  memset(ip, 0, IPV4_HDRLEN);
  ip->vhl = 0x45;
  ip->len[0] = ip_len >> 8;
  ip->len[1] = ip_len & 0xff;
  ip->ttl = 64;
  ip->proto = proto;
  uip_ip4addr_copy(&ip->srcipaddr, src);
  uip_ip4addr_copy(&ip->destipaddr, dst);
  ip->ipchksum = ~uip_htons(chksum(0, (uint8_t *)ip, IPV4_HDRLEN));
}
/*---------------------------------------------------------------------------*/
/* Build an Ethernet-framed IPv4 UDP packet. Returns the frame length. */
static uint16_t
build_ipv4_udp(uint8_t *buf, const uip_ip4addr_t *src,
               const uip_ip4addr_t *dst, uint16_t srcport, uint16_t destport,
               const uint8_t *payload, uint16_t payload_len)
{
  struct ipv4_hdr *ip = (struct ipv4_hdr *)&buf[sizeof(struct eth_hdr)];
  struct udp_hdr *udp = (struct udp_hdr *)((uint8_t *)ip + IPV4_HDRLEN);
  uint16_t ip_len = IPV4_HDRLEN + UDP_HDRLEN + payload_len;
  uint16_t sum;

  fill_eth_hdr((struct eth_hdr *)buf, IP64_ETH_TYPE_IP);
  fill_ipv4_hdr(ip, src, dst, UIP_PROTO_UDP, ip_len);

  udp->srcport = uip_htons(srcport);
  udp->destport = uip_htons(destport);
  udp->udplen = uip_htons(UDP_HDRLEN + payload_len);
  udp->udpchksum = 0;
  memcpy((uint8_t *)udp + UDP_HDRLEN, payload, payload_len);

  /* The UDP checksum covers the IPv4 pseudo header. */
  sum = UDP_HDRLEN + payload_len + UIP_PROTO_UDP;
  sum = chksum(sum, (uint8_t *)&ip->srcipaddr, 2 * sizeof(uip_ip4addr_t));
  sum = chksum(sum, (uint8_t *)udp, UDP_HDRLEN + payload_len);
  udp->udpchksum = ~uip_htons(sum);
  if(udp->udpchksum == 0) {
    udp->udpchksum = 0xffff;
  }

  return sizeof(struct eth_hdr) + ip_len;
}
/*---------------------------------------------------------------------------*/
/* Build an Ethernet-framed IPv4 ICMP echo request. Returns the frame
   length. */
static uint16_t
build_ipv4_icmp_echo(uint8_t *buf, const uip_ip4addr_t *src,
                     const uip_ip4addr_t *dst, uint16_t id, uint16_t seqno,
                     const uint8_t *payload, uint16_t payload_len)
{
  struct ipv4_hdr *ip = (struct ipv4_hdr *)&buf[sizeof(struct eth_hdr)];
  struct icmp_echo_hdr *icmp =
    (struct icmp_echo_hdr *)((uint8_t *)ip + IPV4_HDRLEN);
  uint16_t ip_len = IPV4_HDRLEN + ICMP_ECHO_HDRLEN + payload_len;

  fill_eth_hdr((struct eth_hdr *)buf, IP64_ETH_TYPE_IP);
  fill_ipv4_hdr(ip, src, dst, IP_PROTO_ICMPV4, ip_len);

  icmp->type = ICMP_ECHO;
  icmp->icode = 0;
  icmp->icmpchksum = 0;
  icmp->id = uip_htons(id);
  icmp->seqno = uip_htons(seqno);
  memcpy((uint8_t *)icmp + ICMP_ECHO_HDRLEN, payload, payload_len);

  /* Unlike UDP, the ICMPv4 checksum covers no pseudo header. */
  icmp->icmpchksum = ~uip_htons(chksum(0, (uint8_t *)icmp,
                                       ICMP_ECHO_HDRLEN + payload_len));

  return sizeof(struct eth_hdr) + ip_len;
}
/*---------------------------------------------------------------------------*/
/* Check a captured IPv4 UDP frame the way the receiving IPv4 host would: the
   lengths have to agree with one another and with the frame, and the UDP
   checksum, which covers the pseudo header, has to verify. Without this the
   tests would accept a packet no real host would. */
static bool
ipv4_udp_is_valid(const uint8_t *buf, uint16_t len)
{
  const struct ipv4_hdr *ip =
    (const struct ipv4_hdr *)&buf[sizeof(struct eth_hdr)];
  const struct udp_hdr *udp =
    (const struct udp_hdr *)((const uint8_t *)ip + IPV4_HDRLEN);
  uint16_t ip_len = (ip->len[0] << 8) + ip->len[1];
  uint16_t udp_len = uip_ntohs(udp->udplen);
  uint16_t sum;

  if(len != sizeof(struct eth_hdr) + ip_len ||
     ip_len != IPV4_HDRLEN + udp_len ||
     udp_len < UDP_HDRLEN) {
    return false;
  }

  sum = udp_len + UIP_PROTO_UDP;
  sum = chksum(sum, (const uint8_t *)&ip->srcipaddr,
               2 * sizeof(uip_ip4addr_t));
  sum = chksum(sum, (const uint8_t *)udp, udp_len);

  return sum == 0xffff;
}
/*---------------------------------------------------------------------------*/
/* Hand uIP an IPv6 UDP packet as if it had arrived over the radio from a
   node behind the router, so that the router forwards it towards the IPv4
   network. This is what a mote's traffic looks like to ip64, and it is the
   only way to produce a flow whose IPv6 address is not the router's own. */
static void
inject_from_mote(const uip_ip6addr_t *src, const uip_ip6addr_t *dst,
                 uint16_t srcport, uint16_t destport,
                 const uint8_t *payload, uint16_t payload_len)
{
  struct ipv6_hdr *ip = (struct ipv6_hdr *)uip_buf;
  struct udp_hdr *udp = (struct udp_hdr *)&uip_buf[IPV6_HDRLEN];
  uint16_t udp_len = UDP_HDRLEN + payload_len;
  uint16_t sum;

  memset(ip, 0, IPV6_HDRLEN);
  ip->vtc = 0x60;
  ip->len[0] = udp_len >> 8;
  ip->len[1] = udp_len & 0xff;
  ip->nxthdr = UIP_PROTO_UDP;
  /* High enough to survive the decrement that forwarding applies. */
  ip->hoplim = 64;
  uip_ipaddr_copy(&ip->srcipaddr, src);
  uip_ipaddr_copy(&ip->destipaddr, dst);

  udp->srcport = uip_htons(srcport);
  udp->destport = uip_htons(destport);
  udp->udplen = uip_htons(udp_len);
  udp->udpchksum = 0;
  memcpy((uint8_t *)udp + UDP_HDRLEN, payload, payload_len);

  /* The UDP checksum covers the IPv6 pseudo header. */
  sum = udp_len + UIP_PROTO_UDP;
  sum = chksum(sum, (uint8_t *)&ip->srcipaddr, 2 * sizeof(uip_ip6addr_t));
  sum = chksum(sum, (uint8_t *)udp, udp_len);
  udp->udpchksum = ~uip_htons(sum);
  if(udp->udpchksum == 0) {
    udp->udpchksum = 0xffff;
  }

  uip_len = IPV6_HDRLEN + udp_len;
  tcpip_input();
}
/*---------------------------------------------------------------------------*/
static void
inject_arp_reply(void)
{
  uint8_t buf[sizeof(struct arp_hdr)];
  struct arp_hdr *arp = (struct arp_hdr *)buf;

  memset(buf, 0, sizeof(buf));
  memcpy(arp->ethhdr.dest, ip64_eth_addr.addr, sizeof(arp->ethhdr.dest));
  memcpy(arp->ethhdr.src, router_mac, sizeof(arp->ethhdr.src));
  arp->ethhdr.type = UIP_HTONS(IP64_ETH_TYPE_ARP);
  arp->hwtype = UIP_HTONS(1);
  arp->protocol = UIP_HTONS(IP64_ETH_TYPE_IP);
  arp->hwlen = sizeof(arp->shwaddr);
  arp->protolen = sizeof(uip_ip4addr_t);
  arp->opcode = UIP_HTONS(ARP_REPLY);
  memcpy(arp->shwaddr, router_mac, sizeof(arp->shwaddr));
  uip_ip4addr_copy(&arp->sipaddr, &draddr);
  memcpy(arp->dhwaddr, ip64_eth_addr.addr, sizeof(arp->dhwaddr));
  uip_ip4addr_copy(&arp->dipaddr, &hostaddr);

  ip64_eth_interface_input(buf, sizeof(buf));
}
/*---------------------------------------------------------------------------*/
UNIT_TEST(arp_resolution)
{
  uip_ipaddr_t dest;
  const struct arp_hdr *arp;
  static const uint8_t broadcast_mac[6] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };

  UNIT_TEST_BEGIN();

  ip64_addr_4to6(&serveraddr, &dest);

  /* The ARP cache is empty, so ip64 must ask for the router's MAC address
     before it can transmit anything. */
  ip64_test_driver_reset();
  simple_udp_sendto(&conn, REQUEST, strlen(REQUEST), &dest);
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt,
                ip64_test_driver_count > 0 || etimer_expired(&settle_timer));

  UNIT_TEST_ASSERT(ip64_test_driver_count == 1);
  UNIT_TEST_ASSERT(ip64_test_driver_len >= sizeof(struct arp_hdr));

  arp = (const struct arp_hdr *)ip64_test_driver_buf;
  UNIT_TEST_ASSERT(arp->ethhdr.type == UIP_HTONS(IP64_ETH_TYPE_ARP));
  UNIT_TEST_ASSERT(memcmp(arp->ethhdr.dest, broadcast_mac,
                          sizeof(broadcast_mac)) == 0);
  UNIT_TEST_ASSERT(arp->opcode == UIP_HTONS(ARP_REQUEST));

  /* The server is off link, so the request must ask for the MAC address of
     the default router rather than that of the server. */
  UNIT_TEST_ASSERT(uip_ip4addr_cmp(&arp->dipaddr, &draddr));
  UNIT_TEST_ASSERT(uip_ip4addr_cmp(&arp->sipaddr, &hostaddr));
  UNIT_TEST_ASSERT(memcmp(arp->shwaddr, ip64_eth_addr.addr,
                          sizeof(arp->shwaddr)) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST(udp_round_trip)
{
  uip_ipaddr_t dest;
  const struct eth_hdr *eth;
  const struct ipv4_hdr *ip;
  const struct udp_hdr *udp;
  uint16_t mapped_port;
  uint16_t len;

  UNIT_TEST_BEGIN();

  ip64_addr_4to6(&serveraddr, &dest);

  ip64_test_driver_reset();
  simple_udp_sendto(&conn, REQUEST, strlen(REQUEST), &dest);
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt,
                ip64_test_driver_count > 0 || etimer_expired(&settle_timer));

  UNIT_TEST_ASSERT(ip64_test_driver_count == 1);
  UNIT_TEST_ASSERT(ip64_test_driver_len >=
                   sizeof(struct eth_hdr) + IPV4_HDRLEN + UDP_HDRLEN);

  eth = (const struct eth_hdr *)ip64_test_driver_buf;
  ip = (const struct ipv4_hdr *)&ip64_test_driver_buf[sizeof(struct eth_hdr)];
  udp = (const struct udp_hdr *)((const uint8_t *)ip + IPV4_HDRLEN);

  /* Now that the ARP cache is populated, the packet must go out as IPv4
     to the router's MAC address. */
  UNIT_TEST_ASSERT(eth->type == UIP_HTONS(IP64_ETH_TYPE_IP));
  UNIT_TEST_ASSERT(memcmp(eth->dest, router_mac, sizeof(eth->dest)) == 0);

  UNIT_TEST_ASSERT(uip_ip4addr_cmp(&ip->srcipaddr, &hostaddr));
  UNIT_TEST_ASSERT(uip_ip4addr_cmp(&ip->destipaddr, &serveraddr));
  UNIT_TEST_ASSERT(ip->proto == UIP_PROTO_UDP);
  UNIT_TEST_ASSERT(chksum(0, (const uint8_t *)ip, IPV4_HDRLEN) == 0xffff);

  UNIT_TEST_ASSERT(uip_ntohs(udp->destport) == SERVER_PORT);
  UNIT_TEST_ASSERT(memcmp((const uint8_t *)udp + UDP_HDRLEN,
                          REQUEST, strlen(REQUEST)) == 0);
  UNIT_TEST_ASSERT(ipv4_udp_is_valid(ip64_test_driver_buf,
                                     ip64_test_driver_len));

  /* The source port is translated, so the reply has to be addressed to the
     mapped port for the reverse lookup to find the flow. */
  mapped_port = uip_ntohs(udp->srcport);

  reply_received = false;
  len = build_ipv4_udp(frame, &serveraddr, &hostaddr, SERVER_PORT, mapped_port,
                       (const uint8_t *)REPLY, strlen(REPLY));
  ip64_eth_interface_input(frame, len);
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt,
                reply_received || etimer_expired(&settle_timer));

  UNIT_TEST_ASSERT(reply_received);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST(forwarded_flow)
{
  uip_ipaddr_t dest;
  const struct ipv4_hdr *ip;
  const struct udp_hdr *udp;
  const struct ip64_addrmap_entry *m;
  uint16_t mapped_port;

  UNIT_TEST_BEGIN();

  ip64_addr_4to6(&serveraddr, &dest);

  /* The router already has a mapping of its own, from udp_round_trip, so the
     mote's flow has to be told apart from it. */
  ip64_test_driver_reset();
  inject_from_mote(&moteaddr, &dest, MOTE_PORT, SERVER_PORT,
                   (const uint8_t *)REQUEST, strlen(REQUEST));
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt,
                ip64_test_driver_count > 0 || etimer_expired(&settle_timer));

  UNIT_TEST_ASSERT(ip64_test_driver_count == 1);
  UNIT_TEST_ASSERT(ip64_test_driver_len >=
                   sizeof(struct eth_hdr) + IPV4_HDRLEN + UDP_HDRLEN);

  ip = (const struct ipv4_hdr *)&ip64_test_driver_buf[sizeof(struct eth_hdr)];
  udp = (const struct udp_hdr *)((const uint8_t *)ip + IPV4_HDRLEN);

  /* The mote's packet leaves as IPv4 from the router's own address, which is
     the whole point of the translation. */
  UNIT_TEST_ASSERT(ip->proto == UIP_PROTO_UDP);
  UNIT_TEST_ASSERT(uip_ip4addr_cmp(&ip->srcipaddr, &hostaddr));
  UNIT_TEST_ASSERT(uip_ip4addr_cmp(&ip->destipaddr, &serveraddr));
  UNIT_TEST_ASSERT(chksum(0, (const uint8_t *)ip, IPV4_HDRLEN) == 0xffff);
  UNIT_TEST_ASSERT(uip_ntohs(udp->destport) == SERVER_PORT);
  UNIT_TEST_ASSERT(memcmp((const uint8_t *)udp + UDP_HDRLEN,
                          REQUEST, strlen(REQUEST)) == 0);
  UNIT_TEST_ASSERT(ipv4_udp_is_valid(ip64_test_driver_buf,
                                     ip64_test_driver_len));

  /* A reply would be resolved through the mapped port, so that lookup has to
     yield the mote rather than the router: ip64_4to6() takes the destination
     of the translated packet straight from this entry. */
  mapped_port = uip_ntohs(udp->srcport);
  m = ip64_addrmap_lookup_port(mapped_port, UIP_PROTO_UDP);
  UNIT_TEST_ASSERT(m != NULL);
  UNIT_TEST_ASSERT(uip_ipaddr_cmp(&m->ip6addr, &moteaddr));
  UNIT_TEST_ASSERT(m->ip6port == MOTE_PORT);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST(dns64_rewrite)
{
  uip_ipaddr_t dest;
  const struct ipv4_hdr *ip;
  const struct udp_hdr *udp;
  const uint8_t *dns;
  uint16_t mapped_dns_port;
  uint16_t len;

  UNIT_TEST_BEGIN();

  /* The DNS server is the default router, which is already in the ARP
     cache at this point. */
  ip64_addr_4to6(&draddr, &dest);

  ip64_test_driver_reset();
  simple_udp_sendto(&dnsconn, dns_query, sizeof(dns_query), &dest);
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt,
                ip64_test_driver_count > 0 || etimer_expired(&settle_timer));

  UNIT_TEST_ASSERT(ip64_test_driver_count == 1);
  UNIT_TEST_ASSERT(ip64_test_driver_len >= sizeof(struct eth_hdr) +
                   IPV4_HDRLEN + UDP_HDRLEN + sizeof(dns_query));

  ip = (const struct ipv4_hdr *)&ip64_test_driver_buf[sizeof(struct eth_hdr)];
  udp = (const struct udp_hdr *)((const uint8_t *)ip + IPV4_HDRLEN);
  dns = (const uint8_t *)udp + UDP_HDRLEN;

  /* An IPv4 server cannot answer a AAAA query, so the question type must
     have been rewritten on the way out. */
  UNIT_TEST_ASSERT((dns[DNS_QUESTION_TYPE_OFFSET] << 8) +
                   dns[DNS_QUESTION_TYPE_OFFSET + 1] == DNS_TYPE_A);

  /* The rewrite changed the payload, so the checksum had to be recomputed
     over the new bytes. */
  UNIT_TEST_ASSERT(ipv4_udp_is_valid(ip64_test_driver_buf,
                                     ip64_test_driver_len));

  mapped_dns_port = uip_ntohs(udp->srcport);

  dns_rewritten = false;
  len = build_ipv4_udp(frame, &draddr, &hostaddr, DNS_PORT, mapped_dns_port,
                       dns_response, sizeof(dns_response));
  ip64_eth_interface_input(frame, len);
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt,
                dns_rewritten || etimer_expired(&settle_timer));

  /* The A record must come back as an AAAA record holding the NAT64
     representation of the recorded IPv4 address. */
  UNIT_TEST_ASSERT(dns_rewritten);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST(icmp_echo)
{
  const struct ipv4_hdr *ip;
  const struct icmp_echo_hdr *icmp;
  uint16_t len;

  UNIT_TEST_BEGIN();

  /* An IPv4 host pings the IPv4 address of the router. ip64 passes echo
     requests on to the local IPv6 host, whose reply must come back out as
     ICMPv4 without any mapping being involved. */
  ip64_test_driver_reset();
  len = build_ipv4_icmp_echo(frame, &serveraddr, &hostaddr, ECHO_ID,
                             ECHO_SEQNO, (const uint8_t *)REQUEST,
                             strlen(REQUEST));
  ip64_eth_interface_input(frame, len);
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt,
                ip64_test_driver_count > 0 || etimer_expired(&settle_timer));

  /* The reply carries the same payload as the request, so it has the same
     size as the frame that was injected. */
  UNIT_TEST_ASSERT(ip64_test_driver_count == 1);
  UNIT_TEST_ASSERT(ip64_test_driver_len == sizeof(struct eth_hdr) +
                   IPV4_HDRLEN + ICMP_ECHO_HDRLEN + strlen(REQUEST));

  ip = (const struct ipv4_hdr *)&ip64_test_driver_buf[sizeof(struct eth_hdr)];
  icmp = (const struct icmp_echo_hdr *)((const uint8_t *)ip + IPV4_HDRLEN);

  UNIT_TEST_ASSERT(ip->proto == IP_PROTO_ICMPV4);
  UNIT_TEST_ASSERT(uip_ip4addr_cmp(&ip->srcipaddr, &hostaddr));
  UNIT_TEST_ASSERT(uip_ip4addr_cmp(&ip->destipaddr, &serveraddr));
  UNIT_TEST_ASSERT(chksum(0, (const uint8_t *)ip, IPV4_HDRLEN) == 0xffff);

  /* The identifier, the sequence number, and the payload are echoed back
     unchanged; only the type differs. */
  UNIT_TEST_ASSERT(icmp->type == ICMP_ECHO_REPLY);
  UNIT_TEST_ASSERT(uip_ntohs(icmp->id) == ECHO_ID);
  UNIT_TEST_ASSERT(uip_ntohs(icmp->seqno) == ECHO_SEQNO);
  UNIT_TEST_ASSERT(memcmp((const uint8_t *)icmp + ICMP_ECHO_HDRLEN,
                          REQUEST, strlen(REQUEST)) == 0);
  UNIT_TEST_ASSERT(chksum(0, (const uint8_t *)icmp,
                          ICMP_ECHO_HDRLEN + strlen(REQUEST)) == 0xffff);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST(inbound_ports)
{
  uint16_t len;

  UNIT_TEST_BEGIN();

  /* Unsolicited traffic to a port below the ephemeral range is delivered to
     the local host, which is the only way an IPv4 host can reach a service
     on the router without a mapping having been set up first. */
  service_delivered = false;
  len = build_ipv4_udp(frame, &serveraddr, &hostaddr, SERVER_PORT,
                       SERVICE_PORT, (const uint8_t *)REQUEST,
                       strlen(REQUEST));
  ip64_eth_interface_input(frame, len);
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt,
                service_delivered || etimer_expired(&settle_timer));

  UNIT_TEST_ASSERT(service_delivered);

  /* Traffic to an ephemeral port must be dropped when no mapping resolves
     it, even though a socket is listening on that port. The port used here
     is the one that received the reply in udp_round_trip, so a delivery
     would be noticed; only the mapping, which is keyed on the translated
     port rather than on this one, is missing. */
  UNIT_TEST_ASSERT(ip64_addrmap_lookup_port(LOCAL_PORT,
                                            UIP_PROTO_UDP) == NULL);
  reply_received = false;
  len = build_ipv4_udp(frame, &serveraddr, &hostaddr, SERVER_PORT,
                       LOCAL_PORT, (const uint8_t *)REPLY, strlen(REPLY));
  ip64_eth_interface_input(frame, len);
  /* Nothing to wait for here: the absence of a delivery is what is asserted,
     so this is the one wait that has to run its full course. */
  etimer_set(&settle_timer, SETTLE_TIME);
  PT_WAIT_UNTIL(&unit_test_pt, etimer_expired(&settle_timer));

  UNIT_TEST_ASSERT(!reply_received);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_ip64_process, ev, data)
{
  static struct etimer startup_timer;
  static struct ip64_eth_addr eth_addr = {
    { 0x02, 0x11, 0x22, 0x33, 0x44, 0x55 }
  };

  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  uip_ipaddr(&hostaddr, 192, 168, 1, 50);
  uip_ipaddr(&netmask, 255, 255, 255, 0);
  uip_ipaddr(&draddr, 192, 168, 1, 1);
  uip_ipaddr(&serveraddr, 192, 0, 2, 10);
  uip_ip6addr(&moteaddr, 0xfd00, 0, 0, 0, 0x0211, 0x2233, 0x4455, 0x6677);

  ip64_eth_addr_set(&eth_addr);
  ip64_init();
  ip64_set_hostaddr(&hostaddr);
  ip64_set_netmask(&netmask);
  ip64_set_draddr(&draddr);

  simple_udp_register(&conn, LOCAL_PORT, NULL, SERVER_PORT, udp_received);
  simple_udp_register(&dnsconn, DNS_LOCAL_PORT, NULL, DNS_PORT, dns_received);
  /* Accepts traffic from any port, as an inbound service would. */
  simple_udp_register(&serviceconn, SERVICE_PORT, NULL, 0, service_received);

  /* Let the IPv6 addresses leave the tentative state. */
  etimer_set(&startup_timer, CLOCK_SECOND);
  PROCESS_WAIT_UNTIL(etimer_expired(&startup_timer));

  UNIT_TEST_RUN(arp_resolution);

  /* Answer the request that arp_resolution triggered. Every test that
     follows needs the router in the ARP cache. */
  inject_arp_reply();

  UNIT_TEST_RUN(udp_round_trip);
  UNIT_TEST_RUN(forwarded_flow);
  UNIT_TEST_RUN(dns64_rewrite);
  UNIT_TEST_RUN(icmp_echo);
  UNIT_TEST_RUN(inbound_ports);

  if(!UNIT_TEST_PASSED(arp_resolution) ||
     !UNIT_TEST_PASSED(udp_round_trip) ||
     !UNIT_TEST_PASSED(forwarded_flow) ||
     !UNIT_TEST_PASSED(dns64_rewrite) ||
     !UNIT_TEST_PASSED(icmp_echo) ||
     !UNIT_TEST_PASSED(inbound_ports)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
