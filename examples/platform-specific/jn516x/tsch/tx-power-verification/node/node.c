/*
* Copyright (c) 2015 NXP B.V.
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
* 3. Neither the name of NXP B.V. nor the names of its contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY NXP B.V. AND CONTRIBUTORS ``AS IS'' AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL NXP B.V. OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* This file is part of the Contiki operating system.
*
* Author: Theo van Daele <theo.van.daele@nxp.com>
*
*/
#include "contiki.h"
#include "net/ipv6/uip-ds6.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/uip.h"
#include "net/linkaddr.h"
#include "coap-engine.h"
#include <stdio.h>
#include <stdlib.h>
#include <AppHardwareApi.h>

static void set_tx_power_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);
static void get_tx_power_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset);

static char content[COAP_MAX_CHUNK_SIZE];
static int content_len = 0;

#define CONTENT_PRINTF(...) { if(content_len < sizeof(content)) content_len += snprintf(content+content_len, sizeof(content)-content_len, __VA_ARGS__); }

/*---------------------------------------------------------------------------*/
PROCESS(start_app, "START_APP");
AUTOSTART_PROCESSES(&start_app);
/*---------------------------------------------------------------------------*/

/*********** sensor/ resource ************************************************/
RESOURCE(resource_set_tx_power,
         "title=\"Set TX Power\"",
         NULL,
         set_tx_power_handler,
         set_tx_power_handler,
         NULL);
static void
set_tx_power_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  const uint8_t *request_content = NULL;
  int tx_level;

  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    coap_get_payload(request, &request_content);
    tx_level = atoi((const char *)request_content);
    NETSTACK_RADIO.set_value(RADIO_PARAM_TXPOWER, tx_level);
  }
}

RESOURCE(resource_get_tx_power,
         "title=\"Get TX Power\"",
         get_tx_power_handler,
         NULL,
         NULL,
         NULL);
static void
get_tx_power_handler(coap_message_t *request, coap_message_t *response, uint8_t *buffer, uint16_t preferred_size, int32_t *offset)
{
  int tx_level;
  unsigned int accept = -1;
  coap_get_header_accept(request, &accept);
  if(accept == -1 || accept == TEXT_PLAIN) {
    content_len = 0;
    NETSTACK_RADIO.get_value(RADIO_PARAM_TXPOWER, &tx_level);
    CONTENT_PRINTF("%d", tx_level);
    coap_set_header_content_format(response, TEXT_PLAIN);
    coap_set_payload(response, (uint8_t *)content, content_len);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(start_app, ev, data)
{
  PROCESS_BEGIN();
  static int is_coordinator = 0;

  /* Start network stack */
  if(is_coordinator) {
    NETSTACK_ROUTING.root_start();
  }
  NETSTACK_MAC.on();
  printf("Starting RPL node\n");

  coap_activate_resource(&resource_set_tx_power, "Set-TX-Power");
  coap_activate_resource(&resource_get_tx_power, "Get-TX-Power");

  PROCESS_END();
}
