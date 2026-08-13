/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
/*---------------------------------------------------------------------------*/
/**
 * \file
 *      IEEE 802.15.4 radio driver for the nRF5340 network core using
 *      Nordic's nrf_802154 library. Wraps the asynchronous nrf_802154 API
 *      into Contiki-NG's synchronous radio_driver interface and exposes it
 *      as nrf_ieee_driver (replacing the raw nrf-ieee-driver-arch.c on this
 *      core). Adapted from the nRF54L15 port.
 *
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"
#include "net/mac/framer/frame802154.h"
#include "sys/energest.h"
#include "nrf_802154.h"
#include "nrf_802154_config.h"
#include "nrf.h"

#include <string.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#ifndef LOG_CONF_LEVEL_RADIO
#define LOG_CONF_LEVEL_RADIO LOG_LEVEL_INFO
#endif
#define LOG_MODULE "nrf53-radio"
#define LOG_LEVEL LOG_CONF_LEVEL_RADIO
/*---------------------------------------------------------------------------*/
#define MAX_PAYLOAD_LEN       125 /* 127 - FCS(2) */
#define FRAME_ACK_REQUEST_BIT 0x20

/*
 * Seed the radio's channel from the Contiki build configuration
 * (IEEE802154_CONF_DEFAULT_CHANNEL, default 26), mirroring how the PAN ID is
 * taken from IEEE802154_PANID below. The app core does not push the channel
 * over IPC, so the net core must default to the configured channel; otherwise
 * a non-default IEEE802154_CONF_DEFAULT_CHANNEL silently channel-mismatches
 * the net-core radio against the app core and its peers.
 */
#define DEFAULT_CHANNEL       IEEE802154_DEFAULT_CHANNEL
#define DEFAULT_TX_POWER      0 /* dBm */

/* Timeouts are expressed against nrf_802154_time_get() (microseconds), which
 * is driven by the SL lptimer platform backend (RTC1 on this core). */
#define TX_DONE_TIMEOUT_US    250000ULL
#define TX_ABORT_TIMEOUT_US   10000ULL
#define CCA_DONE_TIMEOUT_US   50000ULL
/*---------------------------------------------------------------------------*/
/* TX buffer in nrf_802154 raw format: [PHR (length incl. FCS)] [PSDU ...]. */
static uint8_t tx_buf[1 + MAX_PAYLOAD_LEN + 2];

/*
 * Software RX ring. The head/tail indices are free-running uint8_t values,
 * so the ring size must be a power of two for the index mapping to stay
 * consistent across their 256-wrap. It must also be at least as large as
 * the library's RX buffer pool: the library can never have more frames
 * outstanding than it has buffers, so enqueue can then never overwrite an
 * unread slot.
 */
#define RX_BUF_COUNT 32
#define RX_BUF_MASK  (RX_BUF_COUNT - 1)
#if (RX_BUF_COUNT & RX_BUF_MASK) != 0
#error RX_BUF_COUNT must be a power of two
#endif
#if RX_BUF_COUNT < NRF_802154_RX_BUFFERS
#error RX_BUF_COUNT must be >= NRF_802154_RX_BUFFERS
#endif
static uint8_t *rx_bufs[RX_BUF_COUNT];
static int8_t rx_rssi[RX_BUF_COUNT];
static uint8_t rx_lqi[RX_BUF_COUNT];
static volatile uint8_t rx_head;
static volatile uint8_t rx_tail;

static volatile bool tx_done;
static volatile bool tx_success;
static volatile nrf_802154_tx_error_t tx_error;

static volatile bool cca_done;
static volatile bool cca_free;

static volatile uint32_t rx_fail_count;
static volatile uint32_t rx_drop_count;
static volatile uint32_t tx_fail_count;

static uint8_t current_channel = DEFAULT_CHANNEL;
static int8_t current_tx_power = DEFAULT_TX_POWER;
static bool radio_is_on;

/* Reflects the hardware configuration applied in init(): frame filtering
 * enabled, auto-ACK enabled. Poll mode is not supported (RX is
 * interrupt-driven), so it is never present in rx_mode. */
static radio_value_t rx_mode =
  RADIO_RX_MODE_ADDRESS_FILTER | RADIO_RX_MODE_AUTOACK;
static radio_value_t tx_mode;
/*---------------------------------------------------------------------------*/
PROCESS(nrf53_radio_process, "nRF53 radio driver");
/*---------------------------------------------------------------------------*/
static const char *
tx_error_name(nrf_802154_tx_error_t error)
{
  switch(error) {
  case NRF_802154_TX_ERROR_NONE:
    return "none";
  case NRF_802154_TX_ERROR_BUSY_CHANNEL:
    return "busy_channel";
  case NRF_802154_TX_ERROR_INVALID_ACK:
    return "invalid_ack";
  case NRF_802154_TX_ERROR_NO_MEM:
    return "no_mem";
  case NRF_802154_TX_ERROR_TIMESLOT_ENDED:
    return "timeslot_ended";
  case NRF_802154_TX_ERROR_NO_ACK:
    return "no_ack";
  case NRF_802154_TX_ERROR_ABORTED:
    return "aborted";
  case NRF_802154_TX_ERROR_TIMESLOT_DENIED:
    return "timeslot_denied";
  default:
    return "unknown";
  }
}
/*---------------------------------------------------------------------------*/
static bool
wait_for_flag(volatile bool *flag, uint64_t timeout_us)
{
  uint64_t deadline = nrf_802154_time_get() + timeout_us;

  while(!*flag && nrf_802154_time_get() < deadline) {
    __WFE();
  }

  return *flag;
}
/*---------------------------------------------------------------------------*/
static void
recover_to_receive_state(void)
{
  if(!radio_is_on) {
    return;
  }

  /*
   * Request sleep to abort any operation still in progress. An aborted
   * transmission reports through nrf_802154_transmit_failed(), which sets
   * tx_done; waiting for it here keeps a late TX completion callback from
   * leaking into the next transmit()'s freshly cleared flags. When no TX is
   * pending (e.g. the CCA timeout path), tx_done is still set from the last
   * completed transmission and the wait returns immediately.
   */
  nrf_802154_sleep();
  (void)wait_for_flag(&tx_done, TX_ABORT_TIMEOUT_US);

  if(!nrf_802154_receive()) {
    LOG_WARN("Failed to restore receive state\n");
  }
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  uint8_t pan_id[2];
  uint8_t short_addr[2];
  uint8_t ext_addr[8];
  int i;

  LOG_INFO("Initializing nrf_802154 radio driver\n");

  nrf_802154_init();

  nrf_802154_channel_set(current_channel);
  nrf_802154_tx_power_set(current_tx_power);

  nrf_802154_auto_ack_set(true);
  nrf_802154_rx_on_when_idle_set(true);

  /*
   * Seed the PAN ID and addresses from this (net) core's configuration and
   * FICR-derived link-layer address as a fallback. The application core's
   * link-layer address differs (each core derives it from its own FICR
   * DEVICEID), so the app core pushes its authoritative addresses over IPC
   * right after radio init (see ipc_radio_init() in nrf-ipc-radio.c); those
   * arrive through set_object() below and override this seeding. Hardware
   * frame filtering and auto-ACK operate on whatever is programmed last.
   */
  pan_id[0] = (uint8_t)(IEEE802154_PANID & 0xFF);
  pan_id[1] = (uint8_t)(IEEE802154_PANID >> 8);
  nrf_802154_pan_id_set(pan_id);

  short_addr[0] = linkaddr_node_addr.u8[LINKADDR_SIZE - 2];
  short_addr[1] = linkaddr_node_addr.u8[LINKADDR_SIZE - 1];
  nrf_802154_short_address_set(short_addr);

  /* Extended address is the link-layer address in little-endian order. */
  for(i = 0; i < 8; i++) {
    ext_addr[i] = (i < LINKADDR_SIZE) ?
      linkaddr_node_addr.u8[LINKADDR_SIZE - 1 - i] : 0;
  }
  nrf_802154_extended_address_set(ext_addr);

  rx_head = 0;
  rx_tail = 0;
  radio_is_on = false;
  /* No transmission is in flight, so the TX completion state is settled;
   * recover_to_receive_state() relies on this. */
  tx_done = true;

  process_start(&nrf53_radio_process, NULL);

  LOG_INFO("Radio initialized, channel %u, PAN 0x%04x\n",
           current_channel, IEEE802154_PANID);

  /*
   * On the net core this driver is consumed only via the IPC radio service,
   * which (like the raw nrf-ieee-driver-arch.c) treats RADIO_TX_OK (0) as a
   * successful init. Match that convention.
   */
  return RADIO_TX_OK;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  if(!radio_is_on) {
    ENERGEST_ON(ENERGEST_TYPE_LISTEN);
    if(!nrf_802154_receive()) {
      ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
      LOG_WARN("Failed to enter receive state\n");
      return 0;
    }
    radio_is_on = true;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  if(radio_is_on) {
    nrf_802154_sleep();
    ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
    radio_is_on = false;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  if(payload_len > MAX_PAYLOAD_LEN) {
    LOG_WARN("TX payload too long: %u\n", payload_len);
    return 1;
  }

  tx_buf[0] = payload_len + 2; /* PHR = PSDU + FCS(2). */
  memcpy(&tx_buf[1], payload, payload_len);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  nrf_802154_transmit_metadata_t metadata = {
    .frame_props = NRF_802154_TRANSMITTED_FRAME_PROPS_DEFAULT_INIT,
    .cca = (tx_mode & RADIO_TX_MODE_SEND_ON_CCA) != 0,
    .tx_power = { .use_metadata_value = false },
    .tx_channel = { .use_metadata_value = false },
  };
  nrf_802154_tx_error_t tx_err;

  /* The frame to send was stored in tx_buf by prepare(); its length is the
   * PHR (tx_buf[0] = PSDU + FCS). Validate the caller's length against the
   * prepared PHR rather than trusting it: transmit() is a separate radio API
   * entry point, and callers such as CSMA invoke it without checking that the
   * preceding prepare() succeeded. Without this check, a skipped or failed
   * prepare() would make transmit() send the stale contents of tx_buf. */
  if(transmit_len > MAX_PAYLOAD_LEN || transmit_len + 2 != tx_buf[0]) {
    LOG_WARN("TX length mismatch: %u vs PHR %u\n", transmit_len, tx_buf[0]);
    return RADIO_TX_ERR;
  }

  if(!radio_is_on) {
    on();
  }

  tx_done = false;
  tx_success = false;
  tx_error = NRF_802154_TX_ERROR_NONE;

  ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);

  tx_err = nrf_802154_transmit_raw(tx_buf, &metadata);

  if(tx_err != NRF_802154_TX_ERROR_NONE) {
    LOG_WARN("TX request rejected: %s (%u)\n", tx_error_name(tx_err), tx_err);
    ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);

    if(tx_err == NRF_802154_TX_ERROR_BUSY_CHANNEL) {
      recover_to_receive_state();
      return RADIO_TX_COLLISION;
    }
    if(tx_err == NRF_802154_TX_ERROR_INVALID_ACK ||
       tx_err == NRF_802154_TX_ERROR_NO_ACK) {
      return RADIO_TX_NOACK;
    }
    return RADIO_TX_ERR;
  }

  if(!wait_for_flag(&tx_done, TX_DONE_TIMEOUT_US)) {
    LOG_WARN("TX timeout\n");
    ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);
    /* Abort the stuck transmission and let the completion flags settle
     * before returning, so a late TX callback cannot corrupt the result of
     * the next transmit(). */
    recover_to_receive_state();
    return RADIO_TX_ERR;
  }

  ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);

  if(tx_success) {
    return RADIO_TX_OK;
  }
  if(tx_error == NRF_802154_TX_ERROR_NO_ACK ||
     tx_error == NRF_802154_TX_ERROR_INVALID_ACK) {
    return RADIO_TX_NOACK;
  }
  if(tx_error == NRF_802154_TX_ERROR_BUSY_CHANNEL) {
    return RADIO_TX_COLLISION;
  }

  LOG_WARN("TX failed: %s (%u)\n", tx_error_name(tx_error), tx_error);
  return RADIO_TX_ERR;
}
/*---------------------------------------------------------------------------*/
static int
send(const void *payload, unsigned short payload_len)
{
  if(prepare(payload, payload_len) != 0) {
    /* Transmitting anyway would send the stale contents of tx_buf. */
    return RADIO_TX_ERR;
  }
  return transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short buf_len)
{
  uint8_t idx;
  uint8_t *p_data;
  uint8_t payload_len;

  if(rx_tail == rx_head) {
    return 0;
  }

  idx = rx_tail & RX_BUF_MASK;
  p_data = rx_bufs[idx];

  if(p_data == NULL) {
    rx_tail++;
    return 0;
  }

  if(p_data[0] < 2) {
    /* Malformed PHR; cannot happen for CRC-checked frames, but guard the
     * unsigned underflow below. */
    nrf_802154_buffer_free_raw(p_data);
    rx_bufs[idx] = NULL;
    rx_tail++;
    return 0;
  }

  /* p_data[0] = PHR (length incl. FCS); payload = PHR - FCS(2). */
  payload_len = p_data[0] - 2;
  if(payload_len > buf_len) {
    payload_len = buf_len;
  }

  memcpy(buf, &p_data[1], payload_len);

  packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rx_rssi[idx]);
  packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, rx_lqi[idx]);

  nrf_802154_buffer_free_raw(p_data);
  rx_bufs[idx] = NULL;
  rx_tail++;

  return payload_len;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  if(!radio_is_on) {
    on();
  }

  cca_done = false;
  cca_free = false;

  if(!nrf_802154_cca()) {
    LOG_WARN("CCA could not start\n");
    return 0;
  }

  if(!wait_for_flag(&cca_done, CCA_DONE_TIMEOUT_US)) {
    LOG_WARN("CCA timeout\n");
    recover_to_receive_state();
    return 0;
  }

  return cca_free ? 1 : 0;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  return (rx_head != rx_tail) ? 1 : 0;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    *value = radio_is_on ? RADIO_POWER_MODE_ON : RADIO_POWER_MODE_OFF;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    *value = (radio_value_t)nrf_802154_channel_get();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RX_MODE:
    *value = rx_mode;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    *value = tx_mode;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    *value = (radio_value_t)nrf_802154_tx_power_get();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RSSI:
    *value = (radio_value_t)nrf_802154_rssi_last_get();
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MIN:
    *value = 11;
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MAX:
    *value = 26;
    return RADIO_RESULT_OK;
  case RADIO_CONST_TXPOWER_MIN:
    /* nRF5340 radio output power range is -40..+3 dBm. */
    *value = -40;
    return RADIO_RESULT_OK;
  case RADIO_CONST_TXPOWER_MAX:
    *value = 3;
    return RADIO_RESULT_OK;
  case RADIO_CONST_MAX_PAYLOAD_LEN:
    *value = MAX_PAYLOAD_LEN;
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    if(value == RADIO_POWER_MODE_ON) {
      on();
    } else {
      off();
    }
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    if(value < 11 || value > 26) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    current_channel = (uint8_t)value;
    nrf_802154_channel_set(current_channel);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    if(value < -40 || value > 3) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    current_tx_power = (int8_t)value;
    nrf_802154_tx_power_set(current_tx_power);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RX_MODE:
    if((value & ~(radio_value_t)(RADIO_RX_MODE_ADDRESS_FILTER |
                                 RADIO_RX_MODE_AUTOACK)) != 0) {
      /* Poll mode is not supported: reception is interrupt-driven and
       * frames are forwarded to the app core by the IPC MAC. */
      return RADIO_RESULT_NOT_SUPPORTED;
    }
    nrf_802154_promiscuous_set((value & RADIO_RX_MODE_ADDRESS_FILTER) == 0);
    nrf_802154_auto_ack_set((value & RADIO_RX_MODE_AUTOACK) != 0);
    rx_mode = value;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    if((value & ~(radio_value_t)RADIO_TX_MODE_SEND_ON_CCA) != 0) {
      return RADIO_RESULT_NOT_SUPPORTED;
    }
    tx_mode = value;
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  (void)param;
  (void)dest;
  (void)size;
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  uint8_t ext_addr[8];
  const uint8_t *addr;
  int i;

  switch(param) {
  case RADIO_PARAM_64BIT_ADDR:
    if(size != 8) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    /* nrf_802154 wants the extended address in little-endian order. */
    addr = (const uint8_t *)src;
    for(i = 0; i < 8; i++) {
      ext_addr[i] = addr[7 - i];
    }
    nrf_802154_extended_address_set(ext_addr);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_PAN_ID:
    if(size != 2) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    nrf_802154_pan_id_set((const uint8_t *)src);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_16BIT_ADDR:
    if(size != 2) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    nrf_802154_short_address_set((const uint8_t *)src);
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
/* nrf_802154 callbacks (invoked from ISR context). */
void
nrf_802154_received_timestamp_raw(uint8_t *p_data, int8_t power,
                                  uint8_t lqi, uint64_t time)
{
  uint8_t idx;

  (void)time;

  if(((uint8_t)(rx_head - rx_tail)) >= RX_BUF_COUNT) {
    /*
     * Ring full -- drop the incoming frame. Only the consumer
     * (radio_read(), process context) may advance rx_tail or free queued
     * buffers; freeing the oldest entry from this ISR would race a read in
     * progress. Unreachable as long as RX_BUF_COUNT >=
     * NRF_802154_RX_BUFFERS, since the library cannot have more frames
     * outstanding than its buffer pool.
     */
    nrf_802154_buffer_free_raw(p_data);
    rx_drop_count++;
    process_poll(&nrf53_radio_process);
    return;
  }

  idx = rx_head & RX_BUF_MASK;
  rx_bufs[idx] = p_data;
  rx_rssi[idx] = power;
  rx_lqi[idx] = lqi;
  rx_head++;

  process_poll(&nrf53_radio_process);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_receive_failed(nrf_802154_rx_error_t error, uint32_t id)
{
  (void)error;
  (void)id;
  rx_fail_count++;
  process_poll(&nrf53_radio_process);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_transmitted_raw(uint8_t *p_frame,
                           const nrf_802154_transmit_done_metadata_t *p_metadata)
{
  bool ack_requested = false;

  if(p_frame != NULL) {
    ack_requested = (p_frame[1] & FRAME_ACK_REQUEST_BIT) != 0;
  }

  if(p_metadata->data.transmitted.p_ack != NULL) {
    nrf_802154_buffer_free_raw(p_metadata->data.transmitted.p_ack);
    tx_success = true;
  } else if(ack_requested) {
    tx_error = NRF_802154_TX_ERROR_NO_ACK;
    tx_success = false;
  } else {
    tx_success = true;
  }

  tx_done = true;
  __SEV();
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_transmit_failed(uint8_t *p_frame,
                           nrf_802154_tx_error_t error,
                           const nrf_802154_transmit_done_metadata_t *p_metadata)
{
  (void)p_frame;
  (void)p_metadata;
  /*
   * NO_ACK and BUSY_CHANNEL are expected link conditions, not driver faults:
   * transmit() returns them as RADIO_TX_NOACK / RADIO_TX_COLLISION and the
   * MAC layer handles retransmission. Only count the genuinely abnormal
   * errors here so the aggregate "TX failed" warning stays meaningful.
   */
  if(error != NRF_802154_TX_ERROR_NO_ACK &&
     error != NRF_802154_TX_ERROR_BUSY_CHANNEL) {
    tx_fail_count++;
  }
  tx_error = error;
  tx_success = false;
  tx_done = true;
  __SEV();
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_cca_done(bool channel_free)
{
  cca_free = channel_free;
  cca_done = true;
  __SEV();
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_cca_failed(nrf_802154_cca_error_t error)
{
  (void)error;
  cca_free = false;
  cca_done = true;
  __SEV();
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_energy_detected(const nrf_802154_energy_detected_t *p_result)
{
  (void)p_result;
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_energy_detection_failed(nrf_802154_ed_error_t error)
{
  (void)error;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(nrf53_radio_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    if(rx_fail_count > 0) {
      if(rx_fail_count > 16) {
        LOG_WARN("RX failed %" PRIu32 " times\n", rx_fail_count);
      }
      rx_fail_count = 0;
    }
    if(rx_drop_count > 0) {
      LOG_WARN("RX ring full, dropped %" PRIu32 " frames\n", rx_drop_count);
      rx_drop_count = 0;
    }
    if(tx_fail_count > 0) {
      LOG_WARN("TX failed %" PRIu32 " times\n", tx_fail_count);
      tx_fail_count = 0;
    }

    while(pending_packet()) {
      int len;
      packetbuf_clear();
      len = radio_read(packetbuf_dataptr(), PACKETBUF_SIZE);
      if(len > 0) {
        packetbuf_set_datalen(len);
        NETSTACK_MAC.input();
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
const struct radio_driver nrf_ieee_driver = {
  init,
  prepare,
  transmit,
  send,
  radio_read,
  channel_clear,
  receiving_packet,
  pending_packet,
  on,
  off,
  get_value,
  set_value,
  get_object,
  set_object
};
/*---------------------------------------------------------------------------*/
