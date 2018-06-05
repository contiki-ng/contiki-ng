/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
*         The 802.15.4 standard CSMA protocol (nonbeacon-enabled)
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#include "net/mac/csma/csma.h"
#include "net/mac/csma/csma-output.h"
#include "net/mac/mac-sequence.h"
#include "net/packetbuf.h"
#include "net/netstack.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSMA"
#define LOG_LEVEL LOG_LEVEL_MAC

/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
  csma_output_packet(sent, ptr);
}
/*---------------------------------------------------------------------------*/
static void
input_packet(void)
{
#if CSMA_SEND_SOFT_ACK
  int original_datalen;
  uint8_t *original_dataptr;

  original_datalen = packetbuf_datalen();
  original_dataptr = packetbuf_dataptr();
#endif

  if(packetbuf_datalen() == CSMA_ACK_LEN) {
    /* Ignore ack packets */
    LOG_DBG("ignored ack\n");
  } else if(NETSTACK_FRAMER.parse() < 0) {
    LOG_ERR("failed to parse %u\n", packetbuf_datalen());
  } else if(!linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                                         &linkaddr_node_addr) &&
            !packetbuf_holds_broadcast()) {
    LOG_WARN("not for us\n");
  } else {
    int duplicate = 0;

    /* Check for duplicate packet. */
    duplicate = mac_sequence_is_duplicate();
    if(duplicate) {
      /* Drop the packet. */
      LOG_WARN("drop duplicate link layer packet from ");
      LOG_WARN_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
      LOG_WARN_(", seqno %u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO));
    } else {
      mac_sequence_register_seqno();
    }

#if CSMA_SEND_SOFT_ACK
    {
      frame802154_t info154;
      frame802154_parse(original_dataptr, original_datalen, &info154);
      if(info154.fcf.frame_type == FRAME802154_DATAFRAME &&
         info154.fcf.ack_required != 0 &&
         linkaddr_cmp((linkaddr_t *)&info154.dest_addr,
                      &linkaddr_node_addr)) {
        uint8_t ackdata[CSMA_ACK_LEN] = {0, 0, 0};

        ackdata[0] = FRAME802154_ACKFRAME;
        ackdata[1] = 0;
        ackdata[2] = info154.seq;
        NETSTACK_RADIO.send(ackdata, CSMA_ACK_LEN);
      }
    }
#endif /* CSMA_SEND_SOFT_ACK */
    if(!duplicate) {
      LOG_INFO("received packet from ");
      LOG_INFO_LLADDR(packetbuf_addr(PACKETBUF_ADDR_SENDER));
      LOG_INFO_(", seqno %u, len %u\n", packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO), packetbuf_datalen());
      NETSTACK_NETWORK.input();
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return NETSTACK_RADIO.off();
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  csma_output_init();
  on();
}
/*---------------------------------------------------------------------------*/
const struct mac_driver csma_driver = {
  "CSMA",
  init,
  send_packet,
  input_packet,
  on,
  off
};
/*---------------------------------------------------------------------------*/
