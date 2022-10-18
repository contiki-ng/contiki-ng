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
 *   A TCP server example.
 * \author
 *   Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */


#include <contiki.h>
#include <net/ipv6/uip-ds6.h>
#include <net/ipv6/tcp-socket.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TCPServer"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(test_tcp_server, "TCP server");
AUTOSTART_PROCESSES(&test_tcp_server);

#define TCP_TEST_PORT 18962
#define SOCKET_BUF_SIZE 128

static struct tcp_socket server_sock;
static uint8_t in_buf[SOCKET_BUF_SIZE];
static uint8_t out_buf[SOCKET_BUF_SIZE];
static size_t bytes_received;
/*****************************************************************************/
static int
data_callback(struct tcp_socket *sock, void *ptr, const uint8_t *input, int len)
{
  if(len >= 0) {
    bytes_received += len;
  }
  LOG_INFO("RECV %d bytes (total %zu)\n", len, bytes_received);

  return 0;
}
/*****************************************************************************/
static void
event_callback(struct tcp_socket *sock, void *ptr, tcp_socket_event_t event)
{
  LOG_INFO("TCP socket event: ");
  switch(event) {
  case TCP_SOCKET_CONNECTED:
    LOG_INFO_("CONNECTED\n");
    break;
  case TCP_SOCKET_CLOSED:
    LOG_INFO_("CLOSED\n");
    break;
  case TCP_SOCKET_TIMEDOUT:
    LOG_INFO_("TIMED OUT\n");
    break;
  case TCP_SOCKET_ABORTED:
    LOG_INFO_("ABORTED\n");
    break;
  case TCP_SOCKET_DATA_SENT:
    LOG_INFO_("DATA SENT\n");
    break;
  default:
    LOG_INFO_("UNKNOWN (%d)\n", (int)event);
    break;
  }
  tcp_socket_unregister(&server_sock);
}
/*****************************************************************************/
PROCESS_THREAD(test_tcp_server, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("Listening for TCP connections on port %d\n", TCP_TEST_PORT);

  int ret = tcp_socket_register(&server_sock, NULL, in_buf, sizeof(in_buf),
                                out_buf, sizeof(out_buf),
                                data_callback, event_callback);
  if(ret < 0) {
    LOG_ERR("Failed to register a TCP socket\n");
    PROCESS_EXIT();
  }

  if(tcp_socket_listen(&server_sock, TCP_TEST_PORT) < 0) {
    LOG_ERR("Failed to listen on port %d\n", TCP_TEST_PORT);
    tcp_socket_unregister(&server_sock);
    PROCESS_EXIT();
  }

  for(;;) {
    PROCESS_YIELD();
  }

  PROCESS_END();
}
/*****************************************************************************/
