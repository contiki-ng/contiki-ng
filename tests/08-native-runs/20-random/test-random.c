/*
 * Copyright (c) 2025, Konrad-Felix Krentz
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

#include "contiki.h"
#include "lib/random.h"
#include "unit-test/unit-test.h"
#include <stdio.h>
#include <string.h>

/**
 * \brief       Counts the number of elements of an array.
 * \param array The array.
 * \return      The number of elements of the array.
 */
#define ARRAY_LENGTH(array) (sizeof(array) / sizeof((array)[0]))

PROCESS(test_random_process, "test");
AUTOSTART_PROCESSES(&test_random_process);
extern const struct random_prng sfc16_prng;

/*---------------------------------------------------------------------------*/
/*
 * Tests conformance with this output of PractRand:
 * ./RNG_output sfc32 16 0x00000000000000 | od -A n -t x1
 * c3 76 46 51 df 09 a8 08 2b 9d 34 30 20 c5 52 fb
 */
UNIT_TEST_REGISTER(sfc32_conformance_1, "Test conformance to sfc32 (1/2)");
UNIT_TEST(sfc32_conformance_1)
{
  static const uint16_t oracle_outputs[] = {
    0x76c3, 0x5146, 0x09df, 0x08a8, 0x9d2b, 0x3034, 0xc520, 0xfb52
  };

  UNIT_TEST_BEGIN();

  sfc32_prng.seed(0ULL);
  for(size_t i = 0; i < ARRAY_LENGTH(oracle_outputs); i++) {
    UNIT_TEST_ASSERT(oracle_outputs[i] == sfc32_prng.rand());
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * Tests conformance with this output of PractRand:
 * ./RNG_output sfc32 16 0x0102030405060708 | od -A n -t x1
 * 1b 7a 18 78 89 f4 2f 22 b6 5c a8 c6 1b 21 cf 51
 */
UNIT_TEST_REGISTER(sfc32_conformance_2, "Test conformance to sfc32 (2/2)");
UNIT_TEST(sfc32_conformance_2)
{
  static const uint16_t oracle_outputs[] = {
    0x7a1b, 0x7818, 0xf489, 0x222f, 0x5cb6, 0xc6a8, 0x211b, 0x51cf
  };

  UNIT_TEST_BEGIN();

  sfc32_prng.seed(0x0102030405060708ULL);
  for(size_t i = 0; i < ARRAY_LENGTH(oracle_outputs); i++) {
    UNIT_TEST_ASSERT(oracle_outputs[i] == sfc32_prng.rand());
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * Tests conformance with this output of PractRand:
 * ./RNG_output sfc16 16 0x00000000000000 | od -A n -t x1
 * 64 0a c6 97 f1 9c 56 7e b5 f4 7a 04 1c 79 c6 6d
 */
UNIT_TEST_REGISTER(sfc16_conformance_1, "Test conformance to sfc16 (1/2)");
UNIT_TEST(sfc16_conformance_1)
{
  static const uint16_t oracle_outputs[] = {
    0x0a64, 0x97c6, 0x9cf1, 0x7e56, 0xf4b5, 0x047a, 0x791c, 0x6dc6
  };

  UNIT_TEST_BEGIN();

  sfc16_prng.seed(0ULL);
  for(size_t i = 0; i < ARRAY_LENGTH(oracle_outputs); i++) {
    UNIT_TEST_ASSERT(oracle_outputs[i] == sfc16_prng.rand());
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
/*
 * Tests conformance with this output of PractRand:
 * ./RNG_output sfc16 16 0x0102030405060708 | od -A n -t x1
 * 7f 13 c1 80 40 ca a1 7e d5 8f 4f 2d f5 3f c4 b0
 */
UNIT_TEST_REGISTER(sfc16_conformance_2, "Test conformance to sfc16 (2/2)");
UNIT_TEST(sfc16_conformance_2)
{
  static const uint16_t oracle_outputs[] = {
    0x137f, 0x80c1, 0xca40, 0x7ea1, 0x8fd5, 0x2d4f, 0x3ff5, 0xb0c4
  };

  UNIT_TEST_BEGIN();

  sfc16_prng.seed(0x0102030405060708ULL);
  for(size_t i = 0; i < ARRAY_LENGTH(oracle_outputs); i++) {
    UNIT_TEST_ASSERT(oracle_outputs[i] == sfc16_prng.rand());
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_random_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(sfc32_conformance_1);
  UNIT_TEST_RUN(sfc32_conformance_2);
  UNIT_TEST_RUN(sfc16_conformance_1);
  UNIT_TEST_RUN(sfc16_conformance_2);

  if(!UNIT_TEST_PASSED(sfc32_conformance_1)
     || !UNIT_TEST_PASSED(sfc32_conformance_2)
     || !UNIT_TEST_PASSED(sfc16_conformance_1)
     || !UNIT_TEST_PASSED(sfc16_conformance_2)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
