/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 */

/**
 * \file
 *         An OMA LWM2M standalone example to demonstrate how to use
 *         the Contiki OMA LWM2M library from a native application.
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#include "lwm2m-engine.h"
#include "lwm2m-rd-client.h"
#include "lwm2m-firmware.h"
#include "lwm2m-server.h"
#include "lwm2m-security.h"
#include "lwm2m-device.h"
#include "coap.h"
#include "coap-timer.h"
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define PSK_DEFAULT_IDENTITY "Client_identity"
#define PSK_DEFAULT_KEY      "secretPSK"

#define WITH_TEST_NOTIFICATION 1

void ipso_sensor_temp_init(void);
void ipso_control_test_init(void);
void ipso_blockwise_test_init(void);
void lwm2m_generic_object_test_init(void);

/* set this above zero to get auto deregister */
static int deregister = -1;

/*---------------------------------------------------------------------------*/
#if WITH_TEST_NOTIFICATION
static void
callback(coap_timer_t *timer)
{
  /* Automatic notification on device timer for test!*/
  lwm2m_notify_observers("3/0/13");
  coap_timer_reset(timer, 10000);
  if(deregister > 0) {
    deregister--;
    if(deregister == 0) {
      printf("Deregistering.\n");
      lwm2m_rd_client_deregister();
    }
  }
}
#endif /* WITH_TEST_NOTIFICATION */
/*---------------------------------------------------------------------------*/
static void
session_callback(struct lwm2m_session_info *si, int state)
{
  printf("Got Session Callback!!! %d\n", state);
}
/*---------------------------------------------------------------------------*/
#ifndef LWM2M_DEFAULT_RD_SERVER
/* Default to leshan.eclipse.org */
#ifdef WITH_DTLS
#define LWM2M_DEFAULT_RD_SERVER "coaps://5.39.83.206"
#else
#define LWM2M_DEFAULT_RD_SERVER "coap://5.39.83.206"
#endif
#endif /* LWM2M_DEFAULT_RD_SERVER */
/*---------------------------------------------------------------------------*/
void
start_application(int argc, char *argv[])
{
  const char *default_server = LWM2M_DEFAULT_RD_SERVER;
  coap_endpoint_t server_ep;
  int has_server_ep = 0;
  char *name = "abcde";

  if(argc > 1) {
    default_server = argv[1];
  }
  if(argc > 2) {
    name = argv[2];
  }

  if(default_server != NULL && *default_server != '\0') {
    if(coap_endpoint_parse(default_server, strlen(default_server), &server_ep) == 0) {
      fprintf(stderr, "failed to parse the server address '%s'\n", default_server);
      exit(1);
    }
    has_server_ep = 1;
  }

  /* Example using network timer */
#if WITH_TEST_NOTIFICATION
  {
    static coap_timer_t nt;
    coap_timer_set_callback(&nt, callback);
    coap_timer_set(&nt, 10000);
  }
#endif /* WITH_TEST_NOTIFICATION */

  /* Initialize the OMA LWM2M engine */
  lwm2m_engine_init();

  ipso_sensor_temp_init();
  ipso_control_test_init();
  ipso_blockwise_test_init();
  lwm2m_generic_object_test_init();

  /* Register default LWM2M objects */

  lwm2m_device_init();
  lwm2m_firmware_init();
  lwm2m_security_init();
  lwm2m_server_init();

  if(has_server_ep) {
    /* start RD client */
    printf("Starting RD client to register at ");
    coap_endpoint_print(&server_ep);
    printf("\n");

#ifdef WITH_DTLS
#if defined(PSK_DEFAULT_IDENTITY) && defined(PSK_DEFAULT_KEY)
    {
      lwm2m_security_server_t *server;
      /* Register new server with instance id, server id, lifetime in seconds */
      if(!lwm2m_server_add(0, 1, 600)) {
        printf("failed to add server object\n");
      }

      server = lwm2m_security_add_server(0, 1,
                                         (uint8_t *)default_server,
                                         strlen(default_server));
      if(server == NULL) {
        printf("failed to add security object\n");
      } else {
        if(lwm2m_security_set_server_psk(server,
                                         (uint8_t *)PSK_DEFAULT_IDENTITY,
                                         strlen(PSK_DEFAULT_IDENTITY),
                                         (uint8_t *)PSK_DEFAULT_KEY,
                                         strlen(PSK_DEFAULT_KEY))) {
          printf("registered security object for endpoint %s\n",
                 default_server);
        } else {
          printf("failed to register security object\n");
        }
      }
    }
#endif /* defined(PSK_DEFAULT_IDENTITY) && defined(PSK_DEFAULT_KEY) */
#endif /* WITH_DTLS */

#define BOOTSTRAP 0
#if BOOTSTRAP
    lwm2m_rd_client_register_with_bootstrap_server(&server_ep);
    lwm2m_rd_client_use_bootstrap_server(1);
#else
    lwm2m_rd_client_register_with_server(&server_ep);
#endif
    lwm2m_rd_client_use_registration_server(1);

    lwm2m_rd_client_init(name);

    printf("Callback: %p\n", session_callback);
    lwm2m_rd_client_set_session_callback(session_callback);

  } else {
    fprintf(stderr, "No registration server specified.\n");
  }
  printf("COAP MAX PACKET: %d (BLOCK:%d)\n", COAP_MAX_PACKET_SIZE, COAP_MAX_BLOCK_SIZE);
}
/*---------------------------------------------------------------------------*/
