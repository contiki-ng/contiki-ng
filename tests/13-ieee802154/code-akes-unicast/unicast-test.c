/*
 * Copyright (c) 2018, Hasso-Plattner-Institut.
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

#include "contiki.h"
#include "services/akes/akes-nbr.h"
#include "services/akes/akes-mac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include <stdio.h>

PROCESS(unicast_test_process, "unicast_test_process");
AUTOSTART_PROCESSES(&unicast_test_process);
static struct etimer timer;
struct akes_nbr_entry *entry;
static linkaddr_t receivers_address;
static uint8_t counter;

/*---------------------------------------------------------------------------*/
static void
on_sent(void *ptr, int status, int transmissions)
{
  switch(status) {
  case MAC_TX_OK:
    printf("unicast %i/8 was acknowledged\n", counter + 1);
    break;
  case MAC_TX_COLLISION:
    printf("=check-me= FAILED - MAC_TX_COLLISION\n");
    break;
  case MAC_TX_NOACK:
    printf("=check-me= FAILED - MAC_TX_NOACK\n");
    break;
  case MAC_TX_DEFERRED:
    break;
  case MAC_TX_ERR:
    printf("=check-me= FAILED - MAC_TX_ERR\n");
    break;
  case MAC_TX_ERR_FATAL:
    printf("=check-me= FAILED - MAC_TX_ERR_FATAL\n");
    break;
  default:
    printf("=check-me= FAILED - invalid status\n");
    break;
  }
  if(status != MAC_TX_DEFERRED) {
    process_poll(&unicast_test_process);
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(unicast_test_process, ev, data)
{
  PROCESS_BEGIN();

  /* wait for session key establishment */
  etimer_set(&timer, CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
    entry = akes_nbr_head();
    if(entry && entry->permanent) {
      linkaddr_copy(&receivers_address, akes_nbr_get_addr(entry));
      break;
    }
    etimer_reset(&timer);
  }

  /* send 8 maximum-length unicast frames to our neighbor */
  for(; counter < 8; counter++) {
    packetbuf_clear();
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &receivers_address);
    memset(packetbuf_dataptr(), 0xFF, NETSTACK_MAC.max_payload());
    packetbuf_set_datalen(NETSTACK_MAC.max_payload());
    NETSTACK_MAC.send(on_sent, NULL);
    PROCESS_YIELD();
    etimer_set(&timer,
        akes_mac_random_clock_time(CLOCK_SECOND, CLOCK_SECOND * 2));
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));
  }

  printf("=check-me= DONE\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
