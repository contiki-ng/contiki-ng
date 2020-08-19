/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "lib/cbor/cbor.h"
#include "lib/cbor/cbor-token.h"
#include "lib/cbor/cbor-decoder.h"
#include "lib/cbor/cbor-encoder.h"
#include "services/unit-test/unit-test.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
PROCESS(cbor_test_process, "CBOR process");
AUTOSTART_PROCESSES(&cbor_test_process);
/*---------------------------------------------------------------------------*/
#define ELEMENTS_NUM(a) sizeof(a) / sizeof(uint8_t)
/*---------------------------------------------------------------------------*/
void
print_test_report(const unit_test_t *utp)
{
  printf("=check-me= ");
  if(utp->result == unit_test_failure) {
    printf("FAILED   - %s: exit at L%u\n", utp->descr, utp->exit_line);
  } else {
    printf("SUCCEEDED - %s\n", utp->descr);
  }
}
/*---------------------------------------------------------------------------*/
/* 19 0D7A # unsigned(3450) */
UNIT_TEST_REGISTER(test_case_01, "Test Case 01");
UNIT_TEST(test_case_01)
{
  static uint8_t decoder_buffer[] = {
    0x19, 0x0D, 0x7A,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_UNSIGNED);
  UNIT_TEST_ASSERT(token.integer.value == 3450);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 18 22 # unsigned(34) */
UNIT_TEST_REGISTER(test_case_02, "Test Case 02");
UNIT_TEST(test_case_02)
{
  static uint8_t decoder_buffer[] = {
    0x18, 0x22,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_UNSIGNED);
  UNIT_TEST_ASSERT(token.integer.value == 34);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 1A 000541F0 # unsigned(344560) */
UNIT_TEST_REGISTER(test_case_03, "Test Case 03");
UNIT_TEST(test_case_03)
{
  static uint8_t decoder_buffer[] = {
    0x1A, 0x00, 0x05, 0x41, 0xF0,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_UNSIGNED);
  UNIT_TEST_ASSERT(token.integer.value == 344560);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 38 63 # negative(99) */
UNIT_TEST_REGISTER(test_case_04, "Test Case 04");
UNIT_TEST(test_case_04)
{
  static uint8_t decoder_buffer[] = {
    0x38, 0x63,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_NEGATIVE);
  UNIT_TEST_ASSERT(token.integer.value == 100);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 24 # negative(4) */
UNIT_TEST_REGISTER(test_case_05, "Test Case 05");
UNIT_TEST(test_case_05)
{
  static uint8_t decoder_buffer[] = {
    0x24,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_NEGATIVE);
  UNIT_TEST_ASSERT(token.integer.value == 5);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 61    # text(1) */
/*    61 # "a" */
UNIT_TEST_REGISTER(test_case_06, "Test Case 06");
UNIT_TEST(test_case_06)
{
  static uint8_t decoder_buffer[] = {
    0x61, 0x61,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_TEXT);
  UNIT_TEST_ASSERT(token.string.length == 1);
  UNIT_TEST_ASSERT(memcmp((const char *)token.string.buffer, "a", token.string.length) == 0);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 78 1A                                   # text(26) */
/*   5468697320697320612076657279206C6F6E6720737472696E67 # "This is a very long string" */
UNIT_TEST_REGISTER(test_case_07, "Test Case 07");
UNIT_TEST(test_case_07)
{
  static uint8_t decoder_buffer[] = {
    0x78, 0x1A, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x76,
    0x65, 0x72, 0x79, 0x20, 0x6C, 0x6F, 0x6E, 0x67, 0x20, 0x73, 0x74, 0x72, 0x69,
    0x6E, 0x67,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_TEXT);
  UNIT_TEST_ASSERT(token.string.length == 26);
  UNIT_TEST_ASSERT(memcmp((const char *)token.string.buffer, "This is a very long string", token.string.length) == 0);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 7F                                      # text(INDEF) */
/*   5468697320697320612076657279206C6F6E6720737472696E67 # "This is a very long string" */
UNIT_TEST_REGISTER(test_case_08, "Test Case 08");
UNIT_TEST(test_case_08)
{
  static uint8_t decoder_buffer[] = {
    0x7F, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x76, 0x65, 0x72,
    0x79, 0x20, 0x6C, 0x6F, 0x6E, 0x67, 0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0xFF,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_TEXT);
  UNIT_TEST_ASSERT(token.string.length == 26);
  UNIT_TEST_ASSERT(memcmp((const char *)token.string.buffer, "This is a very long string", token.string.length) == 0);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 25 # negative(5) */
UNIT_TEST_REGISTER(test_case_09, "Test Case 09");
UNIT_TEST(test_case_09)
{
  static uint8_t decoder_buffer[] = {
    0x88, 0x19, 0x09, 0x92, 0x18, 0x18, 0x1A, 0x00, 0x03, 0xBB, 0x50, 0x38, 0x34, 0x25, 0x78, 0x19, 0x54,
    0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x74, 0x65, 0x78, 0x74, 0x20, 0x69, 0x6E, 0x73, 0x69, 0x64,
    0x65, 0x20, 0x61, 0x72, 0x72, 0x61, 0x79, 0x7F, 0x54, 0x68, 0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61,
    0x20, 0x73, 0x74, 0x72, 0x69, 0x6E, 0x67, 0x20, 0x77, 0x69, 0x74, 0x68, 0x20, 0x61, 0x20, 0x62, 0x72,
    0x65, 0x61, 0x6B, 0xFF, 0x27,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_ARRAY);
  UNIT_TEST_ASSERT(token.collection.length == 8);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_UNSIGNED);
  UNIT_TEST_ASSERT(token.integer.value == 2450);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_UNSIGNED);
  UNIT_TEST_ASSERT(token.integer.value == 24);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_UNSIGNED);
  UNIT_TEST_ASSERT(token.integer.value == 244560);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_NEGATIVE);
  UNIT_TEST_ASSERT(token.integer.value == 53);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_NEGATIVE);
  UNIT_TEST_ASSERT(token.integer.value == 6);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_TEXT);
  UNIT_TEST_ASSERT(token.string.length == 25);
  UNIT_TEST_ASSERT(memcmp((const char *)token.string.buffer, "This is text inside array", token.string.length) == 0);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_TEXT);
  UNIT_TEST_ASSERT(token.string.length == 29);
  UNIT_TEST_ASSERT(memcmp((const char *)token.string.buffer, "This is a string with a break", token.string.length) == 0);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_NEGATIVE);
  UNIT_TEST_ASSERT(token.integer.value == 8);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* 88                                      # array(8) */
/*    19 0992                              # unsigned(2450) */
/*    18 18                                # unsigned(24) */
/*    1A 0003BB50                          # unsigned(244560) */
/*    38 34                                # negative(52) */
/*    25                                   # negative(5) */
/*    78 19                                # text(25) */
/*       54686973206973207465787420696E73696465206172726179 # "This is text inside array" */
/*    7F                                   # text(INDEF) */
/*       54686973206973206120737472696E672077697468206120627265616BFF # "This is a string with a break" */
/*    27                                   # negative(7) */
UNIT_TEST_REGISTER(test_case_10, "Test Case 10");
UNIT_TEST(test_case_10)
{
  static uint8_t decoder_buffer[] = {
    0x25,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_NEGATIVE);
  UNIT_TEST_ASSERT(token.integer.value == 6);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* A1           # map(1) */
/*    63        # text(3) */
/*       4B6579 # "Key" */
/*    0A        # unsigned(10) */
UNIT_TEST_REGISTER(test_case_11, "Test Case 11");
UNIT_TEST(test_case_11)
{
  static uint8_t decoder_buffer[] = {
    0xA1, 0x63, 0x4B, 0x65, 0x79, 0x0A,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_MAP);
  UNIT_TEST_ASSERT(token.collection.length == 1);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_TEXT);
  UNIT_TEST_ASSERT(token.string.length == 3);
  UNIT_TEST_ASSERT(memcmp((const char *)token.string.buffer, "Key", token.string.length) == 0);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_UNSIGNED);
  UNIT_TEST_ASSERT(token.integer.value == 10);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* F4 # primitive(20) */
UNIT_TEST_REGISTER(test_case_12, "Test Case 12");
UNIT_TEST(test_case_12)
{
  static uint8_t decoder_buffer[] = {
    0xF4
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_PRIMITIVE);
  UNIT_TEST_ASSERT(token.primitive.type == CBOR_TOKEN_PRIMITIVE_FALSE);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* F5 # primitive(21) */
UNIT_TEST_REGISTER(test_case_13, "Test Case 13");
UNIT_TEST(test_case_13)
{
  static uint8_t decoder_buffer[] = {
    0xF5
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_PRIMITIVE);
  UNIT_TEST_ASSERT(token.primitive.type == CBOR_TOKEN_PRIMITIVE_TRUE);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* F6 # primitive(22) */
UNIT_TEST_REGISTER(test_case_14, "Test Case 14");
UNIT_TEST(test_case_14)
{
  static uint8_t decoder_buffer[] = {
    0xF6
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_PRIMITIVE);
  UNIT_TEST_ASSERT(token.primitive.type == CBOR_TOKEN_PRIMITIVE_NULL);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* F7 # primitive(23) */
UNIT_TEST_REGISTER(test_case_15, "Test Case 15");
UNIT_TEST(test_case_15)
{
  static uint8_t decoder_buffer[] = {
    0xF7
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_PRIMITIVE);
  UNIT_TEST_ASSERT(token.primitive.type == CBOR_TOKEN_PRIMITIVE_UNDEFINED);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* F9 3100 # primitive(12544) */
UNIT_TEST_REGISTER(test_case_16, "Test Case 16");
UNIT_TEST(test_case_16)
{
  static uint8_t decoder_buffer[] = {
    0xF9, 0x31, 0x00
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_PRIMITIVE);
  UNIT_TEST_ASSERT(token.primitive.type == CBOR_TOKEN_PRIMITIVE_HALF_PRECISION_FLOAT);
  UNIT_TEST_ASSERT(token.primitive.value == 12544);
  /** @todo Implement a way to extract half float that is cross plataform */

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* FA 7F7FFFFF # primitive(2139095039) */
UNIT_TEST_REGISTER(test_case_17, "Test Case 17");
UNIT_TEST(test_case_17)
{
  static uint8_t decoder_buffer[] = {
    0xFA, 0x7F, 0x7F, 0xFF, 0xFF,
  };

  struct cbor_object cbor;
  struct cbor_token token;
  float result;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_PRIMITIVE);
  UNIT_TEST_ASSERT(token.primitive.type == CBOR_TOKEN_PRIMITIVE_SINGLE_PRECISION_FLOAT);
  UNIT_TEST_ASSERT(token.primitive.value == 2139095039);

  /** @todo Ensure that this way of extracting float cross plataform */
  memcpy(&result, &token.primitive.value, sizeof(result));
  UNIT_TEST_ASSERT(result == 3.4028234663852886e+38);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}

/*---------------------------------------------------------------------------*/
/* FB 3FF0000000000002 # primitive(4607182418800017410) */
UNIT_TEST_REGISTER(test_case_18, "Test Case 18");
UNIT_TEST(test_case_18)
{
  static uint8_t decoder_buffer[] = {
    0xFB, 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
  };

  struct cbor_object cbor;
  struct cbor_token token;

  UNIT_TEST_BEGIN();

  cbor_object_init(&cbor, decoder_buffer, ELEMENTS_NUM(decoder_buffer));

  cbor_token_init(&token);

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 1);
  UNIT_TEST_ASSERT(token.type == CBOR_TOKEN_TYPE_PRIMITIVE);
  UNIT_TEST_ASSERT(token.primitive.type == CBOR_TOKEN_PRIMITIVE_DOUBLE_PRECISION_FLOAT);
  UNIT_TEST_ASSERT(token.primitive.value == 4607182418800017410);
  /** @todo Implement a way to extract double that is cross plataform */

  UNIT_TEST_ASSERT(cbor_decode_token(&cbor, &token) == 0);

  UNIT_TEST_END();
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cbor_test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(test_case_01);
  UNIT_TEST_RUN(test_case_02);
  UNIT_TEST_RUN(test_case_03);
  UNIT_TEST_RUN(test_case_04);
  UNIT_TEST_RUN(test_case_05);
  UNIT_TEST_RUN(test_case_06);
  UNIT_TEST_RUN(test_case_07);
  UNIT_TEST_RUN(test_case_08);
  UNIT_TEST_RUN(test_case_09);
  UNIT_TEST_RUN(test_case_10);
  UNIT_TEST_RUN(test_case_11);
  UNIT_TEST_RUN(test_case_12);
  UNIT_TEST_RUN(test_case_13);
  UNIT_TEST_RUN(test_case_14);
  UNIT_TEST_RUN(test_case_15);
  UNIT_TEST_RUN(test_case_16);
  UNIT_TEST_RUN(test_case_17);
  UNIT_TEST_RUN(test_case_18);

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
