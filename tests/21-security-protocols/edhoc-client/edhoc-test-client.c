
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
 *      EDHOC [RFC9528] client test with CoAP Block-Wise Transfer [RFC7959]
 *      system.
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
#include "contiki-net.h"
#include "coap-engine.h"
#include "edhoc-client.h"
#include "edhoc-error.h"
#include "edhoc-exporter.h"
#include "edhoc-cred-rfc9529.h"

#include "sys/log.h"
#define LOG_MODULE "edhoc-client"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/******************************************************************************/
PROCESS(edhoc_test_client, "EDHOC Test Client");
AUTOSTART_PROCESSES(&edhoc_test_client);
/******************************************************************************/
PROCESS_THREAD(edhoc_test_client, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

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

  /* Set the client authentication credentials and add in the storage */
  edhoc_error_t result = edhoc_create_key_list();
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to create key list: %d\n", result);
    PROCESS_EXIT();
  }

  /* Use RFC 9529 test credentials */
  static cose_key_t client_key = auth_rfc9529_static_dh_client;
  static cose_key_t server_key = auth_rfc9529_static_dh_server;
  /* Clear server's private key as client doesn't need it */
  memset(server_key.ecc.priv, 0, sizeof(server_key.ecc.priv));

  result = edhoc_add_key(&client_key);
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to add client authentication key: %d\n", result);
    PROCESS_EXIT();
  }

  result = edhoc_add_key(&server_key);
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to add server authentication key: %d\n", result);
    PROCESS_EXIT();
  }

  edhoc_client_run();

  while(1) {
    PROCESS_WAIT_EVENT();
    int8_t ret = edhoc_client_callback(ev, data);
    if(ret > 0) {
      LOG_INFO("EDHOC session finished successfully\n");
      oscore_ctx_t osc;
      int8_t export_result = edhoc_exporter_oscore(&osc, edhoc_ctx);
      if(export_result < 0) {
        LOG_ERR("Failed to export OSCORE context: error code %d\n",
                export_result);
        break;
      } else {
        LOG_INFO("Export OSCORE CTX success\n");
        print_oscore_ctx(&osc);
      }
      break;
    }
    if(ret < 0) {
      LOG_ERR("EDHOC protocol failure: error code %d\n", ret);
      break;
    }
  }
  edhoc_client_close();
  LOG_INFO("Client finished\n");
  PROCESS_END();
}
/******************************************************************************/
