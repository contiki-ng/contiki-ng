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

#include "net/routing/routing.h"
#include "rpl-border-router.h"
#include "cmd.h"
#include "border-router.h"
#include "border-router-cmds.h"
#include "border-router-cbor.h"
#include "tun6-net.h"
#include "dev/radio.h"
#include "net/mac/mac.h"
#include "net/mac/framer/frame802154.h"

#if BUILD_WITH_NAT64
#include "nat64-platform.h"
#endif /* BUILD_WITH_NAT64 */

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "BR"
#define LOG_LEVEL LOG_LEVEL_INFO

#include <stdlib.h>
#include <stdbool.h>

extern long slip_sent;
extern long slip_received;

static bool is_mac_set;

extern int contiki_argc;
extern char **contiki_argv;

CMD_HANDLERS(border_router_cmd_handler);

PROCESS(border_router_process, "Border router process");

/*---------------------------------------------------------------------------*/
static void
request_mac(void)
{
#if BORDER_ROUTER_SERIAL_RADIO
  /* serialradio: ask the radio to report its EUI-64 so we can adopt it as our
     own link-layer address (see border_router_set_mac()). */
  br_cbor_send_get_addr64(0);
#else
  write_to_slip((uint8_t *)"?M", 2);
#endif
}
/*---------------------------------------------------------------------------*/
void
border_router_set_mac(const uint8_t *data)
{
  if(is_mac_set) {
    /* only set MAC address once */
    return;
  }

  memcpy(uip_lladdr.addr, data, sizeof(uip_lladdr.addr));
  linkaddr_set_node_addr((linkaddr_t *)uip_lladdr.addr);

  /* is this ok - should instead remove all addresses and
     add them back again - a bit messy... ?*/
  PROCESS_CONTEXT_BEGIN(&tcpip_process);
  uip_ds6_init();
  NETSTACK_ROUTING.init();
  PROCESS_CONTEXT_END(&tcpip_process);

  is_mac_set = true;
}
/*---------------------------------------------------------------------------*/
void
border_router_print_stat()
{
  printf("bytes received over SLIP: %ld\n", slip_received);
  printf("bytes sent over SLIP: %ld\n", slip_sent);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(border_router_process, ev, data)
{
  static struct etimer et;

  PROCESS_BEGIN();

  is_mac_set = false;
  prefix_set = 0;

  PROCESS_PAUSE();

  process_start(&border_router_cmd_process, NULL);

  LOG_INFO("RPL-Border router started\n");

  slip_config_handle_arguments(contiki_argc, contiki_argv);

  /* tun init is also responsible for setting up the SLIP connection */
  tun_init();

#if BUILD_WITH_NAT64
  if(nat64_is_enabled()) {
    if(!nat64_platform_init()) {
      LOG_ERR("Failed to initialize NAT64\n");
    }
  }
#endif /* BUILD_WITH_NAT64 */

  while(!is_mac_set) {
    etimer_set(&et, CLOCK_SECOND);
    request_mac();
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
  }

#if BORDER_ROUTER_SERIAL_RADIO
  /* The radio's EUI-64 is now adopted as our link-layer address.  Configure
     the serial radio for border-router operation: matching PAN ID and
     channel, then enable address-filtered, auto-ACKing router mode so that
     unicast traffic to this node is received and acknowledged in hardware. */
  br_cbor_send_set_param(0, RADIO_PARAM_PAN_ID, IEEE802154_PANID);
  br_cbor_send_set_param(0, RADIO_PARAM_CHANNEL, IEEE802154_DEFAULT_CHANNEL);
  br_cbor_send_router_mode(0, true);
#endif /* BORDER_ROUTER_SERIAL_RADIO */

  const char *config_ipaddr = tun6_net_get_prefix();
  if(config_ipaddr != NULL) {
    uip_ipaddr_t prefix;

    if(uiplib_ipaddrconv(config_ipaddr, &prefix)) {
      LOG_INFO("Setting prefix ");
      LOG_INFO_6ADDR(&prefix);
      LOG_INFO_("\n");
      set_prefix_64(&prefix);
    } else {
      LOG_ERR("Parse error: %s\n", config_ipaddr);
      exit(EXIT_FAILURE);
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
