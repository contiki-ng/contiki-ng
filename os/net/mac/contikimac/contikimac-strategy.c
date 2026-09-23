/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
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
 * \file
 *         Uses group session keys for securing frames.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/contikimac/contikimac-strategy.h"
#include "lib/csprng.h"
#include "net/mac/anti-replay.h"
#include "net/mac/contikimac/contikimac-ccm-inputs.h"
#include "net/mac/contikimac/contikimac-framer-potr.h"
#include "net/mac/contikimac/contikimac-nbr.h"
#include "net/mac/contikimac/contikimac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "services/akes/akes-mac.h"
#include "services/akes/akes.h"
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ContikiMAC-strategy"
#define LOG_LEVEL LOG_LEVEL_MAC

#define BEFORE(a, b) ((int16_t)((a) - (b)) < 0)

#if CONTIKIMAC_FRAMER_POTR_ENABLED
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
linkaddr_t contikimac_strategy_sender_of_last_authentic_broadcast;
wake_up_counter_t
    contikimac_strategy_wake_up_counter_at_last_authentic_broadcast;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

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
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK || !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  akes_nbr_entry_t *entry;
  akes_nbr_t *nbr;
#endif

#if !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  if(!packetbuf_holds_broadcast()) {
    /* increment frame counter of unicasts in each (re)transmission */
    entry = akes_nbr_get_receiver_entry();
    if(!entry) {
      LOG_ERR("receiver entry not found\n");
      return false;
    }
    nbr = akes_mac_is_helloack()
          ? akes_nbr_get_receiver_entry()->tentative
          : akes_nbr_get_receiver_entry()->permanent;
    if(!nbr) {
      LOG_ERR("receiver not found\n");
      return false;
    }
    anti_replay_set_counter(&nbr->anti_replay_info);
  }
#endif /* !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

  if(!packetbuf_holds_cmd_frame()) {
    return true;
  }

  switch(packetbuf_get_dispatch_byte()) {
  case AKES_HELLOACK_IDENTIFIER:
  case AKES_HELLOACK_P_IDENTIFIER:
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    entry = akes_nbr_get_receiver_entry();
    if(!entry) {
      LOG_ERR("receiver entry not found\n");
      return false;
    }
    nbr = entry->tentative;
    if(!entry) {
      LOG_ERR("receiver not found\n");
      return false;
    }
    uint8_t *q = contikimac_nbr_get_tentative(nbr->meta)->q;
    if(!csprng_rand(q, CONTIKIMAC_Q_LEN)) {
      LOG_ERR("CSPRNG error\n");
      return false;
    }
    uint8_t *dataptr = packetbuf_dataptr();
    dataptr += AKES_HELLOACK_PIGGYBACK_OFFSET
               + (ANTI_REPLAY_WITH_SUPPRESSION ? 8 : 0);
    memcpy(dataptr, q, CONTIKIMAC_Q_LEN);
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    break;
  case AKES_ACK_IDENTIFIER:
    break;
  default:
    break;
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static bool
after_creation(void)
{
  uint_fast8_t sec_lvl = akes_mac_get_sec_lvl();
  if(sec_lvl) {
    akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
    uint8_t *key;
    if(akes_get_receiver_status() == AKES_NBR_TENTATIVE) {
      if(!entry || !entry->tentative) {
        LOG_ERR_LLADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
        LOG_ERR_("isn't tentative\n");
        return false;
      }
      key = entry->tentative->tentative_pairwise_key;
    } else {
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
      key = packetbuf_holds_broadcast()
            ? akes_mac_group_key
            : entry->permanent->group_key;
#else /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
      key = akes_mac_group_key;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
    }

    uint16_t datalen = packetbuf_datalen();
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    contikimac_ccm_inputs_generate_nonce(contikimac_state.strobe.nonce, true);
    contikimac_state.strobe.shall_encrypt = akes_mac_get_sec_lvl() & (1 << 2);
    if(contikimac_state.strobe.shall_encrypt) {
      contikimac_state.strobe.a_len =
          packetbuf_hdrlen()
          + packetbuf_attr(PACKETBUF_ATTR_UNENCRYPTED_BYTES);
      contikimac_state.strobe.m_len =
          packetbuf_totlen() - contikimac_state.strobe.a_len;
    } else {
      contikimac_state.strobe.a_len = packetbuf_totlen();
      contikimac_state.strobe.m_len = 0;
    }
    contikimac_state.strobe.mic_len = akes_mac_mic_len();
    contikimac_state.strobe.totlen = packetbuf_totlen();
    memcpy(contikimac_state.strobe.unsecured_frame,
           packetbuf_hdrptr(),
           packetbuf_totlen());
    contikimac_state.strobe.strobe_index_offset =
        contikimac_framer_potr_get_strobe_index_offset(
            contikimac_state.strobe.unsecured_frame[0]);

    switch(contikimac_state.strobe.unsecured_frame[0]) {
    case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLO:
      contikimac_state.strobe.phase_offset =
          packetbuf_hdrlen()
          + AKES_HELLO_PIGGYBACK_OFFSET
          + (ANTI_REPLAY_WITH_SUPPRESSION ? 4 : 0);
      break;
    case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK:
      contikimac_state.strobe.phase_offset =
          packetbuf_hdrlen()
          + AKES_HELLOACK_PIGGYBACK_OFFSET
          + (ANTI_REPLAY_WITH_SUPPRESSION ? 8 : 0)
          + CONTIKIMAC_Q_LEN;
      break;
    default:
      break;
    }
    akes_nbr_copy_key(contikimac_state.strobe.key, key);
#else /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    {
      uint8_t *dataptr;
      dataptr = packetbuf_dataptr();
      if(!akes_mac_aead(key, sec_lvl & (1 << 2), dataptr + datalen, true)) {
        LOG_ERR("akes_mac_aead failed\n");
        return false;
      }
    }
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    packetbuf_set_datalen(datalen + akes_mac_mic_len());
  }
  return true;
}
/*---------------------------------------------------------------------------*/
static enum akes_mac_verify_result
verify(akes_nbr_t *sender)
{
  if(!akes_mac_unsecure(
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
         packetbuf_holds_broadcast()
         ? sender->group_key
         : akes_mac_group_key)) {
#else /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
         sender->group_key)) {
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
    return AKES_MAC_VERIFY_RESULT_INAUTHENTIC;
  }

#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  if(packetbuf_holds_broadcast()) {
    linkaddr_copy(&contikimac_strategy_sender_of_last_authentic_broadcast,
                  packetbuf_addr(PACKETBUF_ADDR_SENDER));
    contikimac_strategy_wake_up_counter_at_last_authentic_broadcast =
        contikimac_get_wake_up_counter(contikimac_get_last_wake_up_time());
  }
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

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
  switch(cmd_id) {
  case AKES_HELLO_IDENTIFIER:
#if ANTI_REPLAY_WITH_SUPPRESSION
    memcpy(data, &anti_replay_my_unicast_counter, 4);
    data += 4;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    data += CONTIKIMAC_FRAMER_POTR_PHASE_LEN;
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    data += CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    break;
  case AKES_HELLOACK_IDENTIFIER:
  case AKES_HELLOACK_P_IDENTIFIER:
#if ANTI_REPLAY_WITH_SUPPRESSION
    anti_replay_write_counter(data);
    data += 4;
    anti_replay_write_my_broadcast_counter(data);
    data += 4;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    data += CONTIKIMAC_Q_LEN;
    data += CONTIKIMAC_FRAMER_POTR_PHASE_LEN;
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    data += CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
    break;
  case AKES_ACK_IDENTIFIER:
#if ANTI_REPLAY_WITH_SUPPRESSION
    anti_replay_write_my_broadcast_counter(data);
    data += 4;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
    {
      contikimac_nbr_tentative_t *contikimac_nbr_tentative =
          contikimac_nbr_get_tentative(entry->tentative->meta);
      memcpy(data, contikimac_nbr_tentative->q, CONTIKIMAC_Q_LEN);
      data += CONTIKIMAC_Q_LEN;
      data[0] = contikimac_nbr_tentative->strobe_index;
      data++;
    }
    data[0] = contikimac_get_last_delta();
    data++;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    break;
#if ANTI_REPLAY_WITH_SUPPRESSION
  case AKES_UPDATE_IDENTIFIER:
    anti_replay_write_my_broadcast_counter(data);
    data += 4;
    break;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
  default:
    break;
  }
  return data;
}
/*---------------------------------------------------------------------------*/
static const uint8_t *
read_piggyback(const uint8_t *data,
               uint8_t cmd_id,
               const akes_nbr_entry_t *entry,
               const akes_nbr_tentative_t *meta)
{
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  contikimac_nbr_tentative_t *contikimac_nbr_tentative;
  contikimac_nbr_t *contikimac_nbr;
  rtimer_clock_t phase;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */

  switch(cmd_id) {
  case AKES_HELLO_IDENTIFIER:
#if ANTI_REPLAY_WITH_SUPPRESSION
    /* TODO: endianness */
    memcpy(&entry->tentative->anti_replay_info.last_unicast_counter, data, 4);
    data += 4;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    memcpy(&phase, data, CONTIKIMAC_FRAMER_POTR_PHASE_LEN);
    contikimac_nbr_tentative =
        contikimac_nbr_get_tentative(entry->tentative->meta);
    contikimac_nbr_tentative->phase.t =
        contikimac_get_sfd_timestamp() + phase - WAKE_UP_COUNTER_INTERVAL;
    data += CONTIKIMAC_FRAMER_POTR_PHASE_LEN;
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    contikimac_nbr_tentative->phase.his_wake_up_counter_at_t =
        wake_up_counter_parse(data);
    data += CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    break;
  case AKES_HELLOACK_IDENTIFIER:
  case AKES_HELLOACK_P_IDENTIFIER:
#if ANTI_REPLAY_WITH_SUPPRESSION
    data += 4;
    entry->permanent->anti_replay_info.last_broadcast_counter =
        anti_replay_read_counter(data);
    data += 4;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    contikimac_nbr_tentative =
        contikimac_nbr_get_tentative(entry->tentative->meta);
    uint8_t *hdrptr = packetbuf_hdrptr();
    contikimac_nbr_tentative->strobe_index =
        hdrptr[contikimac_framer_potr_get_strobe_index_offset(hdrptr[0])];
    memcpy(contikimac_nbr_tentative->q, data, CONTIKIMAC_Q_LEN);
    data += CONTIKIMAC_Q_LEN;
    contikimac_nbr = contikimac_nbr_get(entry->permanent);
    memcpy(&phase, data, CONTIKIMAC_FRAMER_POTR_PHASE_LEN);
    contikimac_nbr->phase.t =
        contikimac_get_sfd_timestamp() + phase - WAKE_UP_COUNTER_INTERVAL;
    data += CONTIKIMAC_FRAMER_POTR_PHASE_LEN;
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    contikimac_nbr->phase.his_wake_up_counter_at_t =
        wake_up_counter_parse(data);
    data += CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    break;
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  case AKES_ACK_IDENTIFIER:
#if ANTI_REPLAY_WITH_SUPPRESSION
    entry->permanent->anti_replay_info.last_broadcast_counter =
        anti_replay_read_counter(data);
    data += 4;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
    data += CONTIKIMAC_Q_LEN + 1;
    contikimac_nbr = contikimac_nbr_get(entry->permanent);
    contikimac_nbr_tentative = contikimac_nbr_get_tentative(meta);
    contikimac_nbr->phase.t =
        contikimac_nbr_tentative->t1 - (data[0] << CONTIKIMAC_DELTA_SHIFT);
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    contikimac_nbr->phase.his_wake_up_counter_at_t =
        contikimac_nbr_tentative->predicted_wake_up_counter;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
    data += 1;
    break;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
#if ANTI_REPLAY_WITH_SUPPRESSION
  case AKES_UPDATE_IDENTIFIER:
    entry->permanent->anti_replay_info.last_broadcast_counter =
        anti_replay_read_counter(data);
    data += 4;
    break;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
  default:
    break;
  }

  return data;
}
/*---------------------------------------------------------------------------*/
static void
on_helloack_sent(akes_nbr_t *nbr)
{
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  contikimac_nbr_tentative_t *contikimac_nbr_tentative =
      contikimac_nbr_get_tentative(nbr->meta);
  contikimac_nbr_tentative->t1 = contikimac_get_last_but_one_t1();
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  contikimac_nbr_tentative->predicted_wake_up_counter =
      contikimac_state.strobe.receivers_wake_up_counter;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
  contikimac_nbr_tentative->strobe_index = contikimac_get_last_strobe_index();
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
}
/*---------------------------------------------------------------------------*/
static void
on_fresh_authentic_hello(void)
{
  leaky_bucket_effuse(&contikimac_framer_potr_hello_inc_bucket);
}
/*---------------------------------------------------------------------------*/
static void
on_fresh_authentic_helloack(void)
{
  leaky_bucket_effuse(&contikimac_framer_potr_helloack_inc_bucket);
}
/*---------------------------------------------------------------------------*/
const struct akes_mac_strategy contikimac_strategy = {
  init,
  contikimac_ccm_inputs_generate_nonce,
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
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
static bool
is_permanent_neighbor_expired(akes_nbr_t *nbr)
{
  contikimac_nbr_t *contikimac_nbr = contikimac_nbr_get(nbr);
  return ((RTIMER_NOW() - contikimac_nbr->phase.t)
          >= (AKES_NBR_LIFETIME * RTIMER_SECOND))
#if ANTI_REPLAY_WITH_SUPPRESSION
         || BEFORE(contikimac_nbr->broadcast_expiration_time,
                   (uint16_t)clock_seconds())
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
         ;
}
/*---------------------------------------------------------------------------*/
static void
prolong_permanent_neighbor(akes_nbr_t *nbr)
{
#if ANTI_REPLAY_WITH_SUPPRESSION
  if(akes_mac_is_update()
     || akes_mac_is_helloack()
     || akes_mac_is_ack()
     || packetbuf_holds_broadcast()) {
    contikimac_nbr_get(nbr)->broadcast_expiration_time =
        clock_seconds() + AKES_NBR_LIFETIME;
  }
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
}
/*---------------------------------------------------------------------------*/
const struct akes_delete_strategy contikimac_strategy_delete = {
  is_permanent_neighbor_expired,
  prolong_permanent_neighbor
};
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
/*---------------------------------------------------------------------------*/
#endif /* CONTIKIMAC_FRAMER_POTR_ENABLED */
