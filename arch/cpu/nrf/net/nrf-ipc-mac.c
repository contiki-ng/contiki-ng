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
 *      IPC MAC driver for the nRF5340 network core.
 *
 *      This MAC driver runs on the network core and forwards received
 *      frames to the application core via the IPC shared memory. It
 *      replaces NULLMAC for the net core, enabling fully interrupt-driven
 *      frame reception: the radio ISR triggers the radio driver process,
 *      which reads the frame and calls this MAC's input function.
 *
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "net/mac/mac.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "nrf-ipc.h"

#include <inttypes.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "IPC MAC"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
static volatile struct nrf_ipc_shared_mem *shm = NRF_IPC_SHARED_MEM;
/*---------------------------------------------------------------------------*/
static uint32_t rx_drop_count;
/*---------------------------------------------------------------------------*/
/**
 * 802.15.4 ACK frame constants.
 */
#define ACK_FRAME_LEN         3
#define FCF_ACK_REQUEST_BIT   0x20
#define FRAME802154_ACKFRAME  0x02
/*---------------------------------------------------------------------------*/
/*
 * Whether the underlying net-core radio acknowledges received frames in
 * hardware. The nrf_802154 driver does (nrf_802154_auto_ack_set), so the
 * software ACK below must be disabled to avoid a double ACK. The raw
 * nrf-ieee-driver-arch.c does not, so it stays enabled by default.
 */
#ifdef NRF_IPC_MAC_CONF_HW_AUTOACK
#define NRF_IPC_MAC_HW_AUTOACK NRF_IPC_MAC_CONF_HW_AUTOACK
#else
#define NRF_IPC_MAC_HW_AUTOACK 0
#endif
/*---------------------------------------------------------------------------*/
#if !NRF_IPC_MAC_HW_AUTOACK
/**
 * Send a software ACK for a received frame if the ACK request bit is set.
 */
static void
send_ack_if_needed(const uint8_t *frame, int len)
{
  uint8_t ack[ACK_FRAME_LEN];
  radio_value_t tx_mode;

  if(len < ACK_FRAME_LEN) {
    return;
  }

  /*
   * The net core's nRF radio has no hardware auto-ACK and does not
   * report RADIO_RX_MODE_AUTOACK via get_value(), so the IPC MAC must
   * generate the link-layer ACK in software for every unicast frame
   * that requests one (checked below via the ACK request bit). The
   * app core's CSMA relies on these ACKs to confirm its transmissions.
   */

  /* Do not ACK frames that are themselves ACKs. */
  if((frame[0] & 0x07) == FRAME802154_ACKFRAME) {
    return;
  }

  /* Check the ACK request bit (bit 5 of FCF byte 0). */
  if(!(frame[0] & FCF_ACK_REQUEST_BIT)) {
    return;
  }

  /* Build ACK: [Frame Control = ACK type, 0x00, Sequence Number]. */
  ack[0] = FRAME802154_ACKFRAME;
  ack[1] = 0;
  ack[2] = frame[2];  /* DSN is byte 2 of the received frame. */

  /*
   * 802.15.4 requires ACKs to be sent without CCA. Clear only the
   * SEND_ON_CCA bit and restore the previous TX mode afterwards, so
   * any other (current or future) TX mode flags are preserved.
   */
  if(NETSTACK_RADIO.get_value(RADIO_PARAM_TX_MODE, &tx_mode) != RADIO_RESULT_OK) {
    tx_mode = RADIO_TX_MODE_SEND_ON_CCA;
  }
  NETSTACK_RADIO.set_value(RADIO_PARAM_TX_MODE,
                           tx_mode & ~RADIO_TX_MODE_SEND_ON_CCA);
  NETSTACK_RADIO.send(ack, ACK_FRAME_LEN);
  NETSTACK_RADIO.set_value(RADIO_PARAM_TX_MODE, tx_mode);
}
#endif /* !NRF_IPC_MAC_HW_AUTOACK */
/*---------------------------------------------------------------------------*/
static void
init(void)
{
}
/*---------------------------------------------------------------------------*/
static void
send_packet(mac_callback_t sent, void *ptr)
{
}
/*---------------------------------------------------------------------------*/
/**
 * Called by the radio driver process when a frame has been received.
 * The frame is in packetbuf. We send a software ACK, then forward the
 * raw frame to the application core via shared memory.
 */
static void
packet_input(void)
{
  const uint8_t *frame;
  int len;

  frame = packetbuf_dataptr();
  len = packetbuf_datalen();

  /* Use heartbeat field as RX frame counter (readable via debugger). */
  shm->heartbeat++;

  if(len <= 0 || len > NRF_IPC_MAX_FRAME_LEN) {
    return;
  }

  /*
   * Sort incoming frames into ACK vs data paths. ACK frames (3 bytes,
   * frame type 0x02) go to shm->rx_ack for CSMA's tight ACK detection
   * loop. All other frames go to shm->rx for delivery via the process
   * thread. This separation prevents CSMA's RTIMER_BUSYWAIT from
   * consuming and discarding data frames while checking for ACKs.
   */
  if(len == ACK_FRAME_LEN && (frame[0] & 0x07) == FRAME802154_ACKFRAME) {
    /* ACK frame — goes to the dedicated ACK slot. */
    if(!shm->rx_ack.pending) {
      memcpy((void *)shm->rx_ack.data, frame, ACK_FRAME_LEN);
      __DMB();
      shm->rx_ack.pending = 1;
    }
    nrf_ipc_signal();
    return;
  }

#if !NRF_IPC_MAC_HW_AUTOACK
  /* Send a software ACK for non-ACK frames, before forwarding. The radio
   * does this in hardware when nrf_802154 is used (see NRF_IPC_MAC_HW_AUTOACK). */
  send_ack_if_needed(frame, len);
#endif

  /* Data frame — goes to the main RX slot. */
  if(shm->rx.pending) {
    rx_drop_count++;
    if((rx_drop_count % 100) == 1) {
      LOG_WARN("RX drop (app core busy), total drops: %" PRIu32 "\n",
               rx_drop_count);
    }
    return;
  }

  shm->rx.len = len;
  memcpy((void *)shm->rx.data, frame, len);
  shm->rx.rssi = (int8_t)packetbuf_attr(PACKETBUF_ATTR_RSSI);
  shm->rx.lqi = (uint8_t)packetbuf_attr(PACKETBUF_ATTR_LINK_QUALITY);

  __DMB();

  shm->rx.pending = 1;

  nrf_ipc_signal();
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  return NETSTACK_RADIO.on();
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  return NETSTACK_RADIO.off();
}
/*---------------------------------------------------------------------------*/
static int
max_payload(void)
{
  return NRF_IPC_MAX_FRAME_LEN;
}
/*---------------------------------------------------------------------------*/
const struct mac_driver ipc_mac_driver = {
  "ipc-mac",
  init,
  send_packet,
  packet_input,
  on,
  off,
  max_payload,
};
/*---------------------------------------------------------------------------*/
