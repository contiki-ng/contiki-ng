/*
 * Copyright (c) 2026, zeetee1235.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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
 *         Tests for length-delimited URI matching in CoAP Observe.
 */
#include "contiki.h"
#include "unit-test.h"
#include "coap-observe.h"

#include <stdio.h>
#include <string.h>

PROCESS(test_process, "CoAP Observe tests");
AUTOSTART_PROCESSES(&test_process);

static coap_endpoint_t endpoint;
static coap_endpoint_t other_endpoint;
/* Intentionally not NUL-terminated. */
static const char short_uri[] = { 'o', 'b', 's' };
/*---------------------------------------------------------------------------*/
static int
observe(const coap_endpoint_t *ep, const char *uri, size_t uri_len,
        uint8_t token)
{
  coap_message_t request;
  coap_message_t response;

  coap_init_message(&request, COAP_TYPE_CON, COAP_GET, 1);
  coap_set_src_endpoint(&request, ep);
  coap_set_token(&request, &token, sizeof(token));
  coap_set_header_observe(&request, 0);
  request.uri_path = uri;
  request.uri_path_len = uri_len;
  coap_init_message(&response, COAP_TYPE_ACK, CONTENT_2_05, 1);
  coap_observe_handler(NULL, &request, &response);
  return response.code == CONTENT_2_05
    && coap_is_option(&response, COAP_OPTION_OBSERVE);
}
/*---------------------------------------------------------------------------*/
static void
reset_observers(void)
{
  while(coap_remove_observer_by_client(&endpoint) > 0) {
  }
  while(coap_remove_observer_by_client(&other_endpoint) > 0) {
  }
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(short_request, "a shorter request preserves a longer observer");
UNIT_TEST(short_request)
{
  UNIT_TEST_BEGIN();
  reset_observers();
  UNIT_TEST_ASSERT(observe(&endpoint, "obs/sub", 7, 1));
  UNIT_TEST_ASSERT(observe(&endpoint, short_uri, sizeof(short_uri), 2));
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&endpoint, "obs/sub") == 1);
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&endpoint, "obs") == 1);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(long_request, "a longer request retains prefix replacement");
UNIT_TEST(long_request)
{
  UNIT_TEST_BEGIN();
  reset_observers();
  UNIT_TEST_ASSERT(observe(&endpoint, "obs", 3, 1));
  UNIT_TEST_ASSERT(observe(&endpoint, "obs/sub", 7, 2));
  UNIT_TEST_ASSERT(coap_remove_observer_by_token(&endpoint,
                                                (uint8_t[]){ 1 }, 1) == 0);
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&endpoint, "obs/sub") == 1);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(exact_request, "equal URIs replace observers on the same endpoint");
UNIT_TEST(exact_request)
{
  UNIT_TEST_BEGIN();
  reset_observers();
  UNIT_TEST_ASSERT(observe(&endpoint, "obs", 3, 1));
  UNIT_TEST_ASSERT(observe(&other_endpoint, "obs", 3, 2));
  UNIT_TEST_ASSERT(observe(&endpoint, "oth", 3, 3));
  UNIT_TEST_ASSERT(observe(&endpoint, short_uri, sizeof(short_uri), 4));
  UNIT_TEST_ASSERT(coap_remove_observer_by_token(&endpoint,
                                                (uint8_t[]){ 1 }, 1) == 0);
  UNIT_TEST_ASSERT(coap_remove_observer_by_token(&endpoint,
                                                (uint8_t[]){ 4 }, 1) == 1);
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&other_endpoint, "obs") == 1);
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&endpoint, "oth") == 1);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(string_api, "string removal retains prefix and endpoint matching");
UNIT_TEST(string_api)
{
  UNIT_TEST_BEGIN();
  reset_observers();
  UNIT_TEST_ASSERT(observe(&endpoint, "obs", 3, 1));
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&endpoint, "ob") == 0);
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&endpoint, "oth") == 0);
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&other_endpoint, "obs") == 0);
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(NULL, "obs/sub") == 1);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(empty_request, "an empty URI only replaces an empty observer");
UNIT_TEST(empty_request)
{
  UNIT_TEST_BEGIN();
  reset_observers();
  UNIT_TEST_ASSERT(observe(&endpoint, "obs", 3, 1));
  UNIT_TEST_ASSERT(observe(&endpoint, "", 0, 2));
  UNIT_TEST_ASSERT(observe(&endpoint, "", 0, 3));
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&endpoint, "") == 1);
  UNIT_TEST_ASSERT(coap_remove_observer_by_uri(&endpoint, "obs") == 1);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(truncated_request, "repeated long URIs do not exhaust observers");
UNIT_TEST(truncated_request)
{
  char uri[COAP_OBSERVER_URL_LEN + 2];
  unsigned int i;

  UNIT_TEST_BEGIN();
  reset_observers();
  memset(uri, 'x', sizeof(uri));
  for(i = 0; i <= COAP_MAX_OBSERVERS; i++) {
    UNIT_TEST_ASSERT(observe(&endpoint, uri, sizeof(uri), 1));
  }
  UNIT_TEST_ASSERT(coap_remove_observer_by_token(&endpoint,
                                                (uint8_t[]){ 1 }, 1) == 1);
  UNIT_TEST_ASSERT(coap_remove_observer_by_client(&endpoint) == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(nul_request, "embedded NUL URIs retain bounded replacement");
UNIT_TEST(nul_request)
{
  static const char uri[] = { 'o', 'b', 's', 0, 's', 'u', 'b' };
  unsigned int i;

  UNIT_TEST_BEGIN();
  reset_observers();
  for(i = 0; i <= COAP_MAX_OBSERVERS; i++) {
    UNIT_TEST_ASSERT(observe(&endpoint, uri, sizeof(uri), 1));
  }
  UNIT_TEST_ASSERT(coap_remove_observer_by_token(&endpoint,
                                                (uint8_t[]){ 1 }, 1) == 1);
  UNIT_TEST_ASSERT(coap_remove_observer_by_client(&endpoint) == 0);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();
  endpoint.port = UIP_HTONS(5683);
  other_endpoint.port = UIP_HTONS(5684);
  printf("Run unit-test\n");
  UNIT_TEST_RUN(short_request);
  UNIT_TEST_RUN(long_request);
  UNIT_TEST_RUN(exact_request);
  UNIT_TEST_RUN(string_api);
  UNIT_TEST_RUN(empty_request);
  UNIT_TEST_RUN(truncated_request);
  UNIT_TEST_RUN(nul_request);
  reset_observers();
  if(!UNIT_TEST_PASSED(short_request)
     || !UNIT_TEST_PASSED(long_request)
     || !UNIT_TEST_PASSED(exact_request)
     || !UNIT_TEST_PASSED(string_api)
     || !UNIT_TEST_PASSED(empty_request)
     || !UNIT_TEST_PASSED(truncated_request)
     || !UNIT_TEST_PASSED(nul_request)) {
    printf("=check-me= FAILED\n");
  }
  printf("=check-me= DONE\n");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
