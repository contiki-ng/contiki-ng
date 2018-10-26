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
 *         Special MAC driver and special FRAMER.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "services/akes/akes-mac.h"
#include "services/akes/akes-trickle.h"
#include "services/akes/akes.h"
#include "net/mac/cmd-broker.h"
#include "net/mac/anti-replay.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "lib/csprng.h"
#include "lib/random.h"
#include "dev/watchdog.h"
#include "sys/cc.h"
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-MAC"
#define LOG_LEVEL LOG_LEVEL_MAC

#if AKES_NBR_WITH_GROUP_KEYS
uint8_t akes_mac_group_key[AES_128_KEY_LENGTH];
#endif /* AKES_NBR_WITH_GROUP_KEYS */

/*---------------------------------------------------------------------------*/
void
akes_mac_set_numbers(struct akes_nbr *receiver)
{
#if AKES_NBR_WITH_SEQNOS
  if(receiver) {
    if(akes_mac_is_hello() || akes_mac_is_helloack() || akes_mac_is_ack()) {
      return;
    }
    packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, ++receiver->my_unicast_seqno);
  } else {
#if LLSEC802154_USES_FRAME_COUNTER
    akes_mac_add_security_header(receiver);
#endif /* LLSEC802154_USES_FRAME_COUNTER */
  }
#elif LLSEC802154_USES_FRAME_COUNTER
  akes_mac_add_security_header(receiver);
#endif /* LLSEC802154_USES_FRAME_COUNTER */
}
/*---------------------------------------------------------------------------*/
int
akes_mac_received_duplicate(struct akes_nbr *sender)
{
#if AKES_NBR_WITH_SEQNOS
  uint8_t seqno;

  if(akes_mac_is_hello()
      || akes_mac_is_helloack()
      || akes_mac_is_ack()
      || packetbuf_holds_broadcast()) {
    return 0;
  }

  seqno = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
  if(sender->his_unicast_seqno == seqno) {
    return 1;
  }
  sender->his_unicast_seqno = seqno;
#endif /* AKES_NBR_WITH_SEQNOS */
  return 0;
}
/*---------------------------------------------------------------------------*/
clock_time_t
akes_mac_random_clock_time(clock_time_t min, clock_time_t max)
{
  clock_time_t range;
  uint8_t highest_bit;
  clock_time_t random;
  clock_time_t mask;

  range = max - min;
  if(!range) {
    return min;
  }

  highest_bit = (sizeof(clock_time_t) * 8) - 1;
  if((1 << highest_bit) & range) {
    memset(&mask, 0xFF, sizeof(clock_time_t));
  } else {
    do {
      highest_bit--;
    } while(!((1 << highest_bit) & range));
    mask = (1 << (highest_bit + 1)) - 1;
  }

  do {
    random = random_rand() & mask;
  } while(random > range);

  return min + random;
}
/*---------------------------------------------------------------------------*/
uint8_t
akes_mac_get_cmd_id(void)
{
  return ((uint8_t *)packetbuf_dataptr())[0];
}
/*---------------------------------------------------------------------------*/
static int
is_cmd(uint8_t cmd_id)
{
  if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) != FRAME802154_CMDFRAME) {
    return 0;
  }
  return akes_mac_get_cmd_id() == cmd_id;
}
/*---------------------------------------------------------------------------*/
int
akes_mac_is_hello(void)
{
  return is_cmd(AKES_HELLO_IDENTIFIER);
}
/*---------------------------------------------------------------------------*/
int
akes_mac_is_helloack(void)
{
  return is_cmd(AKES_HELLOACK_IDENTIFIER)
      || is_cmd(AKES_HELLOACK_P_IDENTIFIER);
}
/*---------------------------------------------------------------------------*/
int
akes_mac_is_ack(void)
{
  return is_cmd(AKES_ACK_IDENTIFIER);
}
/*---------------------------------------------------------------------------*/
int
akes_mac_is_update(void)
{
  return is_cmd(AKES_UPDATE_IDENTIFIER);
}
/*---------------------------------------------------------------------------*/
uint8_t
akes_mac_get_sec_lvl(void)
{
  switch(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)) {
  case FRAME802154_CMDFRAME:
    switch(akes_mac_get_cmd_id()) {
    case AKES_HELLO_IDENTIFIER:
      return AKES_MAC_BROADCAST_SEC_LVL & 3;
    case AKES_HELLOACK_IDENTIFIER:
    case AKES_HELLOACK_P_IDENTIFIER:
    case AKES_ACK_IDENTIFIER:
      return AKES_ACKS_SEC_LVL;
    case AKES_UPDATE_IDENTIFIER:
      return AKES_UPDATES_SEC_LVL;
    default:
      return 0;
    }
  case FRAME802154_DATAFRAME:
    return packetbuf_holds_broadcast()
        ? AKES_MAC_BROADCAST_SEC_LVL
        : AKES_MAC_UNICAST_SEC_LVL;
  default:
    LOG_WARN("unhandled frame type %i %02x\n", packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE), akes_mac_get_cmd_id());
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
#if LLSEC802154_USES_FRAME_COUNTER
void
akes_mac_add_security_header(struct akes_nbr *receiver)
{
  if(!anti_replay_set_counter()) {
    watchdog_reboot();
  }
#if LLSEC802154_USES_AUX_HEADER
  packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL, akes_mac_get_sec_lvl());
#endif /* LLSEC802154_USES_AUX_HEADER */
}
#endif /* LLSEC802154_USES_FRAME_COUNTER */
/*---------------------------------------------------------------------------*/
uint8_t *
akes_mac_prepare_command(uint8_t cmd_id, const linkaddr_t *dest)
{
  uint8_t *payload;

  /* reset packetbuf */
  packetbuf_clear();
  payload = packetbuf_dataptr();

  /* create frame */
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, dest);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CMDFRAME);
  payload[0] = cmd_id;

  return payload + 1;
}
/*---------------------------------------------------------------------------*/
void
akes_mac_send_command_frame(void)
{
  AKES_MAC_DECORATED_MAC.send(NULL, NULL);
}
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent, void *ptr)
{
  struct akes_nbr_entry *entry;
  struct akes_nbr *receiver;

  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  if(packetbuf_holds_broadcast()) {
    if(!akes_nbr_count(AKES_NBR_PERMANENT)) {
      mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
      return;
    }
    receiver = NULL;
  } else {
    entry = akes_nbr_get_receiver_entry();
    if(!entry || !entry->permanent) {
      mac_call_sent_callback(sent, ptr, MAC_TX_ERR, 0);
      return;
    }
    receiver = entry->permanent;
  }

  akes_mac_set_numbers(receiver);
  AKES_MAC_STRATEGY.send(sent, ptr);
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  int result;

  if(AKES_MAC_STRATEGY.before_create() == FRAMER_FAILED) {
    LOG_ERR("AKES_MAC_STRATEGY.before_create() failed\n");
    return FRAMER_FAILED;
  }

  result = AKES_MAC_DECORATED_FRAMER.create();
  if(result == FRAMER_FAILED) {
    LOG_ERR("AKES_MAC_DECORATED_FRAMER.create() failed\n");
    return FRAMER_FAILED;
  }
  if(!AKES_MAC_STRATEGY.on_frame_created()) {
    LOG_ERR("AKES_MAC_STRATEGY failed\n");
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
uint8_t
akes_mac_mic_len(void)
{
  return packetbuf_holds_broadcast() ? AKES_MAC_BROADCAST_MIC_LEN : AKES_MAC_UNICAST_MIC_LEN;
}
/*---------------------------------------------------------------------------*/
void
akes_mac_aead(uint8_t *key, int shall_encrypt, uint8_t *result, int forward)
{
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  uint8_t *m;
  uint8_t m_len;
  uint8_t *a;
  uint8_t a_len;

  AKES_MAC_STRATEGY.generate_nonce(nonce, forward);
  a = packetbuf_hdrptr();
  if(shall_encrypt) {
#if AKES_NBR_WITH_GROUP_KEYS && PACKETBUF_WITH_UNENCRYPTED_BYTES
    a_len = packetbuf_hdrlen() + packetbuf_attr(PACKETBUF_ATTR_UNENCRYPTED_BYTES);
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

  AES_128_GET_LOCK();
  CCM_STAR.set_key(key);
  CCM_STAR.aead(nonce,
      m, m_len,
      a, a_len,
      result, akes_mac_mic_len(),
      forward);
  AES_128_RELEASE_LOCK();
}
/*---------------------------------------------------------------------------*/
int
akes_mac_verify(uint8_t *key)
{
  int shall_decrypt;
  uint8_t generated_mic[MAX(AKES_MAC_UNICAST_MIC_LEN, AKES_MAC_BROADCAST_MIC_LEN)];

  shall_decrypt = akes_mac_get_sec_lvl() & (1 << 2);
  packetbuf_set_datalen(packetbuf_datalen() - akes_mac_mic_len());
  akes_mac_aead(key, shall_decrypt, generated_mic, 0);

  return memcmp(generated_mic,
      ((uint8_t *) packetbuf_dataptr()) + packetbuf_datalen(),
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
  struct akes_nbr_entry *entry;

  switch(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)) {
  case FRAME802154_CMDFRAME:
    cmd_broker_publish();
    break;
  case FRAME802154_DATAFRAME:
    entry = akes_nbr_get_sender_entry();
    if(!entry || !entry->permanent) {
      LOG_ERR("ignored incoming frame\n");
      return;
    }

    if((packetbuf_holds_broadcast() || AKES_MAC_UNSECURE_UNICASTS)
        && AKES_MAC_STRATEGY.verify(entry->permanent) != AKES_MAC_VERIFY_SUCCESS) {
      return;
    }

    if(akes_mac_received_duplicate(entry->permanent)) {
      LOG_ERR("received duplicate\n");
      return;
    }

    AKES_DELETE_STRATEGY.prolong_permanent(entry->permanent);

    NETSTACK_NETWORK.input();
    break;
  }
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  AKES_MAC_DECORATED_MAC.init();
  cmd_broker_init();
#if AKES_NBR_WITH_GROUP_KEYS
  csprng_rand(akes_mac_group_key, AES_128_KEY_LENGTH);
#endif /* AKES_NBR_WITH_GROUP_KEYS */
  AKES_MAC_STRATEGY.init();
  akes_init();
}
/*---------------------------------------------------------------------------*/
static int
length(void)
{
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
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
