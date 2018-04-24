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
 * \addtogroup csl
 * @{
 * \file
 *         A CSL-enabled version of practical on-the-fly rejection (POTR)
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/csl/csl-framer-potr.h"
#include "net/mac/csl/csl-framer.h"
#include "net/packetbuf.h"
#include "services/akes/akes-mac.h"
#include "services/akes/akes.h"
#include "services/akes/akes-nbr.h"
#include "net/mac/csl/csl-ccm-inputs.h"

#ifdef CSL_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOS
#define MAX_CONSECUTIVE_INC_HELLOS CSL_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOS
#else /* CSL_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOS */
#define MAX_CONSECUTIVE_INC_HELLOS (20)
#endif /* CSL_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOS */

#ifdef CSL_FRAMER_POTR_CONF_MAX_INC_HELLO_RATE
#define MAX_INC_HELLO_RATE CSL_FRAMER_POTR_CONF_MAX_INC_HELLO_RATE
#else /* CSL_FRAMER_POTR_CONF_MAX_INC_HELLO_RATE */
#define MAX_INC_HELLO_RATE (15) /* 1 HELLO per 15s */
#endif /* CSL_FRAMER_POTR_CONF_MAX_INC_HELLO_RATE */

#ifdef CSL_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOACKS
#define MAX_CONSECUTIVE_INC_HELLOACKS CSL_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOACKS
#else /* CSL_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOACKS */
#define MAX_CONSECUTIVE_INC_HELLOACKS (20)
#endif /* CSL_FRAMER_POTR_CONF_MAX_CONSECUTIVE_INC_HELLOACKS */

#ifdef CSL_FRAMER_POTR_CONF_MAX_INC_HELLOACK_RATE
#define MAX_INC_HELLOACK_RATE CSL_FRAMER_POTR_CONF_MAX_INC_HELLOACK_RATE
#else /* CSL_FRAMER_POTR_CONF_MAX_INC_HELLOACK_RATE */
#define MAX_INC_HELLOACK_RATE (8) /* 1 HELLOACK per 8s */
#endif /* CSL_FRAMER_POTR_CONF_MAX_INC_HELLOACK_RATE */

#define MIN_NORMAL_PAYLOAD_FRAME_LEN (CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN \
    + CSL_FRAMER_POTR_SEQUENCE_NUMBER_LEN \
    + AKES_MAC_UNICAST_MIC_LEN)
#define CSL_FRAMER_POTR_ACK_PAYLOAD_FRAME_LEN (CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN \
    + AKES_ACK_DATALEN \
    + CSL_FRAMER_POTR_ACK_PIGGYBACK_LEN)
#define EXTENDED_FRAME_TYPE (0x7 /* prefix */ \
    | (0x6 << 3) /* unused extended frame type 110 */)

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL-framer-potr"
#define LOG_LEVEL LOG_LEVEL_FRAMER

#if !CSL_COMPLIANT
struct leaky_bucket csl_framer_potr_hello_inc_bucket;
struct leaky_bucket csl_framer_potr_helloack_inc_bucket;

/*---------------------------------------------------------------------------*/
void
csl_framer_potr_write_phase(uint8_t *dst, rtimer_clock_t phase)
{
  dst[0] = (phase >> 8) & 0xFF;
  dst[1] = phase & 0xFF;
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
csl_framer_potr_parse_phase(uint8_t *src)
{
  return (rtimer_clock_t) (src[0] << 8) | src[1];
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_rendezvous_time_len(enum csl_subtype subtype)
{
  switch(subtype) {
  case CSL_SUBTYPE_HELLO:
    return CSL_FRAMER_POTR_LONG_RENDEZVOUS_TIME_LEN;
  default:
    return CSL_FRAMER_POTR_SHORT_RENDEZVOUS_TIME_LEN;
  }
}
/*---------------------------------------------------------------------------*/
static int
has_destination_pan_id(enum csl_subtype subtype)
{
  switch(subtype) {
  case CSL_SUBTYPE_HELLO:
  case CSL_SUBTYPE_HELLOACK:
    return 1;
  default:
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int
has_otp_etc(enum csl_subtype subtype)
{
  switch(subtype) {
  case CSL_SUBTYPE_ACK:
  case CSL_SUBTYPE_NORMAL:
    return 1;
  default:
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int
has_source_address(enum csl_subtype subtype)
{
  switch(subtype) {
  case CSL_SUBTYPE_HELLO:
  case CSL_SUBTYPE_HELLOACK:
    return 1;
  default:
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int
has_seqno(enum csl_subtype subtype)
{
  switch(subtype) {
  case CSL_SUBTYPE_NORMAL:
    return 1;
  default:
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int
get_payload_frame_header_len(enum csl_subtype subtype, int frame_pending)
{
  return 1 /* frame type and subtype */
    + (has_source_address(subtype) ? LINKADDR_SIZE : 0)
    + (has_seqno(subtype) ? 1 : 0)
    + (frame_pending ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
static int
length(void)
{
  return get_payload_frame_header_len(CSL_SUBTYPE_NORMAL, CSL_MAX_BURST_INDEX);
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  int is_command;
  uint8_t pending_frames_len;
  int len;
  uint8_t *p;

  /* prepend header to the packetbuf */
  pending_frames_len = packetbuf_attr(PACKETBUF_ATTR_PENDING);
  len = get_payload_frame_header_len(csl_state.transmit.subtype, pending_frames_len);
  if(!packetbuf_hdralloc(len)) {
    LOG_ERR("packetbuf_hdralloc failed\n");
    return FRAMER_FAILED;
  }
  p = packetbuf_hdrptr();

  /* frame type and flags */
  is_command = (csl_state.transmit.subtype == CSL_SUBTYPE_NORMAL)
      && (packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_CMDFRAME);
  p[0] = EXTENDED_FRAME_TYPE
      | (is_command ? (1 << 6) : 0)
      | (pending_frames_len ? (1 << 7) : 0);
  p += CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN;

  /* source address */
  if(has_source_address(csl_state.transmit.subtype)) {
    memcpy(p, linkaddr_node_addr.u8, LINKADDR_SIZE);
    p += LINKADDR_SIZE;
  }

  /* sequence number */
  if(has_seqno(csl_state.transmit.subtype)) {
    p[0] = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
    p += CSL_FRAMER_POTR_SEQUENCE_NUMBER_LEN;
  }

  /* pending frame's length */
  if(pending_frames_len) {
    p[0] = pending_frames_len;
    p += CSL_FRAMER_POTR_PAYLOAD_FRAMES_LEN_LEN;
  }

  return len;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_min_bytes_for_filtering(void)
{
  return get_payload_frame_header_len(csl_state.duty_cycle.subtype, 0);
}
/*---------------------------------------------------------------------------*/
static int
filter(uint8_t *dst)
{
  int len;
  int frame_pending;
  int is_command;
  uint8_t *p;
  linkaddr_t addr;
  rtimer_clock_t acknowledgement_sfd_timestamp;
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  struct akes_nbr_entry *entry;
  uint8_t phase_len;

  /* parse and validate frame length, frame type, as well as flags */
  if((has_otp_etc(csl_state.duty_cycle.subtype)
      && (csl_state.duty_cycle.next_frames_len != packetbuf_datalen()))) {
    LOG_ERR("unexpected frame length\n");
    return FRAMER_FAILED;
  }
  p = packetbuf_hdrptr();
  if((p[0] & 0x3F) != EXTENDED_FRAME_TYPE) {
    LOG_ERR("unwanted frame type\n");
    return FRAMER_FAILED;
  }
  if(csl_state.duty_cycle.subtype == CSL_SUBTYPE_NORMAL) {
    frame_pending = (1 << 7) & p[0];
    is_command = (1 << 6) & p[0];
  } else {
    frame_pending = 0;
    is_command = 1;
  }
  if(is_command) {
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_CMDFRAME);
  } else {
    packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, FRAME802154_DATAFRAME);
  }
  len = get_payload_frame_header_len(csl_state.duty_cycle.subtype, frame_pending);
  switch(csl_state.duty_cycle.subtype) {
  case CSL_SUBTYPE_HELLO:
    if(packetbuf_totlen() < (len + AKES_HELLO_DATALEN + CSL_FRAMER_POTR_HELLO_PIGGYBACK_LEN)) {
      LOG_ERR("HELLO has invalid length\n");
      return FRAMER_FAILED;
    }
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
    break;
  case CSL_SUBTYPE_HELLOACK:
    if(packetbuf_totlen() != (len + AKES_HELLOACK_DATALEN + CSL_FRAMER_POTR_HELLOACK_PIGGYBACK_LEN)) {
      LOG_ERR("HELLOACK has invalid length\n");
      return FRAMER_FAILED;
    }
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_node_addr);
    break;
  default:
    if(packetbuf_totlen() <= (len + AKES_MAC_UNICAST_MIC_LEN)) {
      LOG_ERR("frame has invalid length\n");
      return FRAMER_FAILED;
    }
    packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_node_addr);
    break;
  }
  p++;

  /* parse and validate source address */
  if(has_source_address(csl_state.duty_cycle.subtype)) {
    memcpy(addr.u8, p, LINKADDR_SIZE);
    if(linkaddr_cmp(&addr, &linkaddr_node_addr)) {
      LOG_ERR("frame from ourselves\n");
      return FRAMER_FAILED;
    }
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &addr);
    p += LINKADDR_SIZE;
  }
  if(csl_state.duty_cycle.subtype == CSL_SUBTYPE_HELLO) {
    if(!akes_is_acceptable_hello()) {
      LOG_ERR("unacceptable HELLO\n");
      return FRAMER_FAILED;
    }
  }

  /* parse sequence number */
  if(has_seqno(csl_state.duty_cycle.subtype)) {
    packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, p[0]);
    p += CSL_FRAMER_POTR_SEQUENCE_NUMBER_LEN;
  }

  /* parse and validate pending frame's length */
  if(frame_pending) {
    if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(CSL_FRAMER_POTR_PAYLOAD_FRAMES_LEN_LEN)) {
      LOG_ERR("could not read at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    if(!p[0]) {
      LOG_ERR("pending frame has no length\n");
      return FRAMER_FAILED;
    }
    packetbuf_set_attr(PACKETBUF_ATTR_PENDING, p[0]);
    csl_state.duty_cycle.next_frames_len = p[0];
    p += CSL_FRAMER_POTR_PAYLOAD_FRAMES_LEN_LEN;
  }

  if(!packetbuf_holds_broadcast()) {
    dst[1] = EXTENDED_FRAME_TYPE;
    if(csl_state.duty_cycle.subtype == CSL_SUBTYPE_HELLOACK) {
      dst[0] = CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN;
    } else {
      phase_len = csl_state.duty_cycle.last_burst_index
          ? 0
          : CSL_FRAMER_POTR_PHASE_LEN;
      dst[0] = CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN
          + phase_len
          + AKES_MAC_UNICAST_MIC_LEN;
      if(phase_len) {
        acknowledgement_sfd_timestamp = csl_get_sfd_timestamp_of_last_payload_frame()
          + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE
              * (1 /* Frame Length */ + packetbuf_totlen() + RADIO_SHR_LEN))
          + RADIO_TRANSMIT_CALIBRATION_TIME;
        csl_framer_potr_write_phase(dst + 2,
            csl_get_phase(acknowledgement_sfd_timestamp));
      }
      csl_ccm_inputs_generate_acknowledgement_nonce(nonce, 1);
      entry = akes_nbr_get_sender_entry();
      AES_128_GET_LOCK();
      CCM_STAR.set_key((csl_state.duty_cycle.subtype == CSL_SUBTYPE_ACK)
          ? entry->tentative->pairwise_key
          : entry->permanent->pairwise_key);
      CCM_STAR.aead(nonce,
          NULL, 0,
          dst + 1, 1 + phase_len,
          dst + 1 + 1 + phase_len,
          AKES_MAC_UNICAST_MIC_LEN, 1);
      AES_128_RELEASE_LOCK();
    }
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
  int len;

  /* remove header from the packetbuf */
  len = get_payload_frame_header_len(csl_state.duty_cycle.subtype,
      packetbuf_attr(PACKETBUF_ATTR_PENDING));
  if(!packetbuf_hdrreduce(len)) {
    LOG_ERR("packetbuf_hdrreduce failed\n");
    return FRAMER_FAILED;
  }

  /* validate command frame identifier */
  switch(csl_state.duty_cycle.subtype) {
  case CSL_SUBTYPE_HELLO:
    if(!akes_mac_is_hello()) {
      LOG_ERR("mismatching subtype and command ID\n");
      return FRAMER_FAILED;
    }
    break;
  case CSL_SUBTYPE_HELLOACK:
    if(!akes_mac_is_helloack()) {
      LOG_ERR("mismatching subtype and command ID\n");
      return FRAMER_FAILED;
    }
    break;
  case CSL_SUBTYPE_ACK:
    if(!akes_mac_is_ack()) {
      LOG_ERR("mismatching subtype and command ID\n");
      return FRAMER_FAILED;
    }
    break;
  default:
    break;
  }

  return len;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_length_of_wake_up_frame(void)
{
  switch(csl_state.transmit.subtype) {
  case CSL_SUBTYPE_HELLO:
    return CSL_FRAMER_POTR_HELLO_WAKE_UP_FRAME_LEN
        - RADIO_PHY_HEADER_LEN;
  case CSL_SUBTYPE_HELLOACK:
    return CSL_FRAMER_POTR_HELLOACK_WAKE_UP_FRAME_LEN
        - RADIO_PHY_HEADER_LEN;
  case CSL_SUBTYPE_ACK:
    return CSL_FRAMER_POTR_ACK_WAKE_UP_FRAME_LEN
        - RADIO_PHY_HEADER_LEN;
  default:
    return CSL_FRAMER_POTR_NORMAL_WAKE_UP_FRAME_LEN
        - RADIO_PHY_HEADER_LEN;
  }
}
/*---------------------------------------------------------------------------*/
static int
create_wake_up_frame(uint8_t *dst)
{
  struct akes_nbr_entry *entry;
  uint8_t payload_frames_length;
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];

  /* frame length */
  dst[0] = get_length_of_wake_up_frame();
  dst++;

  /* frame type and subtype */
  dst[0] = EXTENDED_FRAME_TYPE | (csl_state.transmit.subtype << 6);
  dst += CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN;

  /* destination PAN ID */
  if(has_destination_pan_id(csl_state.transmit.subtype)) {
    dst[0] = (IEEE802154_PANID & 0xFF) ^ csl_get_channel();
    dst[1] = (IEEE802154_PANID >> 8) & 0xFF;
    dst += CSL_FRAMER_POTR_PAN_ID_LEN;
  }

  /* OTP-related fields */
  if(has_otp_etc(csl_state.transmit.subtype)) {
    entry = akes_nbr_get_receiver_entry();

    if(!entry || !entry->permanent) {
      return FRAMER_FAILED;
    }

    /* source index */
    dst[0] = entry->permanent->foreign_index;
    dst += CSL_FRAMER_POTR_SOURCE_INDEX_LEN;

    /* payload frame's length */
    payload_frames_length = packetbuf_totlen();
    dst[0] = payload_frames_length;
    dst += CSL_FRAMER_POTR_PAYLOAD_FRAMES_LEN_LEN;

    /* OTP */
    csl_ccm_inputs_generate_otp_nonce(nonce, 1);
    AES_128_GET_LOCK();
    CCM_STAR.set_key(entry->permanent->pairwise_key);
    CCM_STAR.aead(nonce,
          NULL, 0,
          &payload_frames_length, 1,
          dst, CSL_FRAMER_POTR_OTP_LEN, 1);
    AES_128_RELEASE_LOCK();
    dst += CSL_FRAMER_POTR_OTP_LEN;
  }

  /* rendezvous time is set in csl_framer_update_rendezvous_time */
  csl_state.transmit.rendezvous_time_len =
      get_rendezvous_time_len(csl_state.transmit.subtype);

  return 1;
}
/*---------------------------------------------------------------------------*/
static void
update_rendezvous_time(uint8_t *frame_length)
{
  memcpy(frame_length + frame_length[0] + 1 - csl_state.transmit.rendezvous_time_len,
      &csl_state.transmit.remaining_wake_up_frames,
      csl_state.transmit.rendezvous_time_len);
}
/*---------------------------------------------------------------------------*/
static int
parse_wake_up_frame(void)
{
  uint8_t datalen;
  uint8_t *dataptr;
  uint16_t dst_pid;
  struct akes_nbr_entry *entry;
  struct akes_nbr *nbr;
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  uint8_t otp[CSL_FRAMER_POTR_OTP_LEN];
  uint8_t rendezvous_time_len;
  uint32_t rendezvous_time_symbol_periods;
  rtimer_clock_t rendezvous_time_rtimer_ticks;

  /* frame length */
  datalen = NETSTACK_RADIO.async_read_phy_header_to_packetbuf();
  if((datalen > (CSL_FRAMER_POTR_MAX_WAKE_UP_FRAME_LEN - RADIO_PHY_HEADER_LEN))
      || (datalen < (CSL_FRAMER_POTR_MIN_WAKE_UP_FRAME_LEN - RADIO_PHY_HEADER_LEN))) {
    LOG_WARN("invalid wake-up frame\n");
    return FRAMER_FAILED;
  }

  /* frame type and subtype */
  if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  dataptr = packetbuf_dataptr();
  if((dataptr[0] & 0x3F) != EXTENDED_FRAME_TYPE) {
    LOG_WARN("invalid frame type\n");
    return FRAMER_FAILED;
  }
  csl_state.duty_cycle.subtype = (dataptr[0] >> 6) & 3;
  if(datalen != (get_length_of_wake_up_frame())) {
    LOG_WARN("invalid length\n");
    return FRAMER_FAILED;
  }
  dataptr++;

  /* destination PAN ID */
  if(has_destination_pan_id(csl_state.duty_cycle.subtype)) {
    if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(CSL_FRAMER_POTR_PAN_ID_LEN)) {
      LOG_ERR("could not read at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    dst_pid = (dataptr[0] ^ csl_get_channel())
        | (dataptr[1] << 8);
    if((dst_pid != IEEE802154_PANID)
        && (dst_pid != FRAME802154_BROADCASTPANDID)) {
      LOG_INFO("for another PAN %04x\n", dst_pid);
      return FRAMER_FAILED;
    }
    dataptr += CSL_FRAMER_POTR_PAN_ID_LEN;
  }

  switch(csl_state.duty_cycle.subtype) {
  case CSL_SUBTYPE_HELLO:
    if(leaky_bucket_is_full(&csl_framer_potr_hello_inc_bucket)) {
      LOG_WARN("HELLO bucket is full\n");
      return FRAMER_FAILED;
    }
    break;
  case CSL_SUBTYPE_HELLOACK:
    if(!akes_is_acceptable_helloack()) {
      LOG_ERR("unacceptable HELLOACK\n");
      return FRAMER_FAILED;
    }
    if(leaky_bucket_is_full(&csl_framer_potr_helloack_inc_bucket)) {
      LOG_WARN("HELLOACK bucket is full\n");
      return FRAMER_FAILED;
    }
    break;
  default:
    break;
  }

  if(has_otp_etc(csl_state.duty_cycle.subtype)) {
    /* source index */
    if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(CSL_FRAMER_POTR_SOURCE_INDEX_LEN)) {
      LOG_ERR("could not read at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    nbr = akes_nbr_get_nbr(dataptr[0]);
    if(!nbr) {
      LOG_WARN("invalid index\n");
      return FRAMER_FAILED;
    }
    entry = akes_nbr_get_entry_of(nbr);
    if(!entry) {
      LOG_WARN("outdated index\n");
      return FRAMER_FAILED;
    }
    if((csl_state.duty_cycle.subtype == CSL_SUBTYPE_ACK)
        && !akes_is_acceptable_ack(entry)) {
      LOG_ERR("unacceptable ACK\n");
      return FRAMER_FAILED;
    }
    packetbuf_set_addr(PACKETBUF_ADDR_SENDER, akes_nbr_get_addr(entry));
    CCM_STAR.set_key(nbr->pairwise_key);
    csl_ccm_inputs_generate_otp_nonce(nonce, 0);
    dataptr++;

    /* payload frame's length */
    if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(CSL_FRAMER_POTR_PAYLOAD_FRAMES_LEN_LEN)) {
      LOG_ERR("could not read at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    csl_state.duty_cycle.next_frames_len = dataptr[0];
    switch(csl_state.duty_cycle.subtype) {
    case CSL_SUBTYPE_ACK:
      if(csl_state.duty_cycle.next_frames_len != CSL_FRAMER_POTR_ACK_PAYLOAD_FRAME_LEN) {
        LOG_ERR("ACK has invalid length\n");
        return FRAMER_FAILED;
      }
      break;
    case CSL_SUBTYPE_NORMAL:
      if(csl_state.duty_cycle.next_frames_len <= MIN_NORMAL_PAYLOAD_FRAME_LEN) {
        LOG_ERR("payload frame is too short\n");
        return FRAMER_FAILED;
      }
      break;
    default:
      break;
    }
    dataptr++;

    /* OTP */
    CCM_STAR.aead(nonce,
        NULL, 0,
        &csl_state.duty_cycle.next_frames_len, 1,
        otp, CSL_FRAMER_POTR_OTP_LEN, 0);
    if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(CSL_FRAMER_POTR_OTP_LEN)) {
      LOG_ERR("could not read at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    if(memcmp(otp, dataptr, CSL_FRAMER_POTR_OTP_LEN)) {
      LOG_WARN("invalid OTP\n");
      return FRAMER_FAILED;
    }
    dataptr += CSL_FRAMER_POTR_OTP_LEN;
  }

  /* rendezvous time */
  rendezvous_time_len = get_rendezvous_time_len(csl_state.duty_cycle.subtype);
  if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(rendezvous_time_len)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  memcpy(&csl_state.duty_cycle.remaining_wake_up_frames, dataptr, rendezvous_time_len);
  rendezvous_time_symbol_periods = (RADIO_SYMBOLS_PER_BYTE
          * ((uint32_t)csl_state.duty_cycle.remaining_wake_up_frames
          * (datalen + RADIO_PHY_HEADER_LEN)))
      + (RADIO_SYMBOLS_PER_BYTE
          * (datalen + RADIO_PHY_HEADER_LEN - RADIO_SHR_LEN));
  rendezvous_time_rtimer_ticks = RADIO_TIME_TO_TRANSMIT(rendezvous_time_symbol_periods);
  csl_state.duty_cycle.rendezvous_time = csl_state.duty_cycle.wake_up_frame_sfd_timestamp
      + rendezvous_time_rtimer_ticks
      - 1;
  switch(csl_state.duty_cycle.subtype) {
  case CSL_SUBTYPE_HELLO:
    if(csl_state.duty_cycle.remaining_wake_up_frames
        >= csl_hello_wake_up_sequence_length) {
      LOG_ERR("rendezvous time of HELLO is too late\n");
      return FRAMER_FAILED;
    }
    break;
  default:
    /* check upper bound maintained by SPLO-CSL */
    if(csl_state.duty_cycle.remaining_wake_up_frames
          >= CSL_FRAMER_WAKE_UP_SEQUENCE_LENGTH(
        CSL_MAX_OVERALL_UNCERTAINTY, (datalen + RADIO_PHY_HEADER_LEN))) {
      LOG_ERR("rendezvous time is too late\n");
      return FRAMER_FAILED;
    }
    break;
  }
  switch(csl_state.duty_cycle.subtype) {
  case CSL_SUBTYPE_HELLO:
    leaky_bucket_pour(&csl_framer_potr_hello_inc_bucket);
    break;
  case CSL_SUBTYPE_HELLOACK:
    leaky_bucket_pour(&csl_framer_potr_helloack_inc_bucket);
    break;
  default:
    break;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
prepare_acknowledgement_parsing(void)
{
  switch(csl_state.transmit.subtype) {
  case CSL_SUBTYPE_ACK:
  case CSL_SUBTYPE_NORMAL:
    memcpy(csl_state.transmit.acknowledgement_key,
        akes_nbr_get_receiver_entry()->permanent->pairwise_key,
        AES_128_KEY_LENGTH);
    csl_ccm_inputs_generate_acknowledgement_nonce(csl_state.transmit.acknowledgement_nonce, 0);
    return 1;
  default:
    return 1;
  }
}
/*---------------------------------------------------------------------------*/
static int
parse_acknowledgement(void)
{
  uint8_t len;
  uint8_t expected_len;
  uint8_t acknowledgement[CSL_FRAMER_POTR_MAX_ACKNOWLEDGEMENT_LEN];
  uint8_t phase_len;
  uint8_t expected_mic[AKES_MAC_UNICAST_MIC_LEN];

  phase_len = csl_state.transmit.burst_index ? 0 : CSL_FRAMER_POTR_PHASE_LEN;
  expected_len = (csl_state.transmit.subtype == CSL_SUBTYPE_HELLOACK)
      ? CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN
      : (CSL_FRAMER_POTR_EXTENDED_FRAME_TYPE_LEN
          + phase_len
          + AKES_MAC_UNICAST_MIC_LEN);

  /* frame length */
  len = NETSTACK_RADIO.async_read_phy_header();
  if(len != expected_len) {
    LOG_ERR("acknowledgement frame has invalid length\n");
    return FRAMER_FAILED;
  }

  /* frame type */
  if(!NETSTACK_RADIO.async_read_payload(acknowledgement, 1)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  if(acknowledgement[0] != EXTENDED_FRAME_TYPE) {
    return FRAMER_FAILED;
  }
  if(csl_state.transmit.subtype != CSL_SUBTYPE_HELLOACK) {
    if(aes_128_locked) {
      LOG_ERR("could not validate acknowledgement frame\n");
      return FRAMER_FAILED;
    }

    /* CSL phase */
    if(phase_len) {
      if(!NETSTACK_RADIO.async_read_payload(acknowledgement + 1, 2)) {
        LOG_ERR("could not read at line %i\n", __LINE__);
        return FRAMER_FAILED;
      }
      csl_state.transmit.acknowledgement_phase = csl_framer_potr_parse_phase(acknowledgement + 1);
    }

    /* CCM* MIC */
    AES_128_GET_LOCK();
    CCM_STAR.set_key(csl_state.transmit.acknowledgement_key);
    csl_state.transmit.acknowledgement_nonce[LINKADDR_SIZE] &= ~(0x3F);
    csl_state.transmit.acknowledgement_nonce[LINKADDR_SIZE] |= csl_state.transmit.burst_index;
    CCM_STAR.aead(csl_state.transmit.acknowledgement_nonce,
        NULL, 0,
        acknowledgement, 1 + phase_len,
        expected_mic, AKES_MAC_UNICAST_MIC_LEN, 0);
    AES_128_RELEASE_LOCK();
    if(!NETSTACK_RADIO.async_read_payload(acknowledgement + 1 + phase_len,
        AKES_MAC_UNICAST_MIC_LEN)) {
      LOG_ERR("could not read at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    if(memcmp(expected_mic,
        acknowledgement + 1 + phase_len,
        AKES_MAC_UNICAST_MIC_LEN)) {
      LOG_ERR("inauthentic acknowledgement frame\n");
      return FRAMER_FAILED;
    }
  }

  return len;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  leaky_bucket_init(&csl_framer_potr_hello_inc_bucket,
      MAX_CONSECUTIVE_INC_HELLOS,
      MAX_INC_HELLO_RATE);
  leaky_bucket_init(&csl_framer_potr_helloack_inc_bucket,
      MAX_CONSECUTIVE_INC_HELLOACKS,
      MAX_INC_HELLOACK_RATE);
}
/*---------------------------------------------------------------------------*/
const struct csl_framer csl_framer_potr_csl_framer = {
  get_min_bytes_for_filtering,
  filter,
  get_length_of_wake_up_frame,
  create_wake_up_frame,
  update_rendezvous_time,
  parse_wake_up_frame,
  prepare_acknowledgement_parsing,
  parse_acknowledgement,
  on_unicast_transmitted,
  init
};
/*---------------------------------------------------------------------------*/
const struct framer csl_framer_potr_framer = {
  length,
  create,
  parse
};
/*---------------------------------------------------------------------------*/
#endif /* !CSL_COMPLIANT */

/** @} */
