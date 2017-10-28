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
 *
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/ctimer.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/uip-ds6.h"
#include "net/ipv6/uip-udp-packet.h"
#include "sys/ctimer.h"
#include "rpl.h"
#include "net/ipv6/uipbuf.h"
#include <stdio.h>
#include <string.h>

#include "dev/serial-line.h"
#include "net/ipv6/uip-ds6-route.h"

#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define UIP_IP_BUF       ((struct uip_ip_hdr *)&uip_buf[UIP_LLH_LEN])

#define DEBUG DEBUG_FULL
#include "net/ipv6/uip-debug.h"

#ifndef PERIOD
#define PERIOD 60
#endif

#define START_INTERVAL		(15 * CLOCK_SECOND)
#define SEND_INTERVAL		(PERIOD * CLOCK_SECOND)
#define SEND_TIME		(random_rand() % (SEND_INTERVAL))
#define MAX_PAYLOAD_LEN		30

static struct uip_udp_conn *client_conn;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static int seq_id;
static int reply;

static void
tcpip_handler(void)
{
  if(uip_newdata()) {
    reply++;
  }
}
/*---------------------------------------------------------------------------*/
static void
send_packet(void *ptr)
{
  char buf[MAX_PAYLOAD_LEN];
  rpl_dag_t *dag;
  seq_id++;
  sprintf(buf, "Hello %d from the client", seq_id);

  dag = rpl_get_any_dag();

  if(dag != NULL) {
    uip_udp_packet_sendto(client_conn, buf, strlen(buf),
                          &dag->dag_id, UIP_HTONS(UDP_SERVER_PORT));
  } else {
    PRINTF("Have not joined any DAG yet - could not send\n");
  }
}
/*---------------------------------------------------------------------------*/
static enum netstack_ip_action
ip_input(void)
{
  uint8_t proto = 0;
  uipbuf_get_last_header(uip_buf, uip_len, &proto);
  PRINTF("Incoming packet proto: %d from ", proto);
  PRINT6ADDR(&UIP_IP_BUF->srcipaddr);
  PRINTF("\n");
  return NETSTACK_IP_PROCESS;
}

static enum netstack_ip_action
ip_output(const linkaddr_t *localdest)
{
  uint8_t proto;
  uint8_t is_me = 0;
  uipbuf_get_last_header(uip_buf, uip_len, &proto);
  is_me =  uip_ds6_is_my_addr(&UIP_IP_BUF->srcipaddr);
  PRINTF("%s packet proto: %d to ", is_me ? "Send" : "Fwd ", proto);
  PRINT6ADDR(&UIP_IP_BUF->destipaddr);
  PRINTF("\n");
  if(is_me && proto == UIP_PROTO_UDP) {
    /* fake server-ip address here - just to emulate the UDP-example */
    PRINTF("DATA send to %d 'Hello %d'\n", 1, seq_id);
  }
  return NETSTACK_IP_PROCESS;
}

struct netstack_ip_packet_processor packet_processor = {
  .process_input = ip_input,
  .process_output = ip_output
};

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic;
  static struct ctimer backoff_timer;

  PROCESS_BEGIN();

  PROCESS_PAUSE();

  PRINTF("UDP client process started nbr:%d routes:%d\n",
         NBR_TABLE_CONF_MAX_NEIGHBORS, NETSTACK_MAX_ROUTE_ENTRIES);

  /* new connection with remote host */
  client_conn = udp_new(NULL, UIP_HTONS(UDP_SERVER_PORT), NULL);
  if(client_conn == NULL) {
    PRINTF("No UDP connection available, exiting the process!\n");
    PROCESS_EXIT();
  }
  udp_bind(client_conn, UIP_HTONS(UDP_CLIENT_PORT));

  PRINTF("Created a connection with the server ");
  PRINT6ADDR(&client_conn->ripaddr);
  PRINTF(" local/remote port %u/%u\n",
         UIP_HTONS(client_conn->lport), UIP_HTONS(client_conn->rport));

  /* register packet processor */
  netstack_ip_packet_processor_add(&packet_processor);

  etimer_set(&periodic, SEND_INTERVAL);
  while(1) {
    PROCESS_YIELD();
    if(ev == tcpip_event) {
      tcpip_handler();
    }

    if(etimer_expired(&periodic)) {
      etimer_reset(&periodic);
      ctimer_set(&backoff_timer, SEND_TIME, send_packet, NULL);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
