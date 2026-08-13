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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-ipc nRF5340 Inter-Processor Communication
 * @{
 *
 * \file
 *      IPC protocol definitions for nRF5340 dual-core communication
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */
/*---------------------------------------------------------------------------*/
#ifndef NRF_IPC_H_
#define NRF_IPC_H_
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/**
 * Protocol version. Both cores must agree on this value;
 * the net core checks it at startup and refuses to proceed
 * on mismatch.
 */
#define NRF_IPC_PROTOCOL_VERSION  1
/*---------------------------------------------------------------------------*/
/**
 * Shared memory address. Placed at the top of the application core's
 * RAM1 region (0x20060000--0x2007FFFF, 128 KB), which is accessible
 * from both cores. The nRF5340 application core has 512 KB total
 * SRAM (0x20000000--0x2007FFFF). This address must not overlap with
 * any linker-allocated region on either core.
 *
 * See nRF5340 Product Specification, Section "Memory Map" for details.
 */
#define NRF_IPC_SHARED_MEM_ADDR   0x20070000UL
/*---------------------------------------------------------------------------*/
/**
 * Maximum 802.15.4 frame size carried over IPC.
 *
 * The 802.15.4 PHY payload is 127 bytes; the nRF radio driver
 * strips the 2-byte FCS, yielding up to 125 data bytes. We use
 * 128 to provide a small margin for alignment and future use.
 */
#define NRF_IPC_MAX_FRAME_LEN     128
/*---------------------------------------------------------------------------*/
/**
 * Maximum data size in a command or response message.
 */
#define NRF_IPC_MAX_DATA_LEN      140
/*---------------------------------------------------------------------------*/
/**
 * Timeout in milliseconds for IPC command responses. If the net core
 * does not respond within this time, send_command() returns -1.
 */
#ifndef NRF_IPC_CMD_TIMEOUT_MS
#define NRF_IPC_CMD_TIMEOUT_MS    100
#endif
/*---------------------------------------------------------------------------*/
/**
 * Interval in seconds between net core heartbeat checks on the app core.
 * If the heartbeat counter has not advanced within this many seconds,
 * the app core logs a warning.
 */
#ifndef NRF_IPC_HEARTBEAT_INTERVAL_SEC
#define NRF_IPC_HEARTBEAT_INTERVAL_SEC  10
#endif
/*---------------------------------------------------------------------------*/
/**
 * Size of the log ring buffer for forwarding net core output to the app core.
 */
#define NRF_IPC_LOG_BUF_SIZE      2048
/*---------------------------------------------------------------------------*/
/**
 * IPC command types (app core -> net core).
 *
 * Each command is sent via the shared memory mailbox. The response
 * carries the result code in rsp.data[0]; additional response
 * payload (if any) starts at rsp.data[1].
 */
enum nrf_ipc_cmd_type {
  NRF_IPC_CMD_INIT = 1,     /**< Initialize the radio driver. */
  NRF_IPC_CMD_ON,           /**< Turn the radio on. */
  NRF_IPC_CMD_OFF,          /**< Turn the radio off. */
  NRF_IPC_CMD_SEND,         /**< Transmit a frame (data in cmd.data). */
  NRF_IPC_CMD_CCA,          /**< Perform Clear Channel Assessment. */
  NRF_IPC_CMD_RECEIVING,    /**< Check if a frame is being received. */
  NRF_IPC_CMD_PENDING,      /**< Check for pending received frames. */
  NRF_IPC_CMD_GET_VALUE,    /**< Get a radio parameter (radio_value_t). */
  NRF_IPC_CMD_SET_VALUE,    /**< Set a radio parameter (radio_value_t). */
  NRF_IPC_CMD_GET_OBJECT,   /**< Get a radio parameter (object/blob). */
  NRF_IPC_CMD_SET_OBJECT,   /**< Set a radio parameter (object/blob). */
  /**
   * Read radio diagnostic registers (nRF5340-specific).
   * Response payload (20 bytes):
   *   [0-3]   RADIO STATE register
   *   [4-7]   EVENTS_CRCOK
   *   [8-11]  EVENTS_CRCERROR
   *   [12-15] PACKETPTR
   *   [16-19] FREQUENCY register
   */
  NRF_IPC_CMD_DIAG,
};
/*---------------------------------------------------------------------------*/
/**
 * IPC message structure used for both commands and responses.
 */
struct nrf_ipc_msg {
  volatile uint8_t type;
  volatile uint8_t len;
  volatile uint8_t data[NRF_IPC_MAX_DATA_LEN];
};
/*---------------------------------------------------------------------------*/
/**
 * Shared memory layout between the application core and the network core.
 *
 * The command/response mailbox is used for synchronous operations:
 * the app core writes a command, signals the net core, and busy-waits
 * for the response.
 *
 * The RX area is used for asynchronous frame reception: the net core
 * writes a received frame and signals the app core.
 */
struct nrf_ipc_shared_mem {
  /** Protocol version for compatibility checking. */
  uint32_t version;

  /** Set to 1 by the net core when it has initialized the radio. */
  volatile uint32_t net_ready;

  /** Command mailbox (app -> net). */
  struct nrf_ipc_msg cmd;
  volatile uint8_t cmd_pending;
  volatile uint8_t cmd_seq;  /**< Sequence number set by app core. */

  /** Response mailbox (net -> app). */
  struct nrf_ipc_msg rsp;
  volatile uint8_t rsp_ready;
  volatile uint8_t rsp_seq;  /**< Echoed from cmd_seq by net core. */

  /** Received data frame (net -> app, asynchronous).
   *  Used for non-ACK frames delivered via the process thread. */
  struct {
    volatile uint8_t len;
    volatile uint8_t data[NRF_IPC_MAX_FRAME_LEN];
    volatile int8_t rssi;
    volatile uint8_t lqi;
    volatile uint8_t pending;
  } rx;

  /** Received ACK frame (net -> app, asynchronous).
   *  Separate from rx to avoid CSMA's ACK detection loop consuming
   *  data frames. Only 3-byte 802.15.4 ACK frames go here. */
  struct {
    volatile uint8_t data[3];
    volatile uint8_t pending;
  } rx_ack;

  /** Heartbeat counter (used as IPC MAC RX frame counter). */
  volatile uint32_t heartbeat;

  /** Log ring buffer (net -> app). Single-producer, single-consumer. */
  struct {
    volatile uint16_t head;    /**< Written by net core. */
    volatile uint16_t tail;    /**< Written by app core. */
    volatile uint32_t overflow; /**< Characters dropped due to full buffer. */
    volatile char data[NRF_IPC_LOG_BUF_SIZE];
  } log;
};
/*---------------------------------------------------------------------------*/
/*
 * Ensure the shared memory structure fits within the available region.
 * The region from NRF_IPC_SHARED_MEM_ADDR to end of RAM (0x20080000)
 * is 64 KB. This catches accidental growth from buffer size changes.
 */
_Static_assert(sizeof(struct nrf_ipc_shared_mem) <= 0x10000,
               "nrf_ipc_shared_mem exceeds 64 KB shared memory region");
/*---------------------------------------------------------------------------*/
/**
 * Get a pointer to the shared memory structure.
 */
#define NRF_IPC_SHARED_MEM \
  ((volatile struct nrf_ipc_shared_mem *)NRF_IPC_SHARED_MEM_ADDR)
/*---------------------------------------------------------------------------*/
/**
 * Initialize the IPC transport layer.
 *
 * Configures IPC channels and enables interrupts. The callback
 * process will be polled when an IPC signal is received.
 *
 * \param callback_proc Process to poll on IPC signal, or NULL.
 */
void nrf_ipc_init(struct process *callback_proc);
/*---------------------------------------------------------------------------*/
/**
 * Send an IPC signal to the other core.
 */
void nrf_ipc_signal(void);
/*---------------------------------------------------------------------------*/
#endif /* NRF_IPC_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
