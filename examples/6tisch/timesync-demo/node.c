/*
 * Copyright (c) 2015, SICS Swedish ICT.
 * Copyright (c) 2018, University of Bristol - http://www.bristol.ac.uk
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
 *         A RPL+TSCH node demonstrating application-level time syncrhonization.
 *
 * \author Atis Elsts <atis.elsts@bristol.ac.uk>
 *         Simon Duquennoy <simonduq@sics.se>
 */

#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/mac/tsch/tsch.h"
#include "lib/random.h"
#include "sys/node-id.h"

#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)
/*---------------------------------------------------------------------------*/
static struct simple_udp_connection client_conn, server_conn;

PROCESS(node_process, "RPL Node");
AUTOSTART_PROCESSES(&node_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  uint64_t local_time_clock_ticks = tsch_get_network_uptime_ticks();
  uint64_t remote_time_clock_ticks;

  if(datalen >= sizeof(remote_time_clock_ticks)) {
    memcpy(&remote_time_clock_ticks, data, sizeof(remote_time_clock_ticks));

    LOG_INFO("Received from ");
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_(", created at %lu, now %lu, latency %lu clock ticks, rssi %d, lqi %u\n",
              (unsigned long)remote_time_clock_ticks,
              (unsigned long)local_time_clock_ticks,
              (unsigned long)(local_time_clock_ticks - remote_time_clock_ticks),
              (int16_t)uipbuf_get_attr(UIPBUF_ATTR_RSSI),
              uipbuf_get_attr(UIPBUF_ATTR_LINK_QUALITY));
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer periodic_timer;
  int is_coordinator;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  is_coordinator = 0;

#if CONTIKI_TARGET_COOJA
  is_coordinator = (node_id == 1);
#endif

  if(is_coordinator) {
    NETSTACK_ROUTING.root_start();
  }

  /* Initialize UDP connections */
  simple_udp_register(&server_conn, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback);
  simple_udp_register(&client_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, NULL);

  NETSTACK_MAC.on();

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);

  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(tsch_is_coordinator) {
      break;
    }

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send network uptime timestamp to the DAG root */
      uint64_t network_uptime;
      network_uptime = tsch_get_network_uptime_ticks();
      simple_udp_sendto(&client_conn, &network_uptime, sizeof(network_uptime), &dest_ipaddr);
      LOG_INFO("Sent network uptime timestamp %lu to ", (unsigned long)network_uptime);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
    } else {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL
               - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
