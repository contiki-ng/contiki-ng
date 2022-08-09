/*
 * Copyright (c) 2022, RISE Research Institutes of Sweden AB
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
 */

/**
 * \file
 *   A TCP client example.
 * \author
 *   Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */


#include <contiki.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ipv6/tcp-socket.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TCPClient"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(test_tcp_client, "TCP client");
AUTOSTART_PROCESSES(&test_tcp_client);

#define TCP_TEST_PORT 18962
#define TEST_STREAM_LENGTH 100000
#define SOCKET_BUF_SIZE 128

static struct tcp_socket client_sock;
static uint8_t in_buf[SOCKET_BUF_SIZE];
static uint8_t out_buf[SOCKET_BUF_SIZE];
static size_t total_bytes_received;
static volatile bool can_send;
/*****************************************************************************/
static int
data_callback(struct tcp_socket *sock, void *ptr, const uint8_t *input, int len)
{
  LOG_INFO("RECV %d bytes\n", len);
  if(len >= 0) {
    total_bytes_received += len;
  }

  return 0;
}
/*****************************************************************************/
static void
event_callback(struct tcp_socket *sock, void *ptr, tcp_socket_event_t event)
{
  LOG_INFO_("TCP socket event: ");
  switch(event) {
  case TCP_SOCKET_CONNECTED:
    LOG_INFO("CONNECTED\n");
    can_send = true;
    process_poll(&test_tcp_client);
    break;
  case TCP_SOCKET_CLOSED:
    LOG_INFO("CLOSED\n");
    break;
  case TCP_SOCKET_TIMEDOUT:
    LOG_INFO("TIMED OUT\n");
    break;
  case TCP_SOCKET_ABORTED:
    LOG_INFO("ABORTED\n");
    break;
  case TCP_SOCKET_DATA_SENT:
    LOG_INFO("DATA SENT\n");
    can_send = true;
    process_poll(&test_tcp_client);
    break;
  default:
    LOG_INFO("UNKNOWN (%d)\n", (int)event);
    break;
  }
}
/*****************************************************************************/
PROCESS_THREAD(test_tcp_client, ev, data)
{
  static struct etimer startup_timer;
  static uip_ipaddr_t addr;
  static uint8_t buf[100];
  static size_t bytes_sent;

  PROCESS_BEGIN();

  etimer_set(&startup_timer, CLOCK_SECOND * 3);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&startup_timer));

  addr.u16[0] = UIP_HTONS(0xfe80);
  addr.u16[4] = UIP_HTONS(0x201);
  addr.u16[5] = UIP_HTONS(1);
  addr.u16[6] = UIP_HTONS(1);
  addr.u16[7] = UIP_HTONS(1);

  LOG_INFO("Connecting to the TCP server on address ");
  LOG_6ADDR(LOG_LEVEL_INFO, &addr);
  LOG_INFO_(" and port %d\n", TCP_TEST_PORT);

  int ret = tcp_socket_register(&client_sock, NULL, in_buf, sizeof(in_buf),
                                out_buf, sizeof(out_buf),
                                data_callback, event_callback);
  if(ret < 0) {
    LOG_ERR("Failed to register a TCP socket\n");
    PROCESS_EXIT();
  }

  ret = tcp_socket_connect(&client_sock, &addr, TCP_TEST_PORT);
  if(ret < 0) {
    LOG_ERR("Failed to connect\n");
    PROCESS_EXIT();
  }

  memset(buf, 0xca, sizeof(buf));

  LOG_INFO("Sending %u bytes to the server...\n", TEST_STREAM_LENGTH);

  for(; bytes_sent < TEST_STREAM_LENGTH;) {
    PROCESS_YIELD();
    if(can_send) {
      if(tcp_socket_send(&client_sock, buf, sizeof(buf)) < 0) {
        LOG_ERR("Failed to send %zu bytes\n", sizeof(buf));
        PROCESS_EXIT();
      }
      bytes_sent += sizeof(buf);
      can_send = false;
    }
  }

  LOG_INFO("Sent %zu bytes successfully\n", bytes_sent);
  LOG_INFO("Test OK\n");

  PROCESS_END();
}
/*****************************************************************************/
