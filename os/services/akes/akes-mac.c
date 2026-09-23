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
 *         Special MAC driver and special FRAMER.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "akes/akes-mac.h"
#include "akes/akes-trickle.h"
#include "akes/akes.h"
#include "contiki-net.h"
#include "lib/assert.h"
#include "lib/csprng.h"
#include "lib/random.h"
#include "net/link-stats.h"
#include "net/mac/anti-replay.h"
#include "net/mac/cmd-broker.h"
#include "net/packetbuf.h"
#include "sys/cc.h"
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-MAC"
#define LOG_LEVEL LOG_LEVEL_MAC

#if AKES_NBR_WITH_GROUP_KEYS
uint8_t akes_mac_group_key[AES_128_KEY_LENGTH];
#endif /* AKES_NBR_WITH_GROUP_KEYS */
#if AKES_NBR_WITH_SEQNOS
static uint8_t dsn;
#endif /* AKES_NBR_WITH_SEQNOS */

/*---------------------------------------------------------------------------*/
void
akes_mac_report_to_network_layer(int status, int transmissions)
{
  akes_mac_report_to_network_layer_with_address(
      packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
      status,
      transmissions);
}
/*---------------------------------------------------------------------------*/
void
akes_mac_report_to_network_layer_with_address(const linkaddr_t *address,
                                              int status,
                                              int transmissions)
{
  assert(!linkaddr_cmp(address, &linkaddr_null));
  assert(!linkaddr_cmp(address, &linkaddr_node_addr));
  link_stats_packet_sent(address, status, transmissions);
  NETSTACK_ROUTING.link_callback(address, status, transmissions);
}
/*---------------------------------------------------------------------------*/
void
akes_mac_set_numbers(akes_nbr_t *receiver)
{
#if AKES_NBR_WITH_SEQNOS
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, dsn++);
#endif /* AKES_NBR_WITH_SEQNOS */
#if LLSEC802154_USES_AUX_HEADER
  packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL, akes_mac_get_sec_lvl());
#endif /* LLSEC802154_USES_AUX_HEADER */
#if LLSEC802154_USES_FRAME_COUNTER
  if(AKES_NBR_WITH_SEQNOS && receiver) {
    /* frame counter is incremented in each transmission and retransmission */
    return;
  }
  anti_replay_set_counter(receiver ? &receiver->anti_replay_info : NULL);
#endif /* LLSEC802154_USES_FRAME_COUNTER */
}
/*---------------------------------------------------------------------------*/
bool
akes_mac_received_duplicate(akes_nbr_t *sender)
{
#if AKES_NBR_WITH_SEQNOS
  uint8_t seqno = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
  if(sender->has_active_seqno && sender->seqno == seqno) {
    return true;
  }
  sender->seqno = seqno;
  sender->seqno_timestamp = clock_seconds();
  sender->has_active_seqno = true;
#endif /* AKES_NBR_WITH_SEQNOS */
  return false;
}
/*---------------------------------------------------------------------------*/
static bool
is_cmd(uint8_t cmd_id)
{
  if(!packetbuf_holds_cmd_frame()) {
    return false;
  }
  return packetbuf_get_dispatch_byte() == cmd_id;
}
/*---------------------------------------------------------------------------*/
bool
akes_mac_is_hello(void)
{
  return is_cmd(AKES_HELLO_IDENTIFIER);
}
/*---------------------------------------------------------------------------*/
bool
akes_mac_is_helloack(void)
{
  return is_cmd(AKES_HELLOACK_IDENTIFIER)
         || is_cmd(AKES_HELLOACK_P_IDENTIFIER);
}
/*---------------------------------------------------------------------------*/
bool
akes_mac_is_ack(void)
{
  return is_cmd(AKES_ACK_IDENTIFIER);
}
/*---------------------------------------------------------------------------*/
bool
akes_mac_is_hello_helloack_or_ack(uint8_t dispatch_byte)
{
  switch(dispatch_byte) {
  case AKES_HELLO_IDENTIFIER:
  case AKES_HELLOACK_IDENTIFIER:
  case AKES_HELLOACK_P_IDENTIFIER:
  case AKES_ACK_IDENTIFIER:
    return true;
  default:
    return false;
  }
}
/*---------------------------------------------------------------------------*/
bool
akes_mac_is_update(void)
{
  return is_cmd(AKES_UPDATE_IDENTIFIER);
}
/*---------------------------------------------------------------------------*/
uint_fast8_t
akes_mac_get_sec_lvl(void)
{
  switch(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)) {
  case FRAME802154_CMDFRAME:
    switch(packetbuf_get_dispatch_byte()) {
    case AKES_HELLO_IDENTIFIER:
      return AKES_MAC_BROADCAST_SEC_LVL & 3;
    case AKES_HELLOACK_IDENTIFIER:
    case AKES_HELLOACK_P_IDENTIFIER:
    case AKES_ACK_IDENTIFIER:
      return AKES_ACKS_SEC_LVL;
    case AKES_UPDATE_IDENTIFIER:
      return AKES_UPDATES_SEC_LVL;
    default:
      break;
    }
    /* fall through */
  case FRAME802154_DATAFRAME:
    return packetbuf_holds_broadcast()
           ? AKES_MAC_BROADCAST_SEC_LVL
           : AKES_MAC_UNICAST_SEC_LVL;
  default:
    LOG_WARN("unhandled frame type %i\n",
             packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE));
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
akes_nbr_entry_t *
akes_mac_check_frame(void)
{
  akes_nbr_entry_t *entry = akes_nbr_get_sender_entry();
  if(!entry || !entry->permanent) {
    LOG_ERR("sender is not a permanent neighbor\n");
    return NULL;
  }

  bool is_broadcast = packetbuf_holds_broadcast();
  if((is_broadcast || AKES_MAC_UNSECURE_UNICASTS)
     && (AKES_MAC_STRATEGY.verify(entry->permanent)
         != AKES_MAC_VERIFY_RESULT_SUCCESS)) {
    LOG_ERR("inauthentic frame\n");
    return NULL;
  }

  if(!is_broadcast && akes_mac_received_duplicate(entry->permanent)) {
    LOG_ERR("received duplicate\n");
    return NULL;
  }

  AKES_DELETE_STRATEGY.prolong_permanent_neighbor(entry->permanent);
  return entry;
}
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  akes_nbr_t *receiver;
  if(packetbuf_holds_broadcast()) {
    if(!akes_nbr_count(AKES_NBR_PERMANENT)) {
      goto error;
    }
    receiver = NULL;
  } else {
    akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
    if(!entry || !entry->permanent) {
      goto error;
    }
    receiver = entry->permanent;
  }

  akes_mac_set_numbers(receiver);
  AKES_MAC_STRATEGY.send(sent, ptr);
  return;
error:
  mac_call_sent_callback(sent, ptr, MAC_TX_ERR_FATAL, 0);
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  if(akes_mac_is_update()) {
    akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
    if(!entry || !entry->permanent) {
      LOG_ERR("UPDATE to non-existent permanent neighbor\n");
      return FRAMER_FAILED;
    }
    if(!entry->permanent->is_receiving_update) {
      LOG_WARN("apparently a new session began\n");
      return FRAMER_FAILED;
    }
    if(!AKES_DELETE_STRATEGY.is_permanent_neighbor_expired(entry->permanent)) {
      LOG_WARN("canceling UPDATE\n");
      entry->permanent->is_receiving_update = false;
      return FRAMER_FAILED;
    }
  }

  if(!AKES_MAC_STRATEGY.before_creation()) {
    LOG_ERR("AKES_MAC_STRATEGY.before_creation() failed\n");
    return FRAMER_FAILED;
  }

  int result = AKES_MAC_DECORATED_FRAMER.create();
  if(result == FRAMER_FAILED) {
    LOG_ERR("AKES_MAC_DECORATED_FRAMER.create() failed\n");
    return FRAMER_FAILED;
  }
  if(!AKES_MAC_STRATEGY.after_creation()) {
    LOG_ERR("AKES_MAC_STRATEGY.after_creation() failed\n");
    return FRAMER_FAILED;
  }
  return result;
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
  return AKES_MAC_DECORATED_FRAMER.parse();
}
/*---------------------------------------------------------------------------*/
uint_fast8_t
akes_mac_mic_len(void)
{
  return packetbuf_holds_broadcast()
         ? AKES_MAC_BROADCAST_MIC_LEN
         : AKES_MAC_UNICAST_MIC_LEN;
}
/*---------------------------------------------------------------------------*/
bool
akes_mac_aead(const uint8_t key[static AES_128_KEY_LENGTH],
              bool shall_encrypt,
              uint8_t mic[static AKES_MAC_MIN_MIC_LEN],
              bool forward)
{
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  AKES_MAC_STRATEGY.generate_nonce(nonce, forward);

  uint8_t *a = packetbuf_hdrptr();
  uint16_t a_len;
  uint8_t *m;
  uint16_t m_len;
  if(shall_encrypt) {
#if AKES_NBR_WITH_GROUP_KEYS && PACKETBUF_WITH_UNENCRYPTED_BYTES
    a_len = packetbuf_hdrlen()
            + packetbuf_attr(PACKETBUF_ATTR_UNENCRYPTED_BYTES);
#else /* AKES_NBR_WITH_GROUP_KEYS && PACKETBUF_WITH_UNENCRYPTED_BYTES */
    a_len = packetbuf_hdrlen();
#endif /* AKES_NBR_WITH_GROUP_KEYS && PACKETBUF_WITH_UNENCRYPTED_BYTES */
    m = a + a_len;
    m_len = packetbuf_totlen() - a_len;
  } else {
    a_len = packetbuf_totlen();
    m = NULL;
    m_len = 0;
  }

  while(!CCM_STAR.get_lock()) {
    assert(false);
  }
  bool result = CCM_STAR.set_key(key)
                && CCM_STAR.aead(nonce,
                                 m, m_len,
                                 a, a_len,
                                 mic, akes_mac_mic_len(),
                                 forward);
  CCM_STAR.release_lock();
  return result;
}
/*---------------------------------------------------------------------------*/
bool
akes_mac_unsecure(const uint8_t *key)
{
  uint8_t generated_mic[MAX(AKES_MAC_UNICAST_MIC_LEN,
                            AKES_MAC_BROADCAST_MIC_LEN)];

  packetbuf_set_datalen(packetbuf_datalen() - akes_mac_mic_len());
  akes_mac_aead(key, akes_mac_get_sec_lvl() & (1 << 2), generated_mic, false);

  return !memcmp(generated_mic,
                 ((uint8_t *)packetbuf_dataptr()) + packetbuf_datalen(),
                 akes_mac_mic_len());
}
/*---------------------------------------------------------------------------*/
#if MAC_CONF_WITH_CSMA
static void
input(void)
{
  /* redirect input calls from radio drivers to CSMA */
  csma_driver.input();
}
void
akes_mac_input_from_csma(void)
#else /* MAC_CONF_WITH_CSMA */
static void
input(void)
#endif /* MAC_CONF_WITH_CSMA */
{
  switch(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)) {
  case FRAME802154_CMDFRAME:
    cmd_broker_publish();
    break;
  case FRAME802154_DATAFRAME:
    if(!akes_mac_check_frame()) {
      LOG_ERR("ignored incoming frame\n");
      return;
    }
    NETSTACK_NETWORK.input();
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
#if AKES_NBR_WITH_SEQNOS
  dsn = random_rand();
#endif /* AKES_NBR_WITH_SEQNOS */
  AKES_MAC_DECORATED_MAC.init();
  cmd_broker_init();
#if AKES_NBR_WITH_GROUP_KEYS
  if(!csprng_rand(akes_mac_group_key, AES_128_KEY_LENGTH)) {
    LOG_ERR("TODO handle CSPRNG error\n");
  }
#endif /* AKES_NBR_WITH_GROUP_KEYS */
  AKES_MAC_STRATEGY.init();
  akes_init();
}
/*---------------------------------------------------------------------------*/
static int
length(void)
{
#if LLSEC802154_USES_AUX_HEADER
  packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL, akes_mac_get_sec_lvl());
#endif /* LLSEC802154_USES_AUX_HEADER */
  return AKES_MAC_DECORATED_FRAMER.length() + AKES_MAC_STRATEGY.get_overhead();
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return AKES_MAC_DECORATED_MAC.on();
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return AKES_MAC_DECORATED_MAC.off();
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  return AKES_MAC_DECORATED_MAC.max_payload();
}
/*---------------------------------------------------------------------------*/
const struct mac_driver akes_mac_driver = {
  "AKES",
  init,
  send,
  input,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/
const struct framer akes_mac_framer = {
  length,
  create,
  parse,
};
/*---------------------------------------------------------------------------*/

/** @} */
