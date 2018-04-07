/*
 * Copyright (c) 2016, Yasuyuki Tanaka
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <contiki.h>
#include <net/mac/tsch/tsch.h>
#include <net/ipv6/uip.h>
#include <net/ipv6/uip-ds6.h>
#include <serial-shell.h>

#include <stdio.h>

PROCESS(node_process, "Node");
AUTOSTART_PROCESSES(&node_process);

PROCESS_THREAD(node_process, ev, data)
{
  static struct etimer et;
#if WITH_ULA
  static uip_ipaddr_t ipaddr;
#endif /* WITH_ULA */

#if WITH_TSCH
  static linkaddr_t coordinator_addr =  {{0x00, 0x01, 0x00, 0x01,
                                          0x00, 0x01, 0x00, 0x01}};
#endif /* WITH_TSCH */

  PROCESS_BEGIN();

#if WITH_TSCH
  if(linkaddr_cmp(&coordinator_addr, &linkaddr_node_addr)) {
    tsch_set_coordinator(1);
  }
#endif /* WITH_TSCH */

#if WITH_ULA
  uip_ip6addr(&ipaddr, UIP_DS6_DEFAULT_PREFIX, 0, 0, 0, 0, 0, 0, 0);
  uip_ds6_prefix_add(&ipaddr, UIP_DEFAULT_PREFIX_LEN, 0, 0, 0, 0);
  uip_ds6_set_addr_iid(&ipaddr, &uip_lladdr);
  uip_ds6_addr_add(&ipaddr, 0, ADDR_AUTOCONF);
#endif /* WITH_ULA */

  serial_shell_init();

  etimer_set(&et, CLOCK_SECOND);
  while(1) {
    /*
     * keep doing something so that simulation runs until its timeout
     */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    printf(".\n");
    etimer_reset(&et);
  }

  PROCESS_END();
}
