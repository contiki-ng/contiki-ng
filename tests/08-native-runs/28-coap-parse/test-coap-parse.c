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
 *         Unit tests for the CoAP message parser.
 */

#include "contiki.h"
#include "unit-test.h"
#include "coap.h"

#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "CoAP parser unit tests");
AUTOSTART_PROCESSES(&run_tests);

#define CANARY 0xAA

/*
 * A CoAP message is built in a buffer that has a canary byte directly
 * after the message. The parser must not write to the canary, because
 * the transport is not required to supply any space beyond the message
 * itself.
 */
static uint8_t buffer[COAP_MAX_PACKET_SIZE + 64];

/* Where build_message() put the payload, so that a test can compare
   against what was written rather than recompute it. */
static uint16_t payload_start;

/*
 * Builds a minimal CoAP POST request carrying payload_len payload bytes,
 * and places a canary directly after the message. Returns the message
 * length.
 */
static uint16_t
build_message(size_t payload_len)
{
  uint16_t len = 0;
  size_t i;

  buffer[len++] = (1 << 6);          /* Version 1, type CON, token length 0. */
  buffer[len++] = COAP_POST;         /* Code. */
  buffer[len++] = 0x12;              /* Message ID, high byte. */
  buffer[len++] = 0x34;              /* Message ID, low byte. */
  buffer[len++] = 0xFF;              /* Payload marker. */

  payload_start = len;
  for(i = 0; i < payload_len; i++) {
    buffer[len++] = 'a' + (i % 26);
  }

  buffer[len] = CANARY;

  return len;
}
/*---------------------------------------------------------------------------*/
/* The parser must report the payload without writing past the message. */
UNIT_TEST_REGISTER(test_parse_payload_keeps_canary,
                   "coap_parse_message() does not write past the message");
UNIT_TEST(test_parse_payload_keeps_canary)
{
  coap_message_t message;
  uint16_t len;

  UNIT_TEST_BEGIN();

  len = build_message(10);

  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len) == NO_ERROR);
  UNIT_TEST_ASSERT(message.payload_len == 10);
  UNIT_TEST_ASSERT(memcmp(message.payload, "abcdefghij", 10) == 0);
  UNIT_TEST_ASSERT(buffer[len] == CANARY);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* An oversized payload is truncated, and the message is left intact. */
UNIT_TEST_REGISTER(test_parse_oversized_payload_is_truncated,
                   "an oversized payload is truncated without being modified");
UNIT_TEST(test_parse_oversized_payload_is_truncated)
{
  coap_message_t message;
  uint16_t len;
  uint8_t after_truncation;

  UNIT_TEST_BEGIN();

  len = build_message(COAP_MAX_CHUNK_SIZE + 8);
  /* Kept before parsing, since the parser is given this very buffer. */
  after_truncation = buffer[payload_start + COAP_MAX_CHUNK_SIZE];

  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len) == NO_ERROR);
  UNIT_TEST_ASSERT(message.payload_len == COAP_MAX_CHUNK_SIZE);
  /* The byte after the truncation point belongs to the message, and used
     to be overwritten with a null terminator. */
  UNIT_TEST_ASSERT(message.payload[COAP_MAX_CHUNK_SIZE] == after_truncation);
  UNIT_TEST_ASSERT(buffer[len] == CANARY);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* Binary payloads containing null bytes are reported by length. */
UNIT_TEST_REGISTER(test_parse_payload_with_null_bytes,
                   "a payload containing null bytes is reported by length");
UNIT_TEST(test_parse_payload_with_null_bytes)
{
  coap_message_t message;
  uint16_t len;

  UNIT_TEST_BEGIN();

  len = build_message(4);
  memcpy(&buffer[payload_start], "x\0y\0", 4);

  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len) == NO_ERROR);
  UNIT_TEST_ASSERT(message.payload_len == 4);
  UNIT_TEST_ASSERT(memcmp(message.payload, "x\0y\0", 4) == 0);
  UNIT_TEST_ASSERT(buffer[len] == CANARY);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* A payload marker has to be followed by at least one byte of payload. */
UNIT_TEST_REGISTER(test_parse_rejects_empty_payload,
                   "a payload marker with nothing after it is rejected");
UNIT_TEST(test_parse_rejects_empty_payload)
{
  coap_message_t message;
  uint16_t len;

  UNIT_TEST_BEGIN();

  /* Build a message with one payload byte, then drop that byte, which
     leaves the marker as the last byte of the message. */
  len = build_message(1) - 1;
  buffer[len] = CANARY;

  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len)
                   == BAD_REQUEST_4_00);
  UNIT_TEST_ASSERT(buffer[len] == CANARY);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("\nRunning CoAP parser unit tests\n");

  UNIT_TEST_RUN(test_parse_payload_keeps_canary);
  UNIT_TEST_RUN(test_parse_oversized_payload_is_truncated);
  UNIT_TEST_RUN(test_parse_payload_with_null_bytes);
  UNIT_TEST_RUN(test_parse_rejects_empty_payload);

  if(!UNIT_TEST_PASSED(test_parse_payload_keeps_canary) ||
     !UNIT_TEST_PASSED(test_parse_oversized_payload_is_truncated) ||
     !UNIT_TEST_PASSED(test_parse_payload_with_null_bytes) ||
     !UNIT_TEST_PASSED(test_parse_rejects_empty_payload)) {
    printf("=check-me= FAILED\n");
  } else {
    printf("=check-me= DONE\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
