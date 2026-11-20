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
 *         Unit tests for the source routing header that an RPL root
 *         inserts in packets that it forwards down the DODAG.
 */

#include "contiki.h"
#include "unit-test.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-sr.h"
#include "net/ipv6/uipbuf.h"
#include "net/routing/routing.h"
#if ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#endif

#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "RPL SRH insertion unit tests");
AUTOSTART_PROCESSES(&run_tests);

/*
 * The destination b has one hop, a, between it and the root. a shares
 * 14 bytes with b, so the SRH to b takes 16 bytes with padding.
 */
#define SRH_TO_B_LEN 16

/* Extension headers in the packets to b, smaller with a small buffer. */
#define EXT_TO_B_LEN (UIP_BUFSIZE >= 1280 ? 480 : 48)

/* The number of nodes in the chain below the root in the long-path test. */
#define CHAIN_LEN 32

static uip_ipaddr_t root_addr;
static uip_ipaddr_t a, b;
static void *graph;

/* What the packet looked like before the SRH was inserted. */
static uint8_t original[UIP_BUFSIZE];
static uint16_t original_len;
/*---------------------------------------------------------------------------*/
static bool
add_node(const uip_ipaddr_t *child, const uip_ipaddr_t *parent)
{
  return uip_sr_update_node(graph, child, parent,
                            UIP_SR_INFINITE_LIFETIME) != NULL;
}
/*---------------------------------------------------------------------------*/
/*
 * Build a packet from outside the DODAG to dest, with ext_len bytes
 * of Destination Options, which consist of PadN options, followed by
 * payload_len bytes with no next header.
 */
static void
build_packet(const uip_ipaddr_t *dest, uint16_t ext_len,
             uint16_t payload_len)
{
  uint8_t *p;
  uint16_t left;
  uint16_t i;

  memset(uip_buf, 0, UIP_BUFSIZE);
  UIP_IP_BUF->vtc = 0x60;
  UIP_IP_BUF->ttl = 64;
  uip_ip6addr(&UIP_IP_BUF->srcipaddr, 0x2001, 0xdb8, 0, 0, 0, 0, 0, 1);
  uip_ipaddr_copy(&UIP_IP_BUF->destipaddr, dest);

  p = UIP_IP_PAYLOAD(0);
  if(ext_len > 0) {
    UIP_IP_BUF->proto = UIP_PROTO_DESTO;
    p[0] = UIP_PROTO_NONE;
    p[1] = ext_len / 8 - 1;
    for(i = 2; i < ext_len; i += 2 + p[i + 1]) {
      left = ext_len - i - 2;
      p[i] = UIP_EXT_HDR_OPT_PADN;
      p[i + 1] = left > 255 ? 253 : left;
    }
  } else {
    UIP_IP_BUF->proto = UIP_PROTO_NONE;
  }

  for(i = 0; i < payload_len; i++) {
    p[ext_len + i] = i & 0xff;
  }

  uip_len = UIP_IPH_LEN + ext_len + payload_len;
  uipbuf_set_len_field(UIP_IP_BUF, uip_len - UIP_IPH_LEN);
  /* As the forwarding path in uip6.c leaves it. */
  uip_ext_len = ext_len;

  memcpy(original, uip_buf, uip_len);
  original_len = uip_len;
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_srh_with_ext_headers,
                   "an SRH goes in front of existing extension headers");
UNIT_TEST(test_srh_with_ext_headers)
{
  struct uip_routing_hdr *rh;

  UNIT_TEST_BEGIN();

  /*
   * A packet that fills the buffer once the SRH is in, which leaves no
   * room for its extension headers to be counted a second time.
   */
  build_packet(&b, EXT_TO_B_LEN,
               UIP_BUFSIZE - UIP_IPH_LEN - EXT_TO_B_LEN - SRH_TO_B_LEN);
  UNIT_TEST_ASSERT(NETSTACK_ROUTING.ext_header_update());

  rh = (struct uip_routing_hdr *)UIP_IP_PAYLOAD(0);
  UNIT_TEST_ASSERT(rh->len == (SRH_TO_B_LEN - 8) / 8);
  UNIT_TEST_ASSERT(rh->seg_left == 1);
  UNIT_TEST_ASSERT(uip_ipaddr_cmp(&UIP_IP_BUF->destipaddr, &a));
  UNIT_TEST_ASSERT(rh->next == UIP_PROTO_DESTO);
  UNIT_TEST_ASSERT(uip_len == UIP_BUFSIZE);
  UNIT_TEST_ASSERT(uip_ext_len == EXT_TO_B_LEN + SRH_TO_B_LEN);
  UNIT_TEST_ASSERT(memcmp(UIP_IP_PAYLOAD(SRH_TO_B_LEN),
                          &original[UIP_IPH_LEN],
                          original_len - UIP_IPH_LEN) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_srh_buffer_full,
                   "an SRH that does not fit in the buffer is not inserted");
UNIT_TEST(test_srh_buffer_full)
{
  UNIT_TEST_BEGIN();

  /* One byte short of room for the SRH. */
  build_packet(&b, 0, UIP_BUFSIZE - UIP_IPH_LEN - SRH_TO_B_LEN + 1);
  UNIT_TEST_ASSERT(!NETSTACK_ROUTING.ext_header_update());
  UNIT_TEST_ASSERT(uip_len == original_len);
  UNIT_TEST_ASSERT(memcmp(uip_buf, original, original_len) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
#if UIP_BUFSIZE >= UIP_IPH_LEN + 256 + 64
UNIT_TEST_REGISTER(test_srh_long_path,
                   "an SRH longer than 255 bytes is not truncated");
UNIT_TEST(test_srh_long_path)
{
  uip_ipaddr_t addr[CHAIN_LEN];
  struct uip_routing_hdr *rh;
  uint8_t *last_hop;
  unsigned i;

  UNIT_TEST_BEGIN();

  /*
   * Each address differs from the destination's in its first IID
   * byte, so each of the 31 intermediate hops takes 8 bytes and the
   * SRH 256.
   */
  for(i = 0; i < CHAIN_LEN; i++) {
    uip_ip6addr(&addr[i], 0xfd00, 0, 0, 0, (i + 1) << 8, 0, 0, 0);
    UNIT_TEST_ASSERT(add_node(&addr[i], i == 0 ? &root_addr : &addr[i - 1]));
  }

  build_packet(&addr[CHAIN_LEN - 1], 0, 64);
  UNIT_TEST_ASSERT(NETSTACK_ROUTING.ext_header_update());

  rh = (struct uip_routing_hdr *)UIP_IP_PAYLOAD(0);
  UNIT_TEST_ASSERT(uip_len == original_len + 256);
  UNIT_TEST_ASSERT(rh->len == (256 - 8) / 8);
  UNIT_TEST_ASSERT(rh->seg_left == CHAIN_LEN - 1);
  UNIT_TEST_ASSERT(uip_ipaddr_cmp(&UIP_IP_BUF->destipaddr, &addr[0]));
  /* The last address is the destination's, without the 8 common bytes. */
  last_hop = UIP_IP_PAYLOAD(256 - 8);
  UNIT_TEST_ASSERT(memcmp(last_hop, &addr[CHAIN_LEN - 1].u8[8], 8) == 0);
  UNIT_TEST_ASSERT(memcmp(UIP_IP_PAYLOAD(256), &original[UIP_IPH_LEN],
                          original_len - UIP_IPH_LEN) == 0);

  UNIT_TEST_END();
}
#endif /* UIP_BUFSIZE >= UIP_IPH_LEN + 256 + 64 */
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  uip_ipaddr_t prefix;
  uip_ds6_addr_t *global;

  PROCESS_BEGIN();

  printf("Run unit-test\n");

  uip_ip6addr(&prefix, 0xfd00, 0, 0, 0, 0, 0, 0, 0);
  NETSTACK_ROUTING.root_set_prefix(&prefix, NULL);
  NETSTACK_ROUTING.root_start();

  global = uip_ds6_get_global(ADDR_PREFERRED);
  if(global == NULL) {
    printf("=check-me= FAILED: no global address\n");
    PROCESS_EXIT();
  }
  uip_ipaddr_copy(&root_addr, &global->ipaddr);
#if ROUTING_CONF_RPL_CLASSIC
  graph = rpl_get_any_dag();
#else
  graph = NULL;
#endif

  uip_ip6addr(&a, 0xfd00, 0, 0, 0, 0, 0, 0, 0x100);
  uip_ip6addr(&b, 0xfd00, 0, 0, 0, 0, 0, 0, 0x200);
  if(!add_node(&a, &root_addr) || !add_node(&b, &a)) {
    printf("=check-me= FAILED: could not register a and b\n");
    PROCESS_EXIT();
  }

  UNIT_TEST_RUN(test_srh_with_ext_headers);
  UNIT_TEST_RUN(test_srh_buffer_full);
  if(!UNIT_TEST_PASSED(test_srh_with_ext_headers)
     || !UNIT_TEST_PASSED(test_srh_buffer_full)) {
    printf("=check-me= FAILED\n");
  }

#if UIP_BUFSIZE >= UIP_IPH_LEN + 256 + 64
  UNIT_TEST_RUN(test_srh_long_path);
  if(!UNIT_TEST_PASSED(test_srh_long_path)) {
    printf("=check-me= FAILED\n");
  }
#endif /* UIP_BUFSIZE >= UIP_IPH_LEN + 256 + 64 */

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
