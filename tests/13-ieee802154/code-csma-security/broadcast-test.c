/*
 * Copyright (c) 2018, Hasso-Plattner-Institut.
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
 *
 */

#include "contiki.h"
#include "contiki-net.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uip.h"
#include <stdio.h>

#define UDP_SERVER_PORT 5678
#define UDP_CLIENT_PORT 8765
#define SEND_INTERVAL (10 * CLOCK_SECOND)

PROCESS(broadcast_test_process, "broadcast_test_process");
AUTOSTART_PROCESSES(&broadcast_test_process);
static uip_ipaddr_t ipaddr;
static struct simple_udp_connection server_connection;
static struct simple_udp_connection client_connection;
static struct etimer timer;
static uint8_t counter;
static uint8_t bitmap;

/*---------------------------------------------------------------------------*/
static void
server_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{
  if((datalen != 1) || (data[0] > 7)) {
    printf("=check-me= FAILED - invalid\n");
    return;
  }

  printf("received broadcast %"PRIu8"/8\n", data[0] + 1);

  bitmap |= (1 << data[0]);

  if(bitmap == 0xFF) {
    printf("=check-me= DONE\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
client_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{
  printf("=check-me= FAILED - client_callback should not be called\n");
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(broadcast_test_process, ev, data)
{
  PROCESS_BEGIN();

  uip_create_linklocal_allnodes_mcast(&ipaddr);

  simple_udp_register(&server_connection,
                      UDP_SERVER_PORT,
                      NULL,
                      UDP_CLIENT_PORT,
                      server_callback);
  simple_udp_register(&client_connection,
                      UDP_CLIENT_PORT,
                      NULL,
                      UDP_SERVER_PORT,
                      client_callback);

  if(linkaddr_node_addr.u8[1] == 1) {
    for(; counter < 8; counter++) {
      etimer_set(&timer, CLOCK_SECOND);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
      simple_udp_sendto(&client_connection, &counter, 1, &ipaddr);
    }
  } else {
    printf("=check-me= DONE\n");
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
