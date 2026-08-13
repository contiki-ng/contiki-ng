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
 *         EDHOC server example demonstrating key exchange setup
 *         
 *         This example shows how to set up an EDHOC server using the
 *         default test credentials. It performs a complete EDHOC handshake
 *         with a compatible client and exports OSCORE security context.
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
#include "net/routing/routing.h"
#include "edhoc-server.h"
#include "edhoc-error.h"
#include "edhoc-exporter.h"
#include "edhoc-cred-rfc9529.h"

#include "sys/log.h"
#define LOG_MODULE "EDHOC-Server-Example"
#define LOG_LEVEL LOG_LEVEL_INFO

/*---------------------------------------------------------------------------*/
PROCESS(edhoc_server_example, "EDHOC Server Example");
AUTOSTART_PROCESSES(&edhoc_server_example);
/*---------------------------------------------------------------------------*/
/* The print_oscore_ctx function is provided by the EDHOC module */
extern void print_oscore_ctx(oscore_ctx_t *osc);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(edhoc_server_example, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("Starting...\n");

  /* Initialize network - start as DAG root for this example */
  LOG_INFO("Starting as network root...\n");
  NETSTACK_ROUTING.root_start();

  /* Initialize EDHOC key storage */
  LOG_INFO("Setting up EDHOC credentials...\n");
  LOG_WARN("Using publicly known test credentials -- never use in production!\n\n");
  edhoc_error_t result = edhoc_create_key_list();
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to create key list: %d\n", result);
    PROCESS_EXIT();
  }

  /* Use RFC 9529 test credentials */
  /* Note: For server, we need the full server key (with private key) 
   * and only the client's public key for verification */
  cose_key_t client_key = auth_rfc9529_static_dh_client;
  cose_key_t server_key = auth_rfc9529_static_dh_server;

  /* Set up key pair using helper function */
  result = edhoc_setup_key_pair(&server_key, &client_key, "Server", "Client");
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

  /* Initialize and start EDHOC server */
  LOG_INFO("Starting EDHOC server...\n");
  edhoc_server_init();
  if(!edhoc_server_start()) {
    LOG_ERR("Failed to start EDHOC server\n");
    PROCESS_EXIT();
  }
  LOG_INFO("Listening for client connections...\n");

  /* Main event loop for EDHOC protocol */
  while(1) {
    PROCESS_WAIT_EVENT();

    int8_t result = edhoc_server_callback(ev, data);

    if(result == SERV_HANDSHAKE_COMPLETE) {
      /* EDHOC handshake completed successfully */
      LOG_INFO("EDHOC handshake completed successfully!\n");
      LOG_INFO("Exporting OSCORE security context...\n");

      oscore_ctx_t oscore_ctx;
      int8_t export_result = edhoc_exporter_oscore(&oscore_ctx, edhoc_ctx);

      if(export_result < 0) {
        LOG_ERR("Failed to export OSCORE context: %d\n", export_result);
      } else {
        LOG_INFO("OSCORE context exported successfully!\n");

        print_oscore_ctx(&oscore_ctx);

        LOG_INFO("=== Handshake Complete ===");
        LOG_INFO("The EDHOC handshake has established a secure channel.\n");
        LOG_INFO("The derived OSCORE context can now be used for secure CoAP communication.\n");
      }

      /* Reset server for next connection */
      result = SERV_HANDSHAKE_RESET;
    }

    if(result == SERV_HANDSHAKE_RESET) {
      LOG_INFO("Resetting EDHOC server for next handshake...\n");
      if(!edhoc_server_reset_handshake()) {
        LOG_ERR("Failed to reset EDHOC server handshake state\n");
        PROCESS_EXIT();
      }
      LOG_INFO("Server ready for next EDHOC handshake\n");
    }

    if(result < 0 && result != SERV_HANDSHAKE_RESET && result != SERV_HANDSHAKE_COMPLETE) {
      /* EDHOC handshake failed */
      LOG_ERR("EDHOC server error: %d\n", result);

      /* Try to reset server handshake state */
      if(!edhoc_server_reset_handshake()) {
        LOG_ERR("Failed to reset EDHOC server handshake state after error\n");
        PROCESS_EXIT();
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
