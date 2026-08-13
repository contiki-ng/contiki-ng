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
 *      IPC radio service for the nRF5340 network core.
 *
 *      This is a stripped-down Contiki-NG application that runs on the
 *      network core and provides radio services to the application core
 *      via IPC. It receives commands from the app core, calls the
 *      appropriate nrf_ieee_driver functions, and returns results.
 *
 *      The network core runs Contiki-NG with NULLNET/NULLMAC/NULLROUTING
 *      so only the core OS (processes, timers, radio driver) is active.
 *
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "nrf-ipc.h"
#include "nrf.h"
#include <inttypes.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "IPC Svc"
#define LOG_LEVEL LOG_LEVEL_INFO
/*---------------------------------------------------------------------------*/
static volatile struct nrf_ipc_shared_mem *shm = NRF_IPC_SHARED_MEM;
/*---------------------------------------------------------------------------*/
PROCESS(ipc_radio_service, "IPC Radio Service");
AUTOSTART_PROCESSES(&ipc_radio_service);
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/**
 * Send a response to the app core via the shared memory mailbox.
 *
 * \param result   Result code placed in rsp.data[0].
 * \param data     Optional response payload, or NULL.
 * \param data_len Length of the response payload in bytes.
 */
static void
send_response(uint8_t result, const void *data, uint8_t data_len)
{
  if(data_len > NRF_IPC_MAX_DATA_LEN - 1) {
    LOG_ERR("Response payload too long: %u\n", data_len);
    data = NULL;
    data_len = 0;
    result = RADIO_TX_ERR;
  }

  shm->rsp.type = shm->cmd.type;
  shm->rsp.data[0] = result;
  if(data != NULL && data_len > 0) {
    memcpy((void *)&shm->rsp.data[1], data, data_len);
  }
  shm->rsp.len = 1 + data_len;
  shm->rsp_seq = shm->cmd_seq;

  __DMB();

  shm->rsp_ready = 1;
  shm->cmd_pending = 0;

  nrf_ipc_signal();
}
/*---------------------------------------------------------------------------*/
/**
 * Dispatch and execute a command received from the app core.
 *
 * Reads the command type from shared memory, calls the corresponding
 * radio driver function, and sends the result back via send_response().
 */
static void
handle_command(void)
{
  uint8_t type;
  int result;
  radio_param_t param;
  radio_value_t value;
  uint16_t size;

  type = shm->cmd.type;

  /* Validate command length before accessing data. */
  if(shm->cmd.len > NRF_IPC_MAX_DATA_LEN) {
    LOG_ERR("Invalid command length: %u\n", shm->cmd.len);
    send_response(RADIO_TX_ERR, NULL, 0);
    return;
  }

  switch(type) {
  case NRF_IPC_CMD_INIT:
    LOG_DBG("CMD: init\n");
    result = NETSTACK_RADIO.init();
    LOG_DBG("CMD: init result=%d\n", result);
    /* Remap to the IPC INIT response convention (1 = OK, 0 = failure).
     * The radio driver's init() returns RADIO_TX_OK (= 0) on success. */
    send_response(result == RADIO_TX_OK ? 1 : 0, NULL, 0);
    break;

  case NRF_IPC_CMD_ON:
    LOG_DBG("CMD: on\n");
    result = NETSTACK_RADIO.on();
    send_response(result, NULL, 0);
    break;

  case NRF_IPC_CMD_OFF:
    LOG_DBG("CMD: off\n");
    result = NETSTACK_RADIO.off();
    send_response(result, NULL, 0);
    break;

  case NRF_IPC_CMD_SEND:
    LOG_DBG("CMD: send %u bytes\n", shm->cmd.len);
    result = NETSTACK_RADIO.send((const void *)shm->cmd.data, shm->cmd.len);
    /*
     * After TX, the remote node sends an ACK within ~192us.
     * The ACK triggers the radio ISR which polls the radio driver
     * process, but that process won't run until the next
     * process_run(). We must let it run NOW so the IPC MAC can
     * forward the ACK to shared memory before we return the send
     * result — otherwise the app core's CSMA times out (~400us)
     * waiting for the ACK.
     */
    process_run();
    send_response(result, NULL, 0);
    break;

  case NRF_IPC_CMD_CCA:
    LOG_DBG("CMD: CCA\n");
    result = NETSTACK_RADIO.channel_clear();
    send_response(result, NULL, 0);
    break;

  case NRF_IPC_CMD_RECEIVING:
    result = NETSTACK_RADIO.receiving_packet();
    send_response(result, NULL, 0);
    break;

  case NRF_IPC_CMD_PENDING:
    result = NETSTACK_RADIO.pending_packet();
    send_response(result, NULL, 0);
    break;

  case NRF_IPC_CMD_GET_VALUE:
    param = (radio_param_t)shm->cmd.data[0]
            | ((radio_param_t)shm->cmd.data[1] << 8);
    result = NETSTACK_RADIO.get_value(param, &value);
    send_response(result, &value, sizeof(value));
    break;

  case NRF_IPC_CMD_SET_VALUE:
    param = (radio_param_t)shm->cmd.data[0]
            | ((radio_param_t)shm->cmd.data[1] << 8);
    memcpy(&value, (const void *)&shm->cmd.data[2], sizeof(value));
    result = NETSTACK_RADIO.set_value(param, value);
    send_response(result, NULL, 0);
    break;

  case NRF_IPC_CMD_GET_OBJECT:
    param = (radio_param_t)shm->cmd.data[0]
            | ((radio_param_t)shm->cmd.data[1] << 8);
    memcpy(&size, (const void *)&shm->cmd.data[2], sizeof(size));
    if(size > NRF_IPC_MAX_DATA_LEN - 3) {
      send_response(RADIO_RESULT_INVALID_VALUE, NULL, 0);
    } else {
      uint8_t obj_buf[NRF_IPC_MAX_DATA_LEN - 3];
      result = NETSTACK_RADIO.get_object(param, obj_buf, size);
      if(result == RADIO_RESULT_OK) {
        uint8_t rsp_data[2 + sizeof(obj_buf)];
        memcpy(rsp_data, &size, 2);
        memcpy(&rsp_data[2], obj_buf, size);
        send_response(result, rsp_data, 2 + size);
      } else {
        send_response(result, NULL, 0);
      }
    }
    break;

  case NRF_IPC_CMD_SET_OBJECT:
    param = (radio_param_t)shm->cmd.data[0]
            | ((radio_param_t)shm->cmd.data[1] << 8);
    memcpy(&size, (const void *)&shm->cmd.data[2], sizeof(size));
    result = NETSTACK_RADIO.set_object(param,
                                       (const void *)&shm->cmd.data[4],
                                       size);
    send_response(result, NULL, 0);
    break;

  case NRF_IPC_CMD_DIAG: {
    /* Return radio diagnostic data:
     * [0-3]: RADIO STATE register
     * [4-7]: EVENTS_CRCOK
     * [8-11]: EVENTS_CRCERROR
     * [12-15]: PACKETPTR
     * [16-19]: FREQUENCY register
     */
    uint8_t diag[20];
    uint32_t val;
    val = NRF_RADIO_NS->STATE;
    memcpy(&diag[0], &val, 4);
    val = NRF_RADIO_NS->EVENTS_CRCOK;
    memcpy(&diag[4], &val, 4);
    val = NRF_RADIO_NS->EVENTS_CRCERROR;
    memcpy(&diag[8], &val, 4);
    val = NRF_RADIO_NS->PACKETPTR;
    memcpy(&diag[12], &val, 4);
    val = NRF_RADIO_NS->FREQUENCY;
    memcpy(&diag[16], &val, 4);
    send_response(0, diag, sizeof(diag));
    break;
  }

  default:
    LOG_ERR("Unknown command: %u\n", type);
    send_response(RADIO_TX_ERR, NULL, 0);
    break;
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ipc_radio_service, ev, data)
{
  PROCESS_BEGIN();

  LOG_INFO("IPC Radio Service starting\n");

  /* Initialize the IPC transport on the net core. */
  nrf_ipc_init(&ipc_radio_service);

  /* Verify protocol version written by the app core. */
  if(shm->version != NRF_IPC_PROTOCOL_VERSION) {
    LOG_ERR("Protocol version mismatch: expected %u, got %" PRIu32 "\n",
            NRF_IPC_PROTOCOL_VERSION, shm->version);
    /* Do not set net_ready -- the app core will timeout and report
     * the failure. Halt the net core to prevent undefined behavior. */
    while(1) {
      __WFI();
    }
  }

  /* Signal the app core that the net core is ready. */
  shm->net_ready = 1;
  __DMB();
  nrf_ipc_signal();

  LOG_INFO("Network core ready, waiting for commands\n");

  /*
   * Event-driven command loop. The process wakes only when the app
   * core sends an IPC command (IPC_IRQHandler polls this process).
   *
   * Radio frame reception is handled entirely by interrupt:
   * radio ISR -> nrf_ieee_rf_process -> ipc_mac_driver.input()
   * which forwards the frame to the app core via shared memory.
   * No polling or timers are needed for RX.
   *
   * Between events, the Contiki-NG main loop calls platform_idle()
   * which executes WFI, putting the CPU to sleep. This is critical:
   * 1. Reduces power consumption on the network core.
   * 2. Stops shared SRAM bus traffic so the application core can
   *    boot cleanly even if the net core was left running from a
   *    previous session (FORCEOFF survives soft/pin resets).
   */
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_POLL);

    /* Check for IPC commands from the app core. */
    if(shm->cmd_pending) {
      __DMB();
      handle_command();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
