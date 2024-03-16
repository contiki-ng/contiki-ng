/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 *         Creates and parses the ContikiMAC header.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/contikimac/contikimac-framer-original.h"
#include "lib/aes-128.h"
#include "lib/ccm-star.h"
#include "net/mac/contikimac/contikimac.h"
#include "net/mac/framer/frame802154e-ie.h"
#include "net/packetbuf.h"
#include "services/akes/akes-delete.h"
#include "services/akes/akes-mac.h"
#include <string.h>

#if AKES_MAC_ENABLED
#define LLSEC_OVERHEAD AKES_MAC_STRATEGY.get_overhead()
#else /* AKES_MAC_ENABLED */
#define LLSEC_OVERHEAD 0
#endif /* AKES_MAC_ENABLED */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "ContikiMAC-framer"
#define LOG_LEVEL LOG_LEVEL_MAC

#if !CONTIKIMAC_FRAMER_POTR_ENABLED
/*---------------------------------------------------------------------------*/
static int
filter(void)
{
  if(radio_read_payload_to_packetbuf(
         CONTIKIMAC_FRAMER_ORIGINAL_MIN_BYTES_FOR_FILTERING)) {
    LOG_ERR("radio_read_payload_to_packetbuf failed at line %i\n", __LINE__);
    return FRAMER_FAILED;
  }

  frame802154_t f;
  int len = frame802154_parse(packetbuf_dataptr(), packetbuf_datalen(), &f);
  if(!len) {
    LOG_ERR("frame802154_parse failed\n");
    return FRAMER_FAILED;
  }

  if(len > CONTIKIMAC_FRAMER_ORIGINAL_MIN_BYTES_FOR_FILTERING) {
    LOG_ERR("unexpected header length\n");
    return FRAMER_FAILED;
  }

  if(!packetbuf_hdrreduce(len)) {
    LOG_ERR("packetbuf_hdrreduce failed\n");
    return FRAMER_FAILED;
  }

  if(f.fcf.frame_version != FRAME802154_IEEE802154_2015) {
    LOG_ERR("old frame version\n");
    return FRAMER_FAILED;
  }

  if(!f.fcf.dest_addr_mode) {
    LOG_ERR("no destination address\n");
    return FRAMER_FAILED;
  }

  if(!f.fcf.src_addr_mode) {
    LOG_ERR("no source address\n");
    return FRAMER_FAILED;
  }

  if((f.dest_pid != IEEE802154_PANID)
     && (f.dest_pid != FRAME802154_BROADCASTPANDID)) {
    LOG_WARN("for another PAN %04x\n", f.dest_pid);
    return FRAMER_FAILED;
  }

  bool is_broadcast =
      frame802154_is_broadcast_addr(f.fcf.dest_addr_mode, f.dest_addr);
  if(!is_broadcast
     && !linkaddr_cmp((linkaddr_t *)f.dest_addr, &linkaddr_node_addr)) {
    LOG_WARN("for another node\n");
    return FRAMER_FAILED;
  }

  /* validate source address */
  if(linkaddr_cmp((linkaddr_t *)f.src_addr, &linkaddr_node_addr)) {
    LOG_ERR("frame from ourselves\n");
    return FRAMER_FAILED;
  }

  /* set packetbuf attributes */
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_TYPE, f.fcf.frame_type);
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, f.seq);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER,
                     is_broadcast ? &linkaddr_null : &linkaddr_node_addr);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, (linkaddr_t *)&f.src_addr);
#if AKES_MAC_ENABLED
  frame802154_frame_counter_t fc = {
    fc.u32 = 0
  };
  if(f.fcf.security_enabled) {
    packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL,
                       f.aux_hdr.security_control.security_level);
    if(!f.aux_hdr.security_control.frame_counter_suppression) {
      fc = f.aux_hdr.frame_counter;
    }
  }
#endif /* AKES_MAC_ENABLED */

  /* remove padding bytes */
  int ies_len;
  if(f.fcf.ie_list_present) {
    if(radio_read_payload_to_packetbuf(1)) {
      LOG_ERR("radio_read_payload_to_packetbuf failed at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    uint8_t padding_bytes = ((uint8_t *)packetbuf_dataptr())[0] & 0x7f;
    if(radio_read_payload_to_packetbuf(
           MAX(0,
               3
               + padding_bytes
               - (len - CONTIKIMAC_FRAMER_ORIGINAL_MIN_BYTES_FOR_FILTERING)))) {
      LOG_ERR("radio_read_payload_to_packetbuf failed at line %i\n", __LINE__);
      return FRAMER_FAILED;
    }
    struct ieee802154_ies ies;
    ies_len = frame802154e_parse_information_elements(packetbuf_dataptr(),
                                                      4 + padding_bytes,
                                                      &ies);
    if(ies_len == FRAMER_FAILED) {
      LOG_ERR("frame802154e_parse_information_elements failed\n");
      return FRAMER_FAILED;
    }
    if(!packetbuf_hdrreduce(ies_len)) {
      LOG_ERR("packetbuf_hdrreduce failed\n");
      return FRAMER_FAILED;
    }
  } else {
    ies_len = 0;
  }

  /* prepare acknowledgment frame */
  if(!packetbuf_holds_broadcast()) {
#if AKES_MAC_ENABLED && LLSEC802154_USES_FRAME_COUNTER
    if(f.fcf.frame_type == FRAME802154_DATAFRAME) {
      f.fcf.security_enabled = 1;
    } else {
      /* read Command ID field */
      uint_fast16_t temp =
          (len + ies_len) - NETSTACK_RADIO.async_read_payload_bytes();
      if(radio_read_payload_to_packetbuf(MAX(1, temp))) {
        LOG_ERR("radio_read_payload_to_packetbuf failed at line %i\n",
                __LINE__);
        return FRAMER_FAILED;
      }
      f.fcf.security_enabled = akes_mac_is_helloack() ? 0 : 1;
    }

    akes_nbr_t *nbr;
    if(f.fcf.security_enabled) {
      akes_nbr_entry_t *entry = akes_nbr_get_sender_entry();
      if(!entry) {
        LOG_ERR("entry is NULL\n");
        return FRAMER_FAILED;
      }
      nbr = akes_mac_is_ack() ? entry->tentative : entry->permanent;
      if(!nbr) {
        LOG_ERR("nbr is NULL\n");
        return FRAMER_FAILED;
      }
      anti_replay_set_counter(&nbr->anti_replay_info);
      f.aux_hdr.frame_counter.u16[0] =
          packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1);
      f.aux_hdr.frame_counter.u16[1] =
          packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3);
    } else {
      nbr = NULL;
    }
#else /* AKES_MAC_ENABLED && LLSEC802154_USES_FRAME_COUNTER */
    f.fcf.security_enabled = 0;
#endif /* AKES_MAC_ENABLED && LLSEC802154_USES_FRAME_COUNTER */
    f.fcf.sequence_number_suppression = 1;
    f.fcf.frame_type = FRAME802154_ACKFRAME;
    f.fcf.dest_addr_mode = f.fcf.src_addr_mode;
    f.fcf.src_addr_mode = FRAME802154_NOADDR;
    f.dest_pid = f.src_pid;
    memcpy(f.dest_addr, f.src_addr, 8);

    contikimac_state.duty_cycle.acknowledgment_len =
        frame802154_create(&f, contikimac_state.duty_cycle.acknowledgment);
#if AKES_MAC_ENABLED
    if(f.fcf.security_enabled) {
      uint8_t nonce[CCM_STAR_NONCE_LENGTH];
      AKES_MAC_STRATEGY.generate_nonce(nonce, 1);
#if AKES_NBR_WITH_PAIRWISE_KEYS
      if(!CCM_STAR.set_key(nbr->pairwise_key)
#else /* AKES_NBR_WITH_PAIRWISE_KEYS */
      if(!CCM_STAR.set_key(akes_mac_group_key)
#endif /* AKES_NBR_WITH_PAIRWISE_KEYS */
         || !CCM_STAR.aead(nonce,
                           NULL, 0,
                           contikimac_state.duty_cycle.acknowledgment,
                           contikimac_state.duty_cycle.acknowledgment_len,
                           contikimac_state.duty_cycle.acknowledgment
                           + contikimac_state.duty_cycle.acknowledgment_len,
                           AKES_MAC_UNICAST_MIC_LEN,
                           true)) {
        LOG_ERR("CCM* failed\n");
        return FRAMER_FAILED;
      }
      contikimac_state.duty_cycle.acknowledgment_len +=
          AKES_MAC_UNICAST_MIC_LEN;
    }
#endif /* AKES_MAC_ENABLED */
    crc16_framer_append_checksum(
        contikimac_state.duty_cycle.acknowledgment,
        contikimac_state.duty_cycle.acknowledgment_len);
    contikimac_state.duty_cycle.acknowledgment_len +=
        CRC16_FRAMER_CHECKSUM_LEN;
  }

#if AKES_MAC_ENABLED
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1, fc.u16[0]);
  packetbuf_set_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3, fc.u16[1]);
#endif /* AKES_MAC_ENABLED */

  return 0;
}
/*---------------------------------------------------------------------------*/
static void
prepare_outgoing_frame(frame802154_t *f)
{
  memset(f, 0, sizeof(frame802154_t));
  f->fcf.frame_type = packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE);
  f->fcf.panid_compression = 1;
  f->fcf.frame_version = FRAME802154_VERSION;
  f->fcf.src_addr_mode = LINKADDR_SIZE == 2
                         ? FRAME802154_SHORTADDRMODE
                         : FRAME802154_LONGADDRMODE;
#if AKES_MAC_ENABLED
  f->fcf.sequence_number_suppression = 1;
#else /* AKES_MAC_ENABLED */
  f->seq = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
#endif /* AKES_MAC_ENABLED */
  f->dest_pid = IEEE802154_PANID;
  linkaddr_copy((linkaddr_t *)f->src_addr, &linkaddr_node_addr);

  if(packetbuf_holds_broadcast()) {
    f->fcf.ack_required = 0;
    f->fcf.dest_addr_mode = FRAME802154_SHORTADDRMODE;
    memset(f->dest_addr, 0xff, 2);
  } else {
    f->fcf.ack_required = 1;
    f->fcf.dest_addr_mode = LINKADDR_SIZE == 2
                            ? FRAME802154_SHORTADDRMODE
                            : FRAME802154_LONGADDRMODE;
    linkaddr_copy((linkaddr_t *)f->dest_addr,
                  packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
    if(LINKADDR_SIZE == 8) {
      f->fcf.panid_compression = 0;
    }
  }

#if AKES_MAC_ENABLED
  if(packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL)) {
    f->fcf.security_enabled = 1;
    f->aux_hdr.security_control.security_level =
        packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL);
    f->aux_hdr.frame_counter.u16[0] =
        packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_0_1);
    f->aux_hdr.frame_counter.u16[1] =
        packetbuf_attr(PACKETBUF_ATTR_FRAME_COUNTER_BYTES_2_3);
  }
#endif /* AKES_MAC_ENABLED */
}
/*---------------------------------------------------------------------------*/
static int
length(void)
{
  frame802154_t f;
  prepare_outgoing_frame(&f);
  return frame802154_hdrlen(&f);
}
/*---------------------------------------------------------------------------*/
static int
create(void)
{
  frame802154_t f;
  prepare_outgoing_frame(&f);
  int len = frame802154_hdrlen(&f);

  /* add padding bytes if necessary */
  int ies_len;
  if((len + packetbuf_datalen() + LLSEC_OVERHEAD + CRC16_FRAMER_CHECKSUM_LEN)
     >= CONTIKIMAC_MIN_FRAME_LENGTH) {
    ies_len = 0;
  } else {
    f.fcf.ie_list_present = 1;
    uint_fast16_t prospective_len = len
                                    + 2 /* HT2 IE */
                                    + 2 /* padding IE */
                                    + packetbuf_datalen()
                                    + LLSEC_OVERHEAD
                                    + CRC16_FRAMER_CHECKSUM_LEN;
    struct ieee802154_ies ies = {
      ies.padding_bytes = prospective_len < CONTIKIMAC_MIN_FRAME_LENGTH
                          ? CONTIKIMAC_MIN_FRAME_LENGTH - prospective_len
                          : 0
    };

    /* HT2 IE */
    if(!packetbuf_hdralloc(2)) {
      LOG_ERR("packetbuf_hdralloc failed\n");
      return FRAMER_FAILED;
    }
    if(frame80215e_create_ie_header_list_termination_2(packetbuf_hdrptr(),
                                                       2,
                                                       &ies)
       == FRAMER_FAILED) {
      LOG_ERR("frame80215e_create_ie_header_list_termination_2 failed\n");
      return FRAMER_FAILED;
    }

    /* padding IE */
    if(!packetbuf_hdralloc(2 + ies.padding_bytes)) {
      LOG_ERR("packetbuf_hdralloc failed\n");
      return FRAMER_FAILED;
    }
    if(frame802154e_create_ie_padding(packetbuf_hdrptr(),
                                      2 + ies.padding_bytes,
                                      &ies)
       == FRAMER_FAILED) {
      LOG_ERR("frame802154e_create_ie_padding failed\n");
      return FRAMER_FAILED;
    }

    ies_len = 2 + ies.padding_bytes + 2;
  }

  if(!packetbuf_hdralloc(len)) {
    LOG_ERR("packetbuf_hdralloc failed\n");
    return FRAMER_FAILED;
  }
  frame802154_create(&f, packetbuf_hdrptr());

  return len + ies_len;
}
/*---------------------------------------------------------------------------*/
static int
parse(void)
{
  return packetbuf_hdrlen();
}
/*---------------------------------------------------------------------------*/
const struct framer contikimac_framer_original = {
  length,
  create,
  parse
};
/*---------------------------------------------------------------------------*/
static uint8_t
get_min_bytes_for_filtering(void)
{
  return CONTIKIMAC_FRAMER_ORIGINAL_MIN_BYTES_FOR_FILTERING;
}
/*---------------------------------------------------------------------------*/
static int
prepare_acknowledgment_parsing(void)
{
#if AKES_MAC_ENABLED
  if(contikimac_state.strobe.is_helloack) {
    return 1;
  }

  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    return 0;
  }

  packetbuf_set_addr(PACKETBUF_ADDR_SENDER,
                     packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  AKES_MAC_STRATEGY.generate_nonce(contikimac_state.strobe.nonce, 0);
  contikimac_state.strobe.last_unicast_counter =
      entry->permanent->anti_replay_info.last_unicast_counter;

#if AKES_NBR_WITH_PAIRWISE_KEYS
  akes_nbr_copy_key(contikimac_state.strobe.acknowledgment_key,
                    entry->permanent->pairwise_key);
#else /* AKES_NBR_WITH_PAIRWISE_KEYS */
  akes_nbr_copy_key(contikimac_state.strobe.acknowledgment_key,
                    entry->permanent->group_key);
#endif /* AKES_NBR_WITH_PAIRWISE_KEYS */
#endif /* AKES_MAC_ENABLED */
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
parse_acknowledgment(void)
{
#if AKES_MAC_ENABLED
  contikimac_state.strobe.acknowledgment_len -= CRC16_FRAMER_CHECKSUM_LEN;
  if(!crc16_framer_check_checksum(
         contikimac_state.strobe.acknowledgment,
         contikimac_state.strobe.acknowledgment_len)) {
    LOG_WARN("acknowledgment frame has invalid checksum\n");
    return 0;
  }

  if(contikimac_state.strobe.is_helloack) {
    return 1;
  }

  contikimac_state.strobe.acknowledgment_len -= AKES_MAC_UNICAST_MIC_LEN;
  frame802154_t f;
  if(contikimac_state.strobe.acknowledgment_len
     != frame802154_parse(contikimac_state.strobe.acknowledgment,
                          contikimac_state.strobe.acknowledgment_len,
                          &f)) {
    LOG_WARN("failed to parse acknowledgment frame\n");
    return 0;
  }

  if(f.aux_hdr.frame_counter.u32
     <= contikimac_state.strobe.last_unicast_counter) {
    LOG_ERR("replayed acknowledgment\n");
    return 0;
  }
  contikimac_state.strobe.last_unicast_counter = f.aux_hdr.frame_counter.u32;

  contikimac_state.strobe.nonce[8] = f.aux_hdr.frame_counter.u8[3];
  contikimac_state.strobe.nonce[9] = f.aux_hdr.frame_counter.u8[2];
  contikimac_state.strobe.nonce[10] = f.aux_hdr.frame_counter.u8[1];
  contikimac_state.strobe.nonce[11] = f.aux_hdr.frame_counter.u8[0];

  if(!CCM_STAR.get_lock()) {
    LOG_WARN("CCM* is locked\n");
    return 0;
  }
  uint8_t expected_mic[AKES_MAC_UNICAST_MIC_LEN];
  if(!CCM_STAR.set_key(contikimac_state.strobe.acknowledgment_key)
     || !CCM_STAR.aead(contikimac_state.strobe.nonce,
                       NULL,
                       0,
                       contikimac_state.strobe.acknowledgment,
                       contikimac_state.strobe.acknowledgment_len,
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
            + contikimac_state.strobe.acknowledgment_len,
            AKES_MAC_UNICAST_MIC_LEN)) {
    LOG_ERR("inauthentic acknowledgment frame\n");
    return 0;
  }
  return 1;
#else /* AKES_MAC_ENABLED */
  return crc16_framer_check_checksum(contikimac_state.strobe.acknowledgment,
                                     contikimac_state.strobe.acknowledgment_len
                                     - CRC16_FRAMER_CHECKSUM_LEN);
#endif /* AKES_MAC_ENABLED */
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
#if AKES_MAC_ENABLED
  if(contikimac_state.strobe.is_helloack) {
    return;
  }
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_WARN("receiver is no longer permanent\n");
    return;
  }

  if(entry->permanent->anti_replay_info.last_unicast_counter
     != contikimac_state.strobe.last_unicast_counter) {
    entry->permanent->anti_replay_info.last_unicast_counter =
        contikimac_state.strobe.last_unicast_counter;
    AKES_DELETE_STRATEGY.prolong_permanent_neighbor(entry->permanent);
  }
#endif /* AKES_MAC_ENABLED */
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
const struct contikimac_framer contikimac_framer_original_contikimac_framer = {
  get_min_bytes_for_filtering,
  filter,
  prepare_acknowledgment_parsing,
  parse_acknowledgment,
  on_unicast_transmitted,
  init,
};
/*---------------------------------------------------------------------------*/
#endif /* !CONTIKIMAC_FRAMER_POTR_ENABLED */
