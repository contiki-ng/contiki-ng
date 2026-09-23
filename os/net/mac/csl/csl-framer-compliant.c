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
 * \addtogroup csl
 * @{
 *
 * \file
 *         Assembles IEEE 802.15.4-compliant frames
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/csl/csl-framer-compliant.h"
#include "net/mac/csl/csl.h"
#include "net/mac/csl/csl-framer.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/framer/frame802154e-ie.h"
#include "net/mac/csl/csl-ccm-inputs.h"
#include "services/akes/akes-delete.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL-framer-compliant"
#define LOG_LEVEL LOG_LEVEL_FRAMER

#if LLSEC802154_USES_FRAME_COUNTER
#define EXPECTED_ACKNOWLEDGMENT_MIC_LEN csl_state.transmit.expected_mic_len
#else /* LLSEC802154_USES_FRAME_COUNTER */
#define EXPECTED_ACKNOWLEDGMENT_MIC_LEN 0
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
    f->aux_hdr.security_control.security_level =
        packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL);
    f->aux_hdr.frame_counter.u16[0] =
        packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1);
    f->aux_hdr.frame_counter.u16[1] =
        packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3);
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
  prepare_outgoing_payload_frame(&f);
  int len = frame802154_hdrlen(&f);
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
filter(void)
{
  /* delegate to frame_802154 */
  frame802154_t f;
  int len = frame802154_parse(packetbuf_hdrptr(), packetbuf_totlen(), &f);
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
    LOG_WARN("payload frames without destination address\n");
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
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO,
                     f.fcf.sequence_number_suppression ? 0xffff : f.seq);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, (linkaddr_t *)&f.src_addr);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &csl_state.duty_cycle.receiver);
#if LLSEC802154_USES_FRAME_COUNTER
  /* set security-related packetbuf attributes */
  if(f.fcf.security_enabled) {
    packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL,
                       f.aux_hdr.security_control.security_level);
    if(!f.aux_hdr.security_control.frame_counter_suppression) {
      packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1,
                         f.aux_hdr.frame_counter.u16[0]);
      packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3,
                         f.aux_hdr.frame_counter.u16[1]);
    }
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* prepare acknowledgment frame */
  if(!packetbuf_holds_broadcast()) {
    /* create frame header */
    memset(&f, 0, sizeof(f));
    f.fcf.frame_type = FRAME802154_ACKFRAME;
    f.fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
    f.fcf.panid_compression = 1;
    f.fcf.sequence_number_suppression = 1;
    f.fcf.frame_version = FRAME802154_VERSION;
    f.fcf.ie_list_present = 1;
    linkaddr_copy((linkaddr_t *)f.dest_addr,
                  packetbuf_addr(PACKETBUF_ADDR_SENDER));
#if LLSEC802154_USES_FRAME_COUNTER
    if(packetbuf_holds_data_frame()) {
      f.fcf.security_enabled = 1;
    } else {
      if(radio_read_payload_to_packetbuf(1)) {
        LOG_ERR("failed to read command frame identifier\n");
        return FRAMER_FAILED;
      }
      f.fcf.security_enabled = akes_mac_is_helloack() ? 0 : 1;
    }
    if(f.fcf.security_enabled) {
      f.aux_hdr.security_control.security_level =
          CSL_FRAMER_COMPLIANT_acknowledgment_SEC_LVL;
      anti_replay_set_counter_to(&f.aux_hdr.frame_counter);
    }
#endif /* LLSEC802154_USES_FRAME_COUNTER */
    csl_state.duty_cycle.acknowledgment_len =
        frame802154_create(&f, csl_state.duty_cycle.acknowledgment);
    if(csl_state.duty_cycle.acknowledgment_len
       != (2 + LINKADDR_SIZE + (f.fcf.security_enabled ? 5 : 0))) {
      LOG_ERR("acknowledgment header has unexpected length\n");
      return FRAMER_FAILED;
    }

    /* create IE list */
    rtimer_clock_t acknowledgment_sfd_timestamp =
        csl_get_sfd_timestamp_of_last_payload_frame()
        + RADIO_TIME_TO_TRANSMIT(
            RADIO_SYMBOLS_PER_BYTE
            * (RADIO_HEADER_LEN + packetbuf_totlen() + RADIO_SHR_LEN))
        + RADIO_TRANSMIT_CALIBRATION_TIME;
    struct ieee802154_ies ies = {
      ies.csl_phase =
          csl_get_phase(acknowledgment_sfd_timestamp) >> CSL_PHASE_SHIFT,
      ies.csl_period = WAKE_UP_COUNTER_INTERVAL >> CSL_PHASE_SHIFT
    };
    int ies_len = frame802154e_create_ie_csl(
                      csl_state.duty_cycle.acknowledgment
                      + csl_state.duty_cycle.acknowledgment_len,
                      CSL_FRAMER_COMPLIANT_CSL_IE_LEN,
                      &ies);
    if(ies_len != CSL_FRAMER_COMPLIANT_CSL_IE_LEN) {
      LOG_ERR("acknowledgment's IE list has unexpected length\n");
      return FRAMER_FAILED;
    }
    csl_state.duty_cycle.acknowledgment_len += ies_len;

#if LLSEC802154_USES_FRAME_COUNTER
    if(f.fcf.security_enabled) {
      /* CCM MIC */
      uint8_t nonce[CCM_STAR_NONCE_LENGTH];
      csl_ccm_inputs_generate_acknowledgment_nonce(nonce,
                                                   &linkaddr_node_addr,
                                                   &f.aux_hdr.frame_counter);
#if AKES_NBR_WITH_PAIRWISE_KEYS
      akes_nbr_entry_t *entry = akes_nbr_get_sender_entry();
      if(entry && entry->permanent) {
        if(!CCM_STAR.set_key(entry->permanent->pairwise_key)) {
          LOG_ERR("CCM*.set_key failed\n");
          return FRAMER_FAILED;
        }
      }
#else /* AKES_NBR_WITH_PAIRWISE_KEYS */
      if(!CCM_STAR.set_key(akes_mac_group_key)) {
        LOG_ERR("CCM*.set_key failed\n");
        return FRAMER_FAILED;
      }
#endif /* AKES_NBR_WITH_PAIRWISE_KEYS */
      if(!CCM_STAR.aead(nonce,
                        NULL, 0,
                        csl_state.duty_cycle.acknowledgment, len + ies_len,
                        csl_state.duty_cycle.acknowledgment + len + ies_len,
                        AKES_MAC_UNICAST_MIC_LEN,
                        true)) {
        LOG_ERR("CCM*.aead failed\n");
        return FRAMER_FAILED;
      }
      csl_state.duty_cycle.acknowledgment_len += AKES_MAC_UNICAST_MIC_LEN;
    }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

    /* append checksum */
    crc16_framer_append_checksum(csl_state.duty_cycle.acknowledgment,
                                 csl_state.duty_cycle.acknowledgment_len);
    csl_state.duty_cycle.acknowledgment_len += CRC16_FRAMER_CHECKSUM_LEN;
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
static uint_fast16_t
get_length_of_wake_up_frame(void)
{
  return CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN
         - RADIO_SHR_LEN
         - RADIO_HEADER_LEN;
}
/*---------------------------------------------------------------------------*/
static int
create_wake_up_frame(uint8_t *dst)
{
  /* set frame length */
  dst[0] = CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN
           - RADIO_SHR_LEN
           - RADIO_HEADER_LEN;
  dst++;

  /* create frame header */
  frame802154_t f;
  memset(&f, 0, sizeof(f));
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
  int len = frame802154_create(&f, dst);
  dst += len;

  /* create IE list */
  struct ieee802154_ies ies;
  int ies_len = frame802154e_create_ie_rendezvous_time(
                    dst,
                    CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN - len,
                    &ies);
  if(ies_len == FRAMER_FAILED) {
    LOG_ERR("frame802154e_create_ie_rendezvous_time failed\n");
    return FRAMER_FAILED;
  }

  if((len + ies_len + CRC16_FRAMER_CHECKSUM_LEN)
     != (CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN
         - RADIO_SHR_LEN
         - RADIO_HEADER_LEN)) {
    LOG_ERR("created wake-up frame has unexpected\n");
    return FRAMER_FAILED;
  }

  return CSL_FRAMER_COMPLIANT_WAKE_UP_FRAME_LEN
         - RADIO_SHR_LEN
         - RADIO_HEADER_LEN;
}
/*---------------------------------------------------------------------------*/
static void
update_rendezvous_time(uint8_t *frame_length)
{
  memcpy(frame_length
         + frame_length[0]
         + 1
         - CRC16_FRAMER_CHECKSUM_LEN
         - sizeof(csl_state.transmit.remaining_wake_up_frames),
         &csl_state.transmit.remaining_wake_up_frames,
         sizeof(csl_state.transmit.remaining_wake_up_frames));
  crc16_framer_append_checksum(frame_length + 1,
                               frame_length[0] - CRC16_FRAMER_CHECKSUM_LEN);
}
/*---------------------------------------------------------------------------*/
static int
parse_wake_up_frame(void)
{
  /* parse and validate frame length */
  uint_fast16_t datalen = radio_read_phy_header_to_packetbuf();
  if(datalen != get_length_of_wake_up_frame()) {
    LOG_WARN("unexpected frame length\n");
    return FRAMER_FAILED;
  }

  /* parse and validate frame header on behalf of frame802154 */
  if(radio_read_payload_to_packetbuf(datalen - CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  uint8_t *dataptr = packetbuf_dataptr();
  frame802154_t f;
  int len =
      frame802154_parse(dataptr, datalen - CRC16_FRAMER_CHECKSUM_LEN, &f);
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
  struct ieee802154_ies ies;
  int ies_len = frame802154e_parse_information_elements(
                    dataptr,
                    datalen - len - CRC16_FRAMER_CHECKSUM_LEN,
                    &ies);
  if(ies_len == FRAMER_FAILED) {
    LOG_ERR("frame802154e_parse_information_elements failed\n");
    return FRAMER_FAILED;
  }
  csl_state.duty_cycle.remaining_wake_up_frames = ies.rendezvous_time;
  rtimer_clock_t rendezvous_time_rtimer_ticks =
      RADIO_TIME_TO_TRANSMIT(
          RADIO_SYMBOLS_PER_BYTE
          * (((uint_fast32_t)csl_state.duty_cycle.remaining_wake_up_frames
              * (datalen + RADIO_SHR_LEN + RADIO_HEADER_LEN))
             + (datalen + RADIO_HEADER_LEN)));
  csl_state.duty_cycle.rendezvous_time =
      csl_state.duty_cycle.wake_up_frame_sfd_timestamp
      + rendezvous_time_rtimer_ticks
      - 1;

  /* check checksum */
  if(radio_read_payload_to_packetbuf(CRC16_FRAMER_CHECKSUM_LEN)) {
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
prepare_acknowledgment_parsing(void)
{
#if LLSEC802154_USES_FRAME_COUNTER
  if(csl_state.transmit.is_broadcast || akes_mac_is_helloack()) {
    return 1;
  }
  csl_state.transmit.expected_mic_len =
      CSL_FRAMER_COMPLIANT_ACKNOWLEDGMENT_MIC_LEN;
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_WARN("receiver is not permanent");
    return 0;
  }
  memcpy(csl_state.transmit.acknowledgment_key,
#if AKES_NBR_WITH_PAIRWISE_KEYS
         entry->permanent->pairwise_key,
#else /* AKES_NBR_WITH_PAIRWISE_KEYS */
         entry->permanent->group_key,
#endif /* AKES_NBR_WITH_PAIRWISE_KEYS */
         AES_128_KEY_LENGTH);
  linkaddr_copy(&csl_state.transmit.receiver,
                packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  csl_state.transmit.last_unicast_counter =
      entry->permanent->anti_replay_info.last_unicast_counter;
#endif /* LLSEC802154_USES_FRAME_COUNTER */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
parse_acknowledgment(void)
{
  uint8_t acknowledgment[CSL_FRAMER_COMPLIANT_MAX_ACKNOWLEDGMENT_LEN];
  uint8_t received_checksum[CRC16_FRAMER_CHECKSUM_LEN];
  frame802154_t f;
  int len;
  struct ieee802154_ies ies;
  int ies_len;

  /* parse and validate frame length */
  uint_fast16_t frame_length = NETSTACK_RADIO.async_read_phy_header();
  if(frame_length > CSL_FRAMER_COMPLIANT_MAX_ACKNOWLEDGMENT_LEN) {
    LOG_ERR("acknowledgment frame has invalid length\n");
    return FRAMER_FAILED;
  }

  /* parse and validate frame header */
  if(NETSTACK_RADIO.async_read_payload(acknowledgment,
                                       frame_length
                                       - EXPECTED_ACKNOWLEDGMENT_MIC_LEN
                                       - CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  len = frame802154_parse(acknowledgment,
                          frame_length - CRC16_FRAMER_CHECKSUM_LEN,
                          &f);
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
     || (f.fcf.security_enabled
         && (CSL_FRAMER_COMPLIANT_acknowledgment_SEC_LVL
             != f.aux_hdr.security_control.security_level))) {
    LOG_ERR("unexpected security level\n");
    return FRAMER_FAILED;
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* parse and validate IE list */
  if((frame_length - len
      - EXPECTED_ACKNOWLEDGMENT_MIC_LEN
      - CRC16_FRAMER_CHECKSUM_LEN)
     < CSL_FRAMER_COMPLIANT_CSL_IE_LEN) {
    LOG_ERR("not enough bytes left\n");
    return FRAMER_FAILED;
  }
  ies_len = frame802154e_parse_information_elements(
                acknowledgment + len,
                frame_length
                - len
                - EXPECTED_ACKNOWLEDGMENT_MIC_LEN
                - CRC16_FRAMER_CHECKSUM_LEN,
                &ies);
  if(ies_len == FRAMER_FAILED) {
    LOG_ERR("failed to read CSL IE %" PRIuFAST16 "\n",
            frame_length
            - len
            - EXPECTED_ACKNOWLEDGMENT_MIC_LEN
            - CRC16_FRAMER_CHECKSUM_LEN);
    return FRAMER_FAILED;
  }
  if(!csl_state.transmit.burst_index) {
    csl_state.transmit.acknowledgment_phase = ies.csl_phase << CSL_PHASE_SHIFT;
  }

  if((frame_length - len - ies_len - EXPECTED_ACKNOWLEDGMENT_MIC_LEN)
     != CRC16_FRAMER_CHECKSUM_LEN) {
    LOG_ERR("acknowledgment has payload\n");
    return FRAMER_FAILED;
  }

#if LLSEC802154_USES_FRAME_COUNTER
  /* check MIC */
  if(csl_state.transmit.expected_mic_len) {
    uint8_t nonce[CCM_STAR_NONCE_LENGTH];
    csl_ccm_inputs_generate_acknowledgment_nonce(nonce,
                                                 &csl_state.transmit.receiver,
                                                 &f.aux_hdr.frame_counter);
    if(!CCM_STAR.get_lock()) {
      LOG_ERR("CCM* was locked\n");
      return FRAMER_FAILED;
    }
    uint8_t expected_mic[CSL_FRAMER_COMPLIANT_ACKNOWLEDGMENT_MIC_LEN];
    if(!CCM_STAR.set_key(csl_state.transmit.acknowledgment_key)
       || !CCM_STAR.aead(nonce,
                         NULL, 0,
                         acknowledgment, len + ies_len,
                         expected_mic, sizeof(expected_mic),
                         false)) {
      CCM_STAR.release_lock();
      LOG_ERR("CCM* failed\n");
      return FRAMER_FAILED;
    }
    CCM_STAR.release_lock();
    if(NETSTACK_RADIO.async_read_payload(acknowledgment + len + ies_len,
                                         sizeof(expected_mic))) {
      LOG_ERR("could not read at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    if(memcmp(expected_mic,
              acknowledgment + len + ies_len,
              sizeof(expected_mic))) {
      LOG_ERR("inauthentic MIC\n");
      return FRAMER_FAILED;
    }
    if(f.aux_hdr.frame_counter.u32
       <= csl_state.transmit.last_unicast_counter) {
      LOG_ERR("replayed acknowledgment\n");
      return FRAMER_FAILED;
    }
    csl_state.transmit.last_unicast_counter = f.aux_hdr.frame_counter.u32;
  }
#endif /* LLSEC802154_USES_FRAME_COUNTER */

  /* check checksum */
  crc16_framer_append_checksum(acknowledgment,
                               frame_length - CRC16_FRAMER_CHECKSUM_LEN);
  if(NETSTACK_RADIO.async_read_payload(received_checksum,
                                       CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("could not read at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }
  if(memcmp(acknowledgment + frame_length - CRC16_FRAMER_CHECKSUM_LEN,
            received_checksum,
            CRC16_FRAMER_CHECKSUM_LEN)) {
    LOG_ERR("acknowledgment frame has invalid checksum\n");
    return FRAMER_FAILED;
  }

  return frame_length;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
#if LLSEC802154_USES_FRAME_COUNTER
  if(akes_mac_is_helloack()) {
    return;
  }
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_WARN("receiver is no longer permanent\n");
    return;
  }

  if(entry->permanent->anti_replay_info.last_unicast_counter
     != csl_state.transmit.last_unicast_counter) {
    entry->permanent->anti_replay_info.last_unicast_counter =
        csl_state.transmit.last_unicast_counter;
    AKES_DELETE_STRATEGY.prolong_permanent_neighbor(entry->permanent);
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
  prepare_acknowledgment_parsing,
  parse_acknowledgment,
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
