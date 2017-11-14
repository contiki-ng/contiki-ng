/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
 *  CoAP resource handler for CC26XX software and hardware version
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "coap-engine.h"
#include "coap.h"
#include "sys/clock.h"
#include "coap-server.h"
#include "cc26xx-web-demo.h"

#include "ti-lib.h"

#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
static uint16_t
detect_chip(void)
{
  if(ti_lib_chipinfo_chip_family_is_cc26xx()) {
    if(ti_lib_chipinfo_supports_ieee_802_15_4() == true) {
      if(ti_lib_chipinfo_supports_ble() == true) {
        return 2650;
      } else {
        return 2630;
      }
    } else {
      return 2640;
    }
  } else if(ti_lib_chipinfo_chip_family_is_cc13xx()) {
    if(ti_lib_chipinfo_supports_ble() == false &&
       ti_lib_chipinfo_supports_ieee_802_15_4() == false) {
      return 1310;
    } else if(ti_lib_chipinfo_supports_ble() == true &&
        ti_lib_chipinfo_supports_ieee_802_15_4() == true) {
      return 1350;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static void
res_get_handler_hw(coap_message_t *request, coap_message_t *response,
                   uint8_t *buffer,
                   uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;
  uint16_t chip = detect_chip();

  coap_get_header_accept(request, &accept);

  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_set_header_content_format(response, TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%s on CC%u", BOARD_STRING,
             chip);

    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_JSON) {
    coap_set_header_content_format(response, APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{\"HW Ver\":\"%s on CC%u\"}",
             BOARD_STRING, chip);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_XML) {
    coap_set_header_content_format(response, APPLICATION_XML);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE,
             "<hw-ver val=\"%s on CC%u\"/>", BOARD_STRING,
             chip);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else {
    coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
    coap_set_payload(response, coap_server_supported_msg,
                              strlen(coap_server_supported_msg));
  }
}
/*---------------------------------------------------------------------------*/
static void
res_get_handler_sw(coap_message_t *request, coap_message_t *response,
                   uint8_t *buffer,
                   uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;

  coap_get_header_accept(request, &accept);

  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_set_header_content_format(response, TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%s", CONTIKI_VERSION_STRING);

    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_JSON) {
    coap_set_header_content_format(response, APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{\"SW Ver\":\"%s\"}",
             CONTIKI_VERSION_STRING);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_XML) {
    coap_set_header_content_format(response, APPLICATION_XML);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE,
             "<sw-ver val=\"%s\"/>", CONTIKI_VERSION_STRING);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else {
    coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
    coap_set_payload(response, coap_server_supported_msg,
                              strlen(coap_server_supported_msg));
  }
}
/*---------------------------------------------------------------------------*/
static void
res_get_handler_uptime(coap_message_t *request, coap_message_t *response,
                       uint8_t *buffer,
                       uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;

  coap_get_header_accept(request, &accept);

  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_set_header_content_format(response, TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%lu", clock_seconds());

    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_JSON) {
    coap_set_header_content_format(response, APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{\"uptime\":%lu}",
             clock_seconds());

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_XML) {
    coap_set_header_content_format(response, APPLICATION_XML);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE,
             "<uptime val=\"%lu\" unit=\"sec\"/>", clock_seconds());

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else {
    coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
    coap_set_payload(response, coap_server_supported_msg,
                              strlen(coap_server_supported_msg));
  }
}
/*---------------------------------------------------------------------------*/
static void
res_post_handler_cfg_reset(coap_message_t *request, coap_message_t *response,
                           uint8_t *buffer,
                           uint16_t preferred_size, int32_t *offset)
{
  cc26xx_web_demo_restore_defaults();
}
/*---------------------------------------------------------------------------*/
RESOURCE(res_device_sw,
         "title=\"Software version\";rt=\"text\"",
         res_get_handler_sw,
         NULL,
         NULL,
         NULL);
/*---------------------------------------------------------------------------*/
RESOURCE(res_device_uptime,
         "title=\"Uptime\";rt=\"seconds\"",
         res_get_handler_uptime,
         NULL,
         NULL,
         NULL);
/*---------------------------------------------------------------------------*/
RESOURCE(res_device_hw,
         "title=\"Hardware version\";rt=\"text\"",
         res_get_handler_hw,
         NULL,
         NULL,
         NULL);
/*---------------------------------------------------------------------------*/
RESOURCE(res_device_cfg_reset,
         "title=\"Reset Device Config: POST\";rt=\"Control\"",
         NULL, res_post_handler_cfg_reset, NULL, NULL);
/*---------------------------------------------------------------------------*/
/** @} */
