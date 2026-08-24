/*
 * Copyright (c) 2026, Giridhar Pavan.
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
 *         Unit tests for the JSON parser.
 * \author
 *         Giridhar Pavan
 */

#include "contiki.h"
#include "lib/json/jsonparse.h"
#include "unit-test.h"

#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "JSON parser unit tests");
AUTOSTART_PROCESSES(&run_tests);

/*---------------------------------------------------------------------------*/
static int
parse_literal(const char *literal, int literal_type, char whitespace)
{
  struct jsonparse_state state;
  char json[32];
  int len;

  len = snprintf(json, sizeof(json), "{\"value\":%s%c}", literal, whitespace);
  if(len < 0 || len >= sizeof(json)) {
    return 0;
  }

  jsonparse_setup(&state, json, len);

  return jsonparse_next(&state) == '{' &&
         jsonparse_next(&state) == JSON_TYPE_PAIR_NAME &&
         jsonparse_next(&state) == literal_type &&
         jsonparse_next(&state) == '}';
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_literal_whitespace,
                   "Parse true, false, and null before JSON whitespace");
UNIT_TEST(test_literal_whitespace)
{
  static const struct {
    const char *text;
    int type;
  } literals[] = {
    { "true", JSON_TYPE_TRUE },
    { "false", JSON_TYPE_FALSE },
    { "null", JSON_TYPE_NULL },
  };
  static const char whitespace[] = { ' ', '\n', '\r', '\t' };
  unsigned int i;
  unsigned int j;

  UNIT_TEST_BEGIN();

  for(i = 0; i < sizeof(literals) / sizeof(literals[0]); i++) {
    for(j = 0; j < sizeof(whitespace) / sizeof(whitespace[0]); j++) {
      UNIT_TEST_ASSERT(parse_literal(literals[i].text, literals[i].type,
                                     whitespace[j]));
    }
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");

  UNIT_TEST_RUN(test_literal_whitespace);

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
