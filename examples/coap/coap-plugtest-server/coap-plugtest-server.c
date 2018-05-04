/*
 * Copyright (c) 2013, Institute for Pervasive Computing, ETH Zurich
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
 *      Server for the ETSI IoT CoAP Plugtests, Las Vegas, NV, USA, Nov 2013.
 * \author
 *      Matthias Kovatsch <kovatsch@inf.ethz.ch>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap.h"
#include "coap-transactions.h"
#include "coap-separate.h"
#include "coap-engine.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Plugtest"
#define LOG_LEVEL LOG_LEVEL_PLUGTEST

/*
 * Resources to be activated need to be imported through the extern keyword.
 * The build system automatically compiles the resources in the corresponding
 * sub-directory.
 */
extern coap_resource_t
  res_plugtest_test,
  res_plugtest_validate,
  res_plugtest_create1,
  res_plugtest_create2,
  res_plugtest_create3,
  res_plugtest_longpath,
  res_plugtest_query,
  res_plugtest_locquery,
  res_plugtest_multi,
  res_plugtest_link1,
  res_plugtest_link2,
  res_plugtest_link3,
  res_plugtest_path,
  res_plugtest_separate,
  res_plugtest_large,
  res_plugtest_large_update,
  res_plugtest_large_create,
  res_plugtest_obs,
  res_mirror;

PROCESS(plugtest_server, "PlugtestServer");
AUTOSTART_PROCESSES(&plugtest_server);

PROCESS_THREAD(plugtest_server, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("ETSI IoT CoAP Plugtests Server\n");

  /* Activate the application-specific resources. */
  coap_activate_resource(&res_plugtest_test, "test");
  coap_activate_resource(&res_plugtest_validate, "validate");
  coap_activate_resource(&res_plugtest_create1, "create1");
  coap_activate_resource(&res_plugtest_create2, "create2");
  coap_activate_resource(&res_plugtest_create3, "create3");
  coap_activate_resource(&res_plugtest_longpath, "seg1/seg2/seg3");
  coap_activate_resource(&res_plugtest_query, "query");
  coap_activate_resource(&res_plugtest_locquery, "location-query");
  coap_activate_resource(&res_plugtest_multi, "multi-format");
  coap_activate_resource(&res_plugtest_link1, "link1");
  coap_activate_resource(&res_plugtest_link2, "link2");
  coap_activate_resource(&res_plugtest_link3, "link3");
  coap_activate_resource(&res_plugtest_path, "path");
  coap_activate_resource(&res_plugtest_separate, "separate");
  coap_activate_resource(&res_plugtest_large, "large");
  coap_activate_resource(&res_plugtest_large_update, "large-update");
  coap_activate_resource(&res_plugtest_large_create, "large-create");
  coap_activate_resource(&res_plugtest_obs, "obs");

  coap_activate_resource(&res_mirror, "mirror");

  /* Define application-specific events here. */
  while(1) {
    PROCESS_WAIT_EVENT();
  }                             /* while (1) */

  PROCESS_END();
}
