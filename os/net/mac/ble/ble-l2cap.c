/*
 * Copyright (c) 2017, Graz University of Technology
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
 *    MAC layer that implements BLE L2CAP credit-based flow control
 *    channels to support IPv6 over BLE (RFC 7668)
 *
 * \author
 *    Michael Spoerk <michael.spoerk@tugraz.at>
 */
/*---------------------------------------------------------------------------*/

#include "net/mac/ble/ble-l2cap.h"

#include "net/packetbuf.h"
#include "net/netstack.h"
#include "lib/memb.h"
#include "lib/list.h"

#include <string.h>

#include "../../../dev/ble-hal.h"
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "L2CAP"
#define LOG_LEVEL LOG_LEVEL_MAC
/*---------------------------------------------------------------------------*/
#define MS_TO_CLOCK_SECONDS(X)    ((int)(((double)((X)*CLOCK_SECOND)) / 1000.0))
/*---------------------------------------------------------------------------*/
/* BLE controller */
/* public device address of BLE controller */
static uint8_t ble_addr[BLE_ADDR_SIZE];
/*---------------------------------------------------------------------------*/
/* L2CAP fragmentation buffers and utilities                                 */
typedef struct {
  /* L2CAP Service Data Unit (SDU) (= packet data)*/
  uint8_t sdu[BLE_L2CAP_NODE_MTU];
  /* length of the L2CAP SDU */
  uint16_t sdu_length;
  /* index of the first byte not sent yet */
  uint16_t current_index;
} l2cap_buffer_t;
/*---------------------------------------------------------------------------*/
typedef struct {
  uint16_t cid;
  uint16_t mtu;
  uint16_t mps;
  uint16_t credits;
} ble_mac_l2cap_channel_t;
/*---------------------------------------------------------------------------*/
typedef struct {
  ble_mac_l2cap_channel_t channel_own;
  ble_mac_l2cap_channel_t channel_peer;
  l2cap_buffer_t tx_buffer;
  l2cap_buffer_t rx_buffer;
  linkaddr_t peer_addr;
} l2cap_channel_t;

static uint8_t l2cap_channel_count;
static l2cap_channel_t l2cap_channels[L2CAP_CHANNELS];
static process_event_t l2cap_tx_event;
/*---------------------------------------------------------------------------*/
static l2cap_channel_t *
get_channel_for_addr(const linkaddr_t *peer_addr)
{
  uint8_t i;
  l2cap_channel_t *channel;
  for(i = 0; i < l2cap_channel_count; i++) {
    channel = &l2cap_channels[i];
    if(linkaddr_cmp(peer_addr, &channel->peer_addr) != 0) {
      return channel;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
static l2cap_channel_t *
get_channel_for_cid(uint16_t own_cid)
{
  uint8_t i = own_cid - L2CAP_FLOW_CHANNEL;
  if(i >= 0 && i < l2cap_channel_count) {
    return &l2cap_channels[own_cid - L2CAP_FLOW_CHANNEL];
  } else {
    return NULL;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS(ble_l2cap_tx_process, "BLE L2CAP TX process");
/*---------------------------------------------------------------------------*/
static uint8_t
init_adv_data(char *adv_data)
{
  uint8_t adv_data_len = 0;
  memset(adv_data, 0x00, BLE_ADV_DATA_LEN);
  /* BLE flags */
  adv_data[adv_data_len++] = 2;
  adv_data[adv_data_len++] = 0x01;
  adv_data[adv_data_len++] = 0x05;     /* LE limited  (no BR/EDR support) */
  /* TX power level */
  adv_data[adv_data_len++] = 2;
  adv_data[adv_data_len++] = 0x0A;
  adv_data[adv_data_len++] = 0;        /* 0 dBm */
  /* service UUIDs (16-bit identifiers) */
  adv_data[adv_data_len++] = 3;
  adv_data[adv_data_len++] = 0x03;
  adv_data[adv_data_len++] = 0x20;
  adv_data[adv_data_len++] = 0x18;     /* only IP support service exposed */
  /* service UUIDs (32-bit identifiers) */
  adv_data[adv_data_len++] = 1;
  adv_data[adv_data_len++] = 0x05;     /* empty list */
  /* service UUIDs (128-bit identifiers) */
  adv_data[adv_data_len++] = 1;
  adv_data[adv_data_len++] = 0x07;     /* empty list */
  return adv_data_len;
}
/*---------------------------------------------------------------------------*/
static uint8_t
init_scan_resp_data(char *scan_resp_data)
{
  uint8_t scan_resp_data_len = 0;
  memset(scan_resp_data, 0x00, BLE_SCAN_RESP_DATA_LEN);
  /* complete device name */
  scan_resp_data[scan_resp_data_len++] = 1 + strlen(BLE_DEVICE_NAME);
  scan_resp_data[scan_resp_data_len++] = 0x09;
  memcpy(&scan_resp_data[scan_resp_data_len],
         BLE_DEVICE_NAME, strlen(BLE_DEVICE_NAME));
  scan_resp_data_len += strlen(BLE_DEVICE_NAME);
  /* slave connection interval range */
  scan_resp_data[scan_resp_data_len++] = 5;
  scan_resp_data[scan_resp_data_len++] = 0x12;
  scan_resp_data[scan_resp_data_len++] = (BLE_SLAVE_CONN_INTERVAL_MIN & 0xFF);
  scan_resp_data[scan_resp_data_len++] = ((BLE_SLAVE_CONN_INTERVAL_MIN >> 8) & 0xFF);
  scan_resp_data[scan_resp_data_len++] = (BLE_SLAVE_CONN_INTERVAL_MAX & 0xFF);
  scan_resp_data[scan_resp_data_len++] = ((BLE_SLAVE_CONN_INTERVAL_MAX >> 8) & 0xFF);

  return scan_resp_data_len;
}
/*---------------------------------------------------------------------------*/
void
input_l2cap_conn_req(uint8_t *data)
{
  uint8_t identifier = data[0];
  uint16_t len;
  uint16_t le_psm;
  uint8_t resp_data[18];
  l2cap_channel_t *channel;

  memcpy(&len, &data[1], 2);

  if(len != 10) {
    LOG_WARN("l2cap_conn_req: invalid len: %d\n", len);
    return;
  }

  /* create a new L2CAP connection because of this request */
  if(l2cap_channel_count >= L2CAP_CHANNELS) {
    LOG_WARN("l2cap_conn_req: maximum supported L2CAP channels reached\n");
    return;
  }

  channel = &l2cap_channels[l2cap_channel_count];
  /* parse L2CAP connection data */
  memcpy(&le_psm, &data[3], 2);
  memset(&channel->channel_peer, 0x00, sizeof(ble_mac_l2cap_channel_t));
  memcpy(&channel->channel_peer.cid, &data[5], 2);
  memcpy(&channel->channel_peer.mtu, &data[7], 2);
  memcpy(&channel->channel_peer.mps, &data[9], 2);
  memcpy(&channel->channel_peer.credits, &data[11], 2);
  linkaddr_copy(&channel->peer_addr, packetbuf_addr(PACKETBUF_ADDR_SENDER));

  LOG_INFO("recv CONN_REQ (MTU: %4d, MPS: %4d, credits: %4d)\n",
           channel->channel_peer.mtu, channel->channel_peer.mps, channel->channel_peer.credits);

  l2cap_channel_count++;

  /* create L2CAP connection response */
  /* length */
  resp_data[0] = 0x0E;
  resp_data[1] = 0x00;
  /* channel ID */
  resp_data[2] = 0x05;
  resp_data[3] = 0x00;
  /* code */
  resp_data[4] = L2CAP_CODE_CONN_RSP;
  /* identifier */
  resp_data[5] = identifier;
  /* cmd length */
  resp_data[6] = 0x0A;
  resp_data[7] = 0x00;
  /* node channel information */
  memcpy(&resp_data[8], &channel->channel_own.cid, 2);
  memcpy(&resp_data[10], &channel->channel_own.mtu, 2);
  memcpy(&resp_data[12], &channel->channel_own.mps, 2);
  memcpy(&resp_data[14], &channel->channel_own.credits, 2);
  /* result */
  memset(&resp_data[16], 0x00, 2);

  packetbuf_copyfrom((void *)resp_data, 18);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &channel->peer_addr);
  NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen());
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  uint8_t i;
  l2cap_tx_event = process_alloc_event();
  LOG_DBG("init()\n");
  /* initialize the L2CAP connection parameter */
  for(i = 0; i < L2CAP_CHANNELS; i++) {
    l2cap_channels[i].channel_own.cid = L2CAP_FLOW_CHANNEL + i;
    l2cap_channels[i].channel_own.credits = L2CAP_CREDIT_NEW;
    l2cap_channels[i].channel_own.mps = (BLE_L2CAP_NODE_FRAG_LEN - L2CAP_SUBSEQ_HEADER_SIZE);
    l2cap_channels[i].channel_own.mtu = BLE_L2CAP_NODE_MTU;
  }

  /* Initialize the BLE controller */
  NETSTACK_RADIO.init();
  NETSTACK_RADIO.get_object(RADIO_CONST_BLE_BD_ADDR, &ble_addr, BLE_ADDR_SIZE);

  uint8_t adv_data_len, scan_resp_data_len;
  char adv_data[BLE_ADV_DATA_LEN];
  char scan_resp_data[BLE_SCAN_RESP_DATA_LEN];
  /* set the advertisement parameter */
  NETSTACK_RADIO.set_value(RADIO_PARAM_BLE_ADV_INTERVAL, BLE_ADV_INTERVAL);
  NETSTACK_RADIO.set_value(RADIO_PARAM_BLE_ADV_TYPE, BLE_ADV_DIR_IND_LDC);
  NETSTACK_RADIO.set_value(RADIO_PARAM_BLE_ADV_OWN_ADDR_TYPE, BLE_ADDR_TYPE_PUBLIC);
  NETSTACK_RADIO.set_value(RADIO_PARAM_BLE_ADV_CHANNEL_MAP, 0x01);

  adv_data_len = init_adv_data(adv_data);
  scan_resp_data_len = init_scan_resp_data(scan_resp_data);

  /* set advertisement payload & scan response */
  NETSTACK_RADIO.set_object(RADIO_PARAM_BLE_ADV_PAYLOAD, adv_data, adv_data_len);
  NETSTACK_RADIO.set_object(RADIO_PARAM_BLE_ADV_SCAN_RESPONSE, scan_resp_data, scan_resp_data_len);

  /* enable advertisement */
  NETSTACK_RADIO.set_value(RADIO_PARAM_BLE_ADV_ENABLE, 1);

  NETSTACK_MAC.on();
}
/*---------------------------------------------------------------------------*/
static uint16_t
check_own_l2cap_credits(l2cap_channel_t *channel)
{
  uint16_t credits_new = 0;
  uint16_t credits_current;

  credits_current = channel->channel_own.credits;
  if(credits_current < L2CAP_CREDIT_THRESHOLD) {
    credits_new = L2CAP_CREDIT_NEW;
  }
  LOG_DBG("check for new credits: current credits: %2d, new credits: %2d\n", credits_current, credits_new);
  return credits_new;
}
/*---------------------------------------------------------------------------*/
static void
send_l2cap_credit(l2cap_channel_t *channel, uint16_t credits)
{
  uint8_t len = 4;
  uint8_t data[12];
  /* create L2CAP credit */
  /* length */
  data[0] = len + 4;
  data[1] = 0x00;
  /* channel ID */
  data[2] = 0x05;
  data[3] = 0x00;
  /* code */
  data[4] = L2CAP_CODE_CREDIT;
  /* identifier */
  data[5] = 0xFF;
  /* cmd length */
  data[6] = len;
  data[7] = 0x00;

  memcpy(&data[8], &channel->channel_own.cid, 2);
  data[10] = credits & 0xFF;
  data[11] = credits >> 8;

  channel->channel_own.credits += credits;

  packetbuf_copyfrom((void *)data, len + 8);
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
  packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &channel->peer_addr);
  NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen());
}
/*---------------------------------------------------------------------------*/
static void
send(mac_callback_t sent_callback, void *ptr)
{
  uint8_t i;
  l2cap_channel_t *channel;
  uint16_t data_len = packetbuf_datalen();
  LOG_DBG("send %d\n", data_len);

  /* packet is too long */
  if(data_len > BLE_L2CAP_NODE_MTU) {
    LOG_WARN("send message is too long\n");
    mac_call_sent_callback(sent_callback, ptr, MAC_TX_ERR, 0);
    return;
  }

  for(i = 0; i < l2cap_channel_count; i++) {
    channel = &l2cap_channels[i];
    if((linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &linkaddr_null) != 0)
       || (linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), &channel->peer_addr) != 0)) {
      if(channel->tx_buffer.sdu_length > 0) {
        LOG_WARN("send() another L2CAP message active (trying to send %4d bytes)\n", data_len);
        mac_call_sent_callback(sent_callback, ptr, MAC_TX_COLLISION, 0);
        return;
      }
      LOG_DBG("send() adding to L2CAP CID: %2d\n", channel->channel_own.cid);
      channel->tx_buffer.sdu_length = data_len;
      if(channel->tx_buffer.sdu_length > 0) {
        memcpy(&channel->tx_buffer.sdu, packetbuf_dataptr(), data_len);
        mac_call_sent_callback(sent_callback, ptr, MAC_TX_DEFERRED, 1);
        process_post(&ble_l2cap_tx_process, l2cap_tx_event, (void *)channel);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
input_l2cap_connection_udate_resp(uint8_t *data)
{
  uint16_t len;
  uint16_t result;

  memcpy(&len, &data[1], 2);

  if(len != 2) {
    LOG_WARN("input_l2cap_connection_update_resp: invalid len: %d\n", len);
    return;
  }

  memcpy(&result, &data[3], 2);
  if(result != 0x0000) {
    LOG_WARN("input_l2cap_connection_update_resp: result: 0x%04X\n", result);
    return;
  }
}
/*---------------------------------------------------------------------------*/
void
input_l2cap_credit(uint8_t *data)
{
  uint16_t len;
  uint16_t cid;
  uint16_t credits;
  l2cap_channel_t *channel = get_channel_for_addr(packetbuf_addr(PACKETBUF_ADDR_SENDER));

/*  uint8_t  identifier = data[0]; */
  memcpy(&len, &data[1], 2);

  if(len != 4) {
    LOG_WARN("process_l2cap_credit: invalid len: %d\n", len);
    return;
  }

  /* parse L2CAP credit data */
  memcpy(&cid, &data[3], 2);
  memcpy(&credits, &data[5], 2);

  channel->channel_peer.credits += credits;
}
/*---------------------------------------------------------------------------*/
static void
input_l2cap_frame_signal_channel(uint8_t *data, uint8_t data_len)
{
  if(data[4] == L2CAP_CODE_CREDIT) {
    input_l2cap_credit(&data[5]);
  } else if(data[4] == L2CAP_CODE_CONN_REQ) {
    input_l2cap_conn_req(&data[5]);
  } else if(data[4] == L2CAP_CODE_CONN_UPDATE_RSP) {
    input_l2cap_connection_udate_resp(&data[5]);
  } else {
    LOG_WARN("l2cap_frame_signal_channel: unknown signal channel code: %d\n", data[4]);
  }
}
/*---------------------------------------------------------------------------*/
static void
input_l2cap_frame_flow_channel(l2cap_channel_t *channel, uint8_t *data, uint16_t data_len)
{
  uint16_t frame_len;
  uint16_t payload_len;

  if(data_len < 4) {
    LOG_WARN("l2cap_frame: illegal L2CAP frame data_len: %d\n", data_len);
    /* a L2CAP frame has a minimum length of 4 */
    return;
  }

  if(channel->rx_buffer.sdu_length == 0) {
    /* handle first fragment */
    memcpy(&frame_len, &data[0], 2);
    memcpy(&channel->rx_buffer.sdu_length, &data[4], 2);
    payload_len = frame_len - 2;

    memcpy(channel->rx_buffer.sdu, &data[6], payload_len);
    channel->rx_buffer.current_index = payload_len;
  } else {
    /* subsequent fragment */
    memcpy(&frame_len, &data[0], 2);
    payload_len = frame_len;

    memcpy(&channel->rx_buffer.sdu[channel->rx_buffer.current_index], &data[4], payload_len);
    channel->rx_buffer.current_index += payload_len;
  }

  if((channel->rx_buffer.sdu_length > 0) &&
     (channel->rx_buffer.sdu_length == channel->rx_buffer.current_index)) {
    /* do not use packetbuf_copyfrom here because the packetbuf_attr
     * must not be cleared */
    memcpy(packetbuf_dataptr(), channel->rx_buffer.sdu, channel->rx_buffer.sdu_length);
    packetbuf_set_datalen(channel->rx_buffer.sdu_length);
    NETSTACK_NETWORK.input();

    /* reset counters */
    channel->rx_buffer.sdu_length = 0;
    channel->rx_buffer.current_index = 0;
  }
}
/*---------------------------------------------------------------------------*/
static void
input(void)
{
  uint8_t *data = (uint8_t *)packetbuf_dataptr();
  uint16_t len = packetbuf_datalen();
  uint8_t frame_type = packetbuf_attr(PACKETBUF_ATTR_FRAME_TYPE);
  uint16_t channel_id;
  l2cap_channel_t *channel;
  uint16_t credits;

  if(frame_type == FRAME_BLE_RX_EVENT) {
    memcpy(&channel_id, &data[2], 2);
    channel = get_channel_for_cid(channel_id);
    LOG_DBG("input %d bytes\n", len);
    if(channel_id == L2CAP_SIGNAL_CHANNEL) {
      input_l2cap_frame_signal_channel(data, len);
    } else if(channel == NULL) {
      LOG_WARN("input (RX_EVENT): no channel found for CID: %d\n", channel_id);
      return;
    } else {
      input_l2cap_frame_flow_channel(channel, data, len);
      channel->channel_own.credits--;
      credits = check_own_l2cap_credits(channel);
      if(credits > 0) {
        send_l2cap_credit(channel, credits);
      }
    }
  }
  /* check if there are still fragments left to be transmitted */
  if(frame_type == FRAME_BLE_TX_EVENT) {
    channel = get_channel_for_addr(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
    if(channel == NULL) {
      LOG_WARN("input (TX_EVENT): no channel found for CID: %d\n", channel_id);
    } else if(channel->tx_buffer.sdu_length > 0) {
      process_post(&ble_l2cap_tx_process, l2cap_tx_event, (void *)channel);
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  LOG_DBG("on()\n");
  process_start(&ble_l2cap_tx_process, NULL);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  LOG_DBG("off()\n");
  process_exit(&ble_l2cap_tx_process);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  return BLE_L2CAP_NODE_MTU;
}
/*---------------------------------------------------------------------------*/
const struct mac_driver ble_l2cap_driver = {
  "ble-l2cap",
  init,
  send,
  input,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ble_l2cap_tx_process, ev, data)
{
  uint16_t data_len;
  uint16_t frame_len;
  uint16_t num_buffer;
  l2cap_channel_t *channel = (l2cap_channel_t *)data;
  uint8_t first_fragment;
  uint16_t used_mps;
  uint16_t credits;

  PROCESS_BEGIN();
  LOG_DBG("starting ble_mac_tx_process\n");

  while(1) {
    PROCESS_YIELD_UNTIL(ev == l2cap_tx_event);
    if(channel != NULL) {
      NETSTACK_RADIO.get_value(RADIO_CONST_BLE_BUFFER_AMOUNT, (radio_value_t *)&num_buffer);
      first_fragment = (channel->tx_buffer.current_index == 0);
      used_mps = MIN(channel->channel_own.mps, channel->channel_peer.mps);
      credits = channel->channel_peer.credits;

      LOG_DBG("process: sending - first: %d, used_mps: %3d, num_buffers: %2d, credits: %2d\n",
              first_fragment, used_mps, num_buffer, credits);
      if((channel->tx_buffer.sdu_length > 0) && (num_buffer > 0) && (credits > 0)) {
        packetbuf_clear();
        if(first_fragment) {
          packetbuf_hdralloc(L2CAP_FIRST_HEADER_SIZE);
          used_mps -= L2CAP_FIRST_HEADER_SIZE;
          data_len = MIN(channel->tx_buffer.sdu_length, used_mps);
          frame_len = data_len + 2;

          /* set L2CAP header fields */
          memcpy(packetbuf_hdrptr(), &frame_len, 2);          /* fragment size */
          memcpy(packetbuf_hdrptr() + 2, &channel->channel_peer.cid, 2);    /* L2CAP channel id*/
          memcpy(packetbuf_hdrptr() + 4, &channel->tx_buffer.sdu_length, 2);  /* overall packet size */
        } else {
          packetbuf_hdralloc(L2CAP_SUBSEQ_HEADER_SIZE);
          used_mps -= L2CAP_SUBSEQ_HEADER_SIZE;
          data_len = MIN((channel->tx_buffer.sdu_length - channel->tx_buffer.current_index), used_mps);
          frame_len = data_len;

          /* set L2CAP header fields */
          memcpy(packetbuf_hdrptr(), &frame_len, 2);          /* fragment size */
          memcpy(packetbuf_hdrptr() + 2, &channel->channel_peer.cid, 2);      /* L2CAP channel id*/
        }

        /* copy payload */
        memcpy(packetbuf_dataptr(),
               &channel->tx_buffer.sdu[channel->tx_buffer.current_index],
               data_len);
        packetbuf_set_datalen(data_len);
        channel->tx_buffer.current_index += data_len;

        /* send the fragment */
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &channel->peer_addr);
        packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &linkaddr_node_addr);
        NETSTACK_RADIO.send(packetbuf_hdrptr(), packetbuf_totlen());
        channel->channel_peer.credits--;

        /* reset the L2CAP TX buffer if packet is finished */
        if(channel->tx_buffer.current_index == channel->tx_buffer.sdu_length) {
          channel->tx_buffer.current_index = 0;
          channel->tx_buffer.sdu_length = 0;
        }
      }
    } else {
      LOG_WARN("process. channel is NULL\n");
    }
  }

  PROCESS_END();
  LOG_DBG("stopped ble_mac_tx_process\n");
}
