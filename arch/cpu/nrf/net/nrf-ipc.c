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
 *      IPC transport layer for nRF5340 inter-core communication
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "nrf-ipc.h"
#include "nrf.h"
#include "nrfx_config.h"
/*---------------------------------------------------------------------------*/
/* Only compile on nRF5340 platforms that have the IPC peripheral. */
#ifdef NRF_IPC
/*---------------------------------------------------------------------------*/
#include "hal/nrf_ipc.h"
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "IPC"
#define LOG_LEVEL LOG_LEVEL_DBG
/*---------------------------------------------------------------------------*/
/*
 * IPC channel assignments:
 * - Channel 0: app core -> net core
 * - Channel 1: net core -> app core
 *
 * Both cores use TASKS_SEND[0] to send and EVENTS_RECEIVE[0] to receive,
 * but on different physical channels.
 */
#define IPC_APP_TO_NET_CHANNEL  0
#define IPC_NET_TO_APP_CHANNEL  1
/*---------------------------------------------------------------------------*/
static struct process *ipc_process;
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialize the IPC transport layer.
 *
 * Configures IPC send/receive channels based on which core is
 * running, enables the receive interrupt, and registers a process
 * to poll when an IPC signal arrives from the other core.
 *
 * Channel assignment:
 * - Channel 0: app core -> net core
 * - Channel 1: net core -> app core
 *
 * \param callback_proc  Process to poll on IPC receive, or NULL.
 */
void
nrf_ipc_init(struct process *callback_proc)
{
  ipc_process = callback_proc;

#if defined(NRF5340_XXAA_APPLICATION)
  /*
   * App core sends on channel 0, receives on channel 1.
   * SEND_CNF[0]: TASKS_SEND[0] triggers channel 0
   * RECEIVE_CNF[0]: channel 1 triggers EVENTS_RECEIVE[0]
   */
  nrf_ipc_send_config_set(NRF_IPC, 0,
                           (1UL << IPC_APP_TO_NET_CHANNEL));
  nrf_ipc_receive_config_set(NRF_IPC, 0,
                              (1UL << IPC_NET_TO_APP_CHANNEL));
#elif defined(NRF5340_XXAA_NETWORK)
  /*
   * Net core sends on channel 1, receives on channel 0.
   * SEND_CNF[0]: TASKS_SEND[0] triggers channel 1
   * RECEIVE_CNF[0]: channel 0 triggers EVENTS_RECEIVE[0]
   */
  nrf_ipc_send_config_set(NRF_IPC, 0,
                           (1UL << IPC_NET_TO_APP_CHANNEL));
  nrf_ipc_receive_config_set(NRF_IPC, 0,
                              (1UL << IPC_APP_TO_NET_CHANNEL));
#endif

  /* Clear any pending events and enable the receive interrupt. */
  nrf_ipc_event_clear(NRF_IPC, nrf_ipc_receive_event_get(0));
  nrf_ipc_int_enable(NRF_IPC, (1UL << 0));

  NVIC_ClearPendingIRQ(IPC_IRQn);
  NVIC_EnableIRQ(IPC_IRQn);

  LOG_DBG("IPC initialized\n");
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Send an IPC signal to the other core.
 *
 * Triggers TASKS_SEND[0], which maps to the appropriate physical
 * IPC channel based on the configuration set by nrf_ipc_init().
 */
void
nrf_ipc_signal(void)
{
  nrf_ipc_task_trigger(NRF_IPC, nrf_ipc_send_task_get(0));
}
/*---------------------------------------------------------------------------*/
/**
 * \brief IPC interrupt handler.
 *
 * Called when the other core triggers an IPC signal on our receive
 * channel. Clears the event and polls the registered process.
 */
void
IPC_IRQHandler(void)
{
  if(nrf_ipc_event_check(NRF_IPC, nrf_ipc_receive_event_get(0))) {
    nrf_ipc_event_clear(NRF_IPC, nrf_ipc_receive_event_get(0));

    if(ipc_process != NULL) {
      process_poll(ipc_process);
    }
    /* In TrustZone mode, the secure world's ipc_radio_process will
     * run when the normal world calls tz_api_poll(). RX frame
     * delivery is handled via the tz_radio_notify_rx callback. */
  }
}
/*---------------------------------------------------------------------------*/
#endif /* NRF_IPC */
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
