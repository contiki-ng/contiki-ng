/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden
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
 *         CBOR-over-SLIP protocol implementation for the native border
 *         router talking to the serialradio firmware.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#include "contiki.h"
#include "border-router.h"

#if BORDER_ROUTER_SERIAL_RADIO

#include "border-router-cbor.h"
#include "lib/cbor.h"
#include "lib/crc16.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include <string.h>

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "BR-CBOR"
#define LOG_LEVEL LOG_LEVEL_NONE

/* TX status report from border-router-mac.c */
void packet_sent(uint8_t sessionid, uint8_t status, uint8_t tx);

/* Largest CBOR message we build: a full 802.15.4 frame plus map overhead and
 * the trailing CRC16. */
#define BR_CBOR_BUF_SIZE 280

/*---------------------------------------------------------------------------*/
/* Append a CRC16 (little-endian) to an encoded CBOR message and write the
 * result to SLIP.  The buffer must have room for the two trailing CRC bytes. */
static void
cbor_frame_send(uint8_t *buf, size_t len)
{
  uint16_t crc;

  if(len == 0 || len + 2 > BR_CBOR_BUF_SIZE) {
    LOG_WARN("dropping oversized CBOR frame (%u bytes)\n", (unsigned)len);
    return;
  }

  crc = crc16_data(buf, len, 0);
  buf[len] = crc & 0xff;
  buf[len + 1] = (crc >> 8) & 0xff;

  write_to_slip(buf, (int)(len + 2));
}
/*---------------------------------------------------------------------------*/
void
br_cbor_send_tx_frame(uint8_t msg_id, const uint8_t *frame, uint16_t len)
{
  static uint8_t buf[BR_CBOR_BUF_SIZE];
  cbor_writer_state_t writer;

  cbor_init_writer(&writer, buf, BR_CBOR_BUF_SIZE - 2);
  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_CMD_TX_RAW_FRAME);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_write_text(&writer, "f", 1);
  cbor_write_data(&writer, frame, len);
  cbor_close_map(&writer);

  cbor_frame_send(buf, cbor_end_writer(&writer));
}
/*---------------------------------------------------------------------------*/
void
br_cbor_send_set_param(uint8_t msg_id, uint16_t param, int32_t value)
{
  static uint8_t buf[BR_CBOR_BUF_SIZE];
  cbor_writer_state_t writer;

  cbor_init_writer(&writer, buf, BR_CBOR_BUF_SIZE - 2);
  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_CMD_SET_PARAM);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_write_text(&writer, "p", 1);
  cbor_write_unsigned(&writer, param);
  cbor_write_text(&writer, "v", 1);
  cbor_write_signed(&writer, value);
  cbor_close_map(&writer);

  cbor_frame_send(buf, cbor_end_writer(&writer));
}
/*---------------------------------------------------------------------------*/
void
br_cbor_send_get_addr64(uint8_t msg_id)
{
  static uint8_t buf[BR_CBOR_BUF_SIZE];
  cbor_writer_state_t writer;

  cbor_init_writer(&writer, buf, BR_CBOR_BUF_SIZE - 2);
  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_CMD_GET_ADDR64);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_close_map(&writer);

  cbor_frame_send(buf, cbor_end_writer(&writer));
}
/*---------------------------------------------------------------------------*/
void
br_cbor_send_router_mode(uint8_t msg_id, bool enable)
{
  static uint8_t buf[BR_CBOR_BUF_SIZE];
  cbor_writer_state_t writer;

  cbor_init_writer(&writer, buf, BR_CBOR_BUF_SIZE - 2);
  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_CMD_ROUTER_MODE);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_write_text(&writer, "v", 1);
  cbor_write_unsigned(&writer, enable ? 1 : 0);
  cbor_close_map(&writer);

  cbor_frame_send(buf, cbor_end_writer(&writer));
}
/*---------------------------------------------------------------------------*/
/* os/lib/cbor has no skip primitive; provide a minimal recursive one so that
 * unknown map keys can be stepped over. */
static bool
cbor_skip_value(cbor_reader_state_t *reader)
{
  cbor_major_type_t type = cbor_peek_next(reader);
  uint64_t uval;
  int64_t ival;
  size_t len;

  switch(type) {
  case CBOR_MAJOR_TYPE_UNSIGNED:
    return cbor_read_unsigned(reader, &uval) != CBOR_SIZE_NONE;
  case CBOR_MAJOR_TYPE_SIGNED:
    return cbor_read_signed(reader, &ival) != CBOR_SIZE_NONE;
  case CBOR_MAJOR_TYPE_BYTE_STRING:
    return cbor_read_data(reader, &len) != NULL;
  case CBOR_MAJOR_TYPE_TEXT_STRING:
    return cbor_read_text(reader, &len) != NULL;
  case CBOR_MAJOR_TYPE_ARRAY:
    len = cbor_read_array(reader);
    if(len == SIZE_MAX) {
      return false;
    }
    for(size_t i = 0; i < len; i++) {
      if(!cbor_skip_value(reader)) {
        return false;
      }
    }
    return true;
  case CBOR_MAJOR_TYPE_MAP:
    len = cbor_read_map(reader);
    if(len == SIZE_MAX) {
      return false;
    }
    for(size_t i = 0; i < len; i++) {
      if(!cbor_skip_value(reader) || !cbor_skip_value(reader)) {
        return false;
      }
    }
    return true;
  case CBOR_MAJOR_TYPE_SIMPLE:
    return cbor_read_simple(reader) != CBOR_SIMPLE_VALUE_NONE;
  default:
    return false;
  }
}
/*---------------------------------------------------------------------------*/
void
border_router_cbor_input(const uint8_t *data, int len)
{
  cbor_reader_state_t reader;
  size_t num_pairs;
  uint16_t received_crc, computed_crc;
  uint8_t msg_type = 0;
  uint8_t msg_id = 0;
  uint16_t param = 0;
  int32_t value = 0;
  int16_t rssi = 0;
  uint8_t lqi = 0;
  uint8_t error_code = 0;
  const uint8_t *frame = NULL;
  size_t frame_len = 0;

  /* Verify the trailing CRC16 (little-endian, computed over the CBOR). */
  if(len < 3) {
    return;
  }
  received_crc = data[len - 2] | (data[len - 1] << 8);
  computed_crc = crc16_data(data, len - 2, 0);
  if(received_crc != computed_crc) {
    LOG_WARN("CRC mismatch: got 0x%04x expected 0x%04x\n",
             received_crc, computed_crc);
    return;
  }

  cbor_init_reader(&reader, data, len - 2);
  num_pairs = cbor_read_map(&reader);
  if(num_pairs == SIZE_MAX) {
    LOG_WARN("not a CBOR map\n");
    return;
  }

  for(size_t i = 0; i < num_pairs; i++) {
    const char *key;
    size_t key_len;
    uint64_t u;
    int64_t s;

    key = cbor_read_text(&reader, &key_len);
    if(key == NULL || key_len != 1) {
      if(key == NULL || !cbor_skip_value(&reader)) {
        return;
      }
      continue;
    }

    switch(key[0]) {
    case 't':
      if(cbor_read_unsigned(&reader, &u) != CBOR_SIZE_NONE) {
        msg_type = (uint8_t)u;
      }
      break;
    case 'i':
      if(cbor_read_unsigned(&reader, &u) != CBOR_SIZE_NONE) {
        msg_id = (uint8_t)u;
      }
      break;
    case 'p':
      if(cbor_read_unsigned(&reader, &u) != CBOR_SIZE_NONE) {
        param = (uint16_t)u;
      }
      break;
    case 'v':
      if(cbor_read_signed(&reader, &s) != CBOR_SIZE_NONE) {
        value = (int32_t)s;
      }
      break;
    case 'r':
      if(cbor_read_signed(&reader, &s) != CBOR_SIZE_NONE) {
        rssi = (int16_t)s;
      }
      break;
    case 'l':
      if(cbor_read_unsigned(&reader, &u) != CBOR_SIZE_NONE) {
        lqi = (uint8_t)u;
      }
      break;
    case 'x':
      if(cbor_read_unsigned(&reader, &u) != CBOR_SIZE_NONE) {
        error_code = (uint8_t)u;
      }
      break;
    case 'f':
      frame = cbor_read_data(&reader, &frame_len);
      if(frame == NULL) {
        return;
      }
      break;
    default:
      if(!cbor_skip_value(&reader)) {
        return;
      }
      break;
    }
  }

  switch(msg_type) {
  case SRADIO_EVT_RX_FRAME:
    if(frame != NULL && frame_len > 0) {
      if(frame_len > PACKETBUF_SIZE) {
        /* Would be silently truncated by packetbuf_copyfrom(); drop instead of
           injecting a mangled frame into the network stack. */
        LOG_WARN("RX frame too large (%u > %u), dropping\n",
                 (unsigned)frame_len, (unsigned)PACKETBUF_SIZE);
        break;
      }
      packetbuf_copyfrom(frame, frame_len);
      packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rssi);
      packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, lqi);
      NETSTACK_MAC.input();
    }
    break;

  case SRADIO_EVT_TX_RESPONSE:
    packet_sent(msg_id, (uint8_t)value, 1);
    break;

  case SRADIO_EVT_ADDR64_RESPONSE:
    if(frame != NULL && frame_len == 8) {
      LOG_INFO("Got radio EUI-64\n");
      border_router_set_mac(frame);
    } else {
      LOG_WARN("ADDR64 response with bad length %u\n", (unsigned)frame_len);
    }
    break;

  case SRADIO_EVT_PARAM_RESPONSE:
    LOG_DBG("Param %u = %ld\n", param, (long)value);
    break;

  case SRADIO_EVT_HEARTBEAT:
    LOG_DBG("Radio heartbeat\n");
    break;

  case SRADIO_EVT_PONG:
    LOG_DBG("Radio pong\n");
    break;

  case SRADIO_EVT_ERROR:
    LOG_WARN("Radio reported error %u\n", error_code);
    break;

  default:
    LOG_DBG("Unhandled radio message type %u\n", msg_type);
    break;
  }
}
/*---------------------------------------------------------------------------*/

#endif /* BORDER_ROUTER_SERIAL_RADIO */
