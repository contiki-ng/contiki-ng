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
 * \addtogroup cc13xx-cc26xx-web-demo
 * @{
 *
 * \file
 *  CoAP resource handler for CC13xx/CC26xx software and hardware version
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/clock.h"
#include "net/app-layer/coap/coap.h"
#include "net/app-layer/coap/coap-engine.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
/*---------------------------------------------------------------------------*/
#include "web-demo.h"
#include "coap-server.h"
/*---------------------------------------------------------------------------*/
#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
static const char *
detect_chip(void)
{
  switch(ChipInfo_GetChipType()) {
  case CHIP_TYPE_CC1310:  return "CC1310";
  case CHIP_TYPE_CC1350:  return "CC1350";
  case CHIP_TYPE_CC2620:  return "CC2620";
  case CHIP_TYPE_CC2630:  return "CC2630";
  case CHIP_TYPE_CC2640:  return "CC2640";
  case CHIP_TYPE_CC2650:  return "CC2650";
  case CHIP_TYPE_CC2642:  return "CC2642";
  case CHIP_TYPE_CC2652:  return "CC2652";
  case CHIP_TYPE_CC1312:  return "CC1312";
  case CHIP_TYPE_CC1352:  return "CC1352";
  case CHIP_TYPE_CC1352P: return "CC1352P";
  default:                return "Unknown";
  }
}
/*---------------------------------------------------------------------------*/
static void
res_get_handler_hw(coap_message_t *request, coap_message_t *response,
                   uint8_t *buffer,
                   uint16_t preferred_size, int32_t *offset)
{
  unsigned int accept = -1;
  const char *chip = detect_chip();

  coap_get_header_accept(request, &accept);

  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_set_header_content_format(response, TEXT_PLAIN);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "%s on %s", BOARD_STRING,
             chip);

    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_JSON) {
    coap_set_header_content_format(response, APPLICATION_JSON);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE, "{\"HW Ver\":\"%s on %s\"}",
             BOARD_STRING, chip);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_XML) {
    coap_set_header_content_format(response, APPLICATION_XML);
    snprintf((char *)buffer, REST_MAX_CHUNK_SIZE,
             "<hw-ver val=\"%s on %s\"/>", BOARD_STRING,
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
  web_demo_restore_defaults();
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
