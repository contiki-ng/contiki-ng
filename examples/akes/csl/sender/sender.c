/*
 * Copyright (c) 2017, Hasso-Plattner-Institut.
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
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"
#include "sys/etimer.h"
#include "services/akes/akes-nbr.h"
#include "services/akes/akes-trickle.h"
#include "net/mac/wake-up-counter.h"

#define DEBUG 1
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else /* DEBUG */
#define PRINTF(...)
#endif /* DEBUG */

PROCESS(sender_process, "sender_process");
AUTOSTART_PROCESSES(&sender_process);

/*---------------------------------------------------------------------------*/
static void
on_sent(void *ptr, int status, int transmissions)
{
  static int tx_ok;
  static int tx_collision;
  static int tx_noack;
  static int tx_deferred;
  static int tx_err;
  static int tx_err_fatal;

  switch(status) {
  case MAC_TX_OK:
    tx_ok++;
    break;
  case MAC_TX_COLLISION:
    tx_collision++;
    break;
  case MAC_TX_NOACK:
    tx_noack++;
    break;
  case MAC_TX_DEFERRED:
    tx_deferred++;
    break;
  case MAC_TX_ERR:
    tx_err++;
    break;
  case MAC_TX_ERR_FATAL:
    tx_err_fatal++;
    break;
  }
  PRINTF("OK %i / COLLISION %i / NOACK %i / DEFERRED %i / ERR %i / ERR_FATAL %i\n",
      tx_ok,
      tx_collision,
      tx_noack,
      tx_deferred,
      tx_err,
      tx_err_fatal);
  process_poll(&sender_process);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sender_process, ev, data)
{
  struct akes_nbr_entry *entry;
  static struct etimer periodic_timer;
  static linkaddr_t receivers_address;
  uint8_t payload_len;

  PROCESS_BEGIN();

  /* disable neighbor discovery once we have found the receiver side */
  etimer_set(&periodic_timer, CLOCK_SECOND / WAKE_UP_COUNTER_RATE);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    etimer_reset(&periodic_timer);
    entry = akes_nbr_head();
    if(entry && entry->permanent) {
      akes_trickle_stop();
      linkaddr_copy(&receivers_address, akes_nbr_get_addr(entry));
      break;
    }
  }

  /* send a maximum-length unicast frame to the receiver side */
  while(1) {
    packetbuf_clear();
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &receivers_address);
    payload_len = RADIO_MAX_FRAME_LEN - NETSTACK_FRAMER.length();
    memset(packetbuf_dataptr(), 0xFF, payload_len);
    packetbuf_set_datalen(payload_len);
    NETSTACK_MAC.send(on_sent, NULL);
    PROCESS_YIELD();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
