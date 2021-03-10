
/*
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
 */
/**
 * \file
 *      EDHOC client example [draft-ietf-lake-edhoc-01] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "contiki.h"
#include "contiki-net.h"
#include "coap-engine.h"
#include "edhoc-client-API.h"
#include "rpl.h"
#include "sys/rtimer.h"

rtimer_clock_t time;
oscore_ctx_t osc;
PROCESS(edhoc_example_client, "Edhoc Example Client");
AUTOSTART_PROCESSES(&edhoc_example_client);

PROCESS_THREAD(edhoc_example_client, ev, data)
{
  /*static struct etimer wait_timer; */
#if RPL_NODE == 1
  static struct etimer timer;
#endif
  PROCESS_BEGIN();
#if RPL_NODE == 1
  etimer_set(&timer, CLOCK_SECOND * 10);
  while(1) {
    watchdog_periodic();
    LOG_INFO("Waiting to reach the rpl\n");
    if(rpl_is_reachable()) {
      LOG_INFO("RPL reached\n");
      watchdog_periodic();
      break;
    }
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    etimer_reset(&timer);
  }
#endif

  /*Set the other part authetication key */
  edhoc_create_key_list();
  cose_key_t auth_client = { NULL, { 0x24 }, 1,
                             /* { "Node_101" }, strlen("Node_101"), */
                             { "" }, 0,
                             KEY_TYPE, KEY_CRV,
                             { 0xcd, 0x41, 0x77, 0xba, 0x62, 0x43, 0x33, 0x75, 0xed, 0xe2, 0x79, 0xb5, 0xe1, 0x8e, 0x8b, 0x91, 0xbc, 0x3e, 0xd8, 0xf1, 0xe1, 0x74, 0x47, 0x4a, 0x26, 0xfc, 0x0e, 0xdb, 0x44, 0xea, 0x53, 0x73 },
                             { 0xa0, 0x39, 0x1d, 0xe2, 0x9c, 0x5c, 0x5b, 0xad, 0xda, 0x61, 0x0d, 0x4e, 0x30, 0x1e, 0xaa, 0xa1, 0x84, 0x22, 0x36, 0x77, 0x22, 0x28, 0x9c, 0xd1, 0x8c, 0xbe, 0x66, 0x24, 0xe8, 0x9b, 0x9c, 0xfd },
                             { 0x04, 0xf3, 0x47, 0xf2, 0xbe, 0xad, 0x69, 0x9a, 0xdb, 0x24, 0x73, 0x44, 0xf3, 0x47, 0xf2, 0xbd, 0xac, 0x93, 0xc7, 0xf2, 0xbe, 0xad, 0x6a, 0x9d, 0x2a, 0x9b, 0x24, 0x75, 0x4a, 0x1e, 0x2b, 0x62 }, };

  cose_key_t auth_server = { NULL, { 0x07 }, 1,
                             /* { "Serv_A" }, strlen("Serv_A"), */
                             { "" }, 0,
                             KEY_TYPE, KEY_CRV,
                             { 0x6f, 0x97, 0x02, 0xa6, 0x66, 0x02, 0xd7, 0x8f, 0x5e, 0x81, 0xba, 0xc1, 0xe0, 0xaf, 0x01, 0xf8, 0xb5, 0x28, 0x10, 0xc5, 0x02, 0xe8, 0x7e, 0xbb, 0x7c, 0x92, 0x6c, 0x07, 0x42, 0x6f, 0xd0, 0x2f },
                             { 0xc8, 0xd3, 0x32, 0x74, 0xc7, 0x1c, 0x9b, 0x3e, 0xe5, 0x7d, 0x84, 0x2b, 0xbf, 0x22, 0x38, 0xb8, 0x28, 0x3c, 0xb4, 0x10, 0xec, 0xa2, 0x16, 0xfb, 0x72, 0xa7, 0x8e, 0xa7, 0xa8, 0x70, 0xf8, 0x00 } };
  edhoc_add_key(&auth_client);
  edhoc_add_key(&auth_server);

  /*edhoc_server_set_ad_1("MSG1!",strlen("MSG1!")); 
  edhoc_server_set_ad_3("MSG3!",strlen("MSG3!")); */

  edhoc_client_run();
  while(1) {
    watchdog_periodic();
    PROCESS_WAIT_EVENT();
    watchdog_periodic();
    int8_t re = edhoc_client_callback(ev, &data);
    if(re > 0) {
      LOG_INFO("EDHOC protocol finished success, export here your security context\n");
      if(edhoc_exporter_oscore(&osc, ctx) < 0) {
        LOG_ERR("ERROR IN EXPORT CTX\n");
      } else {
        LOG_INFO("Export OSCORE CTX success\n");
        edhoc_exporter_print_oscore_ctx(&osc);
      }
	  /*LOG_DBG("And Get your Aplication Data\n");
	  char ad2[16];
	  LOG_DBG("AD2:");
	  uint8_t ad2_sz = edhoc_server_get_ad_2(ad2);
	  print_char_8_dbg(ad2,ad2_sz);*/
      break;
    }
    if(re < 0) {
      LOG_ERR("EDHOC protocol ERROR\n");
      break;
    }
  }
  edhoc_client_close();
  LOG_INFO("Client finished\n");
  PROCESS_END();
}
