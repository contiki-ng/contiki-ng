/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
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
 */

/**
 * \file
 *         EDHOC client example demonstrating key exchange setup
 *
 *         This example shows how to set up an EDHOC client using the
 *         RFC 9529 test credentials. It performs a complete EDHOC handshake
 *         with a compatible server and exports OSCORE security context.
 *
 *         SECURITY NOTICE: This example uses test credentials that are
 *         publicly known. Never use these keys in production systems.
 *
 * \author
 *         Marco Tiloca, Peter A Jonsson, Rikard Höglund
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "edhoc-client.h"
#include "edhoc-error.h"
#include "edhoc-exporter.h"
#include "edhoc-cred-rfc9529.h"

#include "sys/log.h"
#define LOG_MODULE "EDHOC-Client-Example"
#define LOG_LEVEL LOG_LEVEL_INFO

/*---------------------------------------------------------------------------*/
PROCESS(edhoc_client_example, "EDHOC Client Example");
AUTOSTART_PROCESSES(&edhoc_client_example);
/*---------------------------------------------------------------------------*/
/* The print_oscore_ctx function is provided by the EDHOC module */
extern void print_oscore_ctx(oscore_ctx_t *osc);
/*---------------------------------------------------------------------------*/
static void
finish_session(void)
{
  LOG_INFO("EDHOC handshake completed successfully!\n");
  LOG_INFO("Exporting OSCORE security context...\n");

  oscore_ctx_t oscore_ctx;
  int8_t export_result = edhoc_exporter_oscore(&oscore_ctx, edhoc_ctx);

  if(export_result < 0) {
    LOG_ERR("Failed to export OSCORE context: %d\n", export_result);
  } else {
    LOG_INFO("OSCORE context exported successfully!\n");
    print_oscore_ctx(&oscore_ctx);

    LOG_INFO("\n=== Example Complete ===\n");
    LOG_INFO("The EDHOC handshake has established a secure channel.\n");
    LOG_INFO("The derived OSCORE context can now be used for secure CoAP communication.\n");
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(edhoc_client_example, ev, data)
{
  static struct etimer timer;

  PROCESS_BEGIN();

  LOG_INFO("Starting...\n");
  LOG_WARN("Using publicly known test credentials -- never use in production!\n\n");

  /* Wait for network to be available */
  etimer_set(&timer, CLOCK_SECOND * 5);
  while(1) {
    if(NETSTACK_ROUTING.node_is_reachable()) {
      LOG_INFO("Network is reachable!\n");
      break;
    }
    LOG_INFO("Waiting for network...\n");
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }

  /* Initialize EDHOC key storage */
  LOG_INFO("Setting up EDHOC credentials...\n");
  edhoc_error_t result = edhoc_create_key_list();
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to create key list: %d\n", result);
    PROCESS_EXIT();
  }

  /* Use RFC 9529 test credentials */
  /* Note: For client, we need the full client key (with private key) 
   * and only the server's public key for verification */
  cose_key_t client_key = auth_rfc9529_static_dh_client;
  cose_key_t server_key = auth_rfc9529_static_dh_server;

  /* Set up key pair using helper function */
  result = edhoc_setup_key_pair(&client_key, &server_key, "Client", "Server");
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to set up key pair: %s\n", edhoc_error_string(result));
    PROCESS_EXIT();
  }

  LOG_INFO("Credentials configured successfully!\n");
  
  /* Print detailed key information for verification */
  edhoc_print_key_info(&client_key, "Client");
  edhoc_print_key_info(&server_key, "Server");
  
  /* Validate key setup */
  if(edhoc_validate_key_setup() != EDHOC_SUCCESS) {
    LOG_WARN("Key setup validation failed, but continuing...\n");
  }

  /* Start EDHOC client */
  LOG_INFO("Starting EDHOC client protocol...\n");
  edhoc_client_run();

  /* Main event loop for EDHOC protocol */
  while(1) {
    PROCESS_WAIT_EVENT();

    int8_t status = edhoc_client_callback(ev, data);

    /* Continue if the handshake is still in progress */
    if(status == 0) {
      continue;
    }

    /* Handle handshake failure */
    if(status < 0) {
      LOG_ERR("EDHOC handshake failed with error: %d\n", status);
      break;
    }

    /* Handle successful handshake completion */
    finish_session();
    break;
  }

  /* Clean up */
  edhoc_client_close();
  LOG_INFO("EDHOC client finished.\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
