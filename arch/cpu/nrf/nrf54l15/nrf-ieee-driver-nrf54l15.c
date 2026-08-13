/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Author: Joakim Eriksson <joakim.eriksson@ri.se>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * IEEE 802.15.4 radio driver for nRF54L15 using Nordic's nrf_802154 library.
 * This file wraps the nrf_802154 API into Contiki-NG's radio_driver interface.
 */

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

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "nrf54l15-radio"
#define LOG_LEVEL LOG_CONF_LEVEL_RADIO

/*---------------------------------------------------------------------------*/
/* IEEE 802.15.4 constants */
#define MAX_PAYLOAD_LEN 125 /* 127 - FCS(2) */
#define ACK_LEN         5
#define FRAME_ACK_REQUEST_BIT 0x20

/* Default radio parameters */
#define DEFAULT_CHANNEL     26
#define DEFAULT_TX_POWER    0 /* dBm */

/* Use the radio timer's microsecond clock so timeouts match the nRF 802154
 * platform backend on nRF54L15. The generic rtimer backend still uses a
 * different tick scale on this port. */
#define TX_DONE_TIMEOUT_US  250000ULL
#define TX_ABORT_TIMEOUT_US 10000ULL
#define CCA_DONE_TIMEOUT_US 50000ULL

/*---------------------------------------------------------------------------*/
/* TX buffer: [PHR byte (length)] [PSDU ...] -- nrf_802154 expects raw format */
static uint8_t tx_buf[1 + MAX_PAYLOAD_LEN + 2]; /* +2 for FCS space */
static volatile uint8_t tx_buf_len;

/* RX state */
/* Stage as many software RX buffers as the driver advertises so that bursts
 * of received frames are not dropped while the upper layer drains them
 * outside of ISR context. */
#define RX_BUF_COUNT NRF_802154_RX_BUFFERS
static uint8_t *rx_bufs[RX_BUF_COUNT];
static int8_t rx_rssi[RX_BUF_COUNT];
static uint8_t rx_lqi[RX_BUF_COUNT];
static volatile uint8_t rx_head;  /* next slot to write into */
static volatile uint8_t rx_tail;  /* next slot to read from */

/* TX completion state */
static volatile bool tx_done;
static volatile bool tx_success;
static volatile nrf_802154_tx_error_t tx_error;

/* CCA completion state */
static volatile bool cca_done;
static volatile bool cca_free;

/* Error counters -- incremented in ISR, logged in process context */
static volatile uint32_t rx_fail_count;
static volatile uint32_t tx_fail_count;

/* Current radio parameters */
static uint8_t current_channel = DEFAULT_CHANNEL;
static int8_t current_tx_power = DEFAULT_TX_POWER;
static bool radio_is_on;

/*---------------------------------------------------------------------------*/
PROCESS(nrf54l15_radio_process, "nRF54L15 radio driver");
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
  case NRF_802154_TX_ERROR_KEY_ID_INVALID:
    return "key_id_invalid";
  case NRF_802154_TX_ERROR_FRAME_COUNTER_ERROR:
    return "frame_counter_error";
  case NRF_802154_TX_ERROR_TIMESTAMP_ENCODING_ERROR:
    return "timestamp_encoding_error";
  case NRF_802154_TX_ERROR_INVALID_REQUEST:
    return "invalid_request";
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

  if(nrf_802154_sleep()) {
    (void)wait_for_flag(&tx_done, TX_ABORT_TIMEOUT_US);
  }

  if(!nrf_802154_receive()) {
    LOG_WARN("Failed to restore receive state\n");
  }
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  LOG_INFO("Initializing nrf_802154 radio driver\n");

  nrf_802154_init();

  /* Set default channel and TX power. */
  nrf_802154_channel_set(current_channel);
  nrf_802154_tx_power_set(current_tx_power);

  /* Enable auto-ACK. */
  nrf_802154_auto_ack_set(true);
  nrf_802154_rx_on_when_idle_set(true);

  /* Set PAN ID from Contiki-NG configuration. */
  uint8_t pan_id[2] = {
    (uint8_t)(IEEE802154_PANID & 0xFF),
    (uint8_t)(IEEE802154_PANID >> 8)
  };
  nrf_802154_pan_id_set(pan_id);

  /* Set short address. */
  uint8_t short_addr[2] = {
    (uint8_t)(linkaddr_node_addr.u8[LINKADDR_SIZE - 2]),
    (uint8_t)(linkaddr_node_addr.u8[LINKADDR_SIZE - 1])
  };
  nrf_802154_short_address_set(short_addr);

  /* Set extended address (8 bytes, little-endian). */
  uint8_t ext_addr[8];
  for(int i = 0; i < 8; i++) {
    if(i < LINKADDR_SIZE) {
      ext_addr[i] = linkaddr_node_addr.u8[LINKADDR_SIZE - 1 - i];
    } else {
      ext_addr[i] = 0;
    }
  }
  nrf_802154_extended_address_set(ext_addr);

  /* Initialize RX buffer pointers. */
  rx_head = 0;
  rx_tail = 0;

  radio_is_on = false;

  /* Start the radio process. */
  process_start(&nrf54l15_radio_process, NULL);

  LOG_INFO("Radio initialized, channel %u, PAN 0x%04x\n",
           current_channel, IEEE802154_PANID);

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  if(!radio_is_on) {
    LOG_DBG("Radio ON\n");
    ENERGEST_ON(ENERGEST_TYPE_LISTEN);
    if(!nrf_802154_receive()) {
      ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
      LOG_WARN("Failed to enter receive state\n");
      return 0;
    }
    LOG_DBG("receive() returned, ticks=%lu\n", (unsigned long)clock_time());
    radio_is_on = true;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  if(radio_is_on) {
    LOG_DBG("Radio OFF\n");
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

  /* PHR byte = payload length + FCS length (2 bytes). */
  tx_buf[0] = payload_len + 2;
  memcpy(&tx_buf[1], payload, payload_len);
  tx_buf_len = payload_len;

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  nrf_802154_transmit_metadata_t metadata = {
    .frame_props = NRF_802154_TRANSMITTED_FRAME_PROPS_DEFAULT_INIT,
    .cca = false,
    .tx_power = { .use_metadata_value = false },
    .tx_channel = { .use_metadata_value = false },
    .tx_timestamp_encode = false,
  };

  /* Ensure the radio is in RX state — nrf_802154 requires this before TX. */
  if(!radio_is_on) {
    on();
  }

  LOG_DBG("TX %u bytes, ch=%u\n", tx_buf[0], nrf_802154_channel_get());

  tx_done = false;
  tx_success = false;
  tx_error = NRF_802154_TX_ERROR_NONE;

  ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);

  /* nrf_802154_transmit_raw() returns nrf_802154_tx_error_t (uint8_t):
   * NRF_802154_TX_ERROR_NONE (0) = accepted, non-zero = error */
  nrf_802154_tx_error_t tx_err = nrf_802154_transmit_raw(tx_buf, &metadata);

  LOG_DBG("TX result=%u\n", tx_err);

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

  /* Wait for TX completion callback with timeout. */
  if(!wait_for_flag(&tx_done, TX_DONE_TIMEOUT_US)) {
    /* The frame was accepted for transmission, but the 802.15.4 stack never
     * reported a final verdict. This is not safe to translate to NOACK:
     * higher layers would poison link metrics and may leave the DAG on a
     * completion-path bug. */
    LOG_WARN("TX timeout\n");
    ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);
    tx_error = NRF_802154_TX_ERROR_ABORTED;
    return RADIO_TX_ERR;
  }

  ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);

  if(tx_success) {
    LOG_DBG("TX OK %u bytes on ch %u\n", tx_buf[0], current_channel);
    return RADIO_TX_OK;
  }

  if(tx_error == NRF_802154_TX_ERROR_NO_ACK) {
    LOG_WARN("TX failed: no ACK\n");
    return RADIO_TX_NOACK;
  }

  if(tx_error == NRF_802154_TX_ERROR_INVALID_ACK) {
    LOG_WARN("TX failed: invalid ACK\n");
    return RADIO_TX_NOACK;
  }

  if(tx_error == NRF_802154_TX_ERROR_BUSY_CHANNEL) {
    LOG_WARN("TX failed: busy channel\n");
    return RADIO_TX_COLLISION;
  }

  LOG_WARN("TX failed: %s (%u)\n", tx_error_name(tx_error), tx_error);
  return RADIO_TX_ERR;
}
/*---------------------------------------------------------------------------*/
static int
send(const void *payload, unsigned short payload_len)
{
  prepare(payload, payload_len);
  return transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short buf_len)
{
  if(rx_tail == rx_head) {
    return 0; /* No pending frames */
  }

  uint8_t idx = rx_tail % RX_BUF_COUNT;
  uint8_t *p_data = rx_bufs[idx];

  if(p_data == NULL) {
    rx_tail++;
    return 0;
  }

  /* p_data[0] = PHR (length including FCS).
   * Actual payload length = PHR - FCS(2). */
  uint8_t frame_len = p_data[0];
  uint8_t payload_len = frame_len - 2; /* Subtract FCS */

  if(payload_len > buf_len) {
    payload_len = buf_len;
  }

  memcpy(buf, &p_data[1], payload_len);

  /* Store RSSI and LQI in packetbuf attributes. */
  packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rx_rssi[idx]);
  packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, rx_lqi[idx]);

  /* Free the buffer back to the nrf_802154 driver. */
  nrf_802154_buffer_free_raw(p_data);
  rx_bufs[idx] = NULL;
  rx_tail++;

  LOG_DBG("RX %u bytes, RSSI %d, LQI %u\n", payload_len,
          rx_rssi[idx], rx_lqi[idx]);

  return payload_len;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  LOG_DBG("CCA\n");

  if(!radio_is_on) {
    on();
  }

  cca_done = false;
  cca_free = false;

  if(!nrf_802154_cca()) {
    LOG_WARN("CCA could not start\n");
    return 0;
  }

  /* Wait for CCA completion callback with timeout. */
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
  /* We cannot easily detect "mid-frame reception" from the nrf_802154 API.
   * Return 0 -- Contiki-NG treats this as "not currently receiving". */
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

  case RADIO_PARAM_PAN_ID:
    return RADIO_RESULT_NOT_SUPPORTED;

  case RADIO_PARAM_16BIT_ADDR:
    return RADIO_RESULT_NOT_SUPPORTED;

  case RADIO_PARAM_RX_MODE:
    *value = 0;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_TX_MODE:
    *value = 0;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_TXPOWER:
    *value = (radio_value_t)nrf_802154_tx_power_get();
    return RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    return RADIO_RESULT_NOT_SUPPORTED;

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
    *value = -20;
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MAX:
    *value = 8;
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
    current_tx_power = (int8_t)value;
    nrf_802154_tx_power_set(current_tx_power);
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RX_MODE:
    return RADIO_RESULT_OK;

  case RADIO_PARAM_TX_MODE:
    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  switch(param) {
  case RADIO_PARAM_64BIT_ADDR: {
    if(size != 8) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    /* Convert to little-endian for nrf_802154. */
    uint8_t ext_addr[8];
    const uint8_t *addr = (const uint8_t *)src;
    for(int i = 0; i < 8; i++) {
      ext_addr[i] = addr[7 - i];
    }
    nrf_802154_extended_address_set(ext_addr);
    return RADIO_RESULT_OK;
  }
  case RADIO_PARAM_PAN_ID: {
    if(size != 2) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    nrf_802154_pan_id_set((const uint8_t *)src);
    return RADIO_RESULT_OK;
  }
  case RADIO_PARAM_16BIT_ADDR: {
    if(size != 2) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    nrf_802154_short_address_set((const uint8_t *)src);
    return RADIO_RESULT_OK;
  }
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
/* nrf_802154 callbacks below are invoked from ISR context. */
void
nrf_802154_received_timestamp_raw(uint8_t *p_data, int8_t power,
                                  uint8_t lqi, uint64_t time)
{
  uint8_t idx = rx_head % RX_BUF_COUNT;

  if(((rx_head - rx_tail) & 0xFF) >= RX_BUF_COUNT) {
    /* RX buffer full -- drop the oldest frame. */
    if(rx_bufs[rx_tail % RX_BUF_COUNT] != NULL) {
      nrf_802154_buffer_free_raw(rx_bufs[rx_tail % RX_BUF_COUNT]);
      rx_bufs[rx_tail % RX_BUF_COUNT] = NULL;
    }
    rx_tail++;
    idx = rx_head % RX_BUF_COUNT;
  }

  rx_bufs[idx] = p_data;
  rx_rssi[idx] = power;
  rx_lqi[idx] = lqi;
  rx_head++;

  /* Signal the Contiki-NG process to deliver the frame. */
  process_poll(&nrf54l15_radio_process);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_receive_failed(nrf_802154_rx_error_t error, uint32_t id)
{
  (void)error;
  (void)id;
  rx_fail_count++;
  process_poll(&nrf54l15_radio_process);
}
/*---------------------------------------------------------------------------*/
void
nrf_802154_transmitted_raw(uint8_t *p_frame,
                           const nrf_802154_transmit_done_metadata_t *p_metadata)
{
  bool ack_requested = false;

  if(p_frame != NULL) {
    /* p_frame[0] is PHR, p_frame[1] is FCF byte 0. */
    ack_requested = (p_frame[1] & FRAME_ACK_REQUEST_BIT) != 0;
  }

  if(p_metadata->data.transmitted.p_ack != NULL) {
    uint8_t *ack = p_metadata->data.transmitted.p_ack;
    uint8_t ack_len = p_metadata->data.transmitted.length;

    (void)ack_len;

    /* Free the ACK buffer back to nrf_802154. */
    nrf_802154_buffer_free_raw(ack);
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

  tx_fail_count++;
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
/* Contiki-NG process for async RX delivery. */
PROCESS_THREAD(nrf54l15_radio_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);

    /* Log any errors that occurred in ISR context. */
    if(rx_fail_count > 0) {
      if(rx_fail_count > 16) {
        LOG_WARN("RX failed %" PRIu32 " times\n", rx_fail_count);
      }
      rx_fail_count = 0;
    }
    if(tx_fail_count > 0) {
      LOG_WARN("TX failed %" PRIu32 " times\n", tx_fail_count);
      tx_fail_count = 0;
    }

    /* Deliver all pending received frames to the MAC layer. */
    while(pending_packet()) {
      packetbuf_clear();
      int len = radio_read(packetbuf_dataptr(), PACKETBUF_SIZE);
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
