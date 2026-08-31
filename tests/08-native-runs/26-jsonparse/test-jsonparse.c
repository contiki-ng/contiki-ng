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

#include <stdbool.h>
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
/*
 * Parse a length-delimited fragment that is NOT NUL-terminated, and that is
 * followed in memory by bytes the parser must never look at. Returns the
 * final parser state so that the caller can check that nothing ran past the
 * supplied length.
 */
static void
parse_prefix(struct jsonparse_state *state, const char *text, int len)
{
  static char buffer[64];
  int type;

  memset(buffer, 'x', sizeof(buffer));
  memcpy(buffer, text, len);

  jsonparse_setup(state, buffer, len);

  do {
    type = jsonparse_next(state);
  } while(type != 0 && type != JSON_TYPE_ERROR);
}
/*---------------------------------------------------------------------------*/
/* Every scan in the parser must stop at the supplied length. */
static bool
stayed_in_bounds(const struct jsonparse_state *state)
{
  return state->pos <= state->len &&
         state->vstart >= 0 && state->vstart <= state->len &&
         state->vlen >= 0 && state->vstart + state->vlen <= state->len;
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_truncated_input,
                   "Reject truncated values without reading past the input");
UNIT_TEST(test_truncated_input)
{
  static const char *const truncated[] = {
    "{\"a\":tru",                /* literal cut short */
    "{\"a\":true",               /* object never closed */
    "{\"a\":\"bc",               /* string never closed */
    "{\"a\":\"b\\",              /* trailing backslash */
    "{\"a\":123",                /* number at the very end */
    "[",                         /* nothing after the array start */
    "{\"a",                      /* pair name never closed */
  };
  static struct jsonparse_state state;
  static unsigned int i;

  UNIT_TEST_BEGIN();

  for(i = 0; i < sizeof(truncated) / sizeof(truncated[0]); i++) {
    parse_prefix(&state, truncated[i], strlen(truncated[i]));
    UNIT_TEST_ASSERT(stayed_in_bounds(&state));
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_no_nul_terminator,
                   "Parse a complete value that is not NUL-terminated");
UNIT_TEST(test_no_nul_terminator)
{
  static const char json[] = "{\"a\":true}";
  static struct jsonparse_state state;

  UNIT_TEST_BEGIN();

  /* strlen() excludes the terminator, so the parser sees the 'x' padding
     that parse_prefix() writes after the value. */
  parse_prefix(&state, json, strlen(json));
  UNIT_TEST_ASSERT(stayed_in_bounds(&state));
  UNIT_TEST_ASSERT(state.error == 0);
  UNIT_TEST_ASSERT(state.depth == 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_invalid_literal,
                   "Reject literals with a wrong length or a bad suffix");
UNIT_TEST(test_invalid_literal)
{
  static const char *const invalid[] = {
    "{\"a\":truex}",
    "{\"a\":tru}",
    "{\"a\":tru }",
    "{\"a\":nul}",
    "{\"a\":falsey}",
  };
  static struct jsonparse_state state;
  static unsigned int i;

  UNIT_TEST_BEGIN();

  for(i = 0; i < sizeof(invalid) / sizeof(invalid[0]); i++) {
    parse_prefix(&state, invalid[i], strlen(invalid[i]));
    UNIT_TEST_ASSERT(stayed_in_bounds(&state));
    UNIT_TEST_ASSERT(state.error == JSON_ERROR_SYNTAX);
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_number_conversion,
                   "Convert numbers that are not NUL-terminated");
UNIT_TEST(test_number_conversion)
{
  static char buffer[32];
  static struct jsonparse_state state;
  static const char json[] = "[42";

  UNIT_TEST_BEGIN();

  memset(buffer, '9', sizeof(buffer));
  memcpy(buffer, json, strlen(json));

  jsonparse_setup(&state, buffer, strlen(json));
  UNIT_TEST_ASSERT(jsonparse_next(&state) == '[');
  UNIT_TEST_ASSERT(jsonparse_next(&state) == JSON_TYPE_NUMBER);
  UNIT_TEST_ASSERT(stayed_in_bounds(&state));
  UNIT_TEST_ASSERT(state.vlen == 2);
  /* The '9' padding must not be read as part of the number. */
  UNIT_TEST_ASSERT(jsonparse_get_value_as_int(&state) == 42);
  UNIT_TEST_ASSERT(jsonparse_get_value_as_long(&state) == 42);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");

  UNIT_TEST_RUN(test_literal_whitespace);
  UNIT_TEST_RUN(test_truncated_input);
  UNIT_TEST_RUN(test_no_nul_terminator);
  UNIT_TEST_RUN(test_invalid_literal);
  UNIT_TEST_RUN(test_number_conversion);

  if(!UNIT_TEST_PASSED(test_literal_whitespace)
     || !UNIT_TEST_PASSED(test_truncated_input)
     || !UNIT_TEST_PASSED(test_no_nul_terminator)
     || !UNIT_TEST_PASSED(test_invalid_literal)
     || !UNIT_TEST_PASSED(test_number_conversion)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
