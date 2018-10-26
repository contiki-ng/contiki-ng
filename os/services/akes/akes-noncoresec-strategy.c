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
 *         Uses group session keys for securing frames.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "services/akes/akes.h"
#include "services/akes/akes-mac.h"
#include "net/mac/anti-replay.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "services/akes/akes-nbr.h"
#include "net/mac/ccm-star-packetbuf.h"
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-noncoresec"
#define LOG_LEVEL LOG_LEVEL_MAC

#if LLSEC802154_USES_FRAME_COUNTER && AKES_NBR_WITH_GROUP_KEYS
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  AKES_MAC_DECORATED_MAC.send(sent, ptr);
}
/*---------------------------------------------------------------------------*/
static int
on_frame_created(void)
{
  uint8_t sec_lvl;
  struct akes_nbr_entry *entry;
  uint8_t *key;
  uint8_t datalen;
  uint8_t *dataptr;

  sec_lvl = akes_mac_get_sec_lvl();
  if(sec_lvl) {
    entry = akes_nbr_get_receiver_entry();
    if(akes_get_receiver_status() == AKES_NBR_TENTATIVE) {
      if(!entry || !entry->tentative) {
        LOG_ERR_("%02x isn't tentative\n", packetbuf_addr(PACKETBUF_ADDR_RECEIVER)->u8[7]);
        return 0;
      }
      key = entry->tentative->tentative_pairwise_key;
    } else {
      key = akes_mac_group_key;
    }

    datalen = packetbuf_datalen();
    dataptr = packetbuf_dataptr();
    akes_mac_aead(key, sec_lvl & (1 << 2), dataptr + datalen, 1);
    packetbuf_set_datalen(datalen + akes_mac_mic_len());
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static enum akes_mac_verify
verify(struct akes_nbr *sender)
{
  if(akes_mac_verify(sender->group_key)) {
    LOG_ERR("inauthentic frame\n");
    return AKES_MAC_VERIFY_INAUTHENTIC;
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
  return akes_mac_mic_len();
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
}
/*---------------------------------------------------------------------------*/
const struct akes_mac_strategy akes_noncoresec_strategy = {
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
  init
};
/*---------------------------------------------------------------------------*/
#endif /* LLSEC802154_USES_FRAME_COUNTER && AKES_NBR_WITH_GROUP_KEYS */

/** @} */
