/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB.
 * Copyright (c) 2020, Industrial System Institute (ISI), Patras, Greece
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */
/**
 * \file
 *      EDHOC [RFC9528] test server with CoAP Block-Wise Transfer [RFC7959]
 *
 *      SECURITY NOTICE: The included credentials are for testing purposes only.
 *      Never use these keys in production systems. The private keys are
 *      publicly known and provide no security.
 *
 * \author
 *      Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund,
 *      Marco Tiloca, Niclas Finne, Nicolas Tsiftes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-lib.h"
#include "contiki-net.h"
#include "edhoc-exporter.h"
#include "edhoc-server.h"
#include "edhoc-error.h"
#include "edhoc-cred-rfc9529.h"

#include "sys/log.h"
#define LOG_MODULE "EDHOCServer"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/******************************************************************************/
PROCESS(edhoc_test_server, "EDHOC Test Server");
AUTOSTART_PROCESSES(&edhoc_test_server);
/******************************************************************************/
PROCESS_THREAD(edhoc_test_server, ev, data)
{
  PROCESS_BEGIN();

  if(IS_NETWORK_ROUTING_ROOT) {
    /* Initialize routing as root */
    NETSTACK_ROUTING.root_start();
  } else {
    static struct etimer timer;
    etimer_set(&timer, CLOCK_SECOND * 5);
    while(1) {
      if(NETSTACK_ROUTING.node_is_reachable()) {
        LOG_INFO("Network reached!\n");
        break;
      }
      LOG_INFO("Waiting for network...\n");
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      etimer_reset(&timer);
    }
  }

  static cose_key_t auth_server = auth_rfc9529_static_dh_server;
  static cose_key_t auth_client = auth_rfc9529_static_dh_client;

  edhoc_error_t result = edhoc_setup_key_pair(&auth_server, &auth_client, "Server", "Client");
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to set up key pair: %s\n", edhoc_error_string(result));
    PROCESS_EXIT();
  }

  /* Print key information for verification */
  edhoc_print_key_info(&auth_server, "Server");
  edhoc_print_key_info(&auth_client, "Client");

  /* Validate key setup */
  edhoc_validate_key_setup();

  edhoc_server_init();
  if(!edhoc_server_start()) {
    LOG_ERR("Failed to start EDHOC server\n");
    PROCESS_EXIT();
  }

  while(1) {
    PROCESS_WAIT_EVENT();
    int8_t res = edhoc_server_callback(ev, data);
    if(res == SERV_HANDSHAKE_COMPLETE) {
      LOG_INFO("EDHOC handshake completed\n");
      oscore_ctx_t osc;
      int8_t export_result = edhoc_exporter_oscore(&osc, edhoc_ctx);
      if(export_result < 0) {
        LOG_ERR("Failed to export OSCORE context: error code %d\n",
                export_result);
      } else {
        print_oscore_ctx(&osc);
      }
      res = SERV_HANDSHAKE_RESET;
    }
    if(res == SERV_HANDSHAKE_RESET) {
      if(!edhoc_server_reset_handshake()) {
        LOG_ERR("Failed to reset EDHOC server handshake state\n");
        PROCESS_EXIT();
      }
      LOG_INFO("Server handshake state reset\n");
    }
    if(res < 0 && res != SERV_HANDSHAKE_RESET && res != SERV_HANDSHAKE_COMPLETE) {
      LOG_ERR("EDHOC server callback error: error code %d\n", res);
    }
  }
  PROCESS_END();
}
/******************************************************************************/
