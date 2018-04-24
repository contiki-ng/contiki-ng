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
 *         Assembles IEEE 802.15.4-compliant frames
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/csl/csl.h"
#include "net/mac/csl/csl-framer-compliant.h"
#include "net/mac/csl/csl-framer.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/framer/frame802154e-ie.h"
#include "net/mac/csl/csl-ccm-inputs.h"
#include "dev/watchdog.h"
#include "services/akes/akes-delete.h"

#define RENDEZVOUS_TIME_LEN (2)

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL-framer-compliant"
#define LOG_LEVEL LOG_LEVEL_FRAMER

#if LLSEC802154_USES_FRAME_COUNTER
#define EXPECTED_ACKNOWLEDGEMENT_MIC_LEN csl_state.transmit.expected_mic_len
#else /* LLSEC802154_USES_FRAME_COUNTER */
#define EXPECTED_ACKNOWLEDGEMENT_MIC_LEN 0
#endif /* LLSEC802154_USES_FRAME_COUNTER */

#if CSL_COMPLIANT

/*---------------------------------------------------------------------------*/
static void
prepare_outgoing_payload_frame(frame802154_t *f)
{
  memset(f, 0, sizeof(frame802154_t));
  f->fcf.frame_type = packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE);
  f->fcf.frame_pending = packetbuf_attr(PACKETBUF_ATTR_PENDING) ? 1 : 0;
  f->fcf.ack_required = packetbuf_holds_broadcast() ? 0 : 1;
  f->fcf.panid_compression = 1;
  f->fcf.frame_version = FRAME802154_VERSION;
  f->fcf.src_addr_mode = FRAME802154_SHORTADDRMODE;
#if LLSEC802154_USES_FRAME_COUNTER
  f->fcf.sequence_number_suppression = 1;
  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL)) {
    f->fcf.security_enabled = 1;
    f->aux_hdr.security_control.security_level = packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL);
    f->aux_hdr.frame_counter.u16[0] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1);
    f->aux_hdr.frame_counter.u16[1] = packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3);
  }
#else /* LLSEC802154_USES_FRAME_COUNTER */
  if(packetbuf_holds_broadcast()
      || (f->fcf.frame_type == FRAME802154_CMDFRAME)) {
    f->fcf.sequence_number_suppression = 1;
  } else {
    f->seq = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */
  linkaddr_copy((linkaddr_t *)f->src_addr, &linkaddr_node_addr);
}
/*---------------------------------------------------------------------------*/
static int
length(void)
{
  frame802154_t f;
  prepare_outgoing_payload_frame(&f);
  return frame802154_hdrlen(&f);
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  frame802154_t f;
  int len;

  prepare_outgoing_payload_frame(&f);
  len = frame802154_hdrlen(&f);
  if(!packetbuf_hdralloc(len)) {
    LOG_ERR("packetbuf_hdralloc failed\n");
    return FRAMER_FAILED;
  }
  frame802154_create(&f, packetbuf_hdrptr());
  return len;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_min_bytes_for_filtering(void)
{
  return 2 /* Frame Control field */
#if !LLSEC802154_USES_FRAME_COUNTER
      + 1 /* Sequence Number field */
#endif /* !LLSEC802154_USES_FRAME_COUNTER */
      + LINKADDR_SIZE /* Source Address field */
      + 5;
}
/*---------------------------------------------------------------------------*/
static int
filter(uint8_t *dst)
{
  frame802154_t f;
  int len;
  uint8_t *frame_length;
  rtimer_clock_t acknowledgement_sfd_timestamp;
  struct ieee802154_ies ies;
  int ies_len;
#if LLSEC802154_USES_FRAME_COUNTER
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
#endif /* LLSEC802154_USES_FRAME_COUNTER */
#if AKES_NBR_WITH_PAIRWISE_KEYS
  struct akes_nbr_entry *entry;
#endif /* AKES_NBR_WITH_PAIRWISE_KEYS */

  /* delegate to frame_802154 */
  len = frame802154_parse(packetbuf_hdrptr(), packetbuf_totlen(), &f);
  if(!len) {
    LOG_ERR("frame802154_parse failed\n");
    return FRAMER_FAILED;
  }

  if(len > get_min_bytes_for_filtering()) {
    LOG_ERR("unexpected header length\n");
    return FRAMER_FAILED;
  }

  if(!packetbuf_hdrreduce(len)) {
    LOG_ERR("packetbuf_hdrreduce failed\n");
    return FRAMER_FAILED;
  }

  /* validate frame type */
  switch(f.fcf.frame_type) {
  case FRAME802154_DATAFRAME:
  case FRAME802154_CMDFRAME:
    break;
  default:
    LOG_WARN("unexpected frame type\n");
    return FRAMER_FAILED;
  }

  /* validate addressing modes */
  if(f.fcf.dest_addr_mode) {
    LOG_WARN("payload frames with destination addresses are not yet handled\n");
    return FRAMER_FAILED;
  }
  if(f.fcf.src_addr_mode != FRAME802154_SHORTADDRMODE) {
    LOG_WARN("invalid source addressing mode\n");
    return FRAMER_FAILED;
  }

#if LLSEC802154_USES_FRAME_COUNTER
  if(!f.fcf.sequence_number_suppression) {
    LOG_ERR("payload frames with sequence numbers are not yet handled\n");
    return FRAMER_FAILED;
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* validate source address */
  if(linkaddr_cmp((linkaddr_t *)&f.src_addr, &linkaddr_node_addr)) {
    LOG_ERR("frame from ourselves\n");
    return FRAMER_FAILED;
  }

  /* set packetbuf attributes */
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, f.fcf.frame_type);
  packetbuf_set_attr(PACKETBUF_ATTR_PENDING, f.fcf.frame_pending);
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, f.fcf.sequence_number_suppression ? 0xffff : f.seq);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, (linkaddr_t *)&f.src_addr);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &csl_state.duty_cycle.receiver);
#if LLSEC802154_USES_FRAME_COUNTER
  /* set security-related packetbuf attributes */
  if(f.fcf.security_enabled) {
    packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL, f.aux_hdr.security_control.security_level);
    if(!f.aux_hdr.security_control.frame_counter_suppression) {
      packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1, f.aux_hdr.frame_counter.u16[0]);
      packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3, f.aux_hdr.frame_counter.u16[1]);
    }
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* prepare acknowledgement frame */
  if(!packetbuf_holds_broadcast()) {
    /* pointer to frame length */
    frame_length = dst;
    dst++;

    /* create frame header */
    memset(&f, 0, sizeof(f));
    f.fcf.frame_type = FRAME802154_ACKFRAME;
    f.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
    f.fcf.panid_compression = 1;
    f.fcf.sequence_number_suppression = 1;
    f.fcf.frame_version = FRAME802154_VERSION;
    f.fcf.ie_list_present = 1;
    linkaddr_copy((linkaddr_t *)f.dest_addr, packetbuf_addr(PACKETBUF_ADDR_SENDER));
#if LLSEC802154_USES_FRAME_COUNTER
    if(packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE) == FRAME802154_DATAFRAME) {
      f.fcf.security_enabled = 1;
    } else {
      NETSTACK_RADIO.async_read_payload_to_packetbuf(1);
      f.fcf.security_enabled = akes_mac_is_helloack() ? 0 : 1;
    }
    if(f.fcf.security_enabled) {
      f.aux_hdr.security_control.security_level = CSL_FRAMER_COMPLIANT_ACKNOWLEDGEMENT_SEC_LVL;
      if(!anti_replay_set_counter_to(&f.aux_hdr.frame_counter)) {
        watchdog_reboot();
      }
    }
#endif /* LLSEC802154_USES_FRAME_COUNTER */
    len = frame802154_create(&f, dst);
    if(len != (2 + LINKADDR_SIZE + (f.fcf.security_enabled ? 5 : 0))) {
      LOG_WARN("acknowledgement header has unexpected length\n");
    }
    dst += len;

    /* create IE list */
    acknowledgement_sfd_timestamp = csl_get_sfd_timestamp_of_last_payload_frame()
          + RADIO_TIME_TO_TRANSMIT(RADIO_SYMBOLS_PER_BYTE
              * (1 /* Frame Length */ + packetbuf_totlen() + RADIO_SHR_LEN))
          + RADIO_TRANSMIT_CALIBRATION_TIME;
    ies.csl_phase = csl_get_phase(acknowledgement_sfd_timestamp);
    ies.csl_period = WAKE_UP_COUNTER_INTERVAL;
    ies_len = frame802154e_create_ie_csl(dst, CSL_FRAMER_COMPLIANT_CSL_IE_LEN, &ies);
    if(ies_len != CSL_FRAMER_COMPLIANT_CSL_IE_LEN) {
      LOG_WARN("acknowledgement's IE list has unexpected length\n");
    }
    dst += ies_len;

    /* set frame length */
    frame_length[0] = len + ies_len + CRC16_FRAMER_CHECKSUM_LEN;

#if LLSEC802154_USES_FRAME_COUNTER
    if(f.fcf.security_enabled) {
      /* CCM MIC */
      frame_length[0] += AKES_MAC_UNICAST_MIC_LEN;
      csl_ccm_inputs_generate_acknowledgement_nonce(nonce,
          &linkaddr_node_addr,
          &f.aux_hdr.frame_counter);
#if AKES_NBR_WITH_PAIRWISE_KEYS
      entry = akes_nbr_get_sender_entry();
      if(entry && entry->permanent) {
        CCM_STAR.set_key(entry->permanent->pairwise_key);
      }
#else /* AKES_NBR_WITH_PAIRWISE_KEYS */
      CCM_STAR.set_key(akes_mac_group_key);
#endif /* AKES_NBR_WITH_PAIRWISE_KEYS */
      CCM_STAR.aead(nonce,
          NULL, 0,
          frame_length + 1, len + ies_len,
          dst, AKES_MAC_UNICAST_MIC_LEN,
          1);
    }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

    /* append checksum */
    crc16_framer_append_checksum(frame_length + 1, frame_length[0] - CRC16_FRAMER_CHECKSUM_LEN);
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
  return packetbuf_hdrlen();
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_length_of_wake_up_frame(void)
{
  return CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN - RADIO_PHY_HEADER_LEN;
}
/*---------------------------------------------------------------------------*/
static int
create_wake_up_frame(uint8_t *dst)
{
  frame802154_t f;
  int len;
  struct ieee802154_ies ies;
  int ies_len;

  /* set frame length */
  dst[0] = CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN - RADIO_PHY_HEADER_LEN;
  dst++;

  /* create frame header */
  memset(&f, 0, sizeof(frame802154_t));
  f.fcf.frame_type = FRAME802154_MPFRAME;
  f.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
  f.fcf.long_frame_control = 1;
  f.fcf.sequence_number_suppression = 1;
  f.fcf.ie_list_present = 1;
  if(packetbuf_holds_broadcast()) {
    memset(f.dest_addr, 0xff, 2);
  } else {
    linkaddr_copy((linkaddr_t *)f.dest_addr,
        packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  }
  f.dest_pid = IEEE802154_PANID;
  len = frame802154_create(&f, dst);
  dst += len;

  /* create IE list */
  ies_len = frame802154e_create_ie_rendezvous_time(dst,
      CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN - len,
      &ies);
  if(ies_len == FRAMER_FAILED) {
    LOG_ERR("frame802154e_create_ie_rendezvous_time failed\n");
    return FRAMER_FAILED;
  }

  if((len + ies_len + CRC16_FRAMER_CHECKSUM_LEN)
      != (CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN - RADIO_PHY_HEADER_LEN)) {
    LOG_ERR("created wake-up frame has unexpected\n");
    return FRAMER_FAILED;
  }

  return CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN - RADIO_PHY_HEADER_LEN;
}
/*---------------------------------------------------------------------------*/
static void
update_rendezvous_time(uint8_t *frame_length)
{
  memcpy(frame_length + frame_length[0] + 1 - CRC16_FRAMER_CHECKSUM_LEN - RENDEZVOUS_TIME_LEN,
      &csl_state.transmit.remaining_wake_up_frames,
      RENDEZVOUS_TIME_LEN);
  crc16_framer_append_checksum(frame_length + 1,
      frame_length[0] - CRC16_FRAMER_CHECKSUM_LEN);
}
/*---------------------------------------------------------------------------*/
static int
parse_wake_up_frame(void)
{
  uint8_t datalen;
  uint8_t *dataptr;
  frame802154_t f;
  int len;
  struct ieee802154_ies ies;
  int ies_len;
  uint32_t rendezvous_time_symbol_periods;
  rtimer_clock_t rendezvous_time_rtimer_ticks;

  /* parse and validate frame length */
  datalen = NETSTACK_RADIO.async_read_phy_header_to_packetbuf();
  if(datalen != get_length_of_wake_up_frame()) {
    LOG_WARN("unexpected frame length\n");
    return FRAMER_FAILED;
  }

  /* parse and validate frame header on behalf of frame802154 */
  if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(datalen - CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  dataptr = packetbuf_dataptr();
  len = frame802154_parse(dataptr, datalen - CRC16_FRAMER_CHECKSUM_LEN, &f);
  if(!len) {
    LOG_ERR("frame802154_parse failed\n");
    return FRAMER_FAILED;
  }
  if(f.fcf.frame_type != FRAME802154_MPFRAME) {
    LOG_ERR("invalid frame type %i\n", f.fcf.frame_type);
    return FRAMER_FAILED;
  }
  if(f.fcf.dest_addr_mode != FRAME802154_SHORTADDRMODE) {
    LOG_ERR("invalid destination addressing mode\n");
    return FRAMER_FAILED;
  }
  if(!f.fcf.ie_list_present) {
    LOG_ERR("no IE list\n");
    return FRAMER_FAILED;
  }
  if(f.fcf.panid_compression) {
    LOG_WARN("wake-up frames without PAN IDs are not yet handled\n");
    return FRAMER_FAILED;
  }
  if(f.fcf.src_addr_mode) {
    LOG_WARN("wake-up frames with source addresses are not yet handled\n");
    return FRAMER_FAILED;
  }
  if((f.dest_pid != IEEE802154_PANID)
      && (f.dest_pid != FRAME802154_BROADCASTPANDID)) {
    LOG_INFO("for another PAN %04x\n", f.dest_pid);
    return FRAMER_FAILED;
  }
  if(!linkaddr_cmp((linkaddr_t *)f.dest_addr, &linkaddr_node_addr)) {
    if(!frame802154_is_broadcast_addr(f.fcf.dest_addr_mode, f.dest_addr)) {
      LOG_INFO("for another node\n");
      return FRAMER_FAILED;
    }
    linkaddr_copy(&csl_state.duty_cycle.receiver, &linkaddr_null);
  } else {
    linkaddr_copy(&csl_state.duty_cycle.receiver, &linkaddr_node_addr);
  }
  dataptr += len;

  /* parse IE list */
  ies_len = frame802154e_parse_information_elements(dataptr, datalen - len - CRC16_FRAMER_CHECKSUM_LEN, &ies);
  if(ies_len == FRAMER_FAILED) {
    LOG_ERR("frame802154e_parse_information_elements failed\n");
    return FRAMER_FAILED;
  }
  csl_state.duty_cycle.remaining_wake_up_frames = ies.rendezvous_time;
  rendezvous_time_symbol_periods = (RADIO_SYMBOLS_PER_BYTE
          * ((uint32_t)csl_state.duty_cycle.remaining_wake_up_frames
          * (datalen + RADIO_PHY_HEADER_LEN)))
      + (RADIO_SYMBOLS_PER_BYTE
          * (datalen + RADIO_PHY_HEADER_LEN - RADIO_SHR_LEN));
  rendezvous_time_rtimer_ticks = RADIO_TIME_TO_TRANSMIT(rendezvous_time_symbol_periods);
  csl_state.duty_cycle.rendezvous_time = csl_state.duty_cycle.wake_up_frame_sfd_timestamp
      + rendezvous_time_rtimer_ticks
      - 1;

  /* check checksum */
  if(!NETSTACK_RADIO.async_read_payload_to_packetbuf(CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  if(!crc16_framer_check_checksum(packetbuf_dataptr(),
      datalen - CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("wake-up frame has invalid checksum\n");
    return FRAMER_FAILED;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
prepare_acknowledgement_parsing(void)
{
#if LLSEC802154_USES_FRAME_COUNTER
  struct akes_nbr_entry *entry;

  if(csl_state.transmit.is_broadcast || akes_mac_is_helloack()) {
    return 1;
  }
  csl_state.transmit.expected_mic_len = AKES_MAC_UNICAST_MIC_LEN;
  entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_WARN("receiver is not permanent");
    return 0;
  }
  memcpy(csl_state.transmit.acknowledgement_key,
#if AKES_NBR_WITH_PAIRWISE_KEYS
      entry->permanent->pairwise_key,
#else /* AKES_NBR_WITH_PAIRWISE_KEYS */
      entry->permanent->group_key,
#endif /* AKES_NBR_WITH_PAIRWISE_KEYS */
      AES_128_KEY_LENGTH);
  linkaddr_copy(&csl_state.transmit.receiver,
      packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  csl_state.transmit.his_unicast_counter =
      entry->permanent->anti_replay_info.his_unicast_counter;
#endif /* LLSEC802154_USES_FRAME_COUNTER */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
parse_acknowledgement(void)
{
  uint8_t frame_length;
  uint8_t acknowledgement[CSL_FRAMER_COMPLIANT_MAX_ACKNOWLEDGEMENT_LEN];
  uint8_t received_checksum[CRC16_FRAMER_CHECKSUM_LEN];
  frame802154_t f;
  int len;
  struct ieee802154_ies ies;
  int ies_len;
#if LLSEC802154_USES_FRAME_COUNTER
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  uint8_t expected_mic[csl_state.transmit.expected_mic_len];
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* parse and validate frame length */
  frame_length = NETSTACK_RADIO.async_read_phy_header();
  if(frame_length > CSL_FRAMER_COMPLIANT_MAX_ACKNOWLEDGEMENT_LEN) {
    LOG_ERR("acknowledgement frame has invalid length\n");
    return FRAMER_FAILED;
  }

  /* parse and validate frame header */
  if(!NETSTACK_RADIO.async_read_payload(acknowledgement,
      frame_length - EXPECTED_ACKNOWLEDGEMENT_MIC_LEN - CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  len = frame802154_parse(acknowledgement, frame_length - CRC16_FRAMER_CHECKSUM_LEN, &f);
  if(!len) {
    LOG_ERR("frame802154_parse failed\n");
    return FRAMER_FAILED;
  }
  if(f.fcf.frame_type != FRAME802154_ACKFRAME) {
    LOG_ERR("unexpected frame type %i\n", f.fcf.frame_type);
    return FRAMER_FAILED;
  }
  if(f.fcf.dest_addr_mode != FRAME802154_SHORTADDRMODE) {
    LOG_ERR("unexpected destination addressing mode\n");
    return FRAMER_FAILED;
  }
  if(!f.fcf.ie_list_present) {
    LOG_ERR("no IE list\n");
    return FRAMER_FAILED;
  }
  if(f.dest_pid
      && (f.dest_pid != IEEE802154_PANID)
      && (f.dest_pid != FRAME802154_BROADCASTPANDID)) {
    LOG_INFO("for another PAN %04x\n", f.dest_pid);
    return FRAMER_FAILED;
  }
  if(!linkaddr_cmp((linkaddr_t *)f.dest_addr, &linkaddr_node_addr)) {
    LOG_INFO("for another node\n");
    return FRAMER_FAILED;
  }
#if LLSEC802154_USES_FRAME_COUNTER
  if((!f.fcf.security_enabled && csl_state.transmit.expected_mic_len)
      || (f.fcf.security_enabled && (CSL_FRAMER_COMPLIANT_ACKNOWLEDGEMENT_SEC_LVL != f.aux_hdr.security_control.security_level))) {
    LOG_ERR("unexpected security level\n");
    return FRAMER_FAILED;
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* parse and validate IE list */
  if(frame_length - len - EXPECTED_ACKNOWLEDGEMENT_MIC_LEN - CRC16_FRAMER_CHECKSUM_LEN < CSL_FRAMER_COMPLIANT_CSL_IE_LEN) {
    LOG_ERR("not enough bytes left\n");
    return FRAMER_FAILED;
  }
  ies_len = frame802154e_parse_information_elements(
      acknowledgement + len, frame_length - len - EXPECTED_ACKNOWLEDGEMENT_MIC_LEN - CRC16_FRAMER_CHECKSUM_LEN, &ies);
  if(ies_len == FRAMER_FAILED) {
    LOG_ERR("failed to read CSL IE %i\n", frame_length - len - EXPECTED_ACKNOWLEDGEMENT_MIC_LEN - CRC16_FRAMER_CHECKSUM_LEN);
    return FRAMER_FAILED;
  }
  if(!csl_state.transmit.burst_index) {
    csl_state.transmit.acknowledgement_phase = ies.csl_phase;
  }

  if((frame_length - len - ies_len - EXPECTED_ACKNOWLEDGEMENT_MIC_LEN) != CRC16_FRAMER_CHECKSUM_LEN) {
    LOG_ERR("acknowledgement has payload\n");
    return FRAMER_FAILED;
  }

#if LLSEC802154_USES_FRAME_COUNTER
  /* check MIC */
  if(csl_state.transmit.expected_mic_len) {
    csl_ccm_inputs_generate_acknowledgement_nonce(nonce,
          &csl_state.transmit.receiver,
          &f.aux_hdr.frame_counter);
    CCM_STAR.set_key(csl_state.transmit.acknowledgement_key);
    CCM_STAR.aead(nonce,
        NULL, 0,
        acknowledgement, len + ies_len,
        expected_mic, CSL_FRAMER_COMPLIANT_ACKNOWLEDGEMENT_MIC_LEN,
        0);
    if(!NETSTACK_RADIO.async_read_payload(acknowledgement + len + ies_len,
        csl_state.transmit.expected_mic_len)) {
      LOG_ERR("could not read at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    if(memcmp(expected_mic, acknowledgement + len + ies_len, CSL_FRAMER_COMPLIANT_ACKNOWLEDGEMENT_MIC_LEN)) {
      LOG_ERR("inauthentic MIC\n");
      return FRAMER_FAILED;
    }
    if(f.aux_hdr.frame_counter.u32 <= csl_state.transmit.his_unicast_counter.u32) {
      LOG_ERR("replayed acknowledgement\n");
      return FRAMER_FAILED;
    }
    csl_state.transmit.his_unicast_counter = f.aux_hdr.frame_counter;
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* check checksum */
  crc16_framer_append_checksum(acknowledgement, frame_length - CRC16_FRAMER_CHECKSUM_LEN);
  if(!NETSTACK_RADIO.async_read_payload(received_checksum, CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  if(memcmp(acknowledgement + frame_length - CRC16_FRAMER_CHECKSUM_LEN, received_checksum, CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("acknowledgement frame has invalid checksum\n");
    return FRAMER_FAILED;
  }

  return frame_length;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
#if LLSEC802154_USES_FRAME_COUNTER
  struct akes_nbr_entry *entry;

  if(akes_mac_is_helloack()) {
    return;
  }
  entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_WARN("receiver is no longer permanent\n");
    return;
  }

  if(entry->permanent->anti_replay_info.his_unicast_counter.u32
      != csl_state.transmit.his_unicast_counter.u32)  {
    entry->permanent->anti_replay_info.his_unicast_counter
        = csl_state.transmit.his_unicast_counter;
    AKES_DELETE_STRATEGY.prolong_permanent(entry->permanent);
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
const struct csl_framer csl_framer_compliant_csl_framer = {
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
const struct framer csl_framer_compliant_framer = {
  length,
  create,
  parse
};
/*---------------------------------------------------------------------------*/
#endif /* CSL_COMPLIANT */

/** @} */
