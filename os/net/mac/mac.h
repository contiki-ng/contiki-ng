/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 *         MAC driver header file
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef MAC_H_
#define MAC_H_

#include "contiki.h"
#include "dev/radio.h"

/**
 *\brief The default channel for IEEE 802.15.4 networks.
 */
#ifdef IEEE802154_CONF_DEFAULT_CHANNEL
#define IEEE802154_DEFAULT_CHANNEL           IEEE802154_CONF_DEFAULT_CHANNEL
#else /* IEEE802154_CONF_DEFAULT_CHANNEL */
#define IEEE802154_DEFAULT_CHANNEL           26
#endif /* IEEE802154_CONF_DEFAULT_CHANNEL */

typedef void (* mac_callback_t)(void *ptr, int status, int transmissions);

static inline void
mac_call_sent_callback(mac_callback_t sent, void *ptr, int status, int num_tx)
{
  if(sent) {
    sent(ptr, status, num_tx);
  }
}

/**
 * The structure of a MAC protocol driver in Contiki.
 */
struct mac_driver {
  char *name;

  /** Initialize the MAC driver */
  void (* init)(void);

  /** Send a packet from the packetbuf  */
  void (* send)(mac_callback_t sent_callback, void *ptr);

  /** Callback for getting notified of incoming packet. */
  void (* input)(void);

  /** Turn the MAC layer on. */
  int (* on)(void);

  /** Turn the MAC layer off. */
  int (* off)(void);

  /** Read out estimated max payload size based on payload in packetbuf */
  int (* max_payload)(void);
};

/* Generic MAC return values. */
enum {
  /**< The MAC layer transmission was OK. */
  MAC_TX_OK,

  /**< The MAC layer transmission could not be performed due to a
     collision. */
  MAC_TX_COLLISION,

  /**< The MAC layer did not get an acknowledgement for the packet. */
  MAC_TX_NOACK,

  /**< The MAC layer deferred the transmission for a later time. */
  MAC_TX_DEFERRED,

  /**< The MAC layer transmission could not be performed because of an
     error. The upper layer may try again later. */
  MAC_TX_ERR,

  /**< The MAC layer transmission could not be performed because of a
     fatal error. The upper layer does not need to try again, as the
     error will be fatal then as well. */
  MAC_TX_ERR_FATAL,

  /**< The MAC layer transmission could not be performed because of
     insufficient queue space, failure to allocate a neighbor,
     or insufficient packet memory space. The upper layer may try again later. */
  MAC_TX_QUEUE_FULL,
};

#endif /* MAC_H_ */
