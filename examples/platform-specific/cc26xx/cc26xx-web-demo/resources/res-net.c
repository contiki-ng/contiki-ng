/*
 * Copyright (c) 2015, Texas Instruments Incorporated - http://www.ti.com/
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup cc26xx-web-demo
 * @{
 *
 * \file
 *  CoAP resource handler for network-related resources
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "coap-engine.h"
#include "coap.h"
#include "net/ipv6/uip-ds6.h"
#include "coap-server.h"
#include "cc26xx-web-demo.h"

#include "ti-lib.h"

#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
extern int def_rt_rssi;
/*---------------------------------------------------------------------------*/
static void
res_get_handler_parent_rssi(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;

  coap_get_header_accept(request, &accept);

  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_set_header_content_format(response, TEXT_PLAIN);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "%d", def_rt_rssi);

    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_JSON) {
    coap_set_header_content_format(response, APPLICATION_JSON);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "{\"Parent RSSI\":\"%d\"}",
             def_rt_rssi);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_XML) {
    coap_set_header_content_format(response, APPLICATION_XML);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE,
             "<parent-rssi val=\"%d\"/>", def_rt_rssi);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else {
    coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
    coap_set_payload(response, coap_server_supported_msg,
                              strlen(coap_server_supported_msg));
  }
}
/*---------------------------------------------------------------------------*/
static void
res_get_handler_pref_parent(coap_message_t *request, coap_message_t *response,
                            uint8_t *buffer,
                            uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;
  char def_rt_str[64];

  coap_get_header_accept(request, &accept);

  memset(def_rt_str, 0, sizeof(def_rt_str));
  cc26xx_web_demo_ipaddr_sprintf(def_rt_str, sizeof(def_rt_str),
                                 uip_ds6_defrt_choose());

  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_set_header_content_format(response, TEXT_PLAIN);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "%s", def_rt_str);

    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_JSON) {
    coap_set_header_content_format(response, APPLICATION_JSON);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "{\"Parent\":\"%s\"}",
             def_rt_str);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_XML) {
    coap_set_header_content_format(response, APPLICATION_XML);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE,
             "<parent=\"%s\"/>", def_rt_str);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else {
    coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
    coap_set_payload(response, coap_server_supported_msg,
                              strlen(coap_server_supported_msg));
  }
}
/*---------------------------------------------------------------------------*/
RESOURCE(res_parent_rssi, "title=\"Parent RSSI\";rt=\"dBm\"",
         res_get_handler_parent_rssi, NULL, NULL, NULL);
/*---------------------------------------------------------------------------*/
RESOURCE(res_parent_ip, "title=\"Preferred Parent\";rt=\"IPv6 address\"",
         res_get_handler_pref_parent, NULL, NULL, NULL);
/*---------------------------------------------------------------------------*/
/** @} */
