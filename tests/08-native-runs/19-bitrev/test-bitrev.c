/*
 * Copyright (c) 2023, RISE Research Institutes of Sweden AB
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
 *         Unit tests for bit reversal library
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#include "contiki.h"
#include "unit-test.h"
#include "lib/bitrev.h"

#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "Bit reversal unit tests");
AUTOSTART_PROCESSES(&run_tests);

/* Test individual byte reversal */
UNIT_TEST_REGISTER(test_bitrev_byte, "Bit reversal for single byte");
UNIT_TEST(test_bitrev_byte)
{
  UNIT_TEST_BEGIN();

  /* Test common patterns */
  UNIT_TEST_ASSERT(bitrev_byte(0x00) == 0x00);
  UNIT_TEST_ASSERT(bitrev_byte(0xFF) == 0xFF);
  UNIT_TEST_ASSERT(bitrev_byte(0xF0) == 0x0F);
  UNIT_TEST_ASSERT(bitrev_byte(0x0F) == 0xF0);
  UNIT_TEST_ASSERT(bitrev_byte(0xAA) == 0x55);
  UNIT_TEST_ASSERT(bitrev_byte(0x55) == 0xAA);
  UNIT_TEST_ASSERT(bitrev_byte(0x01) == 0x80);
  UNIT_TEST_ASSERT(bitrev_byte(0x80) == 0x01);
  UNIT_TEST_ASSERT(bitrev_byte(0x02) == 0x40);
  UNIT_TEST_ASSERT(bitrev_byte(0x40) == 0x02);

  UNIT_TEST_END();
}

/* Test array reversal in-place */
UNIT_TEST_REGISTER(test_bitrev_array, "Bit reversal for byte arrays");
UNIT_TEST(test_bitrev_array)
{
  uint8_t test_array[] = {0xF0, 0x0F, 0xAA, 0x55, 0x01, 0x80};
  uint8_t expected[] = {0x0F, 0xF0, 0x55, 0xAA, 0x80, 0x01};

  UNIT_TEST_BEGIN();

  bitrev_array(test_array, 6);

  for(int i = 0; i < 6; i++) {
    UNIT_TEST_ASSERT(test_array[i] == expected[i]);
  }

  UNIT_TEST_END();
}

/* Test array copy with bit reversal */
UNIT_TEST_REGISTER(test_bitrev_array_copy, "Bit reversal with array copy");
UNIT_TEST(test_bitrev_array_copy)
{
  uint8_t input[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  uint8_t output[8];
  uint8_t expected[] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};

  UNIT_TEST_BEGIN();

  /* Ensure output starts clean */
  memset(output, 0x00, 8);

  bitrev_array_copy(input, output, 8);

  /* Input should remain unchanged */
  uint8_t original_input[] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
  for(int i = 0; i < 8; i++) {
    UNIT_TEST_ASSERT(input[i] == original_input[i]);
  }

  /* Output should contain bit-reversed values */
  for(int i = 0; i < 8; i++) {
    UNIT_TEST_ASSERT(output[i] == expected[i]);
  }

  UNIT_TEST_END();
}

/* Test edge cases */
UNIT_TEST_REGISTER(test_bitrev_edge_cases, "Edge cases for bit reversal");
UNIT_TEST(test_bitrev_edge_cases)
{
  UNIT_TEST_BEGIN();

  /* Test zero-length array (should not crash) */
  uint8_t dummy;
  bitrev_array(&dummy, 0);
  bitrev_array_copy(&dummy, &dummy, 0);

  /* Test single byte array */
  uint8_t single[] = {0xF0};
  bitrev_array(single, 1);
  UNIT_TEST_ASSERT(single[0] == 0x0F);

  UNIT_TEST_END();
}

PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("\nRunning bit reversal library unit tests\n");

  UNIT_TEST_RUN(test_bitrev_byte);
  UNIT_TEST_RUN(test_bitrev_array);
  UNIT_TEST_RUN(test_bitrev_array_copy);
  UNIT_TEST_RUN(test_bitrev_edge_cases);

  if(!UNIT_TEST_PASSED(test_bitrev_byte) ||
     !UNIT_TEST_PASSED(test_bitrev_array) ||
     !UNIT_TEST_PASSED(test_bitrev_array_copy) ||
     !UNIT_TEST_PASSED(test_bitrev_edge_cases)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
