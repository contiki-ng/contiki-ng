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
 *         Uses group session keys for securing frames.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "akes/akes-mac.h"
#include "akes/akes-nbr.h"
#include "akes/akes.h"
#include "net/mac/anti-replay.h"
#include "net/mac/ccm-star-packetbuf.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include <stdbool.h>
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-noncoresec"
#define LOG_LEVEL LOG_LEVEL_MAC

#if LLSEC802154_USES_FRAME_COUNTER && AKES_NBR_WITH_GROUP_KEYS
/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  AKES_MAC_DECORATED_MAC.send(sent, ptr);
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
  uint_fast8_t sec_lvl = akes_mac_get_sec_lvl();
  if(sec_lvl) {
    uint8_t *key;
    if(akes_get_receiver_status() == AKES_NBR_TENTATIVE) {
      akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
      if(!entry || !entry->tentative) {
        return false;
      }
      key = entry->tentative->tentative_pairwise_key;
    } else {
      key = akes_mac_group_key;
    }

    uint16_t datalen = packetbuf_datalen();
    uint8_t *dataptr = packetbuf_dataptr();
    if(!akes_mac_aead(key, (sec_lvl >> 2) & 1, dataptr + datalen, true)) {
      LOG_ERR("akes_mac_aead failed\n");
      return false;
    }
    packetbuf_set_datalen(datalen + akes_mac_mic_len());
  }
  return true;
}
/*---------------------------------------------------------------------------*/
static enum akes_mac_verify_result
verify(akes_nbr_t *sender)
{
  if(!akes_mac_unsecure(sender->group_key)) {
    LOG_ERR("inauthentic frame\n");
    return AKES_MAC_VERIFY_RESULT_INAUTHENTIC;
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
  return akes_mac_mic_len();
}
/*---------------------------------------------------------------------------*/
static uint8_t *
write_piggyback(uint8_t *data, uint8_t cmd_id, akes_nbr_entry_t *entry)
{
  return data;
}
/*---------------------------------------------------------------------------*/
const static uint8_t *
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
const struct akes_mac_strategy akes_noncoresec_strategy = {
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
#endif /* LLSEC802154_USES_FRAME_COUNTER && AKES_NBR_WITH_GROUP_KEYS */

/** @} */
