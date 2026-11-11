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
 *         Unit tests for the CoAP message parser, block handling and
 *         client APIs.
 */

#include "contiki.h"
#include "unit-test.h"
#include "coap.h"
#include "coap-block1.h"
#include "coap-request-state.h"
#include "coap-callback-api.h"
#include "test-transport.h"

#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "CoAP unit tests");
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
/*
 * Builds a CoAP POST request with a single option, numbered from 13 to
 * 268, whose value is the value_len bytes at value, followed by
 * payload_len payload bytes. Returns the message length.
 */
static uint16_t
build_option_message(uint16_t option, const uint8_t *value,
                     uint8_t value_len, size_t payload_len)
{
  uint16_t len = 0;
  size_t i;

  buffer[len++] = (1 << 6);          /* Version 1, type CON, token length 0. */
  buffer[len++] = COAP_POST;         /* Code. */
  buffer[len++] = 0x12;              /* Message ID, high byte. */
  buffer[len++] = 0x34;              /* Message ID, low byte. */

  /* The option delta is encoded as 13 plus an extended byte. */
  buffer[len++] = (13 << 4) | value_len;
  buffer[len++] = option - 13;
  memcpy(&buffer[len], value, value_len);
  len += value_len;

  buffer[len++] = 0xFF;              /* Payload marker. */

  payload_start = len;
  for(i = 0; i < payload_len; i++) {
    buffer[len++] = 'a' + (i % 26);
  }

  return len;
}
/*
 * Builds a CoAP POST request with a Block1 option whose value is
 * block1, encoded in the fewest bytes, followed by payload_len payload
 * bytes. Returns the message length.
 */
static uint16_t
build_block1_message(uint32_t block1, size_t payload_len)
{
  uint8_t value[4];
  uint8_t value_len;
  uint8_t i;

  for(value_len = 0; value_len < 4 && (block1 >> (8 * value_len)); value_len++);
  for(i = 0; i < value_len; i++) {
    value[i] = block1 >> (8 * (value_len - 1 - i));
  }

  return build_option_message(COAP_OPTION_BLOCK1, value, value_len,
                              payload_len);
}
/*---------------------------------------------------------------------------*/
#define BLOCK1_TARGET_LEN 64
#define BLOCK1_GUARD_LEN  32

/*
 * The reassembly buffer handed to coap_block1_handler(), with guard
 * areas before and after it to detect a write outside the buffer.
 */
static uint8_t block1_area[BLOCK1_GUARD_LEN + BLOCK1_TARGET_LEN +
                           BLOCK1_GUARD_LEN];
#define BLOCK1_TARGET (&block1_area[BLOCK1_GUARD_LEN])

static bool
block1_guards_intact(void)
{
  size_t i;

  for(i = 0; i < BLOCK1_GUARD_LEN; i++) {
    if(block1_area[i] != CANARY ||
       block1_area[BLOCK1_GUARD_LEN + BLOCK1_TARGET_LEN + i] != CANARY) {
      return false;
    }
  }
  return true;
}
/*
 * Passes a parsed request to coap_block1_handler(), with fresh guard
 * areas around the target. Returns the handler's result.
 */
static int
call_block1_handler(coap_message_t *request, size_t *assembled_len)
{
  coap_message_t response;

  memset(block1_area, CANARY, sizeof(block1_area));
  *assembled_len = 0;

  coap_init_message(&response, COAP_TYPE_ACK, CONTENT_2_05, request->mid);
  return coap_block1_handler(request, &response, BLOCK1_TARGET,
                             assembled_len, BLOCK1_TARGET_LEN);
}
/*
 * Parses a request with the given Block1 option value and payload length
 * and passes it to coap_block1_handler(). Returns the handler's result.
 */
static int
run_block1(uint32_t block1, size_t payload_len, size_t *assembled_len)
{
  coap_message_t request;
  uint16_t len;

  len = build_block1_message(block1, payload_len);
  if(coap_parse_message(&request, buffer, len) != NO_ERROR) {
    return -2;
  }

  return call_block1_handler(&request, assembled_len);
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
/*
 * An offset of 0xFFFFFFF0 plus a 16-byte payload wraps to zero in 32-bit
 * arithmetic. The parser no longer produces such an offset, so it is set
 * directly to test the handler's own check.
 */
UNIT_TEST_REGISTER(test_block1_offset_wrap_is_rejected,
                   "a Block1 offset that wraps with the payload is rejected");
UNIT_TEST(test_block1_offset_wrap_is_rejected)
{
  coap_message_t request;
  size_t assembled_len;
  uint16_t len;

  UNIT_TEST_BEGIN();

  len = build_block1_message(1 << 4, 16);
  UNIT_TEST_ASSERT(coap_parse_message(&request, buffer, len) == NO_ERROR);
  request.block1_offset = 0xFFFFFFF0;

  UNIT_TEST_ASSERT(call_block1_handler(&request, &assembled_len) == -1);
  UNIT_TEST_ASSERT(coap_status_code == REQUEST_ENTITY_TOO_LARGE_4_13);
  UNIT_TEST_ASSERT(assembled_len == 0);
  UNIT_TEST_ASSERT(block1_guards_intact());

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* A final block that ends exactly at the end of the buffer is accepted. */
UNIT_TEST_REGISTER(test_block1_last_block_fills_buffer,
                   "a final Block1 block that fills the buffer is accepted");
UNIT_TEST(test_block1_last_block_fills_buffer)
{
  size_t assembled_len;

  UNIT_TEST_BEGIN();

  /* Block number 3 of size 16, no more blocks: offset 48, 16 bytes. */
  UNIT_TEST_ASSERT(run_block1(3 << 4, 16, &assembled_len) == 0);
  UNIT_TEST_ASSERT(assembled_len == BLOCK1_TARGET_LEN);
  UNIT_TEST_ASSERT(memcmp(BLOCK1_TARGET + 48, &buffer[payload_start], 16)
                   == 0);
  UNIT_TEST_ASSERT(block1_guards_intact());

  /* One more byte would not fit. */
  UNIT_TEST_ASSERT(run_block1(3 << 4, 17, &assembled_len) == -1);
  UNIT_TEST_ASSERT(block1_guards_intact());

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * RFC 7959 limits a Block option value to three bytes, and reserves the
 * block size exponent 7. Both Block1 and Block2 are checked.
 */
UNIT_TEST_REGISTER(test_parse_rejects_malformed_block_options,
                   "an over-long Block option or SZX 7 is rejected");
UNIT_TEST(test_parse_rejects_malformed_block_options)
{
  static const uint8_t four_bytes[] = { 0xFF, 0xFF, 0xFF, 0xF0 };
  static const uint8_t four_bytes_small[] = { 0x00, 0x00, 0x00, 0x10 };
  static const uint8_t szx_7[] = { 0x17 };
  static const uint8_t three_bytes[] = { 0xFF, 0xFF, 0xF6 };
  coap_message_t message;
  uint16_t len;

  UNIT_TEST_BEGIN();

  /* The longest value allowed, with the largest block size. */
  len = build_option_message(COAP_OPTION_BLOCK1, three_bytes, 3, 1);
  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len) == NO_ERROR);
  UNIT_TEST_ASSERT(message.block1_num == 0xFFFFF);
  UNIT_TEST_ASSERT(message.block1_more == 0);
  UNIT_TEST_ASSERT(message.block1_size == 1024);
  UNIT_TEST_ASSERT(message.block1_offset == 0xFFFFF0UL << 6);
  len = build_option_message(COAP_OPTION_BLOCK2, three_bytes, 3, 1);
  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len) == NO_ERROR);
  UNIT_TEST_ASSERT(message.block2_num == 0xFFFFF);
  UNIT_TEST_ASSERT(message.block2_size == 1024);
  UNIT_TEST_ASSERT(message.block2_offset == 0xFFFFF0UL << 6);

  /* An empty value is block 0 of 16 bytes. */
  len = build_option_message(COAP_OPTION_BLOCK1, three_bytes, 0, 1);
  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len) == NO_ERROR);
  UNIT_TEST_ASSERT(coap_is_option(&message, COAP_OPTION_BLOCK1));
  UNIT_TEST_ASSERT(message.block1_num == 0);
  UNIT_TEST_ASSERT(message.block1_more == 0);
  UNIT_TEST_ASSERT(message.block1_size == 16);
  UNIT_TEST_ASSERT(message.block1_offset == 0);

  /*
   * A value longer than three bytes is outside the range the option is
   * defined over, so it is an unrecognized critical option: 4.02.
   */
  len = build_option_message(COAP_OPTION_BLOCK1, four_bytes, 4, 16);
  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len)
                   == BAD_OPTION_4_02);
  len = build_option_message(COAP_OPTION_BLOCK2, four_bytes, 4, 16);
  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len)
                   == BAD_OPTION_4_02);

  /* The length is checked, not just the value it encodes. */
  len = build_option_message(COAP_OPTION_BLOCK1, four_bytes_small, 4, 1);
  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len)
                   == BAD_OPTION_4_02);

  /* The reserved block size exponent 7 is a format error: 4.00. */
  len = build_option_message(COAP_OPTION_BLOCK1, szx_7, 1, 1);
  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len)
                   == BAD_REQUEST_4_00);
  len = build_option_message(COAP_OPTION_BLOCK2, szx_7, 1, 1);
  UNIT_TEST_ASSERT(coap_parse_message(&message, buffer, len)
                   == BAD_REQUEST_4_00);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* The Block setters do not encode the reserved 2048-byte block size. */
UNIT_TEST_REGISTER(test_block_setters_reject_2048,
                   "the Block setters accept 1024 but not 2048 bytes");
UNIT_TEST(test_block_setters_reject_2048)
{
  coap_message_t message;

  UNIT_TEST_BEGIN();

  coap_init_message(&message, COAP_TYPE_CON, COAP_POST, 0x1234);
  UNIT_TEST_ASSERT(coap_set_header_block2(&message, 0, 0, 2048) == 0);
  UNIT_TEST_ASSERT(coap_set_header_block2(&message, 0, 0, 1024) == 1);
  UNIT_TEST_ASSERT(coap_set_header_block1(&message, 0, 0, 2048) == 0);
  UNIT_TEST_ASSERT(coap_set_header_block1(&message, 0, 0, 1024) == 1);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * Serializes into buffer a 2.05 response with the given message ID,
 * carrying a Block2 option, unless size is 0, and payload_len bytes of
 * payload. The payload is set directly, as a remote server could send
 * more than COAP_MAX_CHUNK_SIZE. Returns the message length, or 0 on
 * failure.
 */
#define MAX_TEST_PAYLOAD (2 * COAP_MAX_BLOCK_SIZE)

static size_t
serialize_block2_response(uint16_t mid, uint32_t num, uint8_t more,
                          uint16_t size, uint16_t payload_len)
{
  static uint8_t payload[MAX_TEST_PAYLOAD];
  coap_message_t message;

  if(payload_len > sizeof(payload)) {
    return 0;
  }
  coap_init_message(&message, COAP_TYPE_ACK, CONTENT_2_05, mid);
  if(size > 0 && !coap_set_header_block2(&message, num, more, size)) {
    return 0;
  }
  memset(payload, 'b', payload_len);
  message.payload = payload;
  message.payload_len = payload_len;
  return coap_serialize_message(&message, buffer);
}
/*---------------------------------------------------------------------------*/
/*
 * Builds a response as serialize_block2_response() does, and parses it
 * into response as the client APIs receive it.
 */
static bool
build_block2_response_len(coap_message_t *response, uint32_t num,
                          uint8_t more, uint16_t size, uint16_t payload_len)
{
  size_t len = serialize_block2_response(0x1234, num, more, size,
                                         payload_len);

  return len > 0 &&
    coap_parse_message(response, buffer, len) == NO_ERROR;
}
/*---------------------------------------------------------------------------*/
/* As above, with a full block of payload, up to MAX_TEST_PAYLOAD bytes. */
static bool
build_block2_response(coap_message_t *response, uint32_t num, uint8_t more,
                      uint16_t size)
{
  return build_block2_response_len(response, num, more, size,
                                   MIN(size, MAX_TEST_PAYLOAD));
}
/*---------------------------------------------------------------------------*/
/*
 * A client that can take larger blocks than the server sends must
 * request the following blocks in the server's block size, since block
 * numbers depend on it.
 */
UNIT_TEST_REGISTER(test_block2_follows_server_size,
                   "the client requests blocks in the server's block size");
UNIT_TEST(test_block2_follows_server_size)
{
  coap_request_state_t state;
  coap_message_t response;

  UNIT_TEST_BEGIN();

  coap_request_state_init_block2(&state);
  state.block_size = 1024;

  UNIT_TEST_ASSERT(build_block2_response(&response, 0, 1, 64));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(state.block_size == 64);
  UNIT_TEST_ASSERT(state.block_num == 1);
  UNIT_TEST_ASSERT(state.more == 1);

  UNIT_TEST_ASSERT(build_block2_response(&response, 1, 0, 64));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(state.block_num == 2);
  UNIT_TEST_ASSERT(state.more == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* A block at another offset than the next one is not accepted. */
UNIT_TEST_REGISTER(test_block2_rejects_wrong_offset,
                   "the client rejects a block at the wrong offset");
UNIT_TEST(test_block2_rejects_wrong_offset)
{
  coap_request_state_t state;
  coap_message_t response;

  UNIT_TEST_BEGIN();

  coap_request_state_init_block2(&state);
  state.block_size = 64;

  UNIT_TEST_ASSERT(build_block2_response(&response, 0, 1, 64));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));

  /* Block 1 of 1024 bytes starts at 1024, not at 64. */
  UNIT_TEST_ASSERT(build_block2_response(&response, 1, 1, 1024));
  UNIT_TEST_ASSERT(!coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(build_block2_response(&response, 2, 1, 64));
  UNIT_TEST_ASSERT(!coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(state.block_num == 1);
  UNIT_TEST_ASSERT(state.block_size == 64);

  /* Nor is a block that has already been received. */
  UNIT_TEST_ASSERT(build_block2_response(&response, 0, 1, 64));
  UNIT_TEST_ASSERT(!coap_request_state_next_block2(&state, &response));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * A block larger than requested is accepted, but the parser keeps only
 * COAP_MAX_CHUNK_SIZE bytes of it. The client must deliver the requested
 * size and request the rest of the block, rather than skip to the end of
 * the server's block.
 */
#if MAX_TEST_PAYLOAD <= COAP_MAX_CHUNK_SIZE
#error "test_block2_accepts_larger_block needs a payload that gets truncated"
#endif
UNIT_TEST_REGISTER(test_block2_accepts_larger_block,
                   "the client keeps the requested part of a larger block");
UNIT_TEST(test_block2_accepts_larger_block)
{
  coap_request_state_t state;
  coap_message_t response;

  UNIT_TEST_BEGIN();

  coap_request_state_init_block2(&state);
  UNIT_TEST_ASSERT(state.block_size == COAP_MAX_BLOCK_SIZE);

  UNIT_TEST_ASSERT(build_block2_response(&response, 0, 1,
                                         2 * COAP_MAX_BLOCK_SIZE));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(response.payload_len == COAP_MAX_BLOCK_SIZE);
  UNIT_TEST_ASSERT(state.block_size == COAP_MAX_BLOCK_SIZE);
  UNIT_TEST_ASSERT(state.block_num == 1);
  UNIT_TEST_ASSERT(state.more == 1);

  /* The rest of a larger final block is requested as well. */
  coap_request_state_init_block2(&state);
  UNIT_TEST_ASSERT(build_block2_response(&response, 0, 0,
                                         2 * COAP_MAX_BLOCK_SIZE));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(response.payload_len == COAP_MAX_BLOCK_SIZE);
  UNIT_TEST_ASSERT(state.block_num == 1);
  UNIT_TEST_ASSERT(state.more == 1);

  /*
   * A larger final block that fits the requested size ends the transfer,
   * also when it fills the requested size exactly.
   */
  coap_request_state_init_block2(&state);
  UNIT_TEST_ASSERT(build_block2_response_len(&response, 0, 0,
                                             2 * COAP_MAX_BLOCK_SIZE, 10));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(response.payload_len == 10);
  UNIT_TEST_ASSERT(state.more == 0);

  coap_request_state_init_block2(&state);
  UNIT_TEST_ASSERT(build_block2_response_len(&response, 0, 0,
                                             2 * COAP_MAX_BLOCK_SIZE,
                                             COAP_MAX_BLOCK_SIZE));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(response.payload_len == COAP_MAX_BLOCK_SIZE);
  UNIT_TEST_ASSERT(state.more == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* A block that is not the last must fill its size. */
UNIT_TEST_REGISTER(test_block2_rejects_short_block,
                   "the client rejects a short block that is not the last");
UNIT_TEST(test_block2_rejects_short_block)
{
  coap_request_state_t state;
  coap_message_t response;

  UNIT_TEST_BEGIN();

  coap_request_state_init_block2(&state);
  UNIT_TEST_ASSERT(build_block2_response_len(&response, 0, 1, 32, 20));
  UNIT_TEST_ASSERT(!coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(state.block_num == 0);

  /* The last block may be shorter. */
  UNIT_TEST_ASSERT(build_block2_response_len(&response, 0, 0, 32, 20));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* A response without a Block2 option can only be the first block. */
UNIT_TEST_REGISTER(test_block2_without_option,
                   "a response without Block2 is accepted only first");
UNIT_TEST(test_block2_without_option)
{
  coap_request_state_t state;
  coap_message_t response;

  UNIT_TEST_BEGIN();

  coap_request_state_init_block2(&state);
  UNIT_TEST_ASSERT(build_block2_response(&response, 0, 0, 0));
  UNIT_TEST_ASSERT(coap_request_state_next_block2(&state, &response));
  UNIT_TEST_ASSERT(state.more == 0);
  UNIT_TEST_ASSERT(!coap_request_state_next_block2(&state, &response));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/* The statuses that the callback API passed to the test callback. */
#define MAX_CALLBACKS 4
static coap_request_status_t callback_status[MAX_CALLBACKS];
static unsigned callback_count;

static void
record_callback(coap_callback_request_state_t *callback_state)
{
  if(callback_count < MAX_CALLBACKS) {
    callback_status[callback_count] = callback_state->state.status;
  }
  callback_count++;
}
/*---------------------------------------------------------------------------*/
/*
 * Parses the last message that the engine sent into message. Returns
 * true if it is a request.
 */
static bool
parse_sent_request(coap_message_t *message)
{
  static uint8_t copy[COAP_MAX_PACKET_SIZE];

  if(test_transport_sent_len == 0) {
    return false;
  }
  memcpy(copy, test_transport_sent, test_transport_sent_len);
  return coap_parse_message(message, copy, test_transport_sent_len)
    == NO_ERROR && message->code == COAP_GET;
}
/*---------------------------------------------------------------------------*/
/*
 * When the server sends smaller blocks than the client can take, the
 * callback API must request the next block in the server's block size.
 */
UNIT_TEST_REGISTER(test_block2_callback_api_follows_server,
                   "the callback API requests the server's block size");
UNIT_TEST(test_block2_callback_api_follows_server)
{
  static coap_callback_request_state_t callback_state;
  static coap_message_t request;
  static coap_endpoint_t ep;
  coap_message_t sent;
  size_t len;

  UNIT_TEST_BEGIN();

  memset(&ep, 0, sizeof(ep));
  uip_ip6addr(&ep.ipaddr, 0xfd00, 0, 0, 0, 0, 0, 0, 1);
  ep.port = UIP_HTONS(COAP_DEFAULT_PORT);
  callback_count = 0;

  /* The first request carries no Block2 option. */
  test_transport_sent_len = 0;
  coap_init_message(&request, COAP_TYPE_CON, COAP_GET, 0);
  coap_set_header_uri_path(&request, "chunked");
  UNIT_TEST_ASSERT(coap_send_request(&callback_state, &ep, &request,
                                     record_callback));
  UNIT_TEST_ASSERT(parse_sent_request(&sent));
  UNIT_TEST_ASSERT(!coap_is_option(&sent, COAP_OPTION_BLOCK2));

  /* The server answers with 16-byte blocks. */
  len = serialize_block2_response(sent.mid, 0, 1, 16, 16);
  UNIT_TEST_ASSERT(len > 0);
  test_transport_sent_len = 0;
  coap_receive(&ep, buffer, len);
  UNIT_TEST_ASSERT(callback_count == 1);
  UNIT_TEST_ASSERT(callback_status[0] == COAP_REQUEST_STATUS_MORE);

  /* The next request asks for block 1 of 16 bytes, at offset 16. */
  UNIT_TEST_ASSERT(parse_sent_request(&sent));
  UNIT_TEST_ASSERT(coap_is_option(&sent, COAP_OPTION_BLOCK2));
  UNIT_TEST_ASSERT(sent.block2_num == 1);
  UNIT_TEST_ASSERT(sent.block2_size == 16);

  /* The last block completes the transfer. */
  len = serialize_block2_response(sent.mid, 1, 0, 16, 16);
  UNIT_TEST_ASSERT(len > 0);
  coap_receive(&ep, buffer, len);
  UNIT_TEST_ASSERT(callback_count == 3);
  UNIT_TEST_ASSERT(callback_status[1] == COAP_REQUEST_STATUS_RESPONSE);
  UNIT_TEST_ASSERT(callback_status[2] == COAP_REQUEST_STATUS_FINISHED);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("\nRunning CoAP unit tests\n");

  coap_engine_init();

  UNIT_TEST_RUN(test_parse_payload_keeps_canary);
  UNIT_TEST_RUN(test_parse_oversized_payload_is_truncated);
  UNIT_TEST_RUN(test_parse_payload_with_null_bytes);
  UNIT_TEST_RUN(test_parse_rejects_empty_payload);
  UNIT_TEST_RUN(test_block1_offset_wrap_is_rejected);
  UNIT_TEST_RUN(test_block1_last_block_fills_buffer);
  UNIT_TEST_RUN(test_parse_rejects_malformed_block_options);
  UNIT_TEST_RUN(test_block_setters_reject_2048);
  UNIT_TEST_RUN(test_block2_follows_server_size);
  UNIT_TEST_RUN(test_block2_rejects_wrong_offset);
  UNIT_TEST_RUN(test_block2_accepts_larger_block);
  UNIT_TEST_RUN(test_block2_rejects_short_block);
  UNIT_TEST_RUN(test_block2_without_option);
  UNIT_TEST_RUN(test_block2_callback_api_follows_server);

  if(!UNIT_TEST_PASSED(test_parse_payload_keeps_canary) ||
     !UNIT_TEST_PASSED(test_parse_oversized_payload_is_truncated) ||
     !UNIT_TEST_PASSED(test_parse_payload_with_null_bytes) ||
     !UNIT_TEST_PASSED(test_parse_rejects_empty_payload) ||
     !UNIT_TEST_PASSED(test_block1_offset_wrap_is_rejected) ||
     !UNIT_TEST_PASSED(test_block1_last_block_fills_buffer) ||
     !UNIT_TEST_PASSED(test_parse_rejects_malformed_block_options) ||
     !UNIT_TEST_PASSED(test_block_setters_reject_2048) ||
     !UNIT_TEST_PASSED(test_block2_follows_server_size) ||
     !UNIT_TEST_PASSED(test_block2_rejects_wrong_offset) ||
     !UNIT_TEST_PASSED(test_block2_accepts_larger_block) ||
     !UNIT_TEST_PASSED(test_block2_rejects_short_block) ||
     !UNIT_TEST_PASSED(test_block2_without_option) ||
     !UNIT_TEST_PASSED(test_block2_callback_api_follows_server)) {
    printf("=check-me= FAILED\n");
  } else {
    printf("=check-me= DONE\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
