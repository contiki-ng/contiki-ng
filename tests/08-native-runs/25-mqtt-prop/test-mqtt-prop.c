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
 *         Unit test of the MQTT-5 input property decoders.
 *
 *         The decoders take the length of each property from the received
 *         packet. A declared length that exceeds the bytes actually received
 *         must be rejected rather than used to read past the input, so each
 *         decoder is fed both a well-formed property and a property whose
 *         declared length overruns the input.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "net/app-layer/mqtt/mqtt.h"
#include "net/app-layer/mqtt/mqtt-prop.h"
#include "unit-test.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "test-mqtt-prop"
#define LOG_LEVEL LOG_LEVEL_NONE

PROCESS(test_process, "test");
AUTOSTART_PROCESSES(&test_process);

static struct mqtt_connection conn;

/*---------------------------------------------------------------------------*/
/*
 * A property that is rejected still advances the parse position past its
 * property identifier, so this is what a rejected property section consumes.
 */
#define PROP_REJECTED 1

/*
 * Present props[0..props_len-1] as the property section of a received PUBLISH,
 * of which only the first recv_len bytes have arrived. Declaring more property
 * bytes than have been received is how a malformed packet drives a decoder
 * past the input, and is also what a property section straddling a full input
 * buffer looks like.
 *
 * Returns the number of property bytes consumed by a full parse: the length of
 * the section when every property was accepted, and PROP_REJECTED when the
 * first property was rejected.
 */
static uint32_t
parse_props(const uint8_t *props, uint8_t props_len, uint8_t recv_len)
{
  uint8_t *section;

  memset(&conn, 0, sizeof(conn));

  /* The property section is preceded by its length, encoded as a Variable
   * Byte Integer. Lengths below 128 occupy a single byte.
   */
  section = &conn.in_packet.payload[MQTT_INPUT_BUFF_SIZE - (props_len + 1)];
  section[0] = props_len;
  memcpy(&section[1], props, props_len);

  conn.in_packet.fhdr = MQTT_FHDR_MSG_TYPE_PUBLISH;
  conn.in_packet.payload_start = section;
  conn.in_packet.remaining_length = MQTT_INPUT_BUFF_SIZE;
  /* Bytes received: everything up to and including the encoded length, plus
   * the requested number of property bytes.
   */
  conn.in_packet.payload_pos =
    (section - conn.in_packet.payload) + 1 + recv_len;

  mqtt_prop_decode_input_props(&conn);
  if(!conn.in_packet.has_props) {
    return 0;
  }

  /*
   * Walk every property in the section. This is the only public entry point
   * that dispatches to all of the decoders; the DEBUG_MQTT gating inside it
   * affects printing only, not decoding.
   */
  mqtt_prop_print_input_props(&conn);

  return conn.in_packet.curr_props_pos - conn.in_packet.props_start;
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_utf8, "UTF-8 string property");
UNIT_TEST(test_utf8)
{
  /*
   * MQTT_VHDR_PROP_CONTENT_TYPE, then a 20-character string. The string is
   * short enough to fit the destination buffer, so nothing but the input
   * length can reject it once fewer bytes than that have been received.
   */
  static const uint8_t str[] = {
    0x03, 0x00, 0x14,
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
    'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't'
  };

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(parse_props(str, sizeof(str), sizeof(str)) == sizeof(str));
  /* The declared string extends past the received input */
  UNIT_TEST_ASSERT(parse_props(str, sizeof(str), 6) == PROP_REJECTED);
  /* Only one byte of the two-byte length prefix has been received */
  UNIT_TEST_ASSERT(parse_props(str, sizeof(str), 2) == PROP_REJECTED);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_fixed_len_int, "Fixed-length integer property");
UNIT_TEST(test_fixed_len_int)
{
  /* MQTT_VHDR_PROP_SESS_EXP_INT, a four-byte integer */
  static const uint8_t valid[] = { 0x11, 0x00, 0x00, 0x00, 0x2A };

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(parse_props(valid, sizeof(valid), sizeof(valid)) ==
                   sizeof(valid));
  /* The same property, with the last two value bytes not yet received */
  UNIT_TEST_ASSERT(parse_props(valid, sizeof(valid), sizeof(valid) - 2) ==
                   PROP_REJECTED);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_binary_data, "Binary data property");
UNIT_TEST(test_binary_data)
{
  /* MQTT_VHDR_PROP_CORRELATION_DATA, then 20 bytes of data */
  static const uint8_t data_prop[] = {
    0x09, 0x00, 0x14,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
    0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0x12, 0x13
  };
  /*
   * A declared length of 259 bytes, which a uint8_t data_len truncates to the
   * 3 bytes that follow it.
   */
  static const uint8_t truncating[] = { 0x09, 0x01, 0x03, 0xAA, 0xBB, 0xCC };

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(parse_props(data_prop, sizeof(data_prop),
                               sizeof(data_prop)) == sizeof(data_prop));
  /* The declared data extends past the received input */
  UNIT_TEST_ASSERT(parse_props(data_prop, sizeof(data_prop), 6) ==
                   PROP_REJECTED);
  /* Only one byte of the two-byte length prefix has been received */
  UNIT_TEST_ASSERT(parse_props(data_prop, sizeof(data_prop), 2) ==
                   PROP_REJECTED);
  UNIT_TEST_ASSERT(parse_props(truncating, sizeof(truncating),
                               sizeof(truncating)) == PROP_REJECTED);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_utf8_pair, "UTF-8 string pair property");
UNIT_TEST(test_utf8_pair)
{
  /* MQTT_VHDR_PROP_USER_PROP, then the string pair "ab" / "cd" */
  static const uint8_t valid[] = {
    0x26, 0x00, 0x02, 'a', 'b', 0x00, 0x02, 'c', 'd'
  };
  /*
   * A first string whose declared length places the second length prefix past
   * the received input. This is the case that read buf_in[len1 + 2] before any
   * bounds check, with len1 taken from the packet.
   */
  static const uint8_t overlong_first[] = {
    0x26, 0x00, 0x0C, 'a', 'b'
  };
  /* A well-formed first string, but a second one that overruns the input */
  static const uint8_t overlong_second[] = {
    0x26, 0x00, 0x02, 'a', 'b', 0x00, 0x0C, 'c', 'd'
  };

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(parse_props(valid, sizeof(valid), sizeof(valid)) ==
                   sizeof(valid));
  UNIT_TEST_ASSERT(parse_props(overlong_first, sizeof(overlong_first),
                               sizeof(overlong_first)) == PROP_REJECTED);
  UNIT_TEST_ASSERT(parse_props(overlong_second, sizeof(overlong_second),
                               sizeof(overlong_second)) == PROP_REJECTED);
  /* Truncated after the first string, before the second length prefix */
  UNIT_TEST_ASSERT(parse_props(valid, sizeof(valid), 5) == PROP_REJECTED);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_vbi, "Variable Byte Integer property");
UNIT_TEST(test_vbi)
{
  /* MQTT_VHDR_PROP_SUB_ID, then the value 1 in a single byte */
  static const uint8_t valid[] = { 0x0B, 0x01 };
  /*
   * A Variable Byte Integer whose continuation bits ask for more bytes than
   * the input holds.
   */
  static const uint8_t unterminated[] = { 0x0B, 0x80, 0x80, 0x80 };

  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(parse_props(valid, sizeof(valid), sizeof(valid)) ==
                   sizeof(valid));
  UNIT_TEST_ASSERT(parse_props(unterminated, sizeof(unterminated), 2) ==
                   PROP_REJECTED);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(test_utf8);
  UNIT_TEST_RUN(test_fixed_len_int);
  UNIT_TEST_RUN(test_binary_data);
  UNIT_TEST_RUN(test_utf8_pair);
  UNIT_TEST_RUN(test_vbi);

  if(!UNIT_TEST_PASSED(test_utf8) ||
     !UNIT_TEST_PASSED(test_fixed_len_int) ||
     !UNIT_TEST_PASSED(test_binary_data) ||
     !UNIT_TEST_PASSED(test_utf8_pair) ||
     !UNIT_TEST_PASSED(test_vbi)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
