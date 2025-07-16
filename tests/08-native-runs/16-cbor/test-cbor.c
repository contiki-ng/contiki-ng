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
    for(int i = 0; i < sizeof(unsigned_values) / sizeof(int64_t); i++) {
      cbor_write_unsigned(&writer, unsigned_values[i]);
      array_size++;
    }
    /* signed values */
    for(int i = 0; i < sizeof(signed_values) / sizeof(int64_t); i++) {
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
    for(int i = 0; i < sizeof(unsigned_values) / sizeof(uint64_t); i++) {
      UNIT_TEST_ASSERT(CBOR_SIZE_NONE != cbor_read_unsigned(&reader, &value));
      UNIT_TEST_ASSERT(unsigned_values[i] == value);
    }
    int64_t signed_value;
    for(int i = 0; i < sizeof(signed_values) / sizeof(uint64_t); i++) {
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
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(test_write_read);

  if(!UNIT_TEST_PASSED(test_write_read)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
