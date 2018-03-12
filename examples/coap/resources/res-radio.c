/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 *
 * This file is part of the Contiki operating system.
 */

/**
 * \file
 *      Example resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include "contiki.h"

#if PLATFORM_HAS_RADIO
#include <stdio.h>
#include <string.h>
#include "coap-engine.h"
#include "net/netstack.h"

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* A simple getter example. Returns the reading of the rssi/lqi from radio sensor */
RESOURCE(res_radio,
         "title=\"RADIO: ?p=rssi\";rt=\"RadioSensor\"",
         res_get_handler,
         NULL,
         NULL,
         NULL);

static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  size_t len = 0;
  const char *p = NULL;
  radio_value_t value;
  int8_t rssi = 0;
  int success = 0;
  unsigned int accept = -1;

  coap_get_header_accept(request, &accept);

  if((len = coap_get_query_variable(request, "p", &p))) {
    if(strncmp(p, "rssi", len) == 0) {
      if(NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value) ==
         RADIO_RESULT_OK) {
        success = 1;
        rssi = (int8_t)value;
      }
    }
  }

  if(success) {
    if(accept == -1 || accept == TEXT_PLAIN) {
      coap_set_header_content_format(response, TEXT_PLAIN);
      snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "%d", rssi);

      coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
    } else if(accept == APPLICATION_JSON) {
      coap_set_header_content_format(response, APPLICATION_JSON);

      snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "{'rssi':%d}", rssi);
      coap_set_payload(response, buffer, strlen((char *)buffer));
    } else {
      coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
      const char *msg = "Supporting content-types text/plain and application/json";
      coap_set_payload(response, msg, strlen(msg));
    }
  } else {
    coap_set_status_code(response, BAD_REQUEST_4_00);
  }
}
#endif /* PLATFORM_HAS_RADIO */
