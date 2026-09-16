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
 *         Unit tests for link-format filtering in /.well-known/core.
 */

#include "contiki.h"
#include "unit-test.h"
#include "coap-engine.h"

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

PROCESS(run_tests, "CoAP link-format filtering unit tests");
AUTOSTART_PROCESSES(&run_tests);

/*
 * Passes a string literal together with its length, so that a query can
 * contain NUL bytes.
 */
#define QUERY(literal) (literal), sizeof(literal) - 1

/* The project configuration makes this hold the whole listing. */
#define PREFERRED_SIZE COAP_MAX_CHUNK_SIZE

/*
 * The link-format output of each resource. The CoAP engine activates
 * /.well-known/core itself before the resources of this test.
 */
#define LINK_CORE    "</.well-known/core>;ct=40"
#define LINK_FOO     "</foo>;title=\"Foo\";rt=\"x\""
#define LINK_FOO_BAR "</foo/bar>;rt=\"y\""
#define LINK_OTHER   "</other>;rt=\"xy\""
#define LINK_PLAIN   "</plain>"

extern coap_resource_t res_well_known_core;

RESOURCE(res_foo, "title=\"Foo\";rt=\"x\"", NULL, NULL, NULL, NULL);
RESOURCE(res_foo_bar, "rt=\"y\"", NULL, NULL, NULL, NULL);
RESOURCE(res_other, "rt=\"xy\"", NULL, NULL, NULL, NULL);
RESOURCE(res_plain, NULL, NULL, NULL, NULL, NULL);

/* The request refers to its Uri-Query in here, so it must outlive it. */
static uint8_t request_buffer[64];

/* The handler may write one byte beyond the preferred size. */
static uint8_t payload_buffer[PREFERRED_SIZE + 1];

/*
 * Sends a GET request with the given Uri-Query to the /.well-known/core
 * handler, and checks the status code and payload of the response. An
 * empty query sends the request without a Uri-Query option.
 */
static bool
check_get(const char *query, size_t query_len,
          uint8_t expected_code, const char *expected_payload)
{
  coap_message_t request;
  coap_message_t response;
  int32_t offset = 0;
  uint16_t len = 0;
  size_t expected_len = strlen(expected_payload);

  request_buffer[len++] = (1 << 6);  /* Version 1, type CON, token length 0. */
  request_buffer[len++] = COAP_GET;  /* Code. */
  request_buffer[len++] = 0x12;      /* Message ID, high byte. */
  request_buffer[len++] = 0x34;      /* Message ID, low byte. */

  if(query_len > 0) {
    /*
     * The option delta 15 does not fit in the nibble, so it is carried in
     * an extended byte, and so is a length of 13 or more.
     */
    if(query_len < 13) {
      request_buffer[len++] = (13 << 4) | query_len;
      request_buffer[len++] = COAP_OPTION_URI_QUERY - 13;
    } else {
      request_buffer[len++] = (13 << 4) | 13;
      request_buffer[len++] = COAP_OPTION_URI_QUERY - 13;
      request_buffer[len++] = query_len - 13;
    }
    memcpy(&request_buffer[len], query, query_len);
    len += query_len;
  }

  if(coap_parse_message(&request, request_buffer, len) != NO_ERROR) {
    printf("Failed to parse the request\n");
    return false;
  }

  coap_init_message(&response, COAP_TYPE_ACK, CONTENT_2_05, 0x1234);
  res_well_known_core.get_handler(&request, &response, payload_buffer,
                                  PREFERRED_SIZE, &offset);

  if(response.code != expected_code
     || response.payload_len != expected_len
     || memcmp(response.payload, expected_payload, expected_len) != 0) {
    printf("Got code %u and payload \"%.*s\"\n", response.code,
           (int)response.payload_len, (char *)response.payload);
    return false;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_no_filter, "all resources are listed without a filter");
UNIT_TEST(test_no_filter)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(check_get(QUERY(""), CONTENT_2_05,
                             LINK_CORE "," LINK_FOO "," LINK_FOO_BAR ","
                             LINK_OTHER "," LINK_PLAIN));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_href_filter, "href filters match the resource URL");
UNIT_TEST(test_href_filter)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(check_get(QUERY("href=/foo*"), CONTENT_2_05,
                             LINK_FOO "," LINK_FOO_BAR));
  UNIT_TEST_ASSERT(check_get(QUERY("href=/foo"), CONTENT_2_05, LINK_FOO));
  UNIT_TEST_ASSERT(check_get(QUERY("href=/nothing"), CONTENT_2_05, ""));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_attribute_filter,
                   "attribute filters match quoted attribute values");
UNIT_TEST(test_attribute_filter)
{
  UNIT_TEST_BEGIN();

  /* A resource without attributes never matches an attribute filter. */
  UNIT_TEST_ASSERT(check_get(QUERY("rt=x"), CONTENT_2_05, LINK_FOO));
  UNIT_TEST_ASSERT(check_get(QUERY("rt=x*"), CONTENT_2_05,
                             LINK_FOO "," LINK_OTHER));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_malformed_filter,
                   "a filter without a name, value or equals sign is rejected");
UNIT_TEST(test_malformed_filter)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(check_get(QUERY("rt="), BAD_REQUEST_4_00, ""));
  UNIT_TEST_ASSERT(check_get(QUERY("rt"), BAD_REQUEST_4_00, ""));
  UNIT_TEST_ASSERT(check_get(QUERY("href=/"), BAD_REQUEST_4_00, ""));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_nul_in_filter, "a filter with a NUL byte is rejected");
UNIT_TEST(test_nul_in_filter)
{
  UNIT_TEST_BEGIN();

  /*
   * Each of these used to end the comparison at the NUL byte, matching at
   * the end of a string and then reading beyond its terminator.
   */
  UNIT_TEST_ASSERT(check_get(QUERY("href=o\0"), BAD_REQUEST_4_00, ""));
  UNIT_TEST_ASSERT(check_get(QUERY("href=o\0b"), BAD_REQUEST_4_00, ""));
  UNIT_TEST_ASSERT(check_get(QUERY("r\0=x"), BAD_REQUEST_4_00, ""));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(run_tests, ev, data)
{
  PROCESS_BEGIN();

  coap_activate_resource(&res_foo, "foo");
  coap_activate_resource(&res_foo_bar, "foo/bar");
  coap_activate_resource(&res_other, "other");
  coap_activate_resource(&res_plain, "plain");

  printf("\nRunning CoAP link-format filtering unit tests\n");

  UNIT_TEST_RUN(test_no_filter);
  UNIT_TEST_RUN(test_href_filter);
  UNIT_TEST_RUN(test_attribute_filter);
  UNIT_TEST_RUN(test_malformed_filter);
  UNIT_TEST_RUN(test_nul_in_filter);

  if(!UNIT_TEST_PASSED(test_no_filter) ||
     !UNIT_TEST_PASSED(test_href_filter) ||
     !UNIT_TEST_PASSED(test_attribute_filter) ||
     !UNIT_TEST_PASSED(test_malformed_filter) ||
     !UNIT_TEST_PASSED(test_nul_in_filter)) {
    printf("=check-me= FAILED\n");
  } else {
    printf("=check-me= DONE\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
