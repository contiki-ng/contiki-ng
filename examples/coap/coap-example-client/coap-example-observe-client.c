/*
 * Copyright (c) 2014, Daniele Alessandrelli.
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
 *      Erbium (Er) CoAP observe client example.
 * \author
 *      Daniele Alessandrelli <daniele.alessandrelli@gmail.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#if PLATFORM_SUPPORTS_BUTTON_HAL
#include "dev/button-hal.h"
#else
#include "dev/button-sensor.h"
#endif

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_APP

/*----------------------------------------------------------------------------*/
/* FIXME: This server address is hard-coded for Cooja */
#define SERVER_NODE(ipaddr)   uip_ip6addr(ipaddr, 0xfe80, 0, 0, 0, 0x0212, \
                                          0x7402, 0x0002, 0x0202)
#define REMOTE_PORT     UIP_HTONS(COAP_DEFAULT_PORT)
/* Toggle interval in seconds */
#define TOGGLE_INTERVAL 30
/* The path of the resource to observe */
#define OBS_RESOURCE_URI "test/push"

/*----------------------------------------------------------------------------*/
static uip_ipaddr_t server_ipaddr[1]; /* holds the server ip address */
static coap_observee_t *obs;

/*----------------------------------------------------------------------------*/
PROCESS(er_example_observe_client, "Erbium Coap Observe Client Example");
AUTOSTART_PROCESSES(&er_example_observe_client);

/*----------------------------------------------------------------------------*/
/*
 * Handle the response to the observe request and the following notifications
 */
static void
notification_callback(coap_observee_t *obs, void *notification,
                      coap_notification_flag_t flag)
{
  int len = 0;
  const uint8_t *payload = NULL;

  printf("Notification handler\n");
  printf("Observee URI: %s\n", obs->url);
  if(notification) {
    len = coap_get_payload(notification, &payload);
  }
  switch(flag) {
  case NOTIFICATION_OK:
    printf("NOTIFICATION OK: %*s\n", len, (char *)payload);
    break;
  case OBSERVE_OK: /* server accepeted observation request */
    printf("OBSERVE_OK: %*s\n", len, (char *)payload);
    break;
  case OBSERVE_NOT_SUPPORTED:
    printf("OBSERVE_NOT_SUPPORTED: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case ERROR_RESPONSE_CODE:
    printf("ERROR_RESPONSE_CODE: %*s\n", len, (char *)payload);
    obs = NULL;
    break;
  case NO_REPLY_FROM_SERVER:
    printf("NO_REPLY_FROM_SERVER: "
           "removing observe registration with token %x%x\n",
           obs->token[0], obs->token[1]);
    obs = NULL;
    break;
  }
}
/*----------------------------------------------------------------------------*/
/*
 * Toggle the observation of the remote resource
 */
void
toggle_observation(void)
{
  if(obs) {
    printf("Stopping observation\n");
    coap_obs_remove_observee(obs);
    obs = NULL;
  } else {
    printf("Starting observation\n");
    obs = coap_obs_request_registration(server_ipaddr, REMOTE_PORT,
                                        OBS_RESOURCE_URI, notification_callback, NULL);
  }
}
/*----------------------------------------------------------------------------*/
/*
 * The main (proto-)thread. It starts/stops the observation of the remote
 * resource every time the timer elapses or the button (if available) is
 * pressed
 */
PROCESS_THREAD(er_example_observe_client, ev, data)
{
  PROCESS_BEGIN();

  static struct etimer et;

  /* store server address in server_ipaddr */
  SERVER_NODE(server_ipaddr);
  /* init timer and button (if available) */
  etimer_set(&et, TOGGLE_INTERVAL * CLOCK_SECOND);
#if PLATFORM_HAS_BUTTON
#if !PLATFORM_SUPPORTS_BUTTON_HAL
  SENSORS_ACTIVATE(button_sensor);
#endif
  printf("Press a button to start/stop observation of remote resource\n");
#endif /* PLATFORM_HAS_BUTTON */

  /* toggle observation every time the timer elapses or the button is pressed */
  while(1) {
    PROCESS_YIELD();
    if(etimer_expired(&et)) {
      printf("--Toggle timer--\n");
      toggle_observation();
      printf("\n--Done--\n");
      etimer_reset(&et);
#if PLATFORM_HAS_BUTTON
#if PLATFORM_SUPPORTS_BUTTON_HAL
    } else if(ev == button_hal_release_event) {
#else
    } else if(ev == sensors_event && data == &button_sensor) {
#endif
      printf("--Toggle tutton--\n");
      toggle_observation();
      printf("\n--Done--\n");
#endif /* PLATFORM_HAS_BUTTON */
    }
  }
  PROCESS_END();
}
