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
 * \addtogroup nrf-ipc
 * @{
 *
 * \file
 *      IPC radio driver for the nRF5340 application core.
 *
 *      This driver implements the Contiki-NG radio API by forwarding
 *      all radio operations over IPC to the network core, which runs
 *      the actual 802.15.4 radio hardware.
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
#include "nrf-ipc.h"
#include "nrf.h"
#include "nrfx.h"
#include <inttypes.h>
#include <string.h>
#ifdef TRUSTZONE_SECURE
#include "trustzone/tz-radio.h"
#endif
/*---------------------------------------------------------------------------*/
/* Only compile on the nRF5340 application core. */
#ifdef NRF5340_XXAA_APPLICATION
/*---------------------------------------------------------------------------*/
#include "hal/nrf_gpio.h"
#include "hal/nrf_reset.h"
#include "hal/nrf_spu.h"
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "IPC Radio"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
#define NET_CORE_INIT_TIMEOUT_MS  1000
/*---------------------------------------------------------------------------*/
static volatile struct nrf_ipc_shared_mem *shm = NRF_IPC_SHARED_MEM;
/*---------------------------------------------------------------------------*/
/* Local buffer for the last received frame. */
static uint8_t rx_frame_buf[NRF_IPC_MAX_FRAME_LEN];
static int rx_frame_len;
static bool rx_frame_pending;
/*---------------------------------------------------------------------------*/
/* TX buffer. */
static uint8_t tx_frame_buf[NRF_IPC_MAX_FRAME_LEN];
/*---------------------------------------------------------------------------*/
PROCESS(ipc_radio_process, "IPC Radio");
/*---------------------------------------------------------------------------*/
static radio_result_t ipc_radio_set_object(radio_param_t param,
                                           const void *src, size_t size);
/*---------------------------------------------------------------------------*/
/**
 * Send a command to the net core and wait for the response.
 *
 * The caller writes the command into shared memory, signals the
 * net core via the IPC peripheral, and busy-waits for the response.
 * A timeout prevents the app core from hanging if the net core
 * becomes unresponsive.
 *
 * \param type The command type (one of \ref nrf_ipc_cmd_type).
 * \param data Pointer to command payload, or NULL.
 * \param len  Length of command payload in bytes.
 * \return The result code from the net core (rsp.data[0]),
 *         or -1 on timeout.
 */
static uint8_t cmd_seq_counter;
/*---------------------------------------------------------------------------*/
static int
send_command(uint8_t type, const void *data, uint8_t len)
{
  clock_time_t start;
  uint8_t seq;

  if(len > NRF_IPC_MAX_DATA_LEN) {
    LOG_ERR("Command payload too long: %u\n", len);
    return -1;
  }

  /* Assign a sequence number so we can detect stale responses. */
  seq = ++cmd_seq_counter;

  /* Write the command. */
  shm->cmd.type = type;
  shm->cmd.len = len;
  if(data != NULL && len > 0) {
    memcpy((void *)shm->cmd.data, data, len);
  }

  /* Ensure data is written before setting the flag. */
  __DMB();

  shm->rsp_ready = 0;
  shm->cmd_seq = seq;
  shm->cmd_pending = 1;

  /* Signal the net core. */
  nrf_ipc_signal();

  /* Busy-wait for the response with timeout. */
  start = clock_time();
  while(!shm->rsp_ready || shm->rsp_seq != seq) {
    if(clock_time() - start >
       (clock_time_t)(NRF_IPC_CMD_TIMEOUT_MS * CLOCK_SECOND / 1000)) {
      LOG_ERR("Command %u timeout\n", type);
      /* Clear the pending flag to prevent the net core from processing
       * a stale command after the timeout. */
      shm->cmd_pending = 0;
      return -1;
    }
  }

  __DMB();

  return (int)shm->rsp.data[0];
}
/*---------------------------------------------------------------------------*/
static void
release_network_core(void)
{
  LOG_DBG("Releasing network core\n");

  /*
   * Do not grant the network core access to UART GPIO pins.
   * The net core's debug output is forwarded over IPC instead,
   * so the app core retains exclusive UART access for clean output.
   */

  /* Place the network MCU in the secure domain. */
  nrf_spu_extdomain_set(NRF_SPU, 0, true, false);

  /*
   * Release the network MCU from force-off.
   * NRF_RESET is a non-secure-only peripheral on nRF5340:
   * - In TZ secure mode: must use NS alias (SPU is freshly configured)
   * - In non-TZ mode: must use S alias (SPU may have stale state
   *   from recovery that blocks the NS alias)
   */
#ifdef TRUSTZONE_SECURE
  nrf_reset_network_force_off(NRF_RESET_NS, false);
#else
  nrf_reset_network_force_off(NRF_RESET, false);
#endif
}
/*---------------------------------------------------------------------------*/
static bool ipc_radio_initialized;
/*---------------------------------------------------------------------------*/
static int
ipc_radio_init(void)
{
  if(ipc_radio_initialized) {
    return 1;
  }

  LOG_DBG("Initializing IPC radio\n");

  /*
   * Force-hold the network core before touching shared memory.
   * FORCEOFF survives soft/pin resets, so the net core may still
   * be running from a previous session, causing bus stalls.
   */
#ifdef TRUSTZONE_SECURE
  nrf_reset_network_force_off(NRF_RESET_NS, true);
#else
  nrf_reset_network_force_off(NRF_RESET, true);
#endif

  /* Clear the shared memory area. */
  memset((void *)shm, 0, sizeof(*shm));
  shm->version = NRF_IPC_PROTOCOL_VERSION;

  /* Initialize the IPC transport on the app core. */
  nrf_ipc_init(&ipc_radio_process);

  /* Start the RX handler process. */
  process_start(&ipc_radio_process, NULL);

  /* Release the network core so it can boot. */
  release_network_core();

  /* Wait for the network core to initialize and signal readiness. */
  LOG_DBG("Waiting for network core...\n");
#ifdef TRUSTZONE_SECURE
  /*
   * In TrustZone mode, this runs inside an NSC call where
   * clock_time() may not advance (RTC interrupt preempted).
   * Use NRFX_WAIT_FOR which provides calibrated microsecond delays.
   */
  {
    bool ready;
    NRFX_WAIT_FOR(shm->net_ready, NET_CORE_INIT_TIMEOUT_MS, 1000, ready);
    if(!ready) {
      LOG_ERR("Network core init timeout\n");
      return 0;
    }
  }
#else
  {
    clock_time_t start = clock_time();
    while(!shm->net_ready) {
      if(clock_time() - start >
         (clock_time_t)(NET_CORE_INIT_TIMEOUT_MS * CLOCK_SECOND / 1000)) {
        LOG_ERR("Network core init timeout\n");
        return 0;
      }
    }
  }
#endif

  LOG_INFO("Network core ready (IPC protocol v%u)\n",
           (unsigned)shm->version);

  /* Send the init command to the net core's radio service. */
  if(send_command(NRF_IPC_CMD_INIT, NULL, 0) != 1) {
    LOG_ERR("Radio init command failed\n");
    return 0;
  }

  /*
   * Push this core's link-layer identity to the net core. The net core
   * seeds the radio from its own FICR-derived link address, which differs
   * from the app core's (each core has its own FICR DEVICEID). With the
   * nrf_802154 net-core radio (NRF_802154=1), frames are filtered and
   * auto-ACKed in hardware against these values, so without this push
   * unicast frames addressed to this node would be silently dropped. The
   * legacy raw radio service does not filter and answers NOT_SUPPORTED,
   * which is harmless.
   */
  {
    uint8_t pan_id[2];
    uint8_t short_addr[2];

    pan_id[0] = (uint8_t)(IEEE802154_PANID & 0xFF);
    pan_id[1] = (uint8_t)(IEEE802154_PANID >> 8);
    if(ipc_radio_set_object(RADIO_PARAM_PAN_ID, pan_id,
                            sizeof(pan_id)) != RADIO_RESULT_OK) {
      LOG_INFO("Net core radio does not take addresses (legacy service)\n");
    } else {
      short_addr[0] = linkaddr_node_addr.u8[LINKADDR_SIZE - 2];
      short_addr[1] = linkaddr_node_addr.u8[LINKADDR_SIZE - 1];
      ipc_radio_set_object(RADIO_PARAM_16BIT_ADDR, short_addr,
                           sizeof(short_addr));
#if LINKADDR_SIZE == 8
      ipc_radio_set_object(RADIO_PARAM_64BIT_ADDR,
                           linkaddr_node_addr.u8, LINKADDR_SIZE);
#endif
      LOG_INFO("Pushed link-layer address to net core\n");
    }
  }

  LOG_INFO("IPC radio operational"
#ifdef TRUSTZONE_SECURE
           " (TrustZone secure)"
#endif
           "\n");

  ipc_radio_initialized = true;
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_prepare(const void *payload, unsigned short payload_len)
{
  if(payload_len > NRF_IPC_MAX_FRAME_LEN) {
    LOG_ERR("Frame too long: %u\n", payload_len);
    return 1;
  }

  memcpy(tx_frame_buf, payload, payload_len);

  return 0;
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_transmit(unsigned short transmit_len)
{
  int result;

  if(transmit_len > sizeof(tx_frame_buf)) {
    LOG_ERR("Frame too long: %u\n", transmit_len);
    return RADIO_TX_ERR;
  }

  LOG_DBG("TX %u bytes\n", transmit_len);

  result = send_command(NRF_IPC_CMD_SEND, tx_frame_buf, transmit_len);

  /* send_command returns -1 on IPC timeout; map to a valid radio_tx_e. */
  return result < 0 ? RADIO_TX_ERR : result;
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_send(const void *payload, unsigned short payload_len)
{
  if(ipc_radio_prepare(payload, payload_len) != 0) {
    return RADIO_TX_ERR;
  }
  return ipc_radio_transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_read(void *buf, unsigned short buf_len)
{
  int len;

  if(!rx_frame_pending) {
    return 0;
  }

  len = rx_frame_len;
  if(len > buf_len) {
    len = buf_len;
  }

  memcpy(buf, rx_frame_buf, len);
  rx_frame_pending = false;

  return len;
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_channel_clear(void)
{
  int result = send_command(NRF_IPC_CMD_CCA, NULL, 0);

  /* Treat IPC timeout as "channel busy" so the caller backs off
   * rather than transmitting blindly. */
  return result < 0 ? 0 : result;
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_receiving_packet(void)
{
  int result = send_command(NRF_IPC_CMD_RECEIVING, NULL, 0);

  return result < 0 ? 0 : result;
}
/*---------------------------------------------------------------------------*/
/**
 * Pull a received frame from shared memory into the local buffer.
 *
 * This is the single code path for consuming frames from the net core.
 * It is called from both the polling context (pending_packet) and the
 * process thread (ipc_radio_process), ensuring consistent handling.
 *
 * \return 1 if a frame was pulled, 0 otherwise.
 */
static int
pull_rx_frame(void)
{
  int len;

  if(!shm->rx.pending) {
    return 0;
  }

  /* Ensure we read the data after observing pending == 1. */
  __DMB();

  len = shm->rx.len;
  if(len > 0 && len <= NRF_IPC_MAX_FRAME_LEN) {
    memcpy(rx_frame_buf, (const void *)shm->rx.data, len);
    rx_frame_len = len;
    rx_frame_pending = true;
  }

  /* Ensure the data copy completes before releasing the slot. */
  __DMB();

  shm->rx.pending = 0;
  return rx_frame_pending ? 1 : 0;
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_pending_packet(void)
{
  if(rx_frame_pending) {
    return 1;
  }

  /*
   * Check the dedicated ACK slot first. CSMA calls pending_packet()
   * + read() in a tight RTIMER_BUSYWAIT loop for ACK detection.
   * The IPC MAC puts ACK frames in shm->rx_ack (separate from data
   * frames in shm->rx) to prevent the ACK detection loop from
   * consuming and discarding data frames.
   */
  if(shm->rx_ack.pending) {
    __DMB();
    memcpy(rx_frame_buf, (const void *)shm->rx_ack.data, 3);
    rx_frame_len = 3;
    rx_frame_pending = true;
    __DMB();
    shm->rx_ack.pending = 0;
    return 1;
  }

  /*
   * Data frames are delivered via the process thread only
   * (ipc_radio_process checks shm->rx.pending). Do NOT call
   * pull_rx_frame() here — it would consume data frames that
   * CSMA then reads as 3-byte ACKs and discards.
   */
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_on(void)
{
  int result = send_command(NRF_IPC_CMD_ON, NULL, 0);

  return result < 0 ? 0 : result;
}
/*---------------------------------------------------------------------------*/
static int
ipc_radio_off(void)
{
  int result = send_command(NRF_IPC_CMD_OFF, NULL, 0);

  return result < 0 ? 0 : result;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
ipc_radio_get_value(radio_param_t param, radio_value_t *value)
{
  uint8_t cmd_data[2];
  int result;

  cmd_data[0] = (uint8_t)(param & 0xff);
  cmd_data[1] = (uint8_t)((param >> 8) & 0xff);

  result = send_command(NRF_IPC_CMD_GET_VALUE, cmd_data, 2);

  if(result < 0) {
    return RADIO_RESULT_ERROR;
  }

  if(result == RADIO_RESULT_OK) {
    /* Value is stored in rsp.data[1..4]. */
    memcpy(value, (const void *)&shm->rsp.data[1], sizeof(radio_value_t));
  }

  return (radio_result_t)result;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
ipc_radio_set_value(radio_param_t param, radio_value_t value)
{
  uint8_t cmd_data[2 + sizeof(radio_value_t)];
  int result;

  cmd_data[0] = (uint8_t)(param & 0xff);
  cmd_data[1] = (uint8_t)((param >> 8) & 0xff);
  memcpy(&cmd_data[2], &value, sizeof(radio_value_t));

  result = send_command(NRF_IPC_CMD_SET_VALUE, cmd_data, sizeof(cmd_data));

  return result < 0 ? RADIO_RESULT_ERROR : (radio_result_t)result;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
ipc_radio_get_object(radio_param_t param, void *dest, size_t size)
{
  uint8_t cmd_data[4];
  int result;

  cmd_data[0] = (uint8_t)(param & 0xff);
  cmd_data[1] = (uint8_t)((param >> 8) & 0xff);
  cmd_data[2] = (uint8_t)(size & 0xff);
  cmd_data[3] = (uint8_t)((size >> 8) & 0xff);

  result = send_command(NRF_IPC_CMD_GET_OBJECT, cmd_data, 4);

  if(result < 0) {
    return RADIO_RESULT_ERROR;
  }

  if(result == RADIO_RESULT_OK) {
    uint16_t rsp_size;
    memcpy(&rsp_size, (const void *)&shm->rsp.data[1], 2);
    if(rsp_size <= size) {
      memcpy(dest, (const void *)&shm->rsp.data[3], rsp_size);
    }
  }

  return (radio_result_t)result;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
ipc_radio_set_object(radio_param_t param, const void *src, size_t size)
{
  uint8_t cmd_data[NRF_IPC_MAX_DATA_LEN];
  int result;

  if(size > NRF_IPC_MAX_DATA_LEN - 4) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  cmd_data[0] = (uint8_t)(param & 0xff);
  cmd_data[1] = (uint8_t)((param >> 8) & 0xff);
  cmd_data[2] = (uint8_t)(size & 0xff);
  cmd_data[3] = (uint8_t)((size >> 8) & 0xff);
  memcpy(&cmd_data[4], src, size);

  result = send_command(NRF_IPC_CMD_SET_OBJECT, cmd_data, 4 + size);

  return result < 0 ? RADIO_RESULT_ERROR : (radio_result_t)result;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver ipc_radio_driver = {
  ipc_radio_init,
  ipc_radio_prepare,
  ipc_radio_transmit,
  ipc_radio_send,
  ipc_radio_read,
  ipc_radio_channel_clear,
  ipc_radio_receiving_packet,
  ipc_radio_pending_packet,
  ipc_radio_on,
  ipc_radio_off,
  ipc_radio_get_value,
  ipc_radio_set_value,
  ipc_radio_get_object,
  ipc_radio_set_object
};
/*---------------------------------------------------------------------------*/
/**
 * Drain the net core's log ring buffer and print each line
 * with a [NET] prefix for clear identification.
 */
static void
drain_net_log(void)
{
  static char line_buf[128];
  static uint8_t line_pos;
  uint16_t tail;
  uint16_t head;

  tail = shm->log.tail;
  head = shm->log.head;
  __DMB();

  /*
   * Drain at most one line per call to avoid blocking the process
   * thread when UART is slow (e.g., ITNS misconfigured after
   * recovery, causing the nrfx UARTE ISR to not fire).
   */
  while(tail != head) {
    char c = shm->log.data[tail];
    tail = (tail + 1) % NRF_IPC_LOG_BUF_SIZE;

    if(line_pos < sizeof(line_buf) - 1) {
      line_buf[line_pos++] = c;
    }

    if(c == '\n' || line_pos >= sizeof(line_buf) - 1) {
      line_buf[line_pos] = '\0';
      printf("[NET] %s", line_buf);
      line_pos = 0;
      break;  /* One line per call, return to process RX frames. */
    }
  }

  shm->log.tail = tail;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ipc_radio_process, ev, data)
{
  static struct etimer rx_poll_timer;

  PROCESS_BEGIN();

  /* Poll shared memory for RX frames every 50ms as a backup
   * in case the IPC interrupt path doesn't deliver them. */
  etimer_set(&rx_poll_timer, CLOCK_SECOND / 20);

  while(1) {
    PROCESS_WAIT_EVENT();

    drain_net_log();

    if(etimer_expired(&rx_poll_timer)) {
      etimer_restart(&rx_poll_timer);
    }


    /*
     * Check for a received frame from the net core. We must read
     * RSSI/LQI from shared memory before pull_rx_frame() releases
     * the slot, since the net core may overwrite them immediately.
     */
    if(shm->rx.pending) {
      int8_t rssi = shm->rx.rssi;
      uint8_t lqi = shm->rx.lqi;

      if(pull_rx_frame()) {
#ifdef TRUSTZONE_SECURE
        /*
         * In the secure world, we do not deliver directly to the MAC
         * (which runs in the normal world). Instead, notify the normal
         * world so it can read the frame via NSC calls.
         */
        tz_radio_notify_rx(rssi, lqi);
#else
        /* Deliver the frame to the MAC layer. */
        int len;
        packetbuf_clear();
        len = ipc_radio_read(packetbuf_dataptr(), PACKETBUF_SIZE);
        if(len > 0) {
          packetbuf_set_datalen(len);
          packetbuf_set_attr(PACKETBUF_ATTR_RSSI, (int)rssi);
          packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, lqi);
          LOG_DBG("RX %d bytes, delivering to MAC\n", len);
          NETSTACK_MAC.input();
        }
#endif /* TRUSTZONE_SECURE */
      }
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#endif /* NRF5340_XXAA_APPLICATION */
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
