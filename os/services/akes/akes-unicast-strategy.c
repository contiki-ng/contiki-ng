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

/**
 * \addtogroup akes
 * @{
 * \file
 *         Secures all frames via pairwise session keys. Broadcast frames are
 *         sent as unicast frames to each permanent neighbor one after another.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "services/akes/akes.h"
#include "services/akes/akes-mac.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "lib/memb.h"
#include "net/nbr-table.h"
#include "net/mac/ccm-star-packetbuf.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-unicast"
#define LOG_LEVEL LOG_LEVEL_MAC

#ifdef AKES_UNICAST_STRATEGY_CONF_MAX_RETRANSMISSIONS
#define MAX_RETRANSMISSIONS AKES_UNICAST_STRATEGY_CONF_MAX_RETRANSMISSIONS
#else /* AKES_UNICAST_STRATEGY_CONF_MAX_RETRANSMISSIONS */
#define MAX_RETRANSMISSIONS 3
#endif /* AKES_UNICAST_STRATEGY_CONF_MAX_RETRANSMISSIONS */

struct ongoing_broadcast {
  uint32_t neighbor_bitmap;
  void *ptr;
  mac_callback_t sent;
  int transmissions;
};

#if LLSEC802154_USES_FRAME_COUNTER && AKES_NBR_WITH_PAIRWISE_KEYS
static void send_broadcast(struct ongoing_broadcast *ob);
static void on_broadcast_sent(void *ptr, int status, int transmissions);
MEMB(ongoing_broadcasts_memb, struct ongoing_broadcast, QUEUEBUF_NUM);

/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  struct ongoing_broadcast *ob;

  if(!akes_mac_is_hello() && packetbuf_holds_broadcast()) {
    ob = memb_alloc(&ongoing_broadcasts_memb);
    if(!ob) {
      LOG_ERR("ongoing_broadcasts_memb is full\n");
      mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
      return;
    }
    ob->neighbor_bitmap = 0;
    ob->sent = sent;
    ob->ptr = ptr;
    ob->transmissions = 0;
    send_broadcast(ob);
  } else {
    AKES_MAC_DECORATED_MAC.send(sent, ptr);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_broadcast(struct ongoing_broadcast *ob)
{
  struct akes_nbr_entry *entry;

  /* find a permanent neighbor that has not received this frame, yet */
  entry = akes_nbr_head();
  while(entry) {
    if(entry->permanent
        && !((1 << akes_nbr_index_of(entry->permanent)) & ob->neighbor_bitmap))  {
      break;
    }
    entry = akes_nbr_next(entry);
  }

  if(!entry) {
    memb_free(&ongoing_broadcasts_memb, ob);
    mac_call_sent_callback(ob->sent, ob->ptr, MAC_TX_OK, ob->transmissions);
    return;
  }

  ob->neighbor_bitmap |= (1 << akes_nbr_index_of(entry->permanent));
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, akes_nbr_get_addr(entry));
  packetbuf_set_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS, MAX_RETRANSMISSIONS);
  akes_mac_set_numbers(entry->permanent);
  AKES_MAC_DECORATED_MAC.send(on_broadcast_sent, ob);
}
/*---------------------------------------------------------------------------*/
static void
on_broadcast_sent(void *ptr, int status, int transmissions)
{
  struct ongoing_broadcast *ob;

  switch(status) {
  case MAC_TX_DEFERRED:
    return;
  case MAC_TX_OK:
  case MAC_TX_COLLISION:
  case MAC_TX_NOACK:
  case MAC_TX_ERR:
  case MAC_TX_ERR_FATAL:
    ob = (struct ongoing_broadcast *)ptr;
    ob->transmissions += transmissions;
    send_broadcast(ob);
    return;
  }
}
/*---------------------------------------------------------------------------*/
static int
on_frame_created(void)
{
  uint8_t *dataptr;
  uint8_t datalen;
  enum akes_nbr_status status;
  struct akes_nbr_entry *entry;
  int8_t max_index;
  uint8_t local_index;

  dataptr = packetbuf_dataptr();
  datalen = packetbuf_datalen();
  if(akes_mac_is_hello()) {
    entry = akes_nbr_head();
    max_index = -1;
    while(entry) {
      if(entry->permanent) {
        local_index = akes_nbr_index_of(entry->permanent);
        akes_mac_aead(entry->permanent->pairwise_key,
            0,
            dataptr + datalen + (local_index * AKES_MAC_BROADCAST_MIC_LEN),
            1);
        if(local_index > max_index) {
          max_index = local_index;
        }
      }
      entry = akes_nbr_next(entry);
    }
    if(max_index >= 0) {
      packetbuf_set_datalen(datalen + ((max_index + 1) * AKES_MAC_BROADCAST_MIC_LEN));
    }
  } else {
    status = akes_get_receiver_status();
    entry = akes_nbr_get_receiver_entry();

    if(!entry || !entry->refs[status]) {
      return 0;
    }

    akes_mac_aead(entry->refs[status]->pairwise_key,
        akes_mac_get_sec_lvl() & (1 << 2),
        dataptr + datalen,
        1);
    packetbuf_set_datalen(datalen + AKES_MAC_UNICAST_MIC_LEN);
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static enum akes_mac_verify
verify(struct akes_nbr *sender)
{
  uint8_t *dataptr;
  uint8_t *micptr;
  uint8_t mic[AKES_MAC_BROADCAST_MIC_LEN];

  if(akes_mac_is_hello()) {
    dataptr = packetbuf_dataptr();
    packetbuf_set_datalen(AKES_HELLO_DATALEN);
    micptr = dataptr
        + AKES_HELLO_DATALEN
        + (sender->foreign_index * AKES_MAC_BROADCAST_MIC_LEN);
    akes_mac_aead(sender->pairwise_key, 0, mic, 0);
    if(memcmp(micptr, mic, AKES_MAC_BROADCAST_MIC_LEN)) {
      LOG_ERR("inauthentic HELLO\n");
      return AKES_MAC_VERIFY_INAUTHENTIC;
    }
  } else {
    if(akes_mac_verify(sender->pairwise_key)) {
      LOG_ERR("inauthentic unicast\n");
      return AKES_MAC_VERIFY_INAUTHENTIC;
    }
  }

  if(anti_replay_was_replayed(&sender->anti_replay_info)) {
    LOG_ERR("replayed\n");
    return AKES_MAC_VERIFY_REPLAYED;
  }

  return AKES_MAC_VERIFY_SUCCESS;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_overhead(void)
{
  return AKES_MAC_UNICAST_MIC_LEN;
}
/*---------------------------------------------------------------------------*/
static uint8_t *
write_piggyback(uint8_t *data, uint8_t cmd_id, struct akes_nbr_entry *entry)
{
  return data;
}
/*---------------------------------------------------------------------------*/
static uint8_t *
read_piggyback(uint8_t *data,
    uint8_t cmd_id,
    struct akes_nbr_entry *entry,
    struct akes_nbr_tentative *meta)
{
  return data;
}
/*---------------------------------------------------------------------------*/
static int
before_create(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
on_helloack_sent(struct akes_nbr *nbr)
{
}
/*---------------------------------------------------------------------------*/
static void
on_fresh_authentic_hello(void)
{
}
/*---------------------------------------------------------------------------*/
static void
on_fresh_authentic_helloack(void)
{
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  memb_init(&ongoing_broadcasts_memb);
}
/*---------------------------------------------------------------------------*/
const struct akes_mac_strategy akes_unicast_strategy = {
  ccm_star_packetbuf_set_nonce,
  send,
  on_frame_created,
  verify,
  get_overhead,
  write_piggyback,
  read_piggyback,
  before_create,
  on_helloack_sent,
  on_fresh_authentic_hello,
  on_fresh_authentic_helloack,
  init,
};
/*---------------------------------------------------------------------------*/
#endif /* LLSEC802154_USES_FRAME_COUNTER && AKES_NBR_WITH_PAIRWISE_KEYS  */
/** @} */
