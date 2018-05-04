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
 *      ETSI Plugtest resource
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <string.h>
#include "coap-engine.h"
#include "coap.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Plugtest"
#define LOG_LEVEL LOG_LEVEL_PLUGTEST

static void res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/*
 * Large resource that can be created using POST method
 */
RESOURCE(res_plugtest_large_create,
         "title=\"Large resource that can be created using POST method\";rt=\"block\"",
         NULL,
         res_post_handler,
         NULL,
         NULL);

static void
res_post_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  coap_message_t *const coap_req = (coap_message_t *)request;

  uint8_t *incoming = NULL;
  size_t len = 0;

  unsigned int ct = -1;

  if(!coap_get_header_content_format(request, &ct)) {
    coap_set_status_code(response, BAD_REQUEST_4_00);
    const char *error_msg = "NoContentType";
    coap_set_payload(response, error_msg, strlen(error_msg));
    return;
  }

  if((len = coap_get_payload(request, (const uint8_t **)&incoming))) {
    if(coap_req->block1_num * coap_req->block1_size + len <= 2048) {
      coap_set_status_code(response, CREATED_2_01);
      coap_set_header_location_path(response, "/nirvana");
      coap_set_header_block1(response, coap_req->block1_num, 0,
                             coap_req->block1_size);
    } else {
      coap_set_status_code(response, REQUEST_ENTITY_TOO_LARGE_4_13);
      const char *error_msg = "2048B max.";
      coap_set_payload(response, error_msg, strlen(error_msg));
      return;
    }
  } else {
    coap_set_status_code(response, BAD_REQUEST_4_00);
    const char *error_msg = "NoPayload";
    coap_set_payload(response, error_msg, strlen(error_msg));
    return;
  }
}
