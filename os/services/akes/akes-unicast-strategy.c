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

/**
 * \addtogroup akes
 * @{
 *
 * \file
 *         Secures all frames via pairwise session keys. Broadcast frames are
 *         sent as unicast frames to each permanent neighbor one after another.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "contiki.h"
#include "akes/akes.h"
#include "akes/akes-mac.h"
#include "akes/akes-nbr.h"
#include "lib/assert.h"
#include "lib/memb.h"
#include "net/mac/ccm-star-packetbuf.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-unicast"
#define LOG_LEVEL LOG_LEVEL_MAC

struct ongoing_broadcast {
  uint32_t neighbor_bitmap;
  void *ptr;
  mac_callback_t sent;
};

#if LLSEC802154_USES_FRAME_COUNTER && AKES_NBR_WITH_PAIRWISE_KEYS && !ANTI_REPLAY_WITH_SUPPRESSION
static void send_broadcast(struct ongoing_broadcast *ob);
static void on_broadcast_sent(void *ptr, int status, int transmissions);
static void quit_broadcast(struct ongoing_broadcast *ob, int status);
MEMB(ongoing_broadcasts_memb, struct ongoing_broadcast, QUEUEBUF_NUM);

/*---------------------------------------------------------------------------*/
static void
init(void)
{
  memb_init(&ongoing_broadcasts_memb);
}
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  if(!akes_mac_is_hello() && packetbuf_holds_broadcast()) {
    struct ongoing_broadcast *ob = memb_alloc(&ongoing_broadcasts_memb);
    if(!ob) {
      LOG_ERR("ongoing_broadcasts_memb is full\n");
      mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
      return;
    }
    assert(AKES_NBR_MAX <= (sizeof(ob->neighbor_bitmap) * 8));
    ob->neighbor_bitmap = 0;
    ob->sent = sent;
    ob->ptr = ptr;
#if LLSEC802154_USES_AUX_HEADER
    packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL,
                       AKES_MAC_UNICAST_SEC_LVL);
#endif /* LLSEC802154_USES_AUX_HEADER */
    send_broadcast(ob);
  } else {
    AKES_MAC_DECORATED_MAC.send(sent, ptr);
  }
}
/*---------------------------------------------------------------------------*/
static void
send_broadcast(struct ongoing_broadcast *ob)
{
  /* find a permanent neighbor that has not received this frame, yet */
  akes_nbr_entry_t *entry;
  for(entry = akes_nbr_head(AKES_NBR_PERMANENT);
      entry;
      entry = akes_nbr_next(entry, AKES_NBR_PERMANENT)) {
    if(ob->neighbor_bitmap & (1ul << akes_nbr_index_of(entry->permanent))) {
      /* this neighbor received this frame already */
      continue;
    }
    break;
  }

  if(!entry) {
    quit_broadcast(ob, MAC_TX_OK);
    return;
  }

  ob->neighbor_bitmap |= 1ul << akes_nbr_index_of(entry->permanent);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, akes_nbr_get_addr(entry));
  AKES_MAC_DECORATED_MAC.send(on_broadcast_sent, ob);
}
/*---------------------------------------------------------------------------*/
static void
on_broadcast_sent(void *ptr, int status, int transmissions)
{
  akes_mac_report_to_network_layer(status, transmissions);
  switch(status) {
  case MAC_TX_DEFERRED:
    /* we expect another callback at a later point in time */
    break;
  case MAC_TX_QUEUE_FULL:
    /* the MAC layer would likely report the same error if we continued */
    quit_broadcast(ptr, MAC_TX_QUEUE_FULL);
    break;
  default:
    send_broadcast(ptr);
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
quit_broadcast(struct ongoing_broadcast *ob, int status)
{
  if(memb_free(&ongoing_broadcasts_memb, ob) == -1) {
    assert(false);
  }
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
  mac_call_sent_callback(ob->sent, ob->ptr, status, 0);
}
/*---------------------------------------------------------------------------*/
static bool
before_creation(void)
{
  return true;
}
/*---------------------------------------------------------------------------*/
static bool
after_creation(void)
{
  uint8_t *dataptr = packetbuf_dataptr();
  uint16_t datalen = packetbuf_datalen();

  if(akes_mac_is_hello()) {
    ssize_t max_index = -1;
    for(akes_nbr_entry_t *entry = akes_nbr_head(AKES_NBR_PERMANENT);
        entry;
        entry = akes_nbr_next(entry, AKES_NBR_PERMANENT)) {
      size_t local_index = akes_nbr_index_of(entry->permanent);
      if(!akes_mac_aead(entry->permanent->pairwise_key,
                        false,
                        dataptr + datalen + (local_index * AKES_MAC_BROADCAST_MIC_LEN),
                        true)) {
        LOG_ERR("akes_mac_aead failed\n");
        return false;
      }
      if(local_index > max_index) {
        max_index = local_index;
      }
    }
    if(max_index >= 0) {
      packetbuf_set_datalen(datalen
                            + ((max_index + 1) * AKES_MAC_BROADCAST_MIC_LEN));
    }
  } else {
    akes_nbr_status_t status = akes_get_receiver_status();
    akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();

    if(!entry || !entry->refs[status]) {
      return false;
    }

    if(!akes_mac_aead(entry->refs[status]->pairwise_key,
                      akes_mac_get_sec_lvl() & (1 << 2),
                      dataptr + datalen,
                      true)) {
      LOG_ERR("akes_mac_aead failed\n");
      return false;
    }
    packetbuf_set_datalen(datalen + AKES_MAC_UNICAST_MIC_LEN);
  }
  return true;
}
/*---------------------------------------------------------------------------*/
static enum akes_mac_verify_result
verify(akes_nbr_t *sender)
{
  if(akes_mac_is_hello()) {
    uint8_t *dataptr = packetbuf_dataptr();
    packetbuf_set_datalen(AKES_HELLO_DATALEN);
    uint8_t *micptr = dataptr
                      + AKES_HELLO_DATALEN
                      + (sender->foreign_index * AKES_MAC_BROADCAST_MIC_LEN);
    uint8_t mic[AKES_MAC_BROADCAST_MIC_LEN];
    if(!akes_mac_aead(sender->pairwise_key, false, mic, false)) {
      LOG_ERR("akes_mac_aead failed\n");
      return AKES_MAC_VERIFY_RESULT_INAUTHENTIC;
    }
    if(memcmp(micptr, mic, AKES_MAC_BROADCAST_MIC_LEN)) {
      LOG_ERR("inauthentic HELLO\n");
      return AKES_MAC_VERIFY_RESULT_INAUTHENTIC;
    }
  } else {
    if(!akes_mac_unsecure(sender->pairwise_key)) {
      LOG_ERR("inauthentic unicast\n");
      return AKES_MAC_VERIFY_RESULT_INAUTHENTIC;
    }
  }

  if(anti_replay_was_replayed(&sender->anti_replay_info)) {
    LOG_ERR("replayed\n");
    return AKES_MAC_VERIFY_RESULT_REPLAYED;
  }

  return AKES_MAC_VERIFY_RESULT_SUCCESS;
}
/*---------------------------------------------------------------------------*/
static uint_fast8_t
get_overhead(void)
{
  return AKES_MAC_UNICAST_MIC_LEN;
}
/*---------------------------------------------------------------------------*/
static uint8_t *
write_piggyback(uint8_t *data, uint8_t cmd_id, akes_nbr_entry_t *entry)
{
  return data;
}
/*---------------------------------------------------------------------------*/
static const uint8_t *
read_piggyback(const uint8_t *data,
               uint8_t cmd_id,
               const akes_nbr_entry_t *entry,
               const akes_nbr_tentative_t *meta)
{
  return data;
}
/*---------------------------------------------------------------------------*/
static void
on_helloack_sent(akes_nbr_t *nbr)
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
const struct akes_mac_strategy akes_unicast_strategy = {
  init,
  ccm_star_packetbuf_set_nonce,
  send,
  before_creation,
  after_creation,
  verify,
  get_overhead,
  write_piggyback,
  read_piggyback,
  on_helloack_sent,
  on_fresh_authentic_hello,
  on_fresh_authentic_helloack,
};
/*---------------------------------------------------------------------------*/
#endif /* LLSEC802154_USES_FRAME_COUNTER && AKES_NBR_WITH_PAIRWISE_KEYS && !ANTI_REPLAY_WITH_SUPPRESSION  */
/** @} */
