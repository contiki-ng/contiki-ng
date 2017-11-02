/*
 * Copyright (c) 2011, Swedish Institute of Computer Science.
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
/**
 * \file
 *         border-router
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#include "contiki.h"
#include "contiki-net.h"
#include "rpl.h"
#include "border-router-common.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"

#define DEBUG DEBUG_FULL
#include "net/ipv6/uip-debug.h"

#include <stdlib.h>

#define MAX_SENSORS 4

extern long slip_sent;
extern long slip_received;

static uint8_t mac_set;

static uint8_t sensor_count = 0;

/* allocate MAX_SENSORS char[32]'s */
static char sensors[MAX_SENSORS][32];

extern int contiki_argc;
extern char **contiki_argv;
extern const char *slip_config_ipaddr;

CMD_HANDLERS(border_router_cmd_handler);

PROCESS(border_router_process, "Border router process");

/*---------------------------------------------------------------------------*/
static void
request_mac(void)
{
  write_to_slip((uint8_t *)"?M", 2);
}
/*---------------------------------------------------------------------------*/
void
border_router_set_mac(const uint8_t *data)
{
  memcpy(uip_lladdr.addr, data, sizeof(uip_lladdr.addr));
  linkaddr_set_node_addr((linkaddr_t *)uip_lladdr.addr);

  /* is this ok - should instead remove all addresses and
     add them back again - a bit messy... ?*/
  PROCESS_CONTEXT_BEGIN(&tcpip_process);
  uip_ds6_init();
  rpl_init();
  PROCESS_CONTEXT_END(&tcpip_process);

  mac_set = 1;
}
/*---------------------------------------------------------------------------*/
void
border_router_print_stat()
{
  printf("bytes received over SLIP: %ld\n", slip_received);
  printf("bytes sent over SLIP: %ld\n", slip_sent);
}
/*---------------------------------------------------------------------------*/
/* Format: <name=value>;<name=value>;...;<name=value>*/
/* this function just cut at ; and store in the sensor array */
void
border_router_set_sensors(const char *data, int len)
{
  int i;
  int last_pos = 0;
  int sc = 0;
  for(i = 0; i < len; i++) {
    if(data[i] == ';') {
      sensors[sc][i - last_pos] = 0;
      memcpy(sensors[sc++], &data[last_pos], i - last_pos);
      last_pos = i + 1; /* skip the ';' */
    }
    if(sc == MAX_SENSORS) {
      sensor_count = sc;
      return;
    }
  }
  sensors[sc][len - last_pos] = 0;
  memcpy(sensors[sc++], &data[last_pos], len - last_pos);
  sensor_count = sc;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();
  prefix_set = 0;

  PROCESS_PAUSE();

  process_start(&border_router_cmd_process, NULL);

  PRINTF("RPL-Border router started\n");

  slip_config_handle_arguments(contiki_argc, contiki_argv);

  /* tun init is also responsible for setting up the SLIP connection */
  tun_init();

  while(!mac_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_mac();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

  if(slip_config_ipaddr != NULL) {
    uip_ipaddr_t prefix;

    if(uiplib_ipaddrconv((const char *)slip_config_ipaddr, &prefix)) {
      PRINTF("Setting prefix ");
      PRINT6ADDR(&prefix);
      PRINTF("\n");
      set_prefix_64(&prefix);
    } else {
      PRINTF("Parse error: %s\n", slip_config_ipaddr);
      exit(0);
    }
  }

  print_local_addresses();

  while(1) {
    etimer_set(&et, CLOCK_SECOND * 2);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    /* do anything here??? */
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
