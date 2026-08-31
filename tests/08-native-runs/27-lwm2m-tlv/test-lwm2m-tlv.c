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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

/**
 * \file
 *         Unit tests for the LwM2M TLV Float conversion.
 *
 *         The vectors are written as raw IEEE 754 encodings rather than
 *         built from host floating-point literals, so that what is under
 *         test is the decoder and not the build machine's own conversions.
 *         Expected values follow the conversion's documented policy:
 *         truncation toward zero, saturation at INT32_MAX where the value
 *         does not fit, zero where every significant bit is shifted out,
 *         and rejection of infinity, NaN and malformed lengths.
 */

#include "contiki.h"
#include "lwm2m-tlv.h"
#include "unit-test.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* The fixpoint scale the LwM2M engine uses (LWM2M_FLOAT32_BITS). */
#define BITS 10

PROCESS(run_tests, "LwM2M TLV unit tests");
AUTOSTART_PROCESSES(&run_tests);

struct float_case {
  uint8_t bytes[8];
  size_t size;
  int32_t value;
  const char *description;
};
/*---------------------------------------------------------------------------*/
static bool
check(const struct float_case *c, uint32_t length)
{
  lwm2m_tlv_t tlv;
  int32_t value = 0x5a5a5a5a;
  size_t size;

  memset(&tlv, 0, sizeof(tlv));
  tlv.type = LWM2M_TLV_TYPE_RESOURCE;
  tlv.id = 1;
  tlv.length = length;
  tlv.value = c->bytes;

  size = lwm2m_tlv_float32_to_fix(&tlv, &value, BITS);
  if(size != c->size) {
    printf("%s: size %u, expected %u\n", c->description,
           (unsigned)size, (unsigned)c->size);
    return false;
  }
  /* A rejected value must leave nothing usable behind. */
  if(value != (c->size == 0 ? 0 : c->value)) {
    printf("%s: value %" PRId32 ", expected %" PRId32 "\n", c->description,
           value, c->size == 0 ? 0 : c->value);
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
static const struct float_case binary32_cases[] = {
  { { 0x3f, 0x80, 0x00, 0x00 },
    4, 1024, "1.0" },
  { { 0x41, 0xbc, 0x00, 0x00 },
    4, 24064, "23.5" },
  { { 0xc0, 0x88, 0x00, 0x00 },
    4, -4352, "-4.25" },
  { { 0x3f, 0x00, 0x00, 0x00 },
    4, 512, "0.5" },
  { { 0xbf, 0x00, 0x00, 0x00 },
    4, -512, "-0.5" },
  { { 0x44, 0x7a, 0x00, 0x00 },
    4, 1024000, "1000.0" },
  { { 0x3a, 0x83, 0x12, 0x6f },
    4, 1, "0.001" },
  { { 0x00, 0x00, 0x00, 0x00 },
    4, 0, "+0.0" },
  { { 0x80, 0x00, 0x00, 0x00 },
    4, 0, "-0.0" },
  { { 0x00, 0x00, 0x00, 0x01 },
    4, 0, "smallest subnormal" },
  { { 0x00, 0x7f, 0xff, 0xff },
    4, 0, "largest subnormal" },
  { { 0x00, 0x80, 0x00, 0x00 },
    4, 0, "smallest normal" },
  { { 0x7f, 0x7f, 0xff, 0xff },
    4, INT32_MAX, "largest finite" },
  { { 0xff, 0x7f, 0xff, 0xff },
    4, -INT32_MAX, "-largest finite" },
  { { 0x39, 0x80, 0x00, 0x00 },
    4, 0, "shift e=-32" },
  { { 0x39, 0xff, 0xff, 0xff },
    4, 0, "shift e=-32, full mantissa" },
  { { 0x3a, 0x00, 0x00, 0x00 },
    4, 0, "shift e=-31" },
  { { 0x3a, 0x7f, 0xff, 0xff },
    4, 0, "shift e=-31, full mantissa" },
  { { 0x3a, 0x80, 0x00, 0x00 },
    4, 1, "shift e=-30" },
  { { 0x3a, 0xff, 0xff, 0xff },
    4, 1, "shift e=-30, full mantissa" },
  { { 0x49, 0x80, 0x00, 0x00 },
    4, 1073741824, "shift e=0" },
  { { 0x49, 0xff, 0xff, 0xff },
    4, 2147483520, "shift e=0, full mantissa" },
  { { 0x59, 0x00, 0x00, 0x00 },
    4, INT32_MAX, "shift e=31" },
  { { 0x59, 0x7f, 0xff, 0xff },
    4, INT32_MAX, "shift e=31, full mantissa" },
  { { 0x59, 0x80, 0x00, 0x00 },
    4, INT32_MAX, "shift e=32" },
  { { 0x59, 0xff, 0xff, 0xff },
    4, INT32_MAX, "shift e=32, full mantissa" },
  { { 0x7f, 0x80, 0x00, 0x00 },
    0, 0, "+infinity" },
  { { 0xff, 0x80, 0x00, 0x00 },
    0, 0, "-infinity" },
  { { 0x7f, 0xc0, 0x00, 0x00 },
    0, 0, "NaN" },};
/*---------------------------------------------------------------------------*/
static const struct float_case binary64_cases[] = {
  { { 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, 1024, "1.0" },
  { { 0x40, 0x37, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, 24064, "23.5" },
  { { 0xc0, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, -4352, "-4.25" },
  { { 0x3f, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, 512, "0.5" },
  { { 0xbf, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, -512, "-0.5" },
  { { 0x40, 0x8f, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, 1024000, "1000.0" },
  { { 0x3f, 0x50, 0x62, 0x4d, 0xd2, 0xf1, 0xa9, 0xfc },
    8, 1, "0.001" },
  { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, 0, "+0.0" },
  { { 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, 0, "-0.0" },
  { { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 },
    8, 0, "smallest subnormal" },
  { { 0x00, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    8, 0, "largest subnormal" },
  { { 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    8, 0, "smallest normal" },
  { { 0x7f, 0xef, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff },
    8, INT32_MAX, "largest finite" },
  { { 0x7f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, 0, "+infinity" },
  { { 0xff, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, 0, "-infinity" },
  { { 0x7f, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
    0, 0, "NaN" },};
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_binary32,
                   "Convert binary32 Float TLVs to fixpoint");
UNIT_TEST(test_binary32)
{
  unsigned int i;

  UNIT_TEST_BEGIN();

  for(i = 0; i < sizeof(binary32_cases) / sizeof(binary32_cases[0]); i++) {
    UNIT_TEST_ASSERT(check(&binary32_cases[i], 4));
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_binary64,
                   "Convert binary64 Float TLVs to fixpoint");
UNIT_TEST(test_binary64)
{
  unsigned int i;

  UNIT_TEST_BEGIN();

  for(i = 0; i < sizeof(binary64_cases) / sizeof(binary64_cases[0]); i++) {
    UNIT_TEST_ASSERT(check(&binary64_cases[i], 8));
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_both_widths_agree,
                   "The same number decodes alike as binary32 and binary64");
UNIT_TEST(test_both_widths_agree)
{
  lwm2m_tlv_t tlv;
  int32_t from32;
  int32_t from64;
  unsigned int i;

  UNIT_TEST_BEGIN();

  /* The two tables open with the same seven numbers, by construction, and
     each is near enough that the two encodings truncate alike. */
  for(i = 0; i < 7; i++) {
    memset(&tlv, 0, sizeof(tlv));
    tlv.type = LWM2M_TLV_TYPE_RESOURCE;
    tlv.id = 1;

    tlv.length = 4;
    tlv.value = binary32_cases[i].bytes;
    UNIT_TEST_ASSERT(lwm2m_tlv_float32_to_fix(&tlv, &from32, BITS) == 4);

    tlv.length = 8;
    tlv.value = binary64_cases[i].bytes;
    UNIT_TEST_ASSERT(lwm2m_tlv_float32_to_fix(&tlv, &from64, BITS) == 8);

    UNIT_TEST_ASSERT(from32 == from64);
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_malformed_length,
                   "Reject a Float TLV that is neither four nor eight bytes");
UNIT_TEST(test_malformed_length)
{
  static const struct float_case any = {
    { 0x3f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, 0, 0, "bad length"
  };
  uint32_t length;

  UNIT_TEST_BEGIN();

  for(length = 0; length <= 16; length++) {
    if(length == 4 || length == 8) {
      continue;
    }
    UNIT_TEST_ASSERT(check(&any, length));
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");

  UNIT_TEST_RUN(test_binary32);
  UNIT_TEST_RUN(test_binary64);
  UNIT_TEST_RUN(test_both_widths_agree);
  UNIT_TEST_RUN(test_malformed_length);

  if(!UNIT_TEST_PASSED(test_binary32)
     || !UNIT_TEST_PASSED(test_binary64)
     || !UNIT_TEST_PASSED(test_both_widths_agree)
     || !UNIT_TEST_PASSED(test_malformed_length)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
