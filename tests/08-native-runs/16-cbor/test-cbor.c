/*
 * Copyright (c) 2025, Siemens AG.
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
 *         Unit test of the CBOR library.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "contiki.h"
#include "lib/cbor.h"
#include "unit-test.h"
#include "sys/cc.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "test-cbor"
#define LOG_LEVEL LOG_LEVEL_NONE

PROCESS(test_process, "test");
AUTOSTART_PROCESSES(&test_process);

/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_write_read, "Basic write read example");
UNIT_TEST(test_write_read)
{
  static const uint8_t foo[] = { 0xA, 0xB, 0xC };
  static const char text_data[] = "Hello!";
  static const uint64_t unsigned_values[] = {
    0, 23, 24, 255, 256, 65535, 65536, 4294967295, 4294967296ULL, UINT64_MAX
  };
  static const int64_t signed_values[] = {
    0, 23, 24, 255, 256, -1, -24, -25, -255, -256, -65536, -65537,
    -4294967296, -4294967297, INT64_MAX, INT64_MIN
  };
  uint8_t buffer[128];
  size_t cbor_size;

  UNIT_TEST_BEGIN();

  /* write a CBOR array that contains a byte array with various values */
  uint8_t array_size = 0;
  {
    cbor_writer_state_t writer;
    cbor_init_writer(&writer, buffer, sizeof(buffer));
    cbor_open_array(&writer);
    /* text */
    cbor_write_text(&writer, text_data, strlen(text_data));
    array_size++;
    /* bytes */
    cbor_write_data(&writer, foo, sizeof(foo));
    array_size++;
    /* unsigned values */
    for(int i = 0; i < CC_ARRAY_LENGTH(unsigned_values); i++) {
      cbor_write_unsigned(&writer, unsigned_values[i]);
      array_size++;
    }
    /* signed values */
    for(int i = 0; i < CC_ARRAY_LENGTH(signed_values); i++) {
      cbor_write_signed(&writer, signed_values[i]);
      array_size++;
    }
    /* simple types */
    cbor_write_undefined(&writer);
    cbor_write_bool(&writer, true);
    cbor_write_bool(&writer, false);
    cbor_write_null(&writer);
    array_size += 4;
    /* map */
    cbor_open_map(&writer);
    cbor_close_map(&writer);
    array_size++;
    cbor_open_map(&writer);
    cbor_write_unsigned(&writer, 47);
    cbor_write_unsigned(&writer, 48);
    cbor_close_map(&writer);
    array_size++;
    cbor_close_array(&writer);
    cbor_size = cbor_end_writer(&writer);
  }

  LOG_DBG("CBOR (%zu bytes): ", cbor_size);
  LOG_DBG_BYTES(buffer, cbor_size);
  LOG_DBG_("\n");

  UNIT_TEST_ASSERT(cbor_size);

  static const uint8_t cbor_data[] = {
    0x98, 0x22, 0x66, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x21, 0x43, 0x0a, 0x0b,
    0x0c, 0x00, 0x17, 0x18, 0x18, 0x18, 0xff, 0x19, 0x01, 0x00, 0x19, 0xff,
    0xff, 0x1a, 0x00, 0x01, 0x00, 0x00, 0x1a, 0xff, 0xff, 0xff, 0xff, 0x1b,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x1b, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x17, 0x18, 0x18, 0x18, 0xff, 0x19,
    0x01, 0x00, 0x20, 0x37, 0x38, 0x18, 0x38, 0xfe, 0x38, 0xff, 0x39, 0xff,
    0xff, 0x3a, 0x00, 0x01, 0x00, 0x00, 0x3a, 0xff, 0xff, 0xff, 0xff, 0x3b,
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x1b, 0x7f, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0x3b, 0x7f, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xf7, 0xf5, 0xf4, 0xf6, 0xa0, 0xa1, 0x18, 0x2f, 0x18, 0x30
  };
  UNIT_TEST_ASSERT(sizeof(cbor_data) == cbor_size);
  UNIT_TEST_ASSERT(!memcmp(cbor_data, buffer, sizeof(cbor_data)));

  /* read the CBOR array and compare with our inputs */
  {
    cbor_reader_state_t reader;
    cbor_init_reader(&reader, buffer, cbor_size);
    UNIT_TEST_ASSERT(CBOR_MAJOR_TYPE_ARRAY == cbor_peek_next(&reader));
    UNIT_TEST_ASSERT(array_size == cbor_read_array(&reader));
    size_t data_size;
    UNIT_TEST_ASSERT(CBOR_MAJOR_TYPE_TEXT_STRING == cbor_peek_next(&reader));
    const char *text = cbor_read_text(&reader, &data_size);
    UNIT_TEST_ASSERT(text);
    UNIT_TEST_ASSERT(data_size == strlen(text_data));
    UNIT_TEST_ASSERT(strncmp(text_data, text, data_size) == 0);
    const uint8_t *data = cbor_read_data(&reader, &data_size);
    UNIT_TEST_ASSERT(data);
    UNIT_TEST_ASSERT(data_size == sizeof(foo));
    UNIT_TEST_ASSERT(!memcmp(foo, data, data_size));
    uint64_t value;
    for(int i = 0; i < CC_ARRAY_LENGTH(unsigned_values); i++) {
      UNIT_TEST_ASSERT(CBOR_SIZE_NONE != cbor_read_unsigned(&reader, &value));
      UNIT_TEST_ASSERT(unsigned_values[i] == value);
    }
    int64_t signed_value;
    for(int i = 0; i < CC_ARRAY_LENGTH(signed_values); i++) {
      UNIT_TEST_ASSERT(CBOR_SIZE_NONE != cbor_read_signed(&reader, &signed_value));
      UNIT_TEST_ASSERT(signed_values[i] == signed_value);
    }
    UNIT_TEST_ASSERT(CBOR_MAJOR_TYPE_SIMPLE == cbor_peek_next(&reader));
    UNIT_TEST_ASSERT(CBOR_SIMPLE_VALUE_UNDEFINED == cbor_read_simple(&reader));
    UNIT_TEST_ASSERT(CBOR_SIMPLE_VALUE_TRUE == cbor_read_simple(&reader));
    UNIT_TEST_ASSERT(CBOR_SIMPLE_VALUE_FALSE == cbor_read_simple(&reader));
    UNIT_TEST_ASSERT(CBOR_SIMPLE_VALUE_NULL == cbor_read_simple(&reader));

    UNIT_TEST_ASSERT(CBOR_MAJOR_TYPE_MAP == cbor_peek_next(&reader));
    UNIT_TEST_ASSERT(0 == cbor_read_map(&reader));

    UNIT_TEST_ASSERT(CBOR_MAJOR_TYPE_MAP == cbor_peek_next(&reader));
    UNIT_TEST_ASSERT(1 == cbor_read_map(&reader));
    UNIT_TEST_ASSERT(CBOR_SIZE_NONE != cbor_read_unsigned(&reader, &value));
    UNIT_TEST_ASSERT(47 == value);
    UNIT_TEST_ASSERT(CBOR_SIZE_NONE != cbor_read_unsigned(&reader, &value));
    UNIT_TEST_ASSERT(48 == value);

    UNIT_TEST_ASSERT(cbor_end_reader(&reader));
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * One CBOR-encoded data item per entry, covering every major type including
 * encodings the writer API cannot produce (1-byte simple values and
 * floating-point values). The float payloads are arbitrary: cbor_skip_next()
 * must advance past them by length without interpreting them.
 */
static const uint8_t enc_uint_0[] = { 0x00 };
static const uint8_t enc_uint_42[] = { 0x18, 0x2a };
static const uint8_t enc_uint_2p32[] = { 0x1b, 0x00, 0x00, 0x00, 0x01,
                                         0x00, 0x00, 0x00, 0x00 };
static const uint8_t enc_neg_1[] = { 0x20 };
static const uint8_t enc_neg_257[] = { 0x39, 0x01, 0x00 };
static const uint8_t enc_bytes_3[] = { 0x43, 0x01, 0x02, 0x03 };
static const uint8_t enc_text_4[] = { 0x64, 't', 'e', 's', 't' };
static const uint8_t enc_false[] = { 0xf4 };
static const uint8_t enc_true[] = { 0xf5 };
static const uint8_t enc_null[] = { 0xf6 };
static const uint8_t enc_undefined[] = { 0xf7 };
static const uint8_t enc_simple_32[] = { 0xf8, 0x20 };
static const uint8_t enc_float16[] = { 0xf9, 0x3c, 0x00 };
static const uint8_t enc_float32[] = { 0xfa, 0x40, 0x49, 0x0f, 0xdb };
static const uint8_t enc_float64[] = { 0xfb, 0x40, 0x09, 0x21, 0xfb,
                                       0x54, 0x44, 0x2d, 0x18 };
static const uint8_t enc_array_2[] = { 0x82, 0x01, 0x02 };
static const uint8_t enc_map_1[] = { 0xa1, 0x01, 0x02 };
/* [1, [2, 3], 4] */
static const uint8_t enc_nested_array[] = { 0x83, 0x01, 0x82, 0x02, 0x03, 0x04 };
/* {"name": "bob", 1: [1, 2, 3]} */
static const uint8_t enc_nested_map[] = {
  0xa2, 0x64, 'n', 'a', 'm', 'e', 0x63, 'b', 'o', 'b',
  0x01, 0x83, 0x01, 0x02, 0x03
};

static const struct {
  const uint8_t *enc;
  size_t len;
} skip_items[] = {
  { enc_uint_0, sizeof(enc_uint_0) },
  { enc_uint_42, sizeof(enc_uint_42) },
  { enc_uint_2p32, sizeof(enc_uint_2p32) },
  { enc_neg_1, sizeof(enc_neg_1) },
  { enc_neg_257, sizeof(enc_neg_257) },
  { enc_bytes_3, sizeof(enc_bytes_3) },
  { enc_text_4, sizeof(enc_text_4) },
  { enc_false, sizeof(enc_false) },
  { enc_true, sizeof(enc_true) },
  { enc_null, sizeof(enc_null) },
  { enc_undefined, sizeof(enc_undefined) },
  { enc_simple_32, sizeof(enc_simple_32) },
  { enc_float16, sizeof(enc_float16) },
  { enc_float32, sizeof(enc_float32) },
  { enc_float64, sizeof(enc_float64) },
  { enc_array_2, sizeof(enc_array_2) },
  { enc_map_1, sizeof(enc_map_1) },
  { enc_nested_array, sizeof(enc_nested_array) },
  { enc_nested_map, sizeof(enc_nested_map) },
};
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_skip_and_accessors,
                   "cbor_skip_next and reader position accessors");
UNIT_TEST(test_skip_and_accessors)
{
  uint8_t buffer[256];
  cbor_reader_state_t reader;
  size_t total = 0;

  UNIT_TEST_BEGIN();

  /* Concatenate every encoding into a single buffer. */
  for(int i = 0; i < CC_ARRAY_LENGTH(skip_items); i++) {
    memcpy(buffer + total, skip_items[i].enc, skip_items[i].len);
    total += skip_items[i].len;
  }

  cbor_init_reader(&reader, buffer, total);

  /* Accessors reflect the initial state. */
  UNIT_TEST_ASSERT(cbor_get_position(&reader) == buffer);
  UNIT_TEST_ASSERT(cbor_get_remaining(&reader) == total);

  /*
   * Skipping each item must consume exactly its encoded length and keep the
   * position and remaining-bytes accessors consistent.
   */
  for(int i = 0; i < CC_ARRAY_LENGTH(skip_items); i++) {
    const uint8_t *position_before = cbor_get_position(&reader);
    size_t remaining_before = cbor_get_remaining(&reader);

    UNIT_TEST_ASSERT(cbor_skip_next(&reader));
    UNIT_TEST_ASSERT(cbor_get_position(&reader)
                     == position_before + skip_items[i].len);
    UNIT_TEST_ASSERT(cbor_get_remaining(&reader)
                     == remaining_before - skip_items[i].len);
  }

  UNIT_TEST_ASSERT(cbor_end_reader(&reader));
  UNIT_TEST_ASSERT(cbor_get_remaining(&reader) == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_skip_interior,
                   "cbor_skip_next skips unknown fields within a structure");
UNIT_TEST(test_skip_interior)
{
  /* [ "skip-me", h'0102', 7, {1: 2} ] -- read 7 after skipping the rest. */
  static const uint8_t cbor_data[] = {
    0x84,
    0x67, 's', 'k', 'i', 'p', '-', 'm', 'e',
    0x42, 0x01, 0x02,
    0x07,
    0xa1, 0x01, 0x02
  };
  cbor_reader_state_t reader;
  uint64_t value;

  UNIT_TEST_BEGIN();

  cbor_init_reader(&reader, cbor_data, sizeof(cbor_data));
  UNIT_TEST_ASSERT(4 == cbor_read_array(&reader));

  /* Skip the leading text and byte strings. */
  UNIT_TEST_ASSERT(cbor_skip_next(&reader));
  UNIT_TEST_ASSERT(cbor_skip_next(&reader));

  /* The third element is readable at the expected position. */
  UNIT_TEST_ASSERT(CBOR_MAJOR_TYPE_UNSIGNED == cbor_peek_next(&reader));
  UNIT_TEST_ASSERT(CBOR_SIZE_NONE != cbor_read_unsigned(&reader, &value));
  UNIT_TEST_ASSERT(7 == value);

  /* Skip the trailing map and land exactly at the end. */
  UNIT_TEST_ASSERT(cbor_skip_next(&reader));
  UNIT_TEST_ASSERT(cbor_end_reader(&reader));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_skip_errors,
                   "cbor_skip_next rejects empty and truncated input");
UNIT_TEST(test_skip_errors)
{
  cbor_reader_state_t reader;

  UNIT_TEST_BEGIN();

  /* Empty buffer. */
  cbor_init_reader(&reader, NULL, 0);
  UNIT_TEST_ASSERT(!cbor_skip_next(&reader));

  /* Byte string that claims more payload than is present. */
  {
    static const uint8_t data[] = { 0x43, 0x01 }; /* len 3, only 1 present */
    cbor_init_reader(&reader, data, sizeof(data));
    UNIT_TEST_ASSERT(!cbor_skip_next(&reader));
  }

  /* Multi-byte integer with a truncated argument. */
  {
    static const uint8_t data[] = { 0x1a, 0x00 }; /* needs 4 argument bytes */
    cbor_init_reader(&reader, data, sizeof(data));
    UNIT_TEST_ASSERT(!cbor_skip_next(&reader));
  }

  /* Array that promises more elements than are encoded. */
  {
    static const uint8_t data[] = { 0x82, 0x01 }; /* 2 elements, only 1 */
    cbor_init_reader(&reader, data, sizeof(data));
    UNIT_TEST_ASSERT(!cbor_skip_next(&reader));
  }

  /* Reserved additional information / break stop code is not skippable. */
  {
    static const uint8_t data[] = { 0xff };
    cbor_init_reader(&reader, data, sizeof(data));
    UNIT_TEST_ASSERT(!cbor_skip_next(&reader));
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(test_write_read);
  UNIT_TEST_RUN(test_skip_and_accessors);
  UNIT_TEST_RUN(test_skip_interior);
  UNIT_TEST_RUN(test_skip_errors);

  if(!UNIT_TEST_PASSED(test_write_read)
     || !UNIT_TEST_PASSED(test_skip_and_accessors)
     || !UNIT_TEST_PASSED(test_skip_interior)
     || !UNIT_TEST_PASSED(test_skip_errors)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
