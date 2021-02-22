/*
 * Copyright (c) 2020, Institute of Electronics and Computer Science (EDI)
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
 *         Benchmark for data colllection: all nodes send packets to the root
 *         in randomized order; root logs the sequence numbers received.
 * \author
 *         Atis Elsts <atis.elsts@edi.lv>
 */

#include "contiki.h"
#include "sys/node-id.h"
#include "lib/random.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/simple-udp.h"
#include "net/routing/routing.h"

#include <inttypes.h>
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

/* Coordinator Node ID */
#if CONTIKI_TARGET_IOTLAB
/* for FIT IoT lab */
#define COORDINATOR_ID 42088
#else
/* for simulations */
#define COORDINATOR_ID 1
#endif

#define UDP_PORT	8765
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
  uint32_t seqnum;
  memcpy(&seqnum, data, sizeof(seqnum));
  LOG_INFO("app receive packet seqnum=%" PRIu32 " from=", seqnum);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
}
/*---------------------------------------------------------------------------*/
PROCESS(node_process, "Node");
AUTOSTART_PROCESSES(&node_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer periodic_timer;
  static uint32_t seqnum;
  static struct simple_udp_connection udp_conn;
  uip_ipaddr_t dest_address;

  PROCESS_BEGIN();

  simple_udp_register(&udp_conn, UDP_PORT, NULL,
                      UDP_PORT, udp_rx_callback);

  if(node_id == COORDINATOR_ID) {
    /* the RPL root automatically becomes TSCH coordinator */
    LOG_INFO("set as root\n");
    NETSTACK_ROUTING.root_start();

  }  else {
    /* initialize the packet generation timer */
    etimer_set(&periodic_timer, APP_WARM_UP_PERIOD_SEC * CLOCK_SECOND
        + random_rand() % (APP_SEND_INTERVAL_SEC * CLOCK_SECOND));

    while(1) {
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

      if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_address)) {
        /* generate a packet with a new seqnum */
        seqnum++;
        LOG_INFO("app generate packet seqnum=%" PRIu32 " node_id=%u\n", seqnum, node_id);
        simple_udp_sendto(&udp_conn, &seqnum, sizeof(seqnum), &dest_address);
      }

      etimer_set(&periodic_timer, APP_SEND_INTERVAL_SEC * CLOCK_SECOND);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
