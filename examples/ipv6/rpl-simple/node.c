/*
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
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/ip/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ip/uip-udp-packet.h"
#include "rpl.h"
#include "rpl-dag-root.h"
#include "node-id.h"
#include "simple-udp.h"
#include "sys/ctimer.h"
#include <stdio.h>
#include <string.h>

#include "dev/serial-line.h"
#include "net/ipv6/uip-ds6-route.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define UDP_PORT	8765

#define START_INTERVAL		(5 * CLOCK_SECOND)
#define SEND_INTERVAL		  (30 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

static uip_ipaddr_t node_ipaddr;
static uip_ipaddr_t root_ipaddr;
static uip_ipaddr_t destination_ipaddr;

/*---------------------------------------------------------------------------*/
PROCESS(node_process, "RPL Simple Process");
AUTOSTART_PROCESSES(&node_process);

/*---------------------------------------------------------------------------*/
static void
receiver(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  unsigned seq_id;
  memcpy(&seq_id, data, sizeof(int));
  if(uip_ip6addr_cmp(&destination_ipaddr, &node_ipaddr)) {
    LOG_INFO("received request %u from ", seq_id);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");

    LOG_INFO("sending reply %u to ", seq_id);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
    simple_udp_sendto(&udp_conn, &seq_id, sizeof(seq_id), sender_addr);
  } else {
    static unsigned reply_count = 0;
    reply_count++;
    LOG_INFO("received reply %u (%u/%u) from ", seq_id, reply_count, seq_id);
    LOG_INFO_6ADDR(sender_addr);
    LOG_INFO_("\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  if(rpl_is_reachable()) {
    static unsigned seq_id = 0;
    LOG_INFO("sending request %u to ", ++seq_id);
    LOG_INFO_6ADDR(&destination_ipaddr);
    LOG_INFO_("\n");
    simple_udp_sendto(&udp_conn, &seq_id, sizeof(seq_id), &destination_ipaddr);
  } else {
    LOG_INFO("not connected yet\n");
  }
}
/*---------------------------------------------------------------------------*/
static void
get_global_address(void)
{
  int i;
  uint8_t state;

  for(i = 0; i < UIP_DS6_ADDR_NB; i++) {
    state = uip_ds6_if.addr_list[i].state;
    if(uip_ds6_if.addr_list[i].isused &&
       state == ADDR_PREFERRED &&
       !uip_is_addr_linklocal(&uip_ds6_if.addr_list[i].ipaddr)) {
      uip_ip6addr_copy(&node_ipaddr, &uip_ds6_if.addr_list[i].ipaddr);
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer;

  PROCESS_BEGIN();

  /* Hardcoded IPv6 addresses */
  uip_ip6addr(&root_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x0212, 0x7401, 0x0001, 0x0101);
  uip_ip6addr(&destination_ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0x0212, 0x7401, 0x0001, 0x0101);

  rpl_dag_root_init(NULL, NULL);
  get_global_address();
  /* Configure root */
  if(uip_ip6addr_cmp(&root_ipaddr, &node_ipaddr)) {
    rpl_dag_root_init_dag_immediately();
  }

  /* New connection with remote host */
  simple_udp_register(&udp_conn, UDP_PORT, NULL, UDP_PORT, receiver);
  /* Wait for START_INTERVAL */
  etimer_set(&periodic, START_INTERVAL);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic));

#if MAC_CONF_WITH_TSCH
  NETSTACK_MAC.on();
#endif /* MAC_CONF_WITH_TSCH */

  if(!uip_ip6addr_cmp(&destination_ipaddr, &node_ipaddr)) {
    /* Send data periodically */
    etimer_set(&periodic, SEND_INTERVAL);
    while(1) {
      PROCESS_YIELD();

      if(etimer_expired(&periodic)) {
        etimer_reset(&periodic);
        ctimer_set(&backoff_timer, random_rand() % (SEND_INTERVAL), send_packet, NULL);
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
