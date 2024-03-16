/*
 * Copyright (c) 2016, Hasso-Plattner-Institut.
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
 *         Practical On-the-fly Rejection (POTR).
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/contikimac/contikimac-framer-potr.h"
#include "lib/aes-128.h"
#include "lib/assert.h"
#include "net/mac/anti-replay.h"
#include "net/mac/contikimac/contikimac-ccm-inputs.h"
#include "net/mac/contikimac/contikimac-framer-original.h"
#include "net/mac/contikimac/contikimac-nbr.h"
#include "net/mac/contikimac/contikimac-strategy.h"
#include "net/mac/contikimac/contikimac.h"
#include "net/mac/llsec802154.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "services/akes/akes-mac.h"
#include "services/akes/akes-nbr.h"
#include "services/akes/akes.h"
#include <string.h>

#ifdef CONTIKIMAC_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOS
#define MAX_CONSECUTIVE_INC_HELLOS \
  CONTIKIMAC_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOS
#else /* CONTIKIMAC_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOS */
#define MAX_CONSECUTIVE_INC_HELLOS (20)
#endif /* CONTIKIMAC_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOS */

#ifdef CONTIKIMAC_FRAMER_POTR_CONF_MAX_INC_HELLO_RATE
#define MAX_INC_HELLO_RATE CONTIKIMAC_FRAMER_POTR_CONF_MAX_INC_HELLO_RATE
#else /* CONTIKIMAC_FRAMER_POTR_CONF_MAX_INC_HELLO_RATE */
#define MAX_INC_HELLO_RATE (15) /* 1 HELLO per 15s */
#endif /* CONTIKIMAC_FRAMER_POTR_CONF_MAX_INC_HELLO_RATE */

#ifdef CONTIKIMAC_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOACKS
#define MAX_CONSECUTIVE_INC_HELLOACKS \
  CONTIKIMAC_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOACKS
#else /* CONTIKIMAC_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOACKS */
#define MAX_CONSECUTIVE_INC_HELLOACKS (20)
#endif /* CONTIKIMAC_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOACKS */

#ifdef CONTIKIMAC_FRAMER_POTR_CONF_MAX_INC_HELLOACK_RATE
#define MAX_INC_HELLOACK_RATE CONTIKIMAC_FRAMER_POTR_CONF_MAX_INC_HELLOACK_RATE
#else /* CONTIKIMAC_FRAMER_POTR_CONF_MAX_INC_HELLOACK_RATE */
#define MAX_INC_HELLOACK_RATE (15) /* 1 HELLOACK per 15s */
#endif /* CONTIKIMAC_FRAMER_POTR_CONF_MAX_INC_HELLOACK_RATE */

#define HELLO_LEN \
  MAX(1 /* Extended Frame Type and Subtype fields */ \
      + CONTIKIMAC_FRAMER_POTR_PAN_ID_LEN /* destination PAN ID */ \
      + LINKADDR_SIZE /* source address */ \
      + CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN \
      + 1 /* number of padding bytes */ \
      + AKES_HELLO_DATALEN \
      + (ANTI_REPLAY_WITH_SUPPRESSION ? 4 : 0) \
      + (CONTIKIMAC_WITH_SECURE_PHASE_LOCK \
         ? CONTIKIMAC_FRAMER_POTR_PHASE_LEN \
         : 0) \
      + CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN \
      + AKES_MAC_BROADCAST_MIC_LEN, CONTIKIMAC_MIN_FRAME_LENGTH)
#define HELLOACK_LEN \
  MAX(1 /* Extended Frame Type and Subtype fields */ \
      + CONTIKIMAC_FRAMER_POTR_PAN_ID_LEN /* destination PAN ID */ \
      + LINKADDR_SIZE /* destination address */ \
      + LINKADDR_SIZE /* source address */ \
      + (ANTI_REPLAY_WITH_SUPPRESSION \
         ? 4 \
         : CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN) \
      + (CONTIKIMAC_WITH_SECURE_PHASE_LOCK ? 1 : 0) /* strobe index */ \
      + 1 /* number of padding bytes */ \
      + AKES_HELLOACK_DATALEN \
      + (ANTI_REPLAY_WITH_SUPPRESSION ? 8 : 0) \
      + (CONTIKIMAC_WITH_SECURE_PHASE_LOCK \
         ? (CONTIKIMAC_Q_LEN + CONTIKIMAC_FRAMER_POTR_PHASE_LEN) \
         : 0) \
      + CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN, \
      CONTIKIMAC_MIN_FRAME_LENGTH)
#define ACK_LEN \
  MAX(1 /* Extended Frame Type and Subtype fields */ \
      + LINKADDR_SIZE /* source address */ \
      + CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN \
      + CONTIKIMAC_FRAMER_POTR_OTP_LEN \
      + (CONTIKIMAC_WITH_SECURE_PHASE_LOCK ? 1 : 0) /* strobe index */ \
      + 1 /* number of padding bytes */ \
      + AKES_ACK_DATALEN \
      + (ANTI_REPLAY_WITH_SUPPRESSION ? 4 : 0) \
      + (CONTIKIMAC_WITH_SECURE_PHASE_LOCK ? CONTIKIMAC_Q_LEN + 1 + 1 : 0) \
      , CONTIKIMAC_MIN_FRAME_LENGTH)
#define MIN_BYTES_FOR_FILTERING \
  MAX(0, 1 + LINKADDR_SIZE + CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN - 4)

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "POTR"
#define LOG_LEVEL LOG_LEVEL_MAC

#if CONTIKIMAC_FRAMER_POTR_ENABLED
leaky_bucket_t contikimac_framer_potr_hello_inc_bucket;
leaky_bucket_t contikimac_framer_potr_helloack_inc_bucket;

/*---------------------------------------------------------------------------*/
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
bool
potr_has_strobe_index(enum potr_frame_type type)
{
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_DATA:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_COMMAND:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK:
    return true;
  default:
    return false;
  }
}
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
/*---------------------------------------------------------------------------*/
static bool
has_destination_pan_id(enum potr_frame_type type)
{
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLO:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK:
    return true;
  default:
    return false;
  }
}
/*---------------------------------------------------------------------------*/
static bool
has_destination_address(enum potr_frame_type type)
{
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK:
    return true;
  default:
    return false;
  }
}
/*---------------------------------------------------------------------------*/
static bool
has_otp(enum potr_frame_type type)
{
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_DATA:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_COMMAND:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_DATA:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_COMMAND:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK:
    return true;
  default:
    return false;
  }
}
/*---------------------------------------------------------------------------*/
static bool
has_seqno(enum potr_frame_type type)
{
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_DATA:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_COMMAND:
    return true;
  default:
    return false;
  }
}
/*---------------------------------------------------------------------------*/
static uint_fast16_t
header_length_of(enum potr_frame_type type)
{
  return contikimac_framer_potr_get_strobe_index_offset(type)
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
         + potr_has_strobe_index(type)
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
         + has_seqno(type)
         + 1 /* number of padding bytes */;
}
/*---------------------------------------------------------------------------*/
uint_fast16_t
contikimac_framer_potr_get_strobe_index_offset(enum potr_frame_type type)
{
  return 1 /* Extended Frame Type and Subtype field */
         + (has_destination_pan_id(type)
            ? CONTIKIMAC_FRAMER_POTR_PAN_ID_LEN
            : 0)
         + (has_destination_address(type) ? LINKADDR_SIZE : 0)
         + LINKADDR_SIZE /* source address */
         + ((ANTI_REPLAY_WITH_SUPPRESSION
             && (type == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK))
            ? 4
            : CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN)
         + (has_otp(type) ? CONTIKIMAC_FRAMER_POTR_OTP_LEN : 0);
}
/*---------------------------------------------------------------------------*/
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
int
contikimac_framer_potr_update_contents(void)
{
  if(contikimac_state.strobe.is_broadcast
     && contikimac_state.strobe.strobes
     && !contikimac_state.strobe.is_hello) {
    return 1;
  }

  if(!contikimac_state.strobe.is_broadcast) {
    /* set strobe index */
    contikimac_state.strobe
        .unsecured_frame[contikimac_state.strobe.strobe_index_offset] =
        contikimac_state.strobe.strobes;
    contikimac_state.strobe.nonce[12] = contikimac_state.strobe.strobes;
    if(NETSTACK_RADIO.async_reprepare(
           contikimac_state.strobe.strobe_index_offset,
           &contikimac_state.strobe.strobes,
           1)) {
      return 0;
    }
  }
  if(contikimac_state.strobe.phase_offset) {
    /* set phase */
    rtimer_clock_t phase = contikimac_get_phase();
    memcpy(contikimac_state.strobe.unsecured_frame
           + contikimac_state.strobe.phase_offset,
           &phase,
           CONTIKIMAC_FRAMER_POTR_PHASE_LEN);
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    wake_up_counter_write(
        contikimac_state.strobe.unsecured_frame
        + contikimac_state.strobe.phase_offset
        + CONTIKIMAC_FRAMER_POTR_PHASE_LEN,
        contikimac_get_wake_up_counter(contikimac_get_sfd_timestamp()));
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
    if(NETSTACK_RADIO.async_reprepare(
           contikimac_state.strobe.phase_offset,
           contikimac_state.strobe.unsecured_frame
           + contikimac_state.strobe.phase_offset,
           CONTIKIMAC_FRAMER_POTR_PHASE_LEN
           + CONTIKIMAC_FRAMER_POTR_ILOS_WAKE_UP_COUNTER_LEN)) {
      return 0;
    }
  }

  uint8_t secured_frame[RADIO_MAX_PAYLOAD];
  memcpy(secured_frame,
         contikimac_state.strobe.unsecured_frame,
         contikimac_state.strobe.totlen);
  if(!CCM_STAR.get_lock()) {
    memset(secured_frame + contikimac_state.strobe.a_len,
           0,
           contikimac_state.strobe.m_len + contikimac_state.strobe.mic_len);
  } else {
    uint8_t *m = contikimac_state.strobe.shall_encrypt
                 ? (secured_frame + contikimac_state.strobe.a_len)
                 : NULL;
    if(!CCM_STAR.set_key(contikimac_state.strobe.key)
       || !CCM_STAR.aead(contikimac_state.strobe.nonce,
                         m,
                         contikimac_state.strobe.m_len,
                         secured_frame,
                         contikimac_state.strobe.a_len,
                         secured_frame + contikimac_state.strobe.totlen,
                         contikimac_state.strobe.mic_len,
                         true)) {
      CCM_STAR.release_lock();
      LOG_ERR("CCM* failed\n");
      return 0;
    }
    CCM_STAR.release_lock();
  }
  if(NETSTACK_RADIO.async_reprepare(
         contikimac_state.strobe.a_len,
         secured_frame + contikimac_state.strobe.a_len,
         contikimac_state.strobe.m_len + contikimac_state.strobe.mic_len)) {
    return 0;
  }
  return 1;
}
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
/*---------------------------------------------------------------------------*/
static int
length(void)
{
  return header_length_of(
      packetbuf_holds_broadcast()
      ? CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_DATA
      : CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_DATA);
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  /* determine frame type */
  enum potr_frame_type type;
  switch(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE)) {
  case FRAME802154_DATAFRAME:
    type = packetbuf_holds_broadcast()
           ? CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_DATA
           : CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_DATA;
    break;
  case FRAME802154_CMDFRAME:
    switch(packetbuf_get_dispatch_byte()) {
    case AKES_HELLO_IDENTIFIER:
      type = CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLO;
      break;
    case AKES_HELLOACK_IDENTIFIER:
    case AKES_HELLOACK_P_IDENTIFIER:
      type = CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK;
      break;
    case AKES_ACK_IDENTIFIER:
      type = CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK;
      break;
    default:
      type = packetbuf_holds_broadcast()
             ? CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_COMMAND
             : CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_COMMAND;
      break;
    }
    break;
  default:
    LOG_ERR("unknown frame type\n");
    return FRAMER_FAILED;
  }

  /* allocate as much space as we need */
  uint_fast16_t basic_len = header_length_of(type);
  uint_fast16_t totlen = basic_len
                         + packetbuf_datalen()
                         + AKES_MAC_STRATEGY.get_overhead();
  uint_fast8_t padding_bytes = totlen < CONTIKIMAC_MIN_FRAME_LENGTH
                               ? CONTIKIMAC_MIN_FRAME_LENGTH - totlen
                               : 0;
  if(!packetbuf_hdralloc(basic_len + padding_bytes)) {
    LOG_ERR("packetbuf_hdralloc failed\n");
    return FRAMER_FAILED;
  }

  /* Frame Type */
  uint8_t *p = packetbuf_hdrptr();
  p[0] = type;
  p += 1;

  /* destination PAN ID */
  if(has_destination_pan_id(type)) {
    p[0] = (IEEE802154_PANID & 0xFF);
    p[1] = (IEEE802154_PANID >> 8) & 0xFF;
    p += CONTIKIMAC_FRAMER_POTR_PAN_ID_LEN;
  }

  /* destination address */
  if(has_destination_address(type)) {
    linkaddr_write(p, packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
    p += LINKADDR_SIZE;
  }

  /* source address */
  linkaddr_write(p, &linkaddr_node_addr);
  p += LINKADDR_SIZE;

#if !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  /* Frame Counter */
#if ANTI_REPLAY_WITH_SUPPRESSION
  if(type == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK) {
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
    anti_replay_write_counter(p);
    p += 4;
#if ANTI_REPLAY_WITH_SUPPRESSION
  } else {
    p[0] = anti_replay_get_counter_lsbs();
    p += 1;
  }
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
#endif /* !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

  /* OTP */
  if(has_otp(type)) {
    uint8_t nonce[CCM_STAR_NONCE_LENGTH];

    contikimac_ccm_inputs_generate_otp_nonce(nonce, 1);
    if(!CCM_STAR.get_lock()) {
      LOG_ERR("CCM* was locked\n");
      return FRAMER_FAILED;
    }

    uint8_t totlen;
    if(type == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK) {
      totlen = ACK_LEN;
      akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
      if(!entry
         || !entry->tentative
         || !CCM_STAR.set_key(entry->tentative->tentative_pairwise_key)) {
        LOG_ERR("receiver is not tentative or CCM*.set_key failed\n");
        goto error;
      }
    } else {
      totlen = packetbuf_totlen() + AKES_MAC_STRATEGY.get_overhead();
      if(!CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED || packetbuf_holds_broadcast()) {
        if(!CCM_STAR.set_key(akes_mac_group_key)) {
          LOG_ERR("CCM*.set_key failed\n");
          goto error;
        }
      } else {
        akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
        if(!entry
           || !entry->permanent
           || !CCM_STAR.set_key(entry->permanent->group_key)) {
          LOG_ERR("receiver is not permanent or CCM*.set_key failed\n");
          goto error;
        }
      }
    }
    if(!CCM_STAR.aead(nonce,
                      NULL, 0,
                      &totlen, 1,
                      p, CONTIKIMAC_FRAMER_POTR_OTP_LEN,
                      true)) {
      LOG_ERR("CCM*.aead failed\n");
      goto error;
    }
    CCM_STAR.release_lock();
#ifdef CONTIKIMAC_FRAMER_POTR_CONF_CORRUPT_OTP
    if(type != CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK) {
      memset(p, 0, CONTIKIMAC_FRAMER_POTR_OTP_LEN);
    }
#endif /* CONTIKIMAC_FRAMER_POTR_CONF_CORRUPT_OTP */
    p += CONTIKIMAC_FRAMER_POTR_OTP_LEN;
  }

#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  if(potr_has_strobe_index(type)) {
    p[0] = 0;
    p += 1;
  }
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */

  if(has_seqno(type)) {
    p[0] = (uint8_t)packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
    p += 1;
  }

  /* padding bytes */
  p[0] = padding_bytes;
  memset(p + 1, 0, padding_bytes);

  return basic_len + padding_bytes;
error:
  CCM_STAR.release_lock();
  return FRAMER_FAILED;
}
/*---------------------------------------------------------------------------*/
static int
filter(void)
{
  uint8_t *p = packetbuf_hdrptr();
  uint8_t totlen = packetbuf_totlen();

  if(radio_read_payload_to_packetbuf(1)) {
    LOG_ERR("failed to read frame type\n");
    return FRAMER_FAILED;
  }
  enum potr_frame_type type = p[0];

  /* Frame Length */
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLO:
    if(totlen != HELLO_LEN) {
      LOG_ERR("HELLO has invalid length\n");
      return FRAMER_FAILED;
    }
    if(leaky_bucket_is_full(&contikimac_framer_potr_hello_inc_bucket)) {
      LOG_WARN("HELLO bucket is full\n");
      return FRAMER_FAILED;
    }
    break;
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK:
    if(totlen != HELLOACK_LEN) {
      LOG_ERR("HELLOACK has invalid length\n");
      return FRAMER_FAILED;
    }
    if(leaky_bucket_is_full(&contikimac_framer_potr_helloack_inc_bucket)) {
      LOG_WARN("HELLOACK bucket is full\n");
      return FRAMER_FAILED;
    }
    break;
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK:
    if(totlen != ACK_LEN) {
      LOG_ERR("ACK has invalid length\n");
      return FRAMER_FAILED;
    }
    break;
  default:
    if(totlen < CONTIKIMAC_MIN_FRAME_LENGTH) {
      LOG_ERR("invalid length\n");
      return FRAMER_FAILED;
    }
    break;
  }

  /* Frame Type */
  bool is_broadcast;
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_DATA:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_COMMAND:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK:
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_node_addr);
    is_broadcast = false;
    break;
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_DATA:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_COMMAND:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLO:
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
    is_broadcast = true;
    break;
  default:
    LOG_ERR("unknown frame type %02x\n", type);
    return FRAMER_FAILED;
  }
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_BROADCAST_DATA:
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_DATA:
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
    break;
  default:
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CMDFRAME);
    break;
  }
  p += 1;

  /* destination PAN ID */
  if(has_destination_pan_id(type)) {
    if(radio_read_payload_to_packetbuf(CONTIKIMAC_FRAMER_POTR_PAN_ID_LEN)) {
      LOG_ERR("failed to read destination PAN ID\n");
      return FRAMER_FAILED;
    }
    uint_fast16_t dst_pid = (p[0]) | (p[1] << 8);
    if((dst_pid != IEEE802154_PANID)
       && (dst_pid != FRAME802154_BROADCASTPANDID)) {
      LOG_INFO("for another PAN %" PRIxFAST16 "\n", dst_pid);
      return FRAMER_FAILED;
    }
    p += CONTIKIMAC_FRAMER_POTR_PAN_ID_LEN;
  }

  /* destination address */
  linkaddr_t addr;
  if(has_destination_address(type)) {
    if(radio_read_payload_to_packetbuf(LINKADDR_SIZE)) {
      LOG_ERR("failed to read destination address\n");
      return FRAMER_FAILED;
    }
    linkaddr_read(&addr, p);
    if(!linkaddr_cmp(&addr, &linkaddr_node_addr)) {
      LOG_ERR("not for us\n");
      return FRAMER_FAILED;
    }
    p += LINKADDR_SIZE;
  }

  /* source address */
  if(radio_read_payload_to_packetbuf(LINKADDR_SIZE)) {
    LOG_ERR("failed to read source address\n");
    return FRAMER_FAILED;
  }
  linkaddr_read(&addr, p);
  if(linkaddr_cmp(&addr, &linkaddr_node_addr)) {
    LOG_ERR("frame from ourselves\n");
    return FRAMER_FAILED;
  }
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  if(is_broadcast
     && ((contikimac_get_wake_up_counter(contikimac_get_last_wake_up_time())
              .u32
          == (contikimac_strategy_wake_up_counter_at_last_authentic_broadcast
                  .u32
              + 1))
         && linkaddr_cmp(
             &addr,
             &contikimac_strategy_sender_of_last_authentic_broadcast))) {
    LOG_WARN("just accepted a broadcast frame from this sender already\n");
    return FRAMER_FAILED;
  }
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &addr);
  akes_nbr_entry_t *entry = akes_nbr_get_sender_entry();
  akes_nbr_t *nbr = NULL;
  switch(type) {
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLO:
    if(!akes_is_acceptable_hello()) {
      LOG_ERR("unacceptable HELLO\n");
      return FRAMER_FAILED;
    }
    leaky_bucket_pour(&contikimac_framer_potr_hello_inc_bucket);
    if(entry) {
      nbr = entry->permanent;
    }
    break;
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK:
    if(!akes_is_acceptable_helloack()) {
      LOG_ERR("unacceptable HELLOACK\n");
      return FRAMER_FAILED;
    }
    leaky_bucket_pour(&contikimac_framer_potr_helloack_inc_bucket);
    break;
  case CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK:
    if(!akes_is_acceptable_ack(entry)) {
      LOG_ERR("unacceptable ACK\n");
      return FRAMER_FAILED;
    }
    nbr = entry->tentative;
    break;
  default:
    if(!entry || !entry->permanent) {
      LOG_ERR("sender is not permanent\n");
      return FRAMER_FAILED;
    }
    nbr = entry->permanent;
    break;
  }
  p += LINKADDR_SIZE;

#if !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  /* Frame Counter */
#if ANTI_REPLAY_WITH_SUPPRESSION
  if(type == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK) {
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
    if(radio_read_payload_to_packetbuf(4)) {
      LOG_ERR("failed to read frame counter\n");
      return FRAMER_FAILED;
    }
    anti_replay_parse_counter(p);
    p += 4;
#if ANTI_REPLAY_WITH_SUPPRESSION
  } else {
    if(radio_read_payload_to_packetbuf(
           CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN)) {
      LOG_ERR("failed to read LSBs of frame counter\n");
      return FRAMER_FAILED;
    }
    if(nbr) {
      anti_replay_restore_counter(&nbr->anti_replay_info, p[0]);
    }
    p += CONTIKIMAC_FRAMER_POTR_FRAME_COUNTER_LEN;
  }
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
#endif /* !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

  /* OTP */
  if(has_otp(type)) {
    uint8_t nonce[CCM_STAR_NONCE_LENGTH];
    contikimac_framer_potr_otp_t otp;

    contikimac_ccm_inputs_generate_otp_nonce(nonce, 0);
    if(!CCM_STAR.set_key(
           type == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACK
           ? nbr->tentative_pairwise_key
           : (!CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED || is_broadcast
              ? nbr->group_key
              : akes_mac_group_key))
       || !CCM_STAR.aead(nonce,
                         NULL, 0,
                         &totlen, 1,
                         otp.u8, CONTIKIMAC_FRAMER_POTR_OTP_LEN,
                         false)) {
      LOG_ERR("CCM* failed\n");
      return FRAMER_FAILED;
    }
    if(radio_read_payload_to_packetbuf(CONTIKIMAC_FRAMER_POTR_OTP_LEN)) {
      LOG_ERR("failed to read OTP\n");
      return FRAMER_FAILED;
    }

    if(memcmp(otp.u8, p, CONTIKIMAC_FRAMER_POTR_OTP_LEN)) {
      LOG_ERR("invalid OTP\n");
      return FRAMER_FAILED;
    }
#if !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    if(anti_replay_was_replayed(&nbr->anti_replay_info)) {
      LOG_ERR("replayed OTP\n");
      return FRAMER_FAILED;
    }
#endif /* !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
  }

  /* prepare acknowledgment frame */
  if(!packetbuf_holds_broadcast()) {
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
    /* read strobe index */
    if(radio_read_payload_to_packetbuf(1)) {
      LOG_ERR("failed to read strobe index\n");
      return FRAMER_FAILED;
    }
    /* create header */
    contikimac_state.duty_cycle.acknowledgment[0] =
        CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACKNOWLEDGMENT;
    contikimac_state.duty_cycle.acknowledgment_len =
        CONTIKIMAC_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN;
    if(type == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_HELLOACK) {
      assert(contikimac_state.duty_cycle.acknowledgment_len
             == CONTIKIMAC_HELLOACK_ACKNOWLEDGMENT_LEN);
    } else {
      contikimac_state.duty_cycle
          .acknowledgment[contikimac_state.duty_cycle.acknowledgment_len++] =
          contikimac_get_last_delta();
#if ANTI_REPLAY_WITH_SUPPRESSION
      if(type == CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_UNICAST_COMMAND) {
        anti_replay_write_my_broadcast_counter(
            contikimac_state.duty_cycle.acknowledgment
            + contikimac_state.duty_cycle.acknowledgment_len);
        contikimac_state.duty_cycle.acknowledgment_len += 4;
        assert(contikimac_state.duty_cycle.acknowledgment_len
               == (CONTIKIMAC_UPDATE_ACKNOWLEDGMENT_LEN
                   - AKES_MAC_UNICAST_MIC_LEN));
      } else
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
        assert(contikimac_state.duty_cycle.acknowledgment_len
               == (CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN
                   - AKES_MAC_UNICAST_MIC_LEN));

      uint8_t nonce[CCM_STAR_NONCE_LENGTH];
      contikimac_ccm_inputs_generate_nonce(nonce, false);
      contikimac_ccm_inputs_to_acknowledgment_nonce(nonce);
      if(!CCM_STAR.aead(nonce,
                        NULL,
                        0,
                        contikimac_state.duty_cycle.acknowledgment,
                        contikimac_state.duty_cycle.acknowledgment_len,
                        contikimac_state.duty_cycle.acknowledgment
                        + contikimac_state.duty_cycle.acknowledgment_len,
                        AKES_MAC_UNICAST_MIC_LEN,
                        true)) {
        LOG_ERR("CCM*.aead failed\n");
        return FRAMER_FAILED;
      }
      contikimac_state.duty_cycle.acknowledgment_len +=
          AKES_MAC_UNICAST_MIC_LEN;
    }
#else /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
    contikimac_state.duty_cycle.acknowledgment_len =
        CONTIKIMAC_DEFAULT_ACKNOWLEDGMENT_LEN;
    contikimac_state.duty_cycle.acknowledgment[0] =
        CONTIKIMAC_FRAMER_POTR_FRAME_TYPE_ACKNOWLEDGMENT;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
  uint8_t *hdrptr = packetbuf_hdrptr();
  enum potr_frame_type type = hdrptr[0];
  uint_fast16_t basic_len = header_length_of(type);

  if(has_seqno(type)) {
    packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, hdrptr[basic_len - 2]);
  }

  uint_fast8_t padding_bytes = hdrptr[basic_len - 1];
  if(!packetbuf_hdrreduce(basic_len + padding_bytes)) {
    LOG_ERR("packetbuf_hdrreduce failed\n");
    return FRAMER_FAILED;
  }

  return basic_len + padding_bytes;
}
/*---------------------------------------------------------------------------*/
const struct framer contikimac_framer_potr = {
  length,
  create,
  parse,
};
/*---------------------------------------------------------------------------*/
static uint8_t
get_min_bytes_for_filtering(void)
{
  return MIN_BYTES_FOR_FILTERING;
}
/*---------------------------------------------------------------------------*/
static int
prepare_acknowledgment_parsing(void)
{
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  if(contikimac_state.strobe.is_helloack) {
    return 1;
  }
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry) {
    LOG_ERR("entry is NULL\n");
    return 0;
  }
  if(contikimac_state.strobe.is_ack) {
    if(!entry->tentative) {
      LOG_ERR("entry->tentative is NULL\n");
      return 0;
    }
    akes_nbr_copy_key(contikimac_state.strobe.acknowledgment_key,
                      entry->tentative->tentative_pairwise_key);
  } else {
    if(!entry->permanent) {
      LOG_ERR("entry->permanent is NULL\n");
      return 0;
    }
    akes_nbr_copy_key(contikimac_state.strobe.acknowledgment_key,
                      CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
                      ? entry->permanent->group_key
                      : akes_mac_group_key);
  }
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
parse_acknowledgment(void)
{
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  if(contikimac_state.strobe.is_helloack) {
    return 1;
  }

  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  memcpy(nonce, contikimac_state.strobe.nonce, CCM_STAR_NONCE_LENGTH);
  if(!CCM_STAR.get_lock()) {
    LOG_WARN("CCM* was locked\n");
    return 0;
  }
  contikimac_ccm_inputs_to_acknowledgment_nonce(nonce);
  uint8_t expected_mic[AKES_MAC_UNICAST_MIC_LEN];
  if(!CCM_STAR.set_key(contikimac_state.strobe.acknowledgment_key)
     || !CCM_STAR.aead(nonce,
                       NULL,
                       0,
                       contikimac_state.strobe.acknowledgment,
                       contikimac_state.strobe.acknowledgment_len
                       - AKES_MAC_UNICAST_MIC_LEN,
                       expected_mic,
                       AKES_MAC_UNICAST_MIC_LEN,
                       false)) {
    CCM_STAR.release_lock();
    LOG_ERR("CCM* failed\n");
    return 0;
  }
  CCM_STAR.release_lock();
  if(memcmp(expected_mic,
            contikimac_state.strobe.acknowledgment
            + contikimac_state.strobe.acknowledgment_len
            - AKES_MAC_UNICAST_MIC_LEN,
            AKES_MAC_UNICAST_MIC_LEN)) {
    LOG_ERR("inauthentic acknowledgment frame\n");
    return 0;
  }
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
  return 1;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
#if !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  if(contikimac_state.strobe.result != MAC_TX_OK) {
    return;
  }
  if(contikimac_state.strobe.is_helloack) {
    return;
  }
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_WARN("receiver is no longer permanent\n");
    return;
  }

#if ANTI_REPLAY_WITH_SUPPRESSION
  if(akes_mac_is_update()) {
    entry->permanent->anti_replay_info.last_broadcast_counter =
        anti_replay_read_counter(contikimac_state.strobe.acknowledgment + 2);
  }
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */

  AKES_DELETE_STRATEGY.prolong_permanent_neighbor(entry->permanent);
#endif /* !CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  leaky_bucket_init(&contikimac_framer_potr_hello_inc_bucket,
                    MAX_CONSECUTIVE_INC_HELLOS,
                    MAX_INC_HELLO_RATE);
  leaky_bucket_init(&contikimac_framer_potr_helloack_inc_bucket,
                    MAX_CONSECUTIVE_INC_HELLOACKS,
                    MAX_INC_HELLOACK_RATE);
}
/*---------------------------------------------------------------------------*/
const struct contikimac_framer contikimac_framer_potr_contikimac_framer = {
  get_min_bytes_for_filtering,
  filter,
  prepare_acknowledgment_parsing,
  parse_acknowledgment,
  on_unicast_transmitted,
  init,
};
/*---------------------------------------------------------------------------*/
#endif /* CONTIKIMAC_FRAMER_POTR_ENABLED */
