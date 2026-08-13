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
 *         Serial Radio Control Interface - Main Implementation
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#include "serial-radio.h"
#include "contiki.h"
#include "dev/radio.h"
#include "dev/slip.h"
#include "lib/cbor.h"
#include "lib/crc16.h"
#include "net/ipv6/uip.h"
#include "net/linkaddr.h"
#include "net/netstack.h"
#include "net/nullnet/nullnet.h"
#include "net/packetbuf.h"
#include "sys/log.h"

/* Platform-specific UART headers */
#ifdef CONTIKI_TARGET_SIMPLELINK
#include "uart0-arch.h"
#endif

#include <stdio.h>
#include <string.h>

#define LOG_MODULE "SerialRadio"
#define LOG_LEVEL LOG_LEVEL_INFO

/*---------------------------------------------------------------------------*/
/* Configuration */
/*---------------------------------------------------------------------------*/

#define VERSION_STRING "serial-radio-1.1-sniff"

#define TX_BUF_SIZE SERIAL_RADIO_CONF_BUF_SIZE
#define RX_BUF_SIZE SERIAL_RADIO_CONF_BUF_SIZE

/*---------------------------------------------------------------------------*/
/* SLIP framing constants */
/*---------------------------------------------------------------------------*/

#define SLIP_END 0xC0
#define SLIP_ESC 0xDB
#define SLIP_ESC_END 0xDC
#define SLIP_ESC_ESC 0xDD

/*---------------------------------------------------------------------------*/
/* Static variables */
/*---------------------------------------------------------------------------*/

static uint8_t tx_buf[TX_BUF_SIZE];
static uint8_t current_msg_id;

/* Scanning state */
static bool scanning;
static uint8_t scan_start_ch;
static uint8_t scan_end_ch;
static uint8_t scan_current_ch;
static uint16_t scan_dwell_ms;
static struct etimer scan_timer;

/* Heartbeat state */
static struct etimer heartbeat_timer;
static uint32_t heartbeat_seq;

/* Fast scan state */
static bool fast_scanning;
static uint8_t fast_scan_start_ch;
static uint8_t fast_scan_end_ch;
static uint32_t fast_scan_seq;
static struct etimer fast_scan_timer;

/* Jamming state */
static bool jamming;
static uint8_t jam_channel;
static uint16_t jam_interval_ms;
static uint8_t jam_payload[127];
static uint8_t jam_payload_len;
static struct etimer jam_timer;
static radio_value_t saved_tx_mode;  /* Saved TX mode before jamming */

/* Sniffing state */
static bool sniffing;
static radio_value_t saved_rx_mode;  /* Saved RX mode before sniffing */

/* Border-router (router) mode state */
static bool router_mode;

/*---------------------------------------------------------------------------*/
/* Process declaration */
/*---------------------------------------------------------------------------*/

PROCESS(serial_radio_process, "Serial Radio");
AUTOSTART_PROCESSES(&serial_radio_process);

/*---------------------------------------------------------------------------*/
/* Forward declarations */
/*---------------------------------------------------------------------------*/

static void send_slip_frame(const uint8_t *data, size_t len);
static void send_error(uint8_t msg_id, uint8_t error_code);
static void send_pong(uint8_t msg_id);
static void send_param_response(uint8_t msg_id, uint16_t param, int32_t value);
static void send_tx_response(uint8_t msg_id, uint8_t status);
static void send_rssi_result(uint8_t channel, int8_t rssi);
static void send_rx_frame_event(const uint8_t *frame, uint16_t len,
                                int8_t rssi, uint8_t lqi);
static void handle_command(const uint8_t *data, size_t len);
static void slip_input_callback(void);
static void rx_packet_callback(const void *data, uint16_t len,
                               const linkaddr_t *src, const linkaddr_t *dest);
static void do_fast_scan_sweep(void);

/*---------------------------------------------------------------------------*/
/* Serial TX helpers */
/*---------------------------------------------------------------------------*/

/* Buffer for outgoing SLIP frames (data + CRC) */
static uint8_t slip_tx_buf[TX_BUF_SIZE + 2];

static void send_slip_frame(const uint8_t *data, size_t len) {
  /* Compute CRC16 over the data */
  uint16_t crc = crc16_data(data, len, 0);

  /* Copy data to slip_tx_buf and append CRC */
  if (len + 2 > sizeof(slip_tx_buf)) {
    return; /* Too large */
  }
  memcpy(slip_tx_buf, data, len);
  slip_tx_buf[len] = crc & 0xFF;
  slip_tx_buf[len + 1] = (crc >> 8) & 0xFF;

  /* Use Contiki-NG's slip_write which handles SLIP framing */
  slip_write(slip_tx_buf, len + 2);
}
/*---------------------------------------------------------------------------*/
/* Response builders */
/*---------------------------------------------------------------------------*/

static void send_error(uint8_t msg_id, uint8_t error_code) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_ERROR);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_write_text(&writer, "x", 1);
  cbor_write_unsigned(&writer, error_code);
  cbor_close_map(&writer);

  size_t len = cbor_end_writer(&writer);
  if (len > 0) {
    send_slip_frame(tx_buf, len);
  }
}
/*---------------------------------------------------------------------------*/
static void send_pong(uint8_t msg_id) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_PONG);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_write_text(&writer, "V", 1);
  cbor_write_text(&writer, VERSION_STRING, strlen(VERSION_STRING));
  cbor_close_map(&writer);

  size_t len = cbor_end_writer(&writer);
  if (len > 0) {
    send_slip_frame(tx_buf, len);
  }
}
/*---------------------------------------------------------------------------*/
static void send_param_response(uint8_t msg_id, uint16_t param, int32_t value) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_PARAM_RESPONSE);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_write_text(&writer, "p", 1);
  cbor_write_unsigned(&writer, param);
  cbor_write_text(&writer, "v", 1);
  cbor_write_signed(&writer, value);
  cbor_close_map(&writer);

  size_t len = cbor_end_writer(&writer);
  if (len > 0) {
    send_slip_frame(tx_buf, len);
  }
}
/*---------------------------------------------------------------------------*/
static void send_tx_response(uint8_t msg_id, uint8_t status) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_TX_RESPONSE);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_write_text(&writer, "v", 1);
  cbor_write_unsigned(&writer, status);
  cbor_close_map(&writer);

  size_t len = cbor_end_writer(&writer);
  if (len > 0) {
    send_slip_frame(tx_buf, len);
  }
}
/*---------------------------------------------------------------------------*/
static void send_rssi_result(uint8_t channel, int8_t rssi) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_RSSI_SCAN_RESULT);
  cbor_write_text(&writer, "c", 1);
  cbor_write_unsigned(&writer, channel);
  cbor_write_text(&writer, "r", 1);
  cbor_write_signed(&writer, rssi);
  cbor_close_map(&writer);

  size_t len = cbor_end_writer(&writer);
  if (len > 0) {
    send_slip_frame(tx_buf, len);
  }
}
/*---------------------------------------------------------------------------*/
static void send_heartbeat(void) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_HEARTBEAT);
  cbor_write_text(&writer, "s", 1);
  cbor_write_unsigned(&writer, heartbeat_seq++);
  cbor_write_text(&writer, "u", 1);
  cbor_write_unsigned(&writer, clock_seconds());
  cbor_close_map(&writer);

  size_t len = cbor_end_writer(&writer);
  if (len > 0) {
    send_slip_frame(tx_buf, len);
  }
}
/*---------------------------------------------------------------------------*/
static void send_fast_scan_result(uint8_t start_ch, uint8_t end_ch,
                                  int16_t *rssi_values) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);
  uint8_t num_channels = end_ch - start_ch + 1;

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_FAST_SCAN_RESULT);
  cbor_write_text(&writer, "n", 1);
  cbor_write_unsigned(&writer, fast_scan_seq++);
  cbor_write_text(&writer, "s", 1);
  cbor_write_unsigned(&writer, start_ch);
  cbor_write_text(&writer, "e", 1);
  cbor_write_unsigned(&writer, end_ch);
  cbor_write_text(&writer, "R", 1);
  cbor_open_array(&writer);
  for (uint8_t i = 0; i < num_channels; i++) {
    cbor_write_signed(&writer, rssi_values[i]);
  }
  cbor_close_array(&writer);
  cbor_close_map(&writer);

  size_t len = cbor_end_writer(&writer);
  if (len > 0) {
    send_slip_frame(tx_buf, len);
  }
}
/*---------------------------------------------------------------------------*/
static void send_rx_frame_event(const uint8_t *frame, uint16_t len,
                                int8_t rssi, uint8_t lqi) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_RX_FRAME);
  cbor_write_text(&writer, "f", 1);
  cbor_write_data(&writer, frame, len);
  cbor_write_text(&writer, "r", 1);
  cbor_write_signed(&writer, rssi);
  cbor_write_text(&writer, "l", 1);
  cbor_write_unsigned(&writer, lqi);
  cbor_close_map(&writer);

  size_t msg_len = cbor_end_writer(&writer);
  if (msg_len > 0) {
    send_slip_frame(tx_buf, msg_len);
  }
}
/*---------------------------------------------------------------------------*/
/* Report this node's link-layer (EUI-64) address to the host so a border
   router can adopt it.  Sent as raw bytes under the 'f' key since the
   8-byte address does not fit the integer-valued PARAM_RESPONSE. */
/* 802.15.4 link addresses are 8 bytes; the border router's GET_ADDR64 handler
   requires exactly 8 bytes and stalls otherwise.  Guard against a build with a
   shorter LINKADDR_SIZE. */
#if LINKADDR_SIZE != 8
#error "serialradio requires an 8-byte (EUI-64) link-layer address (LINKADDR_SIZE == 8)"
#endif
static void send_addr64_response(uint8_t msg_id) {
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, tx_buf, TX_BUF_SIZE);

  cbor_open_map(&writer);
  cbor_write_text(&writer, "t", 1);
  cbor_write_unsigned(&writer, SRADIO_EVT_ADDR64_RESPONSE);
  cbor_write_text(&writer, "i", 1);
  cbor_write_unsigned(&writer, msg_id);
  cbor_write_text(&writer, "f", 1);
  cbor_write_data(&writer, linkaddr_node_addr.u8, LINKADDR_SIZE);
  cbor_close_map(&writer);

  size_t len = cbor_end_writer(&writer);
  if (len > 0) {
    send_slip_frame(tx_buf, len);
  }
}
/*---------------------------------------------------------------------------*/
/* Nullnet RX callback - called when radio receives a packet */
/*---------------------------------------------------------------------------*/
static void rx_packet_callback(const void *data, uint16_t len,
                               const linkaddr_t *src, const linkaddr_t *dest) {
  /* Forward frames to the host both when sniffing (promiscuous) and when
     acting as a border-router radio (address-filtered). */
  if (!sniffing && !router_mode) {
    return;
  }

  /* Get RSSI and LQI from packetbuf attributes */
  int8_t rssi = (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI);
  uint8_t lqi = packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);

  LOG_DBG("RX frame: len=%u rssi=%d lqi=%u\n", len, rssi, lqi);

  /* Send RX frame event to host */
  send_rx_frame_event((const uint8_t *)data, len, rssi, lqi);
}
/*---------------------------------------------------------------------------*/
/* Command handlers */
/*---------------------------------------------------------------------------*/

static void handle_ping(uint8_t msg_id) {
  LOG_DBG("PING received\n");
  send_pong(msg_id);
}
/*---------------------------------------------------------------------------*/
static void handle_get_param(uint8_t msg_id, uint16_t param) {
  radio_value_t value = 0;
  radio_result_t result;

  result = NETSTACK_RADIO.get_value(param, &value);

  if (result == RADIO_RESULT_OK) {
    send_param_response(msg_id, param, value);
  } else {
    send_error(msg_id, ERR_INVALID_PARAM);
  }
}
/*---------------------------------------------------------------------------*/
static void handle_set_param(uint8_t msg_id, uint16_t param, int32_t value) {
  radio_result_t result;

  result = NETSTACK_RADIO.set_value(param, (radio_value_t)value);

  if (result == RADIO_RESULT_OK) {
    /* Echo back the set value as confirmation */
    send_param_response(msg_id, param, value);
  } else {
    send_error(msg_id, ERR_RADIO_ERROR);
  }
}
/*---------------------------------------------------------------------------*/
static void handle_tx_frame(uint8_t msg_id, const uint8_t *frame, size_t len,
                            int channel) {
  int result;
  radio_value_t original_channel = 0;

  /* Bound the attacker-controlled frame length before handing it to the radio
     driver, rather than relying on each driver to reject an over-length frame
     before copying it into its fixed-size TX FIFO. */
  if (len > SERIAL_RADIO_CONF_MAX_FRAME_SIZE) {
    send_error(msg_id, ERR_BUFFER_OVERFLOW);
    return;
  }

  /* If a specific channel is requested, save current and switch */
  if (channel >= 0) {
    NETSTACK_RADIO.get_value(RADIO_PARAM_CHANNEL, &original_channel);
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, channel);
  }

  /* Turn on radio if needed */
  NETSTACK_RADIO.on();

  /* Send the frame */
  result = NETSTACK_RADIO.send(frame, len);

  /* Restore original channel if we changed it */
  if (channel >= 0) {
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, original_channel);
  }

  /* Send response */
  send_tx_response(msg_id, result == RADIO_TX_OK ? 0 : 1);
}
/*---------------------------------------------------------------------------*/
static void handle_scan_start(uint8_t msg_id, uint8_t start_ch, uint8_t end_ch,
                              uint16_t dwell_ms) {
  if (scanning) {
    send_error(msg_id, ERR_SCAN_ACTIVE);
    return;
  }

  scan_start_ch = start_ch;
  scan_end_ch = end_ch;
  scan_current_ch = start_ch;
  scan_dwell_ms = dwell_ms;
  scanning = true;

  LOG_INFO("Starting scan: ch %u-%u, dwell %u ms\n", start_ch, end_ch,
           dwell_ms);

  /* Set to first channel */
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, scan_current_ch);

  /* Start timer via poll to ensure process context */
  process_poll(&serial_radio_process);
}
/*---------------------------------------------------------------------------*/
static void handle_scan_stop(uint8_t msg_id) {
  scanning = false;
  etimer_stop(&scan_timer);
  LOG_INFO("Scan stopped\n");
}
/*---------------------------------------------------------------------------*/
static void handle_rx_on(uint8_t msg_id) {
  /* Save current RX mode before switching to promiscuous */
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_RX_MODE, &saved_rx_mode) != RADIO_RESULT_OK) {
    saved_rx_mode = RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK;
  }

  /* Set promiscuous mode - disable address filtering and auto-ACK */
  if(NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, 0) != RADIO_RESULT_OK) {
    LOG_WARN("Could not set promiscuous mode\n");
  }

  sniffing = true;
  NETSTACK_RADIO.on();
  LOG_INFO("Sniffing enabled (promiscuous mode, saved RX mode: 0x%02x)\n",
           saved_rx_mode);
  send_param_response(msg_id, RADIO_PARAM_RX_MODE, 0);
}
/*---------------------------------------------------------------------------*/
static void handle_rx_off(uint8_t msg_id) {
  sniffing = false;

  /* Restore previous RX mode */
  if(NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, saved_rx_mode) != RADIO_RESULT_OK) {
    LOG_WARN("Could not restore RX mode\n");
  }

  LOG_INFO("Sniffing disabled (restored RX mode: 0x%02x)\n", saved_rx_mode);
  send_param_response(msg_id, RADIO_PARAM_RX_MODE, saved_rx_mode);
}
/*---------------------------------------------------------------------------*/
static void handle_get_addr64(uint8_t msg_id) {
  LOG_INFO("Reporting EUI-64 ");
  LOG_INFO_LLADDR(&linkaddr_node_addr);
  LOG_INFO_("\n");
  send_addr64_response(msg_id);
}
/*---------------------------------------------------------------------------*/
static void handle_router_mode(uint8_t msg_id, bool enable) {
  if(enable) {
    /* Border-router radio: enable hardware address filtering + auto-ACK so
       that unicast frames addressed to this node's EUI-64 are received and
       acknowledged, exactly like a normal 802.15.4 node radio. */
    radio_value_t mode = RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK;
    if(NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, mode) != RADIO_RESULT_OK) {
      LOG_WARN("Could not enable router RX mode\n");
    }
    router_mode = true;
    NETSTACK_RADIO.on();
    LOG_INFO("Router mode enabled (address filter + auto-ACK)\n");
    send_param_response(msg_id, RADIO_PARAM_RX_MODE, mode);
  } else {
    /* Actually clear the hardware RX mode (address filter + auto-ACK), not
       just the router_mode flag, so disabling router mode really returns the
       radio to the promiscuous sniffer default. */
    radio_value_t mode = 0;
    if(NETSTACK_RADIO.set_value(RADIO_PARAM_RX_MODE, mode) != RADIO_RESULT_OK) {
      LOG_WARN("Could not disable router RX mode\n");
    }
    router_mode = false;
    LOG_INFO("Router mode disabled\n");
    send_param_response(msg_id, RADIO_PARAM_RX_MODE, mode);
  }
}
/*---------------------------------------------------------------------------*/
static void handle_fast_scan_start(uint8_t msg_id, uint8_t start_ch,
                                   uint8_t end_ch) {
  if (fast_scanning) {
    send_error(msg_id, ERR_SCAN_ACTIVE);
    return;
  }

  /* Limit to max 32 channels to fit in buffer */
  if (end_ch < start_ch || (end_ch - start_ch + 1) > 32) {
    send_error(msg_id, ERR_INVALID_PARAM);
    return;
  }

  fast_scan_start_ch = start_ch;
  fast_scan_end_ch = end_ch;
  fast_scan_seq = 0;

  LOG_INFO("Starting fast scan: ch %u-%u\n", start_ch, end_ch);

  /* Mark as scanning */
  fast_scanning = true;

  /* Trigger start via poll to ensure process context */
  process_poll(&serial_radio_process);
}
/*---------------------------------------------------------------------------*/
static void handle_fast_scan_stop(uint8_t msg_id) {
  fast_scanning = false;
  etimer_stop(&fast_scan_timer);
  LOG_INFO("Fast scan stopped\n");
}
/*---------------------------------------------------------------------------*/
static void handle_jam_start(uint8_t msg_id, uint8_t channel,
                             uint16_t interval_ms, const uint8_t *payload,
                             size_t payload_len) {
  if (jamming) {
    send_error(msg_id, ERR_SCAN_ACTIVE);
    return;
  }

  jam_channel = channel;
  jam_interval_ms = interval_ms > 0 ? interval_ms : 1; /* Minimum 1ms */

  /* Use provided payload or default random-ish data */
  if (payload != NULL && payload_len > 0 && payload_len <= sizeof(jam_payload)) {
    memcpy(jam_payload, payload, payload_len);
    jam_payload_len = payload_len;
  } else {
    /* Default: 100 bytes of 0xAA pattern */
    jam_payload_len = 100;
    memset(jam_payload, 0xAA, jam_payload_len);
  }

  LOG_INFO("Starting jam: ch %u, interval %u ms, len %u\n", channel,
           jam_interval_ms, jam_payload_len);

  /* Set channel */
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, jam_channel);

  /* Save current TX mode and disable CCA for jamming */
  NETSTACK_RADIO.get_value(RADIO_PARAM_TX_MODE, &saved_tx_mode);
  NETSTACK_RADIO.set_value(RADIO_PARAM_TX_MODE, saved_tx_mode & ~RADIO_TX_MODE_SEND_ON_CCA);
  LOG_INFO("Disabled CCA for jamming (saved tx_mode: %d)\n", (int)saved_tx_mode);

  jamming = true;

  /* Trigger start via poll to ensure process context */
  process_poll(&serial_radio_process);
}
/*---------------------------------------------------------------------------*/
static void handle_jam_stop(uint8_t msg_id) {
  jamming = false;
  etimer_stop(&jam_timer);

  /* Restore TX mode (re-enable CCA) */
  NETSTACK_RADIO.set_value(RADIO_PARAM_TX_MODE, saved_tx_mode);
  LOG_INFO("Jamming stopped, restored tx_mode: %d\n", (int)saved_tx_mode);
}
/*---------------------------------------------------------------------------*/
static void do_jam_transmit(void) {
  /* Send jam packet without CCA (ignore channel busy) */
  NETSTACK_RADIO.on();

  /* Prepare packet in packetbuf for raw transmission */
  packetbuf_clear();
  packetbuf_copyfrom(jam_payload, jam_payload_len);

  /* Transmit - CCA is disabled in handle_jam_start() */
  NETSTACK_RADIO.prepare(packetbuf_hdrptr(), packetbuf_totlen());
  NETSTACK_RADIO.transmit(packetbuf_totlen());
}
/*---------------------------------------------------------------------------*/
static void do_fast_scan_sweep(void) {
  int16_t rssi_values[32];
  radio_value_t rssi = 0;
  uint8_t num_channels = fast_scan_end_ch - fast_scan_start_ch + 1;

  /* Sweep through all channels and collect RSSI */
  for (uint8_t i = 0; i < num_channels; i++) {
    uint8_t ch = fast_scan_start_ch + i;
    NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, ch);
    /* Turn radio RX on after channel change - required for RSSI measurement */
    NETSTACK_RADIO.on();
    /* Delay for radio to settle and RX to become active */
    clock_delay_usec(2000);
    NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rssi);
    rssi_values[i] = (int16_t)rssi;
  }

  /* Send all results in one message */
  send_fast_scan_result(fast_scan_start_ch, fast_scan_end_ch, rssi_values);
}
/*---------------------------------------------------------------------------*/
/* CBOR command parser */
/*---------------------------------------------------------------------------*/

/*
 * Maximum CBOR nesting depth cbor_skip_value() will descend into.  The
 * serialradio protocol only ever wraps scalars and strings in a single flat
 * map, so a small bound is ample; it caps stack usage when skipping an
 * unrecognized value from an untrusted frame, whose nesting depth would
 * otherwise scale with the frame length and overflow the MCU stack.
 */
#define CBOR_MAX_SKIP_DEPTH 8

/* Helper to skip a CBOR value - needed since os/lib/cbor doesn't have skip */
static bool cbor_skip_value(cbor_reader_state_t *reader, int depth) {
  cbor_major_type_t type = cbor_peek_next(reader);
  uint64_t uval;
  int64_t ival;
  size_t len;

  if (depth <= 0) {
    return false;
  }

  switch (type) {
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
    if (len == SIZE_MAX) {
      return false;
    }
    for (size_t i = 0; i < len; i++) {
      if (!cbor_skip_value(reader, depth - 1)) {
        return false;
      }
    }
    return true;
  case CBOR_MAJOR_TYPE_MAP:
    len = cbor_read_map(reader);
    if (len == SIZE_MAX) {
      return false;
    }
    for (size_t i = 0; i < len; i++) {
      if (!cbor_skip_value(reader, depth - 1) ||
          !cbor_skip_value(reader, depth - 1)) {
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

static void handle_command(const uint8_t *data, size_t len) {
  cbor_reader_state_t reader;
  size_t num_pairs;
  uint8_t msg_type = 0;
  uint8_t msg_id = 0;
  uint16_t param = 0;
  int32_t value = 0;
  int channel = -1;
  uint8_t start_ch = 11, end_ch = 26;
  uint16_t dwell_ms = 10;
  const uint8_t *frame_data = NULL;
  size_t frame_len = 0;
  bool has_param = false;
  bool has_value = false;
  bool has_frame = false;

  /* Verify CRC16 */
  if (len < 2) {
    send_error(0, ERR_CRC_FAIL);
    return;
  }

  uint16_t received_crc = data[len - 2] | (data[len - 1] << 8);
  uint16_t computed_crc = crc16_data(data, len - 2, 0);

  if (received_crc != computed_crc) {
    LOG_WARN("CRC mismatch: received 0x%04x, computed 0x%04x\n", received_crc,
             computed_crc);
    send_error(0, ERR_CRC_FAIL);
    return;
  }

  /* Parse CBOR (excluding CRC) */
  cbor_init_reader(&reader, data, len - 2);

  num_pairs = cbor_read_map(&reader);
  if (num_pairs == SIZE_MAX) {
    send_error(0, ERR_CBOR_DECODE);
    return;
  }

  /* Parse map entries */
  for (size_t i = 0; i < num_pairs; i++) {
    const char *key;
    size_t key_len;

    key = cbor_read_text(&reader, &key_len);
    if (key == NULL) {
      send_error(0, ERR_CBOR_DECODE);
      return;
    }

    if (key_len != 1) {
      cbor_skip_value(&reader, CBOR_MAX_SKIP_DEPTH);
      continue;
    }

    switch (key[0]) {
    case KEY_TYPE: {
      uint64_t v;
      if (cbor_read_unsigned(&reader, &v) != CBOR_SIZE_NONE) {
        msg_type = (uint8_t)v;
      }
      break;
    }
    case KEY_ID: {
      uint64_t v;
      if (cbor_read_unsigned(&reader, &v) != CBOR_SIZE_NONE) {
        msg_id = (uint8_t)v;
      }
      break;
    }
    case KEY_PARAM: {
      uint64_t v;
      if (cbor_read_unsigned(&reader, &v) != CBOR_SIZE_NONE) {
        param = (uint16_t)v;
        has_param = true;
      }
      break;
    }
    case KEY_VALUE: {
      int64_t v;
      if (cbor_read_signed(&reader, &v) != CBOR_SIZE_NONE) {
        value = (int32_t)v;
        has_value = true;
      }
      break;
    }
    case KEY_CHANNEL: {
      int64_t v;
      if (cbor_read_signed(&reader, &v) != CBOR_SIZE_NONE) {
        channel = (int)v;
      }
      break;
    }
    case KEY_FRAME: {
      frame_data = cbor_read_data(&reader, &frame_len);
      if (frame_data != NULL) {
        has_frame = true;
      }
      break;
    }
    case KEY_START_CH: {
      uint64_t v;
      if (cbor_read_unsigned(&reader, &v) != CBOR_SIZE_NONE) {
        start_ch = (uint8_t)v;
      }
      break;
    }
    case KEY_END_CH: {
      uint64_t v;
      if (cbor_read_unsigned(&reader, &v) != CBOR_SIZE_NONE) {
        end_ch = (uint8_t)v;
      }
      break;
    }
    case KEY_DWELL: {
      uint64_t v;
      if (cbor_read_unsigned(&reader, &v) != CBOR_SIZE_NONE) {
        dwell_ms = (uint16_t)v;
      }
      break;
    }
    default:
      cbor_skip_value(&reader, CBOR_MAX_SKIP_DEPTH);
      break;
    }
  }

  /* Dispatch command */
  switch (msg_type) {
  case SRADIO_CMD_PING:
    handle_ping(msg_id);
    break;

  case SRADIO_CMD_GET_PARAM:
    if (has_param) {
      handle_get_param(msg_id, param);
    } else {
      send_error(msg_id, ERR_INVALID_CMD);
    }
    break;

  case SRADIO_CMD_SET_PARAM:
    if (has_param && has_value) {
      handle_set_param(msg_id, param, value);
    } else {
      send_error(msg_id, ERR_INVALID_CMD);
    }
    break;

  case SRADIO_CMD_RSSI_SCAN_START:
    handle_scan_start(msg_id, start_ch, end_ch, dwell_ms);
    break;

  case SRADIO_CMD_RSSI_SCAN_STOP:
    handle_scan_stop(msg_id);
    break;

  case SRADIO_CMD_TX_RAW_FRAME:
    if (has_frame && frame_len > 0) {
      handle_tx_frame(msg_id, frame_data, frame_len, channel);
    } else {
      send_error(msg_id, ERR_INVALID_CMD);
    }
    break;

  case SRADIO_CMD_RX_ON:
    handle_rx_on(msg_id);
    break;

  case SRADIO_CMD_RX_OFF:
    handle_rx_off(msg_id);
    break;

  case SRADIO_CMD_FAST_SCAN_START:
    handle_fast_scan_start(msg_id, start_ch, end_ch);
    break;

  case SRADIO_CMD_FAST_SCAN_STOP:
    handle_fast_scan_stop(msg_id);
    break;

  case SRADIO_CMD_JAM_START:
    handle_jam_start(msg_id, channel >= 0 ? channel : 26, dwell_ms,
                     frame_data, frame_len);
    break;

  case SRADIO_CMD_JAM_STOP:
    handle_jam_stop(msg_id);
    break;

  case SRADIO_CMD_GET_ADDR64:
    handle_get_addr64(msg_id);
    break;

  case SRADIO_CMD_ROUTER_MODE:
    /* 'v' selects enable(1)/disable(0); default to enable when absent. */
    handle_router_mode(msg_id, has_value ? (value != 0) : true);
    break;

  default:
    LOG_WARN("Unknown command type: %u\n", msg_type);
    send_error(msg_id, ERR_INVALID_CMD);
    break;
  }
}
/*---------------------------------------------------------------------------*/
/* SLIP RX handling */
/*---------------------------------------------------------------------------*/

/*
 * This callback is called by slip_process after a complete SLIP frame
 * has been received and decoded. The data is in uip_buf with length uip_len.
 */
static void slip_input_callback(void) {
  LOG_DBG("SLIP RX: %u bytes\n", uip_len);
  if (uip_len > 0) {
    handle_command(uip_buf, uip_len);
  }
}
/*---------------------------------------------------------------------------*/
/* Scanning process */
/*---------------------------------------------------------------------------*/

static void do_scan_step(void) {
  radio_value_t rssi;

  /* Read RSSI on current channel */
  NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &rssi);
  send_rssi_result(scan_current_ch, (int8_t)rssi);

  /* Move to next channel */
  scan_current_ch++;

  if (scan_current_ch > scan_end_ch) {
    /* Scan complete - wrap around */
    scan_current_ch = scan_start_ch;
  }

  /* Set next channel */
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, scan_current_ch);

  /* Restart timer */
  etimer_reset(&scan_timer);
}
/*---------------------------------------------------------------------------*/
/* Process */
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(serial_radio_process, ev, data) {
  PROCESS_BEGIN();

  LOG_INFO("=================================\n");
  LOG_INFO("Serial Radio Control Interface\n");
  LOG_INFO("=================================\n");
  LOG_INFO("Platform: " CONTIKI_TARGET_STRING "\n");
  LOG_INFO("Version: %s\n", VERSION_STRING);

  /* Initialize variables */
  current_msg_id = 0;
  heartbeat_seq = 0;
  scanning = false;
  fast_scanning = false;
  fast_scan_seq = 0;
  jamming = false;
  sniffing = false;
  router_mode = false;

  /* Initialize SLIP - this sets up UART with slip_input_byte callback */
  slip_arch_init();
  process_start(&slip_process, NULL);
  slip_set_input_callback(slip_input_callback);

  /* Register nullnet callback for packet sniffing */
  nullnet_set_input_callback(rx_packet_callback);

  /* Turn on radio */
  NETSTACK_RADIO.on();

  /* Start heartbeat timer */
  etimer_set(&heartbeat_timer, SERIAL_RADIO_CONF_HEARTBEAT_INTERVAL);

  /* Send initial heartbeat */
  send_heartbeat();

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_POLL) {
      /* Handle scan start requests - set timers here in process context */
      if (scanning) {
        etimer_set(&scan_timer,
                   (clock_time_t)(scan_dwell_ms * CLOCK_SECOND / 1000));
      }
      if (fast_scanning) {
        do_fast_scan_sweep();
        etimer_set(&fast_scan_timer, CLOCK_SECOND / 10);
      }
      if (jamming) {
        do_jam_transmit();
        etimer_set(&jam_timer,
                   (clock_time_t)(jam_interval_ms * CLOCK_SECOND / 1000));
      }
    }

    if (ev == PROCESS_EVENT_TIMER && data == &scan_timer) {
      if (scanning) {
        do_scan_step();
        etimer_reset(&scan_timer);
      }
    }

    if (ev == PROCESS_EVENT_TIMER && data == &fast_scan_timer) {
      if (fast_scanning) {
        do_fast_scan_sweep();
        etimer_reset(&fast_scan_timer);
      }
    }

    if (ev == PROCESS_EVENT_TIMER && data == &jam_timer) {
      if (jamming) {
        do_jam_transmit();
        etimer_reset(&jam_timer);
      }
    }

    if (ev == PROCESS_EVENT_TIMER && data == &heartbeat_timer) {
      send_heartbeat();
      etimer_reset(&heartbeat_timer);
      LOG_INFO("HB: scan=%d fscan=%d jam=%d ch=%d\n",
               scanning, fast_scanning, jamming, jam_channel);
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/* Initialization */
/*---------------------------------------------------------------------------*/

void serial_radio_init(void) { process_start(&serial_radio_process, NULL); }
/*---------------------------------------------------------------------------*/
/* Public API */
/*---------------------------------------------------------------------------*/

int serial_radio_send_frame(const uint8_t *data, uint16_t len, int channel) {
  handle_tx_frame(0, data, len, channel);
  return 0;
}
/*---------------------------------------------------------------------------*/
int serial_radio_start_scan(uint8_t start_ch, uint8_t end_ch,
                            uint16_t dwell_ms) {
  if (scanning) {
    return ERR_SCAN_ACTIVE;
  }
  handle_scan_start(0, start_ch, end_ch, dwell_ms);
  return 0;
}
/*---------------------------------------------------------------------------*/
void serial_radio_stop_scan(void) {
  scanning = false;
  etimer_stop(&scan_timer);
}
/*---------------------------------------------------------------------------*/
bool serial_radio_is_scanning(void) { return scanning; }
/*---------------------------------------------------------------------------*/
