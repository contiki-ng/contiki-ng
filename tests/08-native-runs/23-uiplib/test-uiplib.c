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
 *         Unit tests for the uIP address manipulation library.
 * \author
 *         Nicolas Tsiftes <nvt@ri.se>
 */

#include "contiki.h"
#include "unit-test.h"
#include "net/ipv6/uiplib.h"
#include "net/ipv6/ip64-addr.h"

#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "uiplib unit tests");
AUTOSTART_PROCESSES(&run_tests);

/*---------------------------------------------------------------------------*/
/* Helper: compare a parsed address against expected raw bytes. */
static int
parse_and_check(const char *str, const uint8_t *expected)
{
  uip_ip6addr_t addr;
  memset(&addr, 0xAA, sizeof(addr));
  if(!uiplib_ip6addrconv(str, &addr)) {
    return 0;
  }
  return memcmp(addr.u8, expected, 16) == 0;
}
/*---------------------------------------------------------------------------*/
/* IPv6 parsing: standard addresses */
UNIT_TEST_REGISTER(test_parse_loopback, "Parse ::1");
UNIT_TEST(test_parse_loopback)
{
  static const uint8_t expected[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("::1", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_unspecified, "Parse ::");
UNIT_TEST(test_parse_unspecified)
{
  static const uint8_t expected[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("::", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_link_local, "Parse fe80::1");
UNIT_TEST(test_parse_link_local)
{
  static const uint8_t expected[] = {
    0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("fe80::1", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_full, "Parse full address without compression");
UNIT_TEST(test_parse_full)
{
  static const uint8_t expected[] = {
    0x20, 0x01, 0x0d, 0xb8, 0x00, 0x01, 0x00, 0x02,
    0x00, 0x03, 0x00, 0x04, 0x00, 0x05, 0x00, 0x06
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("2001:db8:1:2:3:4:5:6", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_mid_compression, "Parse with mid-address ::");
UNIT_TEST(test_parse_mid_compression)
{
  static const uint8_t expected[] = {
    0x20, 0x01, 0x0d, 0xb8, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("2001:db8::1", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_leading_compression, "Parse with leading ::");
UNIT_TEST(test_parse_leading_compression)
{
  static const uint8_t expected[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x02
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("::1:2", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_bracketed, "Parse [bracket] notation");
UNIT_TEST(test_parse_bracketed)
{
  static const uint8_t expected[] = {
    0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("[fe80::1]", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* IPv6 parsing: RFC 4291 mixed notation (trailing IPv4 dotted-decimal) */
UNIT_TEST_REGISTER(test_parse_nat64_mixed, "Parse 64:ff9b::8.8.8.8");
UNIT_TEST(test_parse_nat64_mixed)
{
  static const uint8_t expected[] = {
    0x00, 0x64, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x08, 0x08, 0x08, 0x08
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("64:ff9b::8.8.8.8", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_nat64_mixed_rfc, "Parse 64:ff9b::192.0.2.1");
UNIT_TEST(test_parse_nat64_mixed_rfc)
{
  static const uint8_t expected[] = {
    0x00, 0x64, 0xff, 0x9b, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x02, 0x01
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("64:ff9b::192.0.2.1", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_v4mapped_mixed, "Parse ::FFFF:10.0.0.1");
UNIT_TEST(test_parse_v4mapped_mixed)
{
  static const uint8_t expected[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0xff, 0xff, 0x0a, 0x00, 0x00, 0x01
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("::FFFF:10.0.0.1", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_parse_full_mixed, "Parse full mixed d.d.d.d form");
UNIT_TEST(test_parse_full_mixed)
{
  static const uint8_t expected[] = {
    0x20, 0x01, 0x0d, 0xb8, 0x00, 0x01, 0x00, 0x02,
    0x00, 0x03, 0x00, 0x04, 0xc0, 0xa8, 0x01, 0x01
  };
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(parse_and_check("2001:db8:1:2:3:4:192.168.1.1", expected));
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* Parsing: rejection of invalid input */
UNIT_TEST_REGISTER(test_parse_reject_invalid, "Reject malformed addresses");
UNIT_TEST(test_parse_reject_invalid)
{
  uip_ip6addr_t addr;
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(uiplib_ip6addrconv("not-an-address", &addr) == 0);
  UNIT_TEST_ASSERT(uiplib_ip6addrconv("", &addr) == 0);
  UNIT_TEST_ASSERT(uiplib_ip6addrconv("xyz", &addr) == 0);
  UNIT_TEST_ASSERT(uiplib_ip6addrconv("1:2:3:4:5:6:7:8:9", &addr) == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* Printing: standard IPv6 */
UNIT_TEST_REGISTER(test_print_loopback, "Print ::1");
UNIT_TEST(test_print_loopback)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  uip_ip6addr(&addr, 0, 0, 0, 0, 0, 0, 0, 1);
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "::1") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_print_link_local, "Print fe80::1");
UNIT_TEST(test_print_link_local)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  uip_ip6addr(&addr, 0xfe80, 0, 0, 0, 0, 0, 0, 1);
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "fe80::1") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_print_full, "Print full address");
UNIT_TEST(test_print_full)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  uip_ip6addr(&addr, 0x2001, 0xdb8, 1, 2, 3, 4, 5, 6);
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "2001:db8:1:2:3:4:5:6") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* Printing: IPv4-mapped */
UNIT_TEST_REGISTER(test_print_v4mapped, "Print ::FFFF:d.d.d.d");
UNIT_TEST(test_print_v4mapped)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  uip_ip6addr(&addr, 0, 0, 0, 0, 0, 0xffff, 0x0a01, 0x0203);
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "::FFFF:10.1.2.3") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* Printing: NAT64 */
UNIT_TEST_REGISTER(test_print_nat64, "Print 64:ff9b::d.d.d.d");
UNIT_TEST(test_print_nat64)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  uip_ip6addr(&addr, 0x0064, 0xff9b, 0, 0, 0, 0, 0x0808, 0x0808);
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "64:ff9b::8.8.8.8") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_print_nat64_rfc, "Print 64:ff9b::192.0.2.1");
UNIT_TEST(test_print_nat64_rfc)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  uip_ip6addr(&addr, 0x0064, 0xff9b, 0, 0, 0, 0, 0xc000, 0x0201);
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "64:ff9b::192.0.2.1") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* Round-trip: parse then print */
UNIT_TEST_REGISTER(test_roundtrip_nat64, "Round-trip 64:ff9b::192.0.2.1");
UNIT_TEST(test_roundtrip_nat64)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(uiplib_ip6addrconv("64:ff9b::192.0.2.1", &addr));
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "64:ff9b::192.0.2.1") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_roundtrip_v4mapped, "Round-trip ::FFFF:10.0.0.1");
UNIT_TEST(test_roundtrip_v4mapped)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(uiplib_ip6addrconv("::FFFF:10.0.0.1", &addr));
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "::FFFF:10.0.0.1") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_roundtrip_plain, "Round-trip 2001:db8::1");
UNIT_TEST(test_roundtrip_plain)
{
  uip_ip6addr_t addr;
  char buf[UIPLIB_IPV6_MAX_STR_LEN];
  UNIT_TEST_BEGIN();
  UNIT_TEST_ASSERT(uiplib_ip6addrconv("2001:db8::1", &addr));
  uiplib_ipaddr_snprint(buf, sizeof(buf), &addr);
  UNIT_TEST_ASSERT(strcmp(buf, "2001:db8::1") == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* uip_nat64addr macro */
UNIT_TEST_REGISTER(test_nat64_macro, "uip_nat64addr macro");
UNIT_TEST(test_nat64_macro)
{
  uip_ip6addr_t macro_addr;
  uip_ip6addr_t manual_addr;
  UNIT_TEST_BEGIN();

  uip_nat64addr(&macro_addr, 8, 8, 8, 8);
  uip_ip6addr(&manual_addr, 0x0064, 0xff9b, 0, 0, 0, 0, 0x0808, 0x0808);
  UNIT_TEST_ASSERT(uip_ip6addr_cmp(&macro_addr, &manual_addr));

  uip_nat64addr(&macro_addr, 192, 0, 2, 1);
  uip_ip6addr(&manual_addr, 0x0064, 0xff9b, 0, 0, 0, 0, 0xc000, 0x0201);
  UNIT_TEST_ASSERT(uip_ip6addr_cmp(&macro_addr, &manual_addr));

  uip_nat64addr(&macro_addr, 10, 0, 0, 1);
  uip_ip6addr(&manual_addr, 0x0064, 0xff9b, 0, 0, 0, 0, 0x0a00, 0x0001);
  UNIT_TEST_ASSERT(uip_ip6addr_cmp(&macro_addr, &manual_addr));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* IPv4 parsing */
UNIT_TEST_REGISTER(test_parse_ipv4, "Parse IPv4 addresses");
UNIT_TEST(test_parse_ipv4)
{
  uip_ip4addr_t addr;
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(uiplib_ip4addrconv("192.168.1.1", &addr) > 0);
  UNIT_TEST_ASSERT(addr.u8[0] == 192 && addr.u8[1] == 168 &&
                   addr.u8[2] == 1 && addr.u8[3] == 1);

  UNIT_TEST_ASSERT(uiplib_ip4addrconv("10.0.0.1", &addr) > 0);
  UNIT_TEST_ASSERT(addr.u8[0] == 10 && addr.u8[1] == 0 &&
                   addr.u8[2] == 0 && addr.u8[3] == 1);

  UNIT_TEST_ASSERT(uiplib_ip4addrconv("255.255.255.255", &addr) > 0);
  UNIT_TEST_ASSERT(addr.u8[0] == 255 && addr.u8[1] == 255 &&
                   addr.u8[2] == 255 && addr.u8[3] == 255);

  UNIT_TEST_ASSERT(uiplib_ip4addrconv("0.0.0.0", &addr) > 0);
  UNIT_TEST_ASSERT(addr.u8[0] == 0 && addr.u8[1] == 0 &&
                   addr.u8[2] == 0 && addr.u8[3] == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("\nRunning uiplib unit tests\n");

  UNIT_TEST_RUN(test_parse_loopback);
  UNIT_TEST_RUN(test_parse_unspecified);
  UNIT_TEST_RUN(test_parse_link_local);
  UNIT_TEST_RUN(test_parse_full);
  UNIT_TEST_RUN(test_parse_mid_compression);
  UNIT_TEST_RUN(test_parse_leading_compression);
  UNIT_TEST_RUN(test_parse_bracketed);
  UNIT_TEST_RUN(test_parse_nat64_mixed);
  UNIT_TEST_RUN(test_parse_nat64_mixed_rfc);
  UNIT_TEST_RUN(test_parse_v4mapped_mixed);
  UNIT_TEST_RUN(test_parse_full_mixed);
  UNIT_TEST_RUN(test_parse_reject_invalid);
  UNIT_TEST_RUN(test_print_loopback);
  UNIT_TEST_RUN(test_print_link_local);
  UNIT_TEST_RUN(test_print_full);
  UNIT_TEST_RUN(test_print_v4mapped);
  UNIT_TEST_RUN(test_print_nat64);
  UNIT_TEST_RUN(test_print_nat64_rfc);
  UNIT_TEST_RUN(test_roundtrip_nat64);
  UNIT_TEST_RUN(test_roundtrip_v4mapped);
  UNIT_TEST_RUN(test_roundtrip_plain);
  UNIT_TEST_RUN(test_nat64_macro);
  UNIT_TEST_RUN(test_parse_ipv4);

  if(!UNIT_TEST_PASSED(test_parse_loopback) ||
     !UNIT_TEST_PASSED(test_parse_unspecified) ||
     !UNIT_TEST_PASSED(test_parse_link_local) ||
     !UNIT_TEST_PASSED(test_parse_full) ||
     !UNIT_TEST_PASSED(test_parse_mid_compression) ||
     !UNIT_TEST_PASSED(test_parse_leading_compression) ||
     !UNIT_TEST_PASSED(test_parse_bracketed) ||
     !UNIT_TEST_PASSED(test_parse_nat64_mixed) ||
     !UNIT_TEST_PASSED(test_parse_nat64_mixed_rfc) ||
     !UNIT_TEST_PASSED(test_parse_v4mapped_mixed) ||
     !UNIT_TEST_PASSED(test_parse_full_mixed) ||
     !UNIT_TEST_PASSED(test_parse_reject_invalid) ||
     !UNIT_TEST_PASSED(test_print_loopback) ||
     !UNIT_TEST_PASSED(test_print_link_local) ||
     !UNIT_TEST_PASSED(test_print_full) ||
     !UNIT_TEST_PASSED(test_print_v4mapped) ||
     !UNIT_TEST_PASSED(test_print_nat64) ||
     !UNIT_TEST_PASSED(test_print_nat64_rfc) ||
     !UNIT_TEST_PASSED(test_roundtrip_nat64) ||
     !UNIT_TEST_PASSED(test_roundtrip_v4mapped) ||
     !UNIT_TEST_PASSED(test_roundtrip_plain) ||
     !UNIT_TEST_PASSED(test_nat64_macro) ||
     !UNIT_TEST_PASSED(test_parse_ipv4)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
