/*
 * Copyright (c) 2019, RISE Research Institutes of Sweden AB
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
 *   Packet injector for various protocol implementations in Contiki-NG.
 * \author
 *   Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"

/* Standard C and POSIX headers. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

/* Contiki-NG headers. */
#include <dev/ble-hal.h>
#include <net/ipv6/uip.h>
#include <net/ipv6/uiplib.h>
#include <net/mac/ble/ble-l2cap.h>
#include <net/netstack.h>
#include <net/packetbuf.h>
#include <net/ipv6/sicslowpan.h>
#include <net/app-layer/coap/coap.h>
#include <net/app-layer/coap/coap-engine.h>

/* Log configuration. */
#include "sys/log.h"
#define LOG_MODULE "PacketInjector"
#define LOG_LEVEL LOG_LEVEL_INFO

#define TEST_PROTOCOL_DEFAULT "uip"
#define TEST_BUFFER_SIZE 2000

#define TEST_COAP_ENDPOINT "fdfd::100"
#define TEST_COAP_PORT 8293

extern int contiki_argc;
extern char **contiki_argv;

typedef bool (*protocol_function_t)(char *, int);

/*---------------------------------------------------------------------------*/
PROCESS(packet_injector_process, "Packet injector process");
AUTOSTART_PROCESSES(&packet_injector_process);
/*---------------------------------------------------------------------------*/
static int
read_packet(const char *filename, char *buf, int max_len)
{
  int fd;
  int len;

  /* Read packet data from a file. */
  fd = open(filename, O_RDONLY);
  if(fd < 0) {
    LOG_ERR("open: %s\n", strerror(errno));
    return -1;
  }

  len = read(fd, buf, max_len);
  if(len < 0) {
    LOG_ERR("read: %s\n", strerror(errno));
    return -1;
  }

  return len;
}
/*---------------------------------------------------------------------------*/
static void
set_uip_buf(char *data, int len)
{
  if(len > sizeof(uip_buf)) {
    LOG_DBG("Adjusting the input length from %d to %d to fit the uIP buffer\n",
            len, (int)sizeof(uip_buf));
    len = sizeof(uip_buf);
  }

  uip_len = len;

  /* Fill uIP buffer with packet data. */
  memcpy(uip_buf, data, len);
}
/*---------------------------------------------------------------------------*/
static bool
inject_coap_packet(char *data, int len)
{
  static coap_endpoint_t end_point;

  uiplib_ipaddrconv(TEST_COAP_ENDPOINT, &end_point.ipaddr);
  end_point.port = TEST_COAP_PORT;
  end_point.secure = 0;

  coap_endpoint_print(&end_point);

  coap_receive(&end_point, (uint8_t *)data, len);

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
inject_tcpip_packet(char *data, int len)
{
  set_uip_buf(data, len);
  tcpip_input();

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
inject_uip_packet(char *data, int len)
{
  set_uip_buf(data, len);
  uip_input();

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
inject_ble_l2cap_packet(char *data, int len)
{
  packetbuf_copyfrom(data, len);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME_BLE_RX_EVENT);

  ble_l2cap_driver.input();

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
inject_sicslowpan_packet(char *data, int len)
{
  packetbuf_copyfrom(data, len);

  NETSTACK_NETWORK.input();
  NETSTACK_FRAMER.parse();

  sicslowpan_driver.input();

  return true;
}
/*---------------------------------------------------------------------------*/
protocol_function_t
select_protocol(const char *protocol_name)
{
  struct proto_mapper {
    const char *protocol_name;
    protocol_function_t function;
  };
  struct proto_mapper map[] = {
    {"coap", inject_coap_packet},
    {"ble-l2cap", inject_ble_l2cap_packet},
    {"sicslowpan", inject_sicslowpan_packet},
    {"tcpip", inject_tcpip_packet},
    {"uip", inject_uip_packet}
  };
  int i;

  if(protocol_name == NULL) {
    return NULL;
  }

  for(i = 0; i < sizeof(map) / sizeof(map[0]); i++) {
    if(strcasecmp(protocol_name, map[i].protocol_name) == 0) {
      return map[i].function;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
void
process_packet(const char *filename, const char *protocol_name, protocol_function_t protocol_input) {
  static char file_buf[TEST_BUFFER_SIZE];
  static int len;

  LOG_INFO("Using input file \"%s\"\n", filename);

  len = read_packet(filename, file_buf, TEST_BUFFER_SIZE);
  if(len < 0) {
    LOG_ERR("Unable to read packet data: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  }

  LOG_INFO("Injecting a packet of %d bytes into %s\n", len, protocol_name);

  if(protocol_input(file_buf, len) == false) {
    exit(EXIT_FAILURE);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(packet_injector_process, ev, data)
{
  static const char *filename;
  static const char *protocol_name;
  static protocol_function_t protocol_input;

  PROCESS_BEGIN();

  protocol_name = getenv("TEST_PROTOCOL");
  if(protocol_name == NULL) {
    protocol_name = TEST_PROTOCOL_DEFAULT;
  }

  protocol_input = select_protocol(protocol_name);
  if(protocol_input == NULL) {
    LOG_ERR("unsupported protocol: \"%s\"\n",
            protocol_name == NULL ? "<null>" : protocol_name);
    exit(EXIT_FAILURE);
  }

  for(int i = 1; i < contiki_argc; i++) {
    filename = contiki_argv[i];
    process_packet(filename, protocol_name, protocol_input);
  }

  exit(EXIT_SUCCESS);

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
