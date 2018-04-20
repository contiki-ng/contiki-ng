/*
 * Copyright (c) 2014, Nimbus Centre for Embedded Systems Research, Cork Institute of Technology.
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
 *      SHT11 Sensor Resource
 *
 *      This is a simple GET resource that returns the temperature in Celsius
 *      and the humidity reading from the SHT11.
 * \author
 *      Pablo Corbalan <paul.corbalan@gmail.com>
 */

#include "contiki.h"

#if PLATFORM_HAS_SHT11

#include <stdio.h>
#include <string.h>
#include "coap-engine.h"
#include "dev/sht11/sht11-sensor.h"

static void res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

/* Get Method Example. Returns the reading from temperature and humidity sensors. */
RESOURCE(res_sht11,
         "title=\"Temperature and Humidity\";rt=\"Sht11\"",
         res_get_handler,
         NULL,
         NULL,
         NULL);

static void
res_get_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  /* Temperature in Celsius (t in 14 bits resolution at 3 Volts)
   * T = -39.60 + 0.01*t
   */
  uint16_t temperature = ((sht11_sensor.value(SHT11_SENSOR_TEMP) / 10) - 396) / 10;
  /* Relative Humidity in percent (h in 12 bits resolution)
   * RH = -4 + 0.0405*h - 2.8e-6*(h*h)
   */
  uint16_t rh = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);

  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);

  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_set_header_content_format(response, TEXT_PLAIN);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "%u;%u", temperature, rh);

    coap_set_payload(response, (uint8_t *)buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_XML) {
    coap_set_header_content_format(response, APPLICATION_XML);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "<Temperature =\"%u\" Humidity=\"%u\"/>", temperature, rh);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else if(accept == APPLICATION_JSON) {
    coap_set_header_content_format(response, APPLICATION_JSON);
    snprintf((char *)buffer, COAP_MAX_CHUNK_SIZE, "{'Sht11':{'Temperature':%u,'Humidity':%u}}", temperature, rh);

    coap_set_payload(response, buffer, strlen((char *)buffer));
  } else {
    coap_set_status_code(response, NOT_ACCEPTABLE_4_06);
    const char *msg = "Supporting content-types text/plain, application/xml, and application/json";
    coap_set_payload(response, msg, strlen(msg));
  }
}
#endif /* PLATFORM_HAS_SHT11 */
