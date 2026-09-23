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
 * \addtogroup akes
 * @{
 *
 * \file
 *         Uses pairwise session keys for securing unicast frames
 *         and EBEAP for securing broadcast frames.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "akes/akes-nbr.h"
#include "akes/akes.h"
#include "dev/watchdog.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "net/mac/mac.h"
#include "net/mac/anti-replay.h"
#include "net/mac/ccm-star-packetbuf.h"
#include "net/mac/cmd-broker.h"
#include "net/mac/framer/crc16-framer.h"
#include "net/nbr-table.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "sys/cc.h"
#include <stdbool.h>
#include <string.h>

#define MAX_BUFFERED_MICS (5)
#define WITH_BROADCAST_ENCRYPTION (AKES_MAC_BROADCAST_SEC_LVL & (1 << 2))
#define ANNOUNCE_IDENTIFIER (0x0D)

#ifdef AKES_CORESEC_STRATEGY_CONF_CUT_CHECKSUM
#define CUT_CHECKSUM AKES_CORESEC_STRATEGY_CONF_CUT_CHECKSUM
#else /* AKES_CORESEC_STRATEGY_CONF_CUT_CHECKSUM */
#define CUT_CHECKSUM (1)
#endif /* AKES_CORESEC_STRATEGY_CONF_CUT_CHECKSUM */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-coresec"
#define LOG_LEVEL LOG_LEVEL_MAC

struct mic {
  uint8_t u8[AKES_MAC_BROADCAST_MIC_LEN];
};

#if LLSEC802154_USES_FRAME_COUNTER \
    && AKES_NBR_WITH_PAIRWISE_KEYS \
    && (!WITH_BROADCAST_ENCRYPTION || AKES_NBR_WITH_GROUP_KEYS)

static cmd_broker_result_t on_command(uint8_t cmd_id, uint8_t *payload);

static struct mic mics[MAX_BUFFERED_MICS];
static uint8_t next_mic_index;
static cmd_broker_subscription_t subscription = { NULL, on_command };

/*---------------------------------------------------------------------------*/
static void
init(void)
{
  cmd_broker_subscribe(&subscription);
}
/*---------------------------------------------------------------------------*/
/**
 * Payload format:
 * | 0x0d | 0x00 | CCM*-MIC for neighbor 0 | ... | CCM*-MIC for last neighbor |
 */
static bool
prepare_announce(void)
{
  uint8_t announced_mics[NBR_TABLE_MAX_NEIGHBORS * AKES_MAC_BROADCAST_MIC_LEN];
  memset(announced_mics, 0, sizeof(announced_mics));
  uint_fast16_t announced_mics_len = 0;
  for(akes_nbr_entry_t *entry = akes_nbr_head(AKES_NBR_PERMANENT);
      entry;
      entry = akes_nbr_next(entry, AKES_NBR_PERMANENT)) {
    size_t offset = akes_nbr_index_of(entry->permanent)
                    * AKES_MAC_BROADCAST_MIC_LEN;
    if(!akes_mac_aead(entry->permanent->pairwise_key,
                      false,
                      announced_mics + offset,
                      true)) {
      return false;
    }
    announced_mics_len = MAX(announced_mics_len,
                             offset + AKES_MAC_BROADCAST_MIC_LEN);
  }

  uint8_t *payload =
      cmd_broker_prepare_command(ANNOUNCE_IDENTIFIER, &linkaddr_null);

  /* write payload */
  /* TODO We are assuming that all MICs fit within a single ANNOUNCE command */
  *payload++ = 0; /* local index */
  memcpy(payload, announced_mics, announced_mics_len);
  packetbuf_set_datalen(1 /* command frame identifier */
                        + 1 /* local index */
                        + announced_mics_len);
  return true;
}
/*---------------------------------------------------------------------------*/
static void
send_broadcast(mac_callback_t sent, void *ptr)
{
  /* send ANNOUNCE */
  struct queuebuf *qb = queuebuf_new_from_packetbuf();
  if(!qb) {
    LOG_ERR("queuebuf_new_from_packetbuf failed\n");
    mac_call_sent_callback(sent, ptr, MAC_TX_QUEUE_FULL, 0);
    return;
  }
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
  if(NETSTACK_FRAMER.create() == FRAMER_FAILED) {
    queuebuf_free(qb);
    goto error;
  }
#if CUT_CHECKSUM
  packetbuf_set_datalen(packetbuf_datalen() - CRC16_FRAMER_CHECKSUM_LEN);
#endif /* CUT_CHECKSUM */
  if(!prepare_announce()) {
    queuebuf_free(qb);
    goto error;
  }
  AKES_MAC_DECORATED_MAC.send(NULL, NULL);
  watchdog_periodic();

  /* send actual broadcast */
  queuebuf_to_packetbuf(qb);
  queuebuf_free(qb);
#if WITH_BROADCAST_ENCRYPTION
  uint8_t ignore[AKES_MAC_BROADCAST_MIC_LEN];
  if(akes_mac_get_sec_lvl() & (1 << 2)) {
    if(!akes_mac_aead(akes_mac_group_key, true, ignore, true)) {
      goto error;
    }
  }
#endif /* WITH_BROADCAST_ENCRYPTION */
  AKES_MAC_DECORATED_MAC.send(sent, ptr);
  return;
error:
  LOG_ERR("failed to send broadcast\n");
  mac_call_sent_callback(sent, ptr, MAC_TX_ERR_FATAL, 0);
}
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  if(packetbuf_holds_broadcast()) {
    send_broadcast(sent, ptr);
  } else {
    AKES_MAC_DECORATED_MAC.send(sent, ptr);
  }
}
/*---------------------------------------------------------------------------*/
static bool
is_mic_stored(const uint8_t *mic)
{
  for(size_t i = 0; i < MAX_BUFFERED_MICS; i++) {
    if(!memcmp(mic, mics[i].u8, AKES_MAC_BROADCAST_MIC_LEN)) {
      return true;
    }
  }
  return false;
}
/*---------------------------------------------------------------------------*/
static cmd_broker_result_t
on_command(uint8_t cmd_id, uint8_t *payload)
{
  if(cmd_id != ANNOUNCE_IDENTIFIER) {
    return CMD_BROKER_UNCONSUMED;
  }

  akes_nbr_entry_t *entry = akes_nbr_get_sender_entry();
  if(!entry || !entry->permanent) {
    return CMD_BROKER_CONSUMED;
  }

  LOG_INFO("received ANNOUNCE\n");

  /* calculate CCM*-MIC location */
  payload +=
      1 + (entry->permanent->foreign_index * AKES_MAC_BROADCAST_MIC_LEN);

  /* check if CCM*-MIC location is within ANNOUNCE */
  const uint8_t *max_payload =
      ((uint8_t *)packetbuf_dataptr()) + packetbuf_datalen() - 1;
  if(payload + AKES_MAC_BROADCAST_MIC_LEN - 1 > max_payload) {
    LOG_ERR("out of bounds\n");
    return CMD_BROKER_CONSUMED;
  }

  /*
   * check if contained CCM*-MIC is already stored, e.g.,
   * due to duplicated ANNOUNCE
   */
  if(is_mic_stored(payload)) {
    LOG_WARN("already stored\n");
    return CMD_BROKER_CONSUMED;
  }

  /* store CCM*-MIC */
  memcpy(mics[next_mic_index].u8, payload, AKES_MAC_BROADCAST_MIC_LEN);
  if(++next_mic_index == MAX_BUFFERED_MICS) {
    next_mic_index = 0;
  }

  return CMD_BROKER_CONSUMED;
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

  if(sec_lvl && !packetbuf_holds_broadcast()) {
    akes_nbr_status_t status = akes_get_receiver_status();
    akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();

    if(!entry || !entry->refs[status]) {
      return false;
    }

    uint8_t *dataptr = packetbuf_dataptr();
    uint16_t datalen = packetbuf_datalen();

    if(!akes_mac_aead(entry->refs[status]->pairwise_key,
                      sec_lvl & (1 << 2),
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
static bool
unsecure_broadcast(akes_nbr_t *sender)
{
  uint8_t mic[AKES_MAC_BROADCAST_MIC_LEN];
#if WITH_BROADCAST_ENCRYPTION
  if(!akes_mac_aead(sender->group_key,
                    akes_mac_get_sec_lvl() & (1 << 2),
                    mic,
                    false)) {
    goto error;
  }
#endif /* WITH_BROADCAST_ENCRYPTION */
  if(!akes_mac_aead(sender->pairwise_key, false, mic, false)) {
    goto error;
  }
  return is_mic_stored(mic);
error:
  LOG_ERR("akes_mac_aead failed\n");
  return false;
}
/*---------------------------------------------------------------------------*/
static enum akes_mac_verify_result
verify(akes_nbr_t *sender)
{
  if(packetbuf_holds_broadcast()) {
    if(!unsecure_broadcast(sender)) {
      LOG_ERR("inauthentic broadcast\n");
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
  return packetbuf_holds_broadcast() ? 0 : AKES_MAC_UNICAST_MIC_LEN;
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
const struct akes_mac_strategy akes_coresec_strategy = {
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
#endif

/** @} */
