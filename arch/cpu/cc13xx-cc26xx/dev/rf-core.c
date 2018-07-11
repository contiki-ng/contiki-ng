/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup simplelink
 * @{
 *
 * \file
 * Implementation of common CC13xx/CC26xx RF functionality
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/watchdog.h"
#include "sys/cc.h"
#include "sys/process.h"
#include "sys/energest.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/mac.h"

#include "rf-core.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "netstack-settings.h"
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE  "RF Core"
#define LOG_LEVEL   LOG_LEVEL_DBG
/*---------------------------------------------------------------------------*/
#ifdef NDEBUG
# define PRINTF(...)
#else
# define PRINTF(...)  printf(__VA_ARGS__)
#endif
/*---------------------------------------------------------------------------*/
#define CMD_FS_RETRIES          3

#define RF_EVENTS_CMD_DONE      (RF_EventCmdDone   | RF_EventLastCmdDone | \
                                 RF_EventFGCmdDone | RF_EventLastFGCmdDone)

#define CMD_STATUS(cmd)         (CC_ACCESS_NOW(RF_Op, cmd).status)

#define CMD_HANDLE_OK(handle)   (((handle) != RF_ALLOC_ERROR) && \
                                 ((handle) != RF_SCHEDULE_CMD_ERROR))

#define EVENTS_CMD_DONE(events) (((events) & RF_EVENTS_CMD_DONE) != 0)
/*---------------------------------------------------------------------------*/
static RF_Object  rf_netstack;
static RF_Object  rf_ble;

typedef struct {
  RF_CmdHandle  handle;
  RF_Callback   cb;
  RF_EventMask  bm_event;
} rf_cmd_t;

static rf_cmd_t   netstack_rx;
/*---------------------------------------------------------------------------*/
static inline bool
cmd_rx_is_active(void)
{
  /*
   * Active in this case means RX command is either running to be running,
   * that is ACTIVE for running or PENDING for to be running.
   */
  const uint16_t status = CMD_STATUS(netstack_cmd_rx);
  return (status == ACTIVE) ||
         (status == PENDING);
}
/*---------------------------------------------------------------------------*/
static uint_fast8_t
cmd_rx_disable(void)
{
  const bool is_active = cmd_rx_is_active();

  if (is_active) {
    CMD_STATUS(netstack_cmd_rx) = DONE_STOPPED;
    RF_cancelCmd(&rf_netstack, netstack_rx.handle, RF_ABORT_GRACEFULLY);
    netstack_rx.handle = 0;
  }

  return (uint_fast8_t)is_active;
}
/*---------------------------------------------------------------------------*/
static rf_result_t
cmd_rx_restore(uint_fast8_t rx_key)
{
  const bool was_active = (rx_key != 0) ? true : false;

  if (!was_active) {
    return RF_RESULT_OK;
  }

  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority   = RF_PriorityNormal;
  sched_params.endTime    = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(netstack_cmd_rx) = PENDING;

  netstack_rx.handle = RF_scheduleCmd(&rf_netstack,
    (RF_Op*)&netstack_cmd_rx,
    &sched_params,
    netstack_rx.cb,
    netstack_rx.bm_event
  );

  if (!CMD_HANDLE_OK(netstack_rx.handle)) {
    PRINTF("cmd_rx_restore: unable to schedule RX command handle=%d status=0x%04x",
           netstack_rx.handle, CMD_STATUS(netstack_cmd_rx));
    return RF_RESULT_ERROR;
  }

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
rf_result_t
rf_yield(void)
{
  /* Force abort of any ongoing RF operation */
  RF_flushCmd(&rf_netstack, RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);
  RF_flushCmd(&rf_ble,      RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);

  /* Trigger a manual power-down */
  RF_yield(&rf_netstack);
  RF_yield(&rf_ble);

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
rf_result_t
rf_set_tx_power(RF_Handle handle, RF_TxPowerTable_Entry *table, int8_t dbm)
{
  const RF_Stat stat = RF_setTxPower(handle,
    RF_TxPowerTable_findValue(table, dbm)
  );

  return (stat == RF_StatSuccess)
    ? RF_RESULT_OK
    : RF_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
rf_result_t
rf_get_tx_power(RF_Handle handle, RF_TxPowerTable_Entry *table, int8_t *dbm)
{
  *dbm = RF_TxPowerTable_findPowerLevel(table,
    RF_getTxPower(handle)
  );

  return (*dbm != RF_TxPowerTable_INVALID_DBM)
    ? RF_RESULT_OK
    : RF_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
RF_Handle
netstack_open(RF_Params *params)
{
  return RF_open(&rf_netstack, &netstack_mode, (RF_RadioSetup*)&netstack_cmd_radio_setup, params);
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_sched_fs(void)
{
  const uint_fast8_t rx_key = cmd_rx_disable();

  /*
   * For IEEE-mode, restarting IEEE_RX recalibrates the synth by using the
   * channel field in the IEEE_RX commando. It is assumed this field is
   * already configured before this functions is called.
   * However, if IEEE_RX wasn't active, manually calibrate the synth
   * with CMD_FS.
   *
   * For Prop-mode, the synth is always manually calibrated with CMD_FS.
   */
#if (RF_CORE_CONF_MODE == RF_CORE_MODE_2_4_GHZ)
  if (rx_key) {
    cmd_rx_restore(rx_key);
    return RF_RESULT_OK;
  }
#endif /* RF_CORE_CONF_MODE == RF_CORE_MODE_2_4_GHZ */

  RF_EventMask events;
  bool synth_error = false;
  uint8_t num_tries = 0;

  do {
    CMD_STATUS(netstack_cmd_fs) = PENDING;

    events = RF_runCmd(&rf_netstack,
      (RF_Op*)&netstack_cmd_fs,
      RF_PriorityNormal,
      NULL,
      0
    );

    synth_error = (EVENTS_CMD_DONE(events))
               && (CMD_STATUS(netstack_cmd_fs) == ERROR_SYNTH_PROG);

  } while (synth_error && (num_tries++ < CMD_FS_RETRIES));

  cmd_rx_restore(rx_key);

  return (CMD_STATUS(netstack_cmd_fs) == DONE_OK)
    ? RF_RESULT_OK
    : RF_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_sched_tx(RF_Callback cb, RF_EventMask bm_event)
{
  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority   = RF_PriorityNormal;
  sched_params.endTime    = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(netstack_cmd_tx) = PENDING;

  RF_CmdHandle tx_handle = RF_scheduleCmd(&rf_netstack,
    (RF_Op*)&netstack_cmd_tx,
    &sched_params,
    cb,
    bm_event
  );

  if (!CMD_HANDLE_OK(tx_handle)) {
    PRINTF("netstack_sched_tx: unable to schedule TX command handle=%d status=0x%04x\n",
           tx_handle, CMD_STATUS(netstack_cmd_tx));
    return RF_RESULT_ERROR;
  }

  /*
   * IEEE_TX can be scheduled while IEEE_RX is running, as the radio supports
   * FG commands to be scheduled while a BG command is running.
   * For Prop-mode, RX must be turned off as usual.
   */
  const uint_fast8_t rx_key = (RF_CORE_CONF_MODE != RF_CORE_MODE_2_4_GHZ)
    ? cmd_rx_disable()
    : false;

  if (rx_key) {
    ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);
  } else {
    ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
  }

  /* Wait until TX operation finishes */
  RF_EventMask tx_events = RF_pendCmd(&rf_netstack, tx_handle, 0);

  if (rx_key) {
    ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);
  } else {
    ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
  }

  cmd_rx_restore(rx_key);

  if (!EVENTS_CMD_DONE(tx_events)) {
    PRINTF("netstack_sched_tx: TX command pend error events=0x%08llx status=0x%04x\n",
           tx_events, CMD_STATUS(netstack_cmd_tx));
    return RF_RESULT_ERROR;
  }

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_sched_rx(RF_Callback cb, RF_EventMask bm_event)
{
  if (cmd_rx_is_active()) {
    PRINTF("netstack_sched_rx: already in RX\n");
    return RF_RESULT_OK;
  }

  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority   = RF_PriorityNormal;
  sched_params.endTime    = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(netstack_cmd_rx) = PENDING;

  netstack_rx.cb = cb;
  netstack_rx.bm_event = bm_event;
  netstack_rx.handle = RF_scheduleCmd(&rf_netstack,
    (RF_Op*)&netstack_cmd_rx,
    &sched_params,
    cb,
    bm_event
  );

  if (!CMD_HANDLE_OK(netstack_rx.handle)) {
    PRINTF("netstack_sched_rx: unable to schedule RX command handle=%d status=0x%04x\n",
           netstack_rx.handle, CMD_STATUS(netstack_cmd_rx));
    return RF_RESULT_ERROR;
  }

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_stop_rx(void)
{
  if (!cmd_rx_is_active()) {
    PRINTF("netstack_stop_rx: RX not active\n");
    return RF_RESULT_OK;
  }

  CMD_STATUS(netstack_cmd_rx) = DONE_STOPPED;
  const RF_Stat stat = RF_cancelCmd(&rf_netstack, netstack_rx.handle, RF_ABORT_GRACEFULLY);
  netstack_rx.handle = 0;

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

  return (stat == RF_StatSuccess)
    ? RF_RESULT_OK
    : RF_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
RF_Handle
ble_open(RF_Params *params)
{
  return RF_open(&rf_ble, &ble_mode, (RF_RadioSetup*)&ble_cmd_radio_setup, params);
}
/*---------------------------------------------------------------------------*/
rf_result_t
ble_sched_beacon(RF_Callback cb, RF_EventMask bm_event)
{
  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority   = RF_PriorityNormal;
  sched_params.endTime    = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(ble_cmd_beacon) = PENDING;

  RF_CmdHandle beacon_handle = RF_scheduleCmd(&rf_ble,
    (RF_Op*)&ble_cmd_beacon,
    &sched_params,
    cb,
    bm_event
  );

  if (!CMD_HANDLE_OK(beacon_handle)) {
    PRINTF("ble_sched_beacon: unable to schedule BLE Beacon command handle=%d status=0x%04x\n",
           beacon_handle, CMD_STATUS(ble_cmd_beacon));
    return RF_RESULT_ERROR;
  }

  const uint_fast8_t rx_key = cmd_rx_disable();

  /* Wait until Beacon operation finishes */
  RF_EventMask beacon_events = RF_pendCmd(&rf_ble, beacon_handle, 0);
  if (!EVENTS_CMD_DONE(beacon_events)) {
    PRINTF("ble_sched_beacon: Beacon command pend error events=0x%08llx status=0x%04x\n",
           beacon_events, CMD_STATUS(ble_cmd_beacon));

    cmd_rx_restore(rx_key);
    return RF_RESULT_ERROR;
  }

  cmd_rx_restore(rx_key);
  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
PROCESS(rf_core_process, "RF Core Process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rf_core_process, ev, data)
{
  int len;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    do {
      //watchdog_periodic();
      packetbuf_clear();
      len = NETSTACK_RADIO.read(packetbuf_dataptr(), PACKETBUF_SIZE);

      if(len > 0) {
        packetbuf_set_datalen(len);

        NETSTACK_MAC.input();
      }
    } while(len > 0);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/** @} */
