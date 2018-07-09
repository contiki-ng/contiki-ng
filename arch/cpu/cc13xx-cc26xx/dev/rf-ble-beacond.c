/*
 * Copyright (c) 2015, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2017, University of Bristol - http://www.bristol.ac.uk/
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
 * \addtogroup rf-core-ble
 * @{
 *
 * \file
 * Implementation of the CC13xx/CC26xx RF BLE driver
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/process.h"
#include "sys/clock.h"
#include "sys/cc.h"
#include "sys/etimer.h"
#include "net/netstack.h"
#include "net/linkaddr.h"

#include "netstack-settings.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/rf_ble_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)

#include <ti/drivers/rf/RF.h>
#include <ti/drivers/dpl/HwiP.h>
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
#if RF_BLE_BEACOND_ENABLED
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_e
rf_ble_beacond_config(clock_time_t interval, const char *name)
{
  return RF_BLE_BEACOND_DISABLED;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_e
rf_ble_beacond_start(void)
{
  return RF_BLE_BEACOND_DISABLED;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_e rf_ble_beacond_stop(void)
{
  return RF_BLE_BEACOND_DISABLED;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_ble_is_active(void)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_e
rf_ble_set_tx_power(radio_value_t power)
{
  return RF_BLE_BEACOND_DISABLED;
}
/*---------------------------------------------------------------------------*/
radio_value_t
rf_ble_get_tx_power(void);
{
  return (radio_value_t)0;
}
/*---------------------------------------------------------------------------*/
#else /* RF_BLE_BEACOND_ENABLED */
/*---------------------------------------------------------------------------*/
/* BLE Advertisement channels. Not to be changed by the user. */
typedef enum {
  BLE_ADV_CHANNEL_37 = (1 << 0),
  BLE_ADV_CHANNEL_38 = (1 << 1),
  BLE_ADV_CHANNEL_39 = (1 << 2),

  BLE_ADV_CHANNEL_MASK = BLE_ADV_CHANNEL_37
                       | BLE_ADV_CHANNEL_38
                       | BLE_ADV_CHANNEL_39,
} ble_adv_channel_e;
/*---------------------------------------------------------------------------*/
/* Maximum BLE advertisement size. Not to be changed by the user. */
#define BLE_ADV_MAX_SIZE        31
/*---------------------------------------------------------------------------*/
/* BLE Intervals: Send a burst of advertisements every BLE_ADV_INTERVAL secs */
#define BLE_ADV_INTERVAL      (CLOCK_SECOND * 5)
#define BLE_ADV_DUTY_CYCLE    (CLOCK_SECOND / 10)
#define BLE_ADV_MESSAGES      10

/* BLE Advertisement-related macros */
#define BLE_ADV_TYPE_DEVINFO      0x01
#define BLE_ADV_TYPE_NAME         0x09
#define BLE_ADV_TYPE_MANUFACTURER 0xFF
#define BLE_ADV_NAME_BUF_LEN        BLE_ADV_MAX_SIZE
#define BLE_ADV_PAYLOAD_BUF_LEN     64
#define BLE_UUID_SIZE               16
/*---------------------------------------------------------------------------*/
typedef struct {
  /* Outgoing frame buffer */
  uint8_t tx_buf[BLE_ADV_PAYLOAD_BUF_LEN] CC_ALIGN(4);
  uint8_t ble_params_buf[32] CC_ALIGN(4);

  /* Config data */
  size_t adv_name_len;
  char adv_name[BLE_ADV_NAME_BUF_LEN];

  /* Indicates whether deamon is active or not */
  bool is_active;

  /* Periodic timer for sending out BLE advertisements */
  clock_time_t  ble_adv_interval;
  struct etimer ble_adv_et

  /* RF driver */
  RF_Object           rf_object;
  RF_Handle           rf_handle;
} ble_beacond_t;

static ble_beacond_t ble_beacond;
/*---------------------------------------------------------------------------*/
/* Configuration for TX power table */
#ifdef BLE_MODE_CONF_TX_POWER_TABLE
#   define TX_POWER_TABLE  BLE_MODE_CONF_TX_POWER_TABLE
#else
#   define TX_POWER_TABLE  rf_ble_tx_power_table
#endif

#ifdef BLE_MODE_CONF_TX_POWER_TABLE_SIZE
#   define TX_POWER_TABLE_SIZE  BLE_MODE_CONF_TX_POWER_TABLE_SIZE
#else
#   define TX_POWER_TABLE_SIZE  RF_BLE_TX_POWER_TABLE_SIZE
#endif

/* TX power table convenience macros */
#define TX_POWER_MIN  (TX_POWER_TABLE[0].power)
#define TX_POWER_MAX  (TX_POWER_TABLE[TX_POWER_TABLE_SIZE - 1].power)

#define TX_POWER_IN_RANGE(dbm)  (((dbm) >= TX_POWER_MIN) && ((dbm) <= TX_POWER_MAX))
/*---------------------------------------------------------------------------*/
PROCESS(ble_beacon_process, "CC13xx / CC26xx RF BLE Beacon Process");
/*---------------------------------------------------------------------------*/
static int
send_ble_adv_nc(int channel, uint8_t *adv_payload, int adv_payload_len)
{
  uint32_t cmd_status;
  rfc_CMD_BLE_ADV_NC_t cmd;
  rfc_bleAdvPar_t *params;

  params = (rfc_bleAdvPar_t *)ble_params_buf;

  /* Clear both buffers */
  memset(&cmd, 0x00, sizeof(cmd));
  memset(ble_params_buf, 0x00, sizeof(ble_params_buf));

  /* Adv NC */
  cmd.commandNo = CMD_BLE_ADV_NC;
  cmd.condition.rule = COND_NEVER;
  cmd.whitening.bOverride = 0;
  cmd.whitening.init = 0;
  cmd.pParams = params;
  cmd.channel = channel;

  /* Set up BLE Advertisement parameters */
  params->pDeviceAddress = (uint16_t *)BLE_ADDRESS_PTR;
  params->endTrigger.triggerType = TRIG_NEVER;
  params->endTime = TRIG_NEVER;

  /* Set up BLE Advertisement parameters */
  params = (rfc_bleAdvPar_t *)ble_params_buf;
  params->advLen = adv_payload_len;
  params->pAdvData = adv_payload;

  if(rf_core_send_cmd((uint32_t)&cmd, &cmd_status) == RF_CORE_CMD_ERROR) {
    PRINTF("send_ble_adv_nc: Chan=%d CMDSTA=0x%08lx, status=0x%04x\n",
           channel, cmd_status, cmd.status);
    return RF_CORE_CMD_ERROR;
  }

  /* Wait until the command is done */
  if(rf_core_wait_cmd_done(&cmd) != RF_CORE_CMD_OK) {
    PRINTF("send_ble_adv_nc: Chan=%d CMDSTA=0x%08lx, status=0x%04x\n",
           channel, cmd_status, cmd.status);
    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_e
rf_ble_beacond_config(clock_time_t interval, const char *name)
{
  if(name != NULL) {
    const size_t name_len = strlen(name);
    if(name_len == 0 || name_len >= BLE_ADV_NAME_BUF_LEN) {
      return RF_BLE_BEACOND_ERROR;
    }

    /* name_len + 1 for zero termination char */
    ble_beacond_cfg.adv_name_len = name_len + 1;
    memcpy(ble_beacond_cfg.adv_name, name, name_len + 1);
  }

  if(interval != 0) {
    ble_beacond_cfg.ble_adv_interval = interval;
  }

  return RF_BLE_BEACOND_OK;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_e
rf_ble_beacond_start(void)
{
  /* Check if RF handle has already been opened */
  if (ble_beacond.rf_handle != NULL) {
    return RF_BLE_BEACOND_OK;
  }

  /* Sanity check of Beacond config */
  if(beacond_config.adv_name_len == 0) {
    return RF_BLE_BEACOND_ERROR;
  }

  /* Open RF handle */
  RF_Params rf_params;
  RF_Params_init(&rf_params);

  ble_beacond.rf_handle = RF_open(&ble_beacond.rf_object, &rf_ble_mode,
                                  (RF_RadioSetup*)&rf_cmd_ieee_radio_setup, &rf_params);

  if (ble_beacond.rf_handle == NULL) {
    PRINTF("init: unable to open BLE RF driver\n");
    return RF_BLE_BEACOND_ERROR;
  }

  ble_beacond.is_active = false;

  process_start(&ble_beacon_process, NULL);

  return RF_BLE_BEACOND_OK;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_e
rf_ble_beacond_stop()
{
  if (ble_beacond.rf_handle == NULL) {
    return RF_BLE_BEACOND_OK;
  }

  /* Force abort of any ongoing RF operation */
  RF_flushCmd(ieee_radio.rf_handle, RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);
  RF_close(ble_beacond.rf_handle);

  ble_beacond.rf_handle = NULL;

  process_exit(&ble_beacon_process);

  return RF_BLE_BEACOND_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_ble_is_active()
{
  return ble_beacon_on;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_e
rf_ble_set_tx_power(radio_value_t dBm)
{
  if (!TX_POWER_IN_RANGE(dBm)) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  const RF_Stat stat = RF_setTxPower(
    ble_beacond.rf_handle,
    RF_TxPowerTable_findValue(TX_POWER_TABLE, (int8_t)dBm)
  );

  return (stat == RF_StatSuccess)
    ? RF_BLE_BEACOND_OK
    : RF_BLE_BEACOND_ERROR;
}
/*---------------------------------------------------------------------------*/
radio_value_t
rf_ble_get_tx_power(void)
{
  return (radio_value_t)RF_TxPowerTable_findPowerLevel(
    TX_POWER_TABLE,
    RF_getTxPower(ble_beacond.rf_handle)
  );
}
/*---------------------------------------------------------------------------*/
static rf_ble_beacond_result_e
rf_ble_beacon_single(uint8_t channel, uint8_t *data, uint8_t len)
{
  uint32_t cmd_status;
  bool interrupts_disabled;
  uint8_t j, channel_selected;
  uint8_t was_on;

  /* Adhere to the maximum BLE advertisement payload size */
  if(len > BLE_ADV_NAME_BUF_LEN) {
    len = BLE_ADV_NAME_BUF_LEN;
  }

  /*
   * Under ContikiMAC, some IEEE-related operations will be called from an
   * interrupt context. We need those to see that we are in BLE mode.
   */
  const uintptr_t key = HwiP_disable();

  /*
   * First, determine our state:
   *
   * If we are running CSMA, we are likely in IEEE RX mode. We need to
   * abort the IEEE BG Op before entering BLE mode.
   * If we are ContikiMAC, we are likely off, in which case we need to
   * boot the CPE before entering BLE mode
   */
  was_on = rf_core_is_accessible();

  if(was_on) {
    /*
     * We were on: If we are in the process of receiving a frame, abort the
     * BLE beacon burst. Otherwise, terminate the primary radio Op so we
     * can switch to BLE mode
     */
    if(NETSTACK_RADIO.receiving_packet()) {
      PRINTF("rf_ble_beacon_single: We were receiving\n");

      /* Abort this pass */
      return;
    }

    rf_core_primary_mode_abort();
  } else {

    oscillators_request_hf_xosc();

    /* We were off: Boot the CPE */
    if(rf_core_boot() != RF_CORE_CMD_OK) {
      /* Abort this pass */
      PRINTF("rf_ble_beacon_single: rf_core_boot() failed\n");
      return;
    }

    oscillators_switch_to_hf_xosc();

    /* Enter BLE mode */
    if(rf_radio_setup() != RF_CORE_CMD_OK) {
      /* Continue so we can at least try to restore our previous state */
      PRINTF("rf_ble_beacon_single: Error entering BLE mode\n");
    } else {

      for(j = 0; j < 3; j++) {
        channel_selected = (channel >> j) & 0x01;
        if(channel_selected == 1) {
          if(send_ble_adv_nc(37 + j, data, len) != RF_CORE_CMD_OK) {
            /* Continue... */
            PRINTF("rf_ble_beacon_single: Channel=%d, "
                   "Error advertising\n", 37 + j);
          }
        }
      }
    }

    /* Send a CMD_STOP command to RF Core */
    if(rf_core_send_cmd(CMDR_DIR_CMD(CMD_STOP), &cmd_status) != RF_CORE_CMD_OK) {
      /* Continue... */
      PRINTF("rf_ble_beacon_single: status=0x%08lx\n", cmd_status);
    }

    if(was_on) {
      /* We were on, go back to previous primary mode */
      rf_core_primary_mode_restore();
    } else {
      /* We were off. Shut back off */
      rf_core_power_down();

      /* Switch HF clock source to the RCOSC to preserve power */
      oscillators_switch_to_hf_rc();
    }
  }

  HwiP_restore(key);
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ble_beacon_process, ev, data)
{
  static size_t i;
  static size_t p;

  PROCESS_BEGIN();

  while(1) {
    etimer_set(&beacond_config.ble_adv_et, beacond_config.ble_adv_interval);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ble_adv_et) ||
                             (ev == PROCESS_EVENT_EXIT));

    if(ev == PROCESS_EVENT_EXIT) {
      PROCESS_EXIT();
    }

    /* Set the adv payload each pass: The device name may have changed */
    p = 0;

    /* device info */
    payload[p++] = (uint8_t)0x02;          /* 2 bytes */
    payload[p++] = (uint8_t)BLE_ADV_TYPE_DEVINFO;
    payload[p++] = (uint8_t)0x1a;          /* LE general discoverable + BR/EDR */
    payload[p++] = (uint8_t)beacond_config.adv_name_len;
    payload[p++] = (uint8_t)BLE_ADV_TYPE_NAME;
    memcpy(payload + p, beacond_config.adv_name, beacond_config.adv_name_len);
    p += beacond_config.adv_name_len;

    /*
     * Send BLE_ADV_MESSAGES beacon bursts. Each burst on all three
     * channels, with a BLE_ADV_DUTY_CYCLE interval between bursts
     */
    rf_ble_beacon_single(BLE_ADV_CHANNEL_ALL, payload, p);
    for(i = 1; i < BLE_ADV_MESSAGES; i++) {
      etimer_set(&beacond_config.ble_adv_et, BLE_ADV_DUTY_CYCLE);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ble_adv_et));

      rf_ble_beacon_single(BLE_ADV_CHANNEL_ALL, payload, p);
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#endif /* RF_BLE_BEACOND_ENABLED */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
