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
/**
 * \addtogroup cc13xx-cc26xx-rf-sched
 * @{
 *
 * \file
 *        Implementation of the CC13xx/CC26xx RF scheduler.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/watchdog.h"
#include "sys/cc.h"
#include "sys/etimer.h"
#include "sys/process.h"
#include "sys/energest.h"
#include "net/netstack.h"
#include "net/packetbuf.h"
#include "net/mac/mac.h"
#include "lib/random.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_ble_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)
#if defined(DeviceFamily_CC13X0)
#include "driverlib/rf_ieee_mailbox.h"
#else
#include DeviceFamily_constructPath(driverlib/rf_ieee_mailbox.h)
#endif

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "rf/rf.h"
#include "rf/sched.h"
#include "rf/data-queue.h"
#include "rf/settings.h"
#include "rf/radio-mode.h"
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Radio"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
#define CMD_FS_RETRIES          3

#define RF_EVENTS_CMD_DONE      (RF_EventCmdDone | RF_EventLastCmdDone | \
                                 RF_EventFGCmdDone | RF_EventLastFGCmdDone)

#define CMD_STATUS(cmd)         (CC_ACCESS_NOW(RF_Op, cmd).status)

#define CMD_HANDLE_OK(handle)   (((handle) != RF_ALLOC_ERROR) && \
                                 ((handle) != RF_SCHEDULE_CMD_ERROR))

#define EVENTS_CMD_DONE(events) (((events) & RF_EVENTS_CMD_DONE) != 0)
/*---------------------------------------------------------------------------*/
/* BLE advertisement channel range (inclusive) */
#define BLE_ADV_CHANNEL_MIN     37
#define BLE_ADV_CHANNEL_MAX     39

/* Number of BLE advertisement channels */
#define NUM_BLE_ADV_CHANNELS    (BLE_ADV_CHANNEL_MAX - BLE_ADV_CHANNEL_MIN + 1)
/*---------------------------------------------------------------------------*/
/* Synth re-calibration every 3 minutes */
#define SYNTH_RECAL_INTERVAL    (CLOCK_SECOND * 60 * 3)
/* Set re-calibration interval with a jitter of 10 seconds */
#define SYNTH_RECAL_JITTER      (CLOCK_SECOND * 10)

static struct etimer synth_recal_timer;
/*---------------------------------------------------------------------------*/
#if (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X0_CC26X0)
typedef rfc_CMD_BLE_ADV_NC_t ble_cmd_adv_nc_t;
#elif (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X2_CC26X2)
typedef rfc_CMD_BLE5_ADV_NC_t ble_cmd_adv_nc_t;
#endif
/*---------------------------------------------------------------------------*/
static RF_Object rf_netstack;

#if RF_CONF_BLE_BEACON_ENABLE
static RF_Object rf_ble;
#endif

static volatile RF_CmdHandle cmd_rx_handle;

static volatile bool rf_is_on;
static volatile bool rf_start_recalib_timer;
static volatile bool rx_buf_full;

static rfc_CMD_SYNC_STOP_RAT_t netstack_cmd_stop_rat;
static rfc_CMD_SYNC_START_RAT_t netstack_cmd_start_rat;

simplelink_radio_mode_t *radio_mode;
/*---------------------------------------------------------------------------*/
static void
cmd_rx_cb(RF_Handle client, RF_CmdHandle command, RF_EventMask events)
{
  /* Unused arguments */
  (void)client;
  (void)command;

  if(radio_mode->poll_mode) {
    return;
  }

  if(events & RF_EventRxEntryDone) {
    process_poll(&rf_sched_process);
  }

  if(events & RF_EventRxBufFull) {
    rx_buf_full = true;
    process_poll(&rf_sched_process);
  }
}
/*---------------------------------------------------------------------------*/
static inline clock_time_t
synth_recal_interval(void)
{
  /*
   * Add jitter centered around SYNTH_RECAL_INTERVAL, giving a plus/minus
   * jitter seconds halved.
   */
  return SYNTH_RECAL_INTERVAL + (random_rand() % SYNTH_RECAL_JITTER) - (SYNTH_RECAL_JITTER / 2);
}
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

  if(is_active) {
    CMD_STATUS(netstack_cmd_rx) = DONE_STOPPED;
    RF_cancelCmd(&rf_netstack, cmd_rx_handle, RF_ABORT_GRACEFULLY);
    cmd_rx_handle = 0;
  }

  return (uint_fast8_t)is_active;
}
/*---------------------------------------------------------------------------*/
static rf_result_t
cmd_rx_restore(uint_fast8_t rx_key)
{
  const bool was_active = (rx_key != 0) ? true : false;

  if(!was_active) {
    return RF_RESULT_OK;
  }

  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority = RF_PriorityNormal;
  sched_params.endTime = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(netstack_cmd_rx) = PENDING;

  cmd_rx_handle = RF_scheduleCmd(
      &rf_netstack,
      (RF_Op *)&netstack_cmd_rx,
      &sched_params,
      cmd_rx_cb,
      RF_EventRxEntryDone | RF_EventRxBufFull);

  if(!CMD_HANDLE_OK(cmd_rx_handle)) {
    LOG_ERR("Unable to restore RX command, handle=%d status=0x%04x",
            cmd_rx_handle, CMD_STATUS(netstack_cmd_rx));
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
#if RF_CONF_BLE_BEACON_ENABLE
  RF_flushCmd(&rf_ble, RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);
#endif

  /* Stop SYNC RAT to get current RAT */
  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority = RF_PriorityNormal;
  sched_params.endTime = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(netstack_cmd_stop_rat) = PENDING;

  RF_scheduleCmd(
      &rf_netstack,
      (RF_Op *)&netstack_cmd_stop_rat,
      &sched_params,
      NULL,
      0);

  /* Trigger a manual power-down */
  RF_yield(&rf_netstack);
#if RF_CONF_BLE_BEACON_ENABLE
  RF_yield(&rf_ble);
#endif

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
  rf_is_on = false;

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
rf_result_t
rf_restart_rat(void)
{
  RF_ScheduleCmdParams sched_params;

  /* Stop SYNC RAT */
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority = RF_PriorityNormal;
  sched_params.endTime = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(netstack_cmd_stop_rat) = PENDING;

  RF_scheduleCmd(
      &rf_netstack,
      (RF_Op *)&netstack_cmd_stop_rat,
      &sched_params,
      NULL,
      0);

  /* Start SYNC RAT */
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority = RF_PriorityNormal;
  sched_params.endTime = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  netstack_cmd_start_rat.rat0 = 0;
  CMD_STATUS(netstack_cmd_start_rat) = PENDING;

  RF_scheduleCmd(
      &rf_netstack,
      (RF_Op *)&netstack_cmd_start_rat,
      &sched_params,
      NULL,
      0);

  return RF_RESULT_OK;
}

/*---------------------------------------------------------------------------*/
rf_result_t
rf_set_tx_power(RF_Handle handle, RF_TxPowerTable_Entry *table, int8_t dbm)
{
  const RF_Stat stat = RF_setTxPower(handle, RF_TxPowerTable_findValue(table, dbm));

  return (stat == RF_StatSuccess)
         ? RF_RESULT_OK
         : RF_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
rf_result_t
rf_get_tx_power(RF_Handle handle, RF_TxPowerTable_Entry *table, int8_t *dbm)
{
  *dbm = RF_TxPowerTable_findPowerLevel(table, RF_getTxPower(handle));

  return (*dbm != RF_TxPowerTable_INVALID_DBM)
         ? RF_RESULT_OK
         : RF_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
RF_Handle
netstack_open(RF_Params *params)
{
  netstack_cmd_stop_rat.commandNo = CMD_SYNC_STOP_RAT;
  netstack_cmd_stop_rat.condition.rule = COND_NEVER;
  netstack_cmd_start_rat.commandNo = CMD_SYNC_START_RAT;
  netstack_cmd_start_rat.condition.rule = COND_NEVER;
  return RF_open(&rf_netstack, &netstack_mode, (RF_RadioSetup *)&netstack_cmd_radio_setup, params);
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_sched_fs(void)
{
  const uint_fast8_t rx_key = cmd_rx_disable();

  /*
   * For IEEE-mode, restarting CMD_IEEE_RX re-calibrates the synth by using the
   * channel field in the CMD_IEEE_RX command. It is assumed this field is
   * already configured before this function is called. However, if
   * CMD_IEEE_RX wasn't active, manually calibrate the synth with CMD_FS.
   *
   * For Prop-mode, the synth is always manually calibrated with CMD_FS.
   */
#if (RF_MODE == RF_MODE_2_4_GHZ)
  if(rx_key) {
    cmd_rx_restore(rx_key);
    return RF_RESULT_OK;
  }
#endif /* RF_MODE == RF_MODE_2_4_GHZ */

  if(radio_mode->poll_mode) {
    /*
     * In poll mode; cannot execute the command, can just schedule it.
     * Retrying is not possible.
     */
    RF_ScheduleCmdParams sched_params;
    RF_ScheduleCmdParams_init(&sched_params);

    sched_params.priority = RF_PriorityNormal;
    sched_params.endTime = 0;
    sched_params.allowDelay = RF_AllowDelayAny;

    CMD_STATUS(netstack_cmd_fs) = PENDING;

    RF_CmdHandle fs_handle = RF_scheduleCmd(
        &rf_netstack,
        (RF_Op *)&netstack_cmd_fs,
        &sched_params,
        NULL,
        0);

    cmd_rx_restore(rx_key);

    if(!CMD_HANDLE_OK(fs_handle)) {
      LOG_ERR("Unable to schedule FS command, handle=%d status=0x%04x\n",
          fs_handle, CMD_STATUS(netstack_cmd_fs));
      return RF_RESULT_ERROR;
    }

    return RF_RESULT_OK;

  } else {
    /*
     * Not in poll mode. Execute the command immediately,
     * wait for result, retry if neccessary.
     */
    RF_EventMask events;
    bool synth_error = false;
    uint8_t num_tries = 0;

    do {
      CMD_STATUS(netstack_cmd_fs) = PENDING;

      events = RF_runCmd(
          &rf_netstack,
          (RF_Op *)&netstack_cmd_fs,
          RF_PriorityNormal,
          NULL,
          0);

      synth_error = (EVENTS_CMD_DONE(events)) && (CMD_STATUS(netstack_cmd_fs) == ERROR_SYNTH_PROG);

    } while(synth_error && (num_tries++ < CMD_FS_RETRIES));

    cmd_rx_restore(rx_key);

    return (CMD_STATUS(netstack_cmd_fs) == DONE_OK)
        ? RF_RESULT_OK
        : RF_RESULT_ERROR;
  }
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_sched_ieee_tx(uint16_t payload_length, bool ack_request)
{
  rf_result_t res;
  RF_EventMask tx_events = 0;

  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority = RF_PriorityNormal;
  sched_params.endTime = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  const bool rx_is_active = cmd_rx_is_active();
  const bool rx_needed = (ack_request && !rx_is_active);

  /*
   * If we expect ACK after transmission, RX must be running to be able to
   * run the RX_ACK command. Therefore, turn on RX before starting the
   * chained TX command.
   */
  if(rx_needed) {
    res = netstack_sched_rx(false);
    if(res != RF_RESULT_OK) {
      return res;
    }
  }

  CMD_STATUS(netstack_cmd_tx) = PENDING;

  RF_CmdHandle tx_handle = RF_scheduleCmd(
      &rf_netstack,
      (RF_Op *)&netstack_cmd_tx,
      &sched_params,
      NULL,
      0);

  if(!CMD_HANDLE_OK(tx_handle)) {
    LOG_ERR("Unable to schedule TX command, handle=%d status=0x%04x\n",
            tx_handle, CMD_STATUS(netstack_cmd_tx));
    return RF_RESULT_ERROR;
  }

  if(rx_is_active) {
    ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);
  } else {
    ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
  }

  /* Wait until TX operation finishes */
  if(radio_mode->poll_mode) {
    const uint16_t frame_length = payload_length + RADIO_PHY_HEADER_LEN + RADIO_PHY_OVERHEAD;
    RTIMER_BUSYWAIT_UNTIL((CMD_STATUS(netstack_cmd_tx) & 0xC00) != 0,
        US_TO_RTIMERTICKS(RADIO_BYTE_AIR_TIME * frame_length + 300));
  } else {
    tx_events = RF_pendCmd(&rf_netstack, tx_handle, 0);
  }

  /* Stop RX if it was turned on only for ACK */
  if(rx_needed) {
    netstack_stop_rx();
  }

  if(rx_is_active) {
    ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);
  } else {
    ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
  }

  if(radio_mode->poll_mode) {
    if(CMD_STATUS(netstack_cmd_tx) != IEEE_DONE_OK) {
      LOG_ERR("Pending on scheduled TX command generated error, status=0x%04x\n",
              CMD_STATUS(netstack_cmd_tx));
      return RF_RESULT_ERROR;
    }
  } else {
    if(!EVENTS_CMD_DONE(tx_events)) {
      LOG_ERR("Pending on TX comand generated error, events=0x%08llx status=0x%04x\n",
              tx_events, CMD_STATUS(netstack_cmd_tx));
      return RF_RESULT_ERROR;
    }
  }

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_sched_prop_tx(uint16_t payload_length)
{
  RF_EventMask tx_events = 0;

  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority = RF_PriorityNormal;
  sched_params.endTime = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(netstack_cmd_tx) = PENDING;

  RF_CmdHandle tx_handle = RF_scheduleCmd(
      &rf_netstack,
      (RF_Op *)&netstack_cmd_tx,
      &sched_params,
      NULL,
      0);

  if(!CMD_HANDLE_OK(tx_handle)) {
    LOG_ERR("Unable to schedule TX command, handle=%d status=0x%04x\n",
            tx_handle, CMD_STATUS(netstack_cmd_tx));
    return RF_RESULT_ERROR;
  }

  /*
   * Prop TX requires any on-going RX operation to be stopped to be
   * able to transmit. Therefore, disable RX if running.
   */
  const bool rx_key = cmd_rx_disable();

  if(rx_key) {
    ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);
  } else {
    ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
  }

  /* Wait until TX operation finishes */
  if(radio_mode->poll_mode) {
    const uint16_t frame_length = payload_length + RADIO_PHY_HEADER_LEN + RADIO_PHY_OVERHEAD;
    RTIMER_BUSYWAIT_UNTIL((CMD_STATUS(netstack_cmd_tx) & 0xC00) != 0,
        US_TO_RTIMERTICKS(RADIO_BYTE_AIR_TIME * frame_length + 1200));
  } else {
    tx_events = RF_pendCmd(&rf_netstack, tx_handle, 0);
  }

  cmd_rx_restore(rx_key);

  if(rx_key) {
    ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);
  } else {
    ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
  }

  if(radio_mode->poll_mode) {
    if(CMD_STATUS(netstack_cmd_tx) != PROP_DONE_OK) {
      LOG_ERR("Pending on scheduled TX command generated error, status=0x%04x\n",
              CMD_STATUS(netstack_cmd_tx));
      return RF_RESULT_ERROR;
    }
  } else {
    if(!EVENTS_CMD_DONE(tx_events)) {
      LOG_ERR("Pending on scheduled TX command generated error, events=0x%08llx status=0x%04x\n",
              tx_events, CMD_STATUS(netstack_cmd_tx));
      return RF_RESULT_ERROR;
    }
  }

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_sched_rx(bool start)
{
  if(cmd_rx_is_active()) {
    LOG_WARN("Already in RX when scheduling RX\n");
    return RF_RESULT_OK;
  }

  RF_ScheduleCmdParams sched_params;
  if(start) {
    /* Start SYNC RAT */
    RF_ScheduleCmdParams_init(&sched_params);

    sched_params.priority = RF_PriorityNormal;
    sched_params.endTime = 0;
    sched_params.allowDelay = RF_AllowDelayAny;

    netstack_cmd_start_rat.rat0 = 0;
    CMD_STATUS(netstack_cmd_start_rat) = PENDING;

    RF_scheduleCmd(
        &rf_netstack,
        (RF_Op *)&netstack_cmd_start_rat,
        &sched_params,
        NULL,
        0);
  }

  RF_ScheduleCmdParams_init(&sched_params);

  sched_params.priority = RF_PriorityNormal;
  sched_params.endTime = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  CMD_STATUS(netstack_cmd_rx) = PENDING;

  cmd_rx_handle = RF_scheduleCmd(
      &rf_netstack,
      (RF_Op *)&netstack_cmd_rx,
      &sched_params,
      cmd_rx_cb,
      RF_EventRxEntryDone | RF_EventRxBufFull);

  if(!CMD_HANDLE_OK(cmd_rx_handle)) {
    LOG_ERR("Unable to schedule RX command, handle=%d status=0x%04x\n",
            cmd_rx_handle, CMD_STATUS(netstack_cmd_rx));
    return RF_RESULT_ERROR;
  }

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);
  rf_is_on = true;

  if(start) {
    rf_start_recalib_timer = true;
    process_poll(&rf_sched_process);
  }

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
rf_result_t
netstack_stop_rx(void)
{
  if(!cmd_rx_is_active()) {
    LOG_WARN("RX not active when stopping RX\n");
    return RF_RESULT_OK;
  }

  CMD_STATUS(netstack_cmd_rx) = DONE_STOPPED;
  const RF_Stat stat = RF_cancelCmd(&rf_netstack, cmd_rx_handle, RF_ABORT_GRACEFULLY);
  cmd_rx_handle = 0;

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

  return (stat == RF_StatSuccess)
         ? RF_RESULT_OK
         : RF_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
RF_Handle
ble_open(RF_Params *params)
{
#if RF_CONF_BLE_BEACON_ENABLE
  return RF_open(&rf_ble, &ble_mode, (RF_RadioSetup *)&ble_cmd_radio_setup, params);

#else
  return (RF_Handle)NULL;
#endif
}
/*---------------------------------------------------------------------------*/
#if RF_CONF_BLE_BEACON_ENABLE
static RF_Op *
init_ble_adv_array(ble_cmd_adv_nc_t *ble_adv_array, uint8_t bm_channel)
{
  RF_Op *first_ble_adv = NULL;
  ble_cmd_adv_nc_t *cmd_adv_37 = &ble_adv_array[0];
  ble_cmd_adv_nc_t *cmd_adv_38 = &ble_adv_array[1];
  ble_cmd_adv_nc_t *cmd_adv_39 = &ble_adv_array[2];

  /* Setup channel 37 advertisement if enabled */
  if(bm_channel & BLE_ADV_CHANNEL_37) {
    /* Default configurations from ble_cmd_adv_nc */
    memcpy(cmd_adv_37, &ble_cmd_adv_nc, sizeof(ble_cmd_adv_nc));

    cmd_adv_37->channel = 37;
    /* Magic number: initialization for whitener, specific for channel 37 */
    cmd_adv_37->whitening.init = 0x65;

    /*
     * The next advertisement is chained depending on whether they are
     * enbled or not. If both 38 and 39 are disabled, then there is no
     * chaining.
     */
    if(bm_channel & BLE_ADV_CHANNEL_38) {
      cmd_adv_37->pNextOp = (RF_Op *)cmd_adv_38;
      cmd_adv_37->condition.rule = COND_ALWAYS;
    } else if(bm_channel & BLE_ADV_CHANNEL_39) {
      cmd_adv_37->pNextOp = (RF_Op *)cmd_adv_39;
      cmd_adv_37->condition.rule = COND_ALWAYS;
    } else {
      cmd_adv_37->pNextOp = NULL;
      cmd_adv_37->condition.rule = COND_NEVER;
    }

    /* Channel 37 will always be first if enabled */
    first_ble_adv = (RF_Op *)cmd_adv_37;
  }

  /* Setup channel 38 advertisement if enabled */
  if(bm_channel & BLE_ADV_CHANNEL_38) {
    memcpy(cmd_adv_38, &ble_cmd_adv_nc, sizeof(ble_cmd_adv_nc));

    cmd_adv_38->channel = 38;
    /* Magic number: initialization for whitener, specific for channel 38 */
    cmd_adv_38->whitening.init = 0x66;

    /*
     * The next advertisement is chained depending on whether they are
     * enbled or not. If 39 is disabled, then there is no chaining.
     */
    if(bm_channel & BLE_ADV_CHANNEL_39) {
      cmd_adv_38->pNextOp = (RF_Op *)cmd_adv_39;
      cmd_adv_38->condition.rule = COND_ALWAYS;
    } else {
      cmd_adv_38->pNextOp = NULL;
      cmd_adv_38->condition.rule = COND_NEVER;
    }

    /*
     * Channel 38 is only first if the first_ble_adv pointer is not
     * set by channel 37.
     */
    if(first_ble_adv == NULL) {
      first_ble_adv = (RF_Op *)cmd_adv_38;
    }
  }

  /* Setup channel 39 advertisement if enabled */
  if(bm_channel & BLE_ADV_CHANNEL_39) {
    memcpy(cmd_adv_39, &ble_cmd_adv_nc, sizeof(ble_cmd_adv_nc));

    cmd_adv_39->channel = 39;
    /* Magic number: initialization for whitener, specific for channel 39 */
    cmd_adv_39->whitening.init = 0x67;

    /* Channel 39 is always the last advertisement in the chain */
    cmd_adv_39->pNextOp = NULL;
    cmd_adv_39->condition.rule = COND_NEVER;

    /*
     * Channel 39 is only first if the first_ble_adv pointer is not
     * set by channel 37 or channel 38.
     */
    if(first_ble_adv == NULL) {
      first_ble_adv = (RF_Op *)cmd_adv_39;
    }
  }

  return first_ble_adv;
}
#endif /* RF_CONF_BLE_BEACON_ENABLE */
/*---------------------------------------------------------------------------*/
rf_result_t
ble_sched_beacons(uint8_t bm_channel)
{
#if RF_CONF_BLE_BEACON_ENABLE
  /*
   * Allocate the advertisement commands on the stack rather than statically
   * to RAM in order to save space. We don't need them after the
   * advertisements have been transmitted.
   */
  ble_cmd_adv_nc_t ble_cmd_adv_nc_array[NUM_BLE_ADV_CHANNELS];

  RF_Op *initial_adv = NULL;
  RF_ScheduleCmdParams sched_params;
  RF_CmdHandle beacon_handle;
  RF_EventMask beacon_events;
  rf_result_t rf_result;

  /* If no channels are mapped, then early return OK */
  if((bm_channel & BLE_ADV_CHANNEL_ALL) == 0) {
    return RF_RESULT_OK;
  }

  initial_adv = init_ble_adv_array(ble_cmd_adv_nc_array, bm_channel);

  if(initial_adv == NULL) {
    LOG_ERR("Initializing BLE Advertisement chain failed\n");
    return RF_RESULT_ERROR;
  }

  RF_ScheduleCmdParams_init(&sched_params);
  sched_params.priority = RF_PriorityNormal;
  sched_params.endTime = 0;
  sched_params.allowDelay = RF_AllowDelayAny;

  /*
   * The most efficient way to schedule the command is as follows:
   *   1. Schedule the BLE advertisement chain
   *   2. Reschedule the RX command IF it was running.
   *   3. Pend on the BLE avertisement chain
   */
  beacon_handle = RF_scheduleCmd(
    &rf_ble,
    initial_adv,
    &sched_params,
    NULL,
    0);

  if(!CMD_HANDLE_OK(beacon_handle)) {
    LOG_ERR("Unable to schedule BLE Beacon command, handle=%d status=0x%04x\n",
            beacon_handle, CMD_STATUS(ble_cmd_adv_nc));

    return RF_RESULT_ERROR;
  }

  /* Note that this only reschedules RX if it is running */
  rf_result = cmd_rx_restore(cmd_rx_disable());

  /* Wait until Beacon operation finishes */
  beacon_events = RF_pendCmd(&rf_ble, beacon_handle, 0);

  if(rf_result != RF_RESULT_OK) {
    LOG_ERR("Rescheduling CMD_RX failed when BLE advertising\n");

    return RF_RESULT_ERROR;
  }

  if(!EVENTS_CMD_DONE(beacon_events)) {
    LOG_ERR("Pending on scheduled BLE Beacon command generated error, events=0x%08llx status=0x%04x\n",
            beacon_events, CMD_STATUS(ble_cmd_adv_nc));

    return RF_RESULT_ERROR;
  }

  return RF_RESULT_OK;

#else
  return RF_RESULT_ERROR;
#endif
}
/*---------------------------------------------------------------------------*/
PROCESS(rf_sched_process, "RF Scheduler Process");
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rf_sched_process, ev, data)
{
  int len;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL((ev == PROCESS_EVENT_POLL) ||
                        (ev == PROCESS_EVENT_TIMER));

    /* start the synth re-calibration timer once. */
    if(rf_start_recalib_timer) {
      rf_start_recalib_timer = false;
      if(rf_is_on) {
        clock_time_t interval = synth_recal_interval();
        LOG_INFO("Starting synth re-calibration timer, next timeout %lu\n",
                 (unsigned long)interval);
        etimer_set(&synth_recal_timer, interval);
      }
    }

    if(ev == PROCESS_EVENT_POLL) {
      do {
        watchdog_periodic();

        packetbuf_clear();
        len = NETSTACK_RADIO.read(packetbuf_dataptr(), PACKETBUF_SIZE);

        /*
         * RX will stop if the RX buffers are full. In this case, restart
         * RX after we've freed at least on packet.
         */
        if(rx_buf_full) {
          LOG_ERR("RX buffer full, restart RX status=0x%04x\n", CMD_STATUS(netstack_cmd_rx));
          rx_buf_full = false;

          /* Restart RX. */
          netstack_stop_rx();
          netstack_sched_rx(false);
        }

        if(len > 0) {
          packetbuf_set_datalen(len);

          NETSTACK_MAC.input();
        }
        /* Only break when no more packets pending */
      } while(NETSTACK_RADIO.pending_packet());
    }

    /* Scheduling CMD_FS will re-calibrate the synth. */
    if((ev == PROCESS_EVENT_TIMER) &&
       etimer_expired(&synth_recal_timer)) {
      if(rf_is_on) {
        clock_time_t interval = synth_recal_interval();
        LOG_DBG("Re-calibrate synth, next interval %lu\n",
                (unsigned long)interval);
        netstack_sched_fs();
        etimer_set(&synth_recal_timer, interval);
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/** @} */
