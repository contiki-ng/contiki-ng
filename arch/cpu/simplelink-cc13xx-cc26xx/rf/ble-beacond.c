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
 * \addtogroup cc13xx-cc26xx-rf-ble
 * @{
 *
 * \file
 *        Implementation for the CC13xx/CC26xx BLE Beacon Daemon.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/cc.h"
#include "sys/clock.h"
#include "sys/etimer.h"
#include "sys/process.h"
#include "net/linkaddr.h"
#include "net/netstack.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/chipinfo.h)
#include DeviceFamily_constructPath(driverlib/rf_ble_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "rf/rf.h"
#include "rf/sched.h"
#include "rf/ble-addr.h"
#include "rf/ble-beacond.h"
#include "rf/tx-power.h"
#include "rf/settings.h"
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Radio"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
#if RF_CONF_BLE_BEACON_ENABLE
/*---------------------------------------------------------------------------*/
/* Maximum BLE advertisement size. Not to be changed by the user. */
#define BLE_ADV_MAX_SIZE            31
/*---------------------------------------------------------------------------*/
/*
 * BLE Intervals: Send a burst of advertisements every BLE_ADV_INTERVAL
 * specified in milliseconds.
 */
#define BLE_ADV_INTERVAL                        ((100 * CLOCK_SECOND) / 1000)

/* GAP Advertisement data types */
#define BLE_ADV_TYPE_FLAGS                      0x01
#define BLE_ADV_TYPE_16BIT_MORE                 0x02
#define BLE_ADV_TYPE_16BIT_COMPLETE             0x03
#define BLE_ADV_TYPE_32BIT_MORE                 0x04
#define BLE_ADV_TYPE_32BIT_COMPLETE             0x05
#define BLE_ADV_TYPE_128BIT_MORE                0x06
#define BLE_ADV_TYPE_128BIT_COMPLETE            0x07
#define BLE_ADV_TYPE_LOCAL_NAME_SHORT           0x08
#define BLE_ADV_TYPE_LOCAL_NAME_COMPLETE        0x09
#define BLE_ADV_TYPE_POWER_LEVEL                0x0A
#define BLE_ADV_TYPE_OOB_CLASS_OF_DEVICE        0x0D
#define BLE_ADV_TYPE_OOB_SIMPLE_PAIRING_HASHC   0x0E
#define BLE_ADV_TYPE_OOB_SIMPLE_PAIRING_RANDR   0x0F
#define BLE_ADV_TYPE_SM_TK                      0x10
#define BLE_ADV_TYPE_SM_OOB_FLAG                0x11
#define BLE_ADV_TYPE_SLAVE_CONN_INTERVAL_RANGE  0x12
#define BLE_ADV_TYPE_SIGNED_DATA                0x13
#define BLE_ADV_TYPE_SERVICE_LIST_16BIT         0x14
#define BLE_ADV_TYPE_SERVICE_LIST_128BIT        0x15
#define BLE_ADV_TYPE_SERVICE_DATA               0x16
#define BLE_ADV_TYPE_PUBLIC_TARGET_ADDR         0x17
#define BLE_ADV_TYPE_RANDOM_TARGET_ADDR         0x18
#define BLE_ADV_TYPE_APPEARANCE                 0x19
#define BLE_ADV_TYPE_ADV_INTERVAL               0x1A
#define BLE_ADV_TYPE_LE_BD_ADDR                 0x1B
#define BLE_ADV_TYPE_LE_ROLE                    0x1C
#define BLE_ADV_TYPE_SIMPLE_PAIRING_HASHC_256   0x1D
#define BLE_ADV_TYPE_SIMPLE_PAIRING_RANDR_256   0x1E
#define BLE_ADV_TYPE_SERVICE_DATA_32BIT         0x20
#define BLE_ADV_TYPE_SERVICE_DATA_128BIT        0x21
#define BLE_ADV_TYPE_3D_INFO_DATA               0x3D
#define BLE_ADV_TYPE_MANUFACTURER_SPECIFIC      0xFF

/* GAP Advertisement data type flags */

/* Discovery Mode: LE Limited Discoverable Mode */
#define BLE_ADV_TYPE_FLAGS_LIMITED              0x01
/* Discovery Mode: LE General Discoverable Mode */
#define BLE_ADV_TYPE_FLAGS_GENERAL              0x02
/* Discovery Mode: BR/EDR Not Supported */
#define BLE_ADV_TYPE_FLAGS_BREDR_NOT_SUPPORTED  0x04

#define BLE_ADV_NAME_BUF_LEN        BLE_ADV_MAX_SIZE
#define BLE_ADV_PAYLOAD_BUF_LEN     64
#define BLE_UUID_SIZE               16
/*---------------------------------------------------------------------------*/
typedef struct {
  /* Outgoing frame buffer */
  uint8_t tx_buf[BLE_ADV_PAYLOAD_BUF_LEN] CC_ALIGN(4);

  /* Config data */
  size_t adv_name_len;
  char adv_name[BLE_ADV_NAME_BUF_LEN];

  /* Indicates whether deamon is active or not */
  bool is_active;

  /* Periodic timer for sending out BLE advertisements */
  clock_time_t ble_adv_interval;
  struct etimer ble_adv_et;

  /* RF driver */
  RF_Handle rf_handle;

  /* BLE command specific structures. Common accross BLE and BLE5. */
  uint8_t ble_mac_addr[6];
  rfc_bleAdvPar_t ble_adv_par;
  rfc_bleAdvOutput_t ble_adv_output;
} ble_beacond_t;

static ble_beacond_t ble_beacond;
/*---------------------------------------------------------------------------*/
PROCESS(ble_beacond_process, "RF BLE Beacon Daemon Process");
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_init(void)
{
  ble_cmd_radio_setup.config.frontEndMode = RF_2_4_GHZ_FRONT_END_MODE;
  ble_cmd_radio_setup.config.biasMode = RF_2_4_GHZ_BIAS_MODE;

  RF_Params rf_params;
  RF_Params_init(&rf_params);

  rf_params.nInactivityTimeout = RF_CONF_INACTIVITY_TIMEOUT;

  ble_beacond.rf_handle = ble_open(&rf_params);

  if(ble_beacond.rf_handle == NULL) {
    return RF_BLE_BEACOND_ERROR;
  }

  /*
   * It is important that the contents of the BLE MAC address is copied into
   * RAM, as the System CPU, and subsequently flash, goes idle when pending
   * on an RF command. This causes pend to hang forever.
   */
  ble_addr_le_cpy(ble_beacond.ble_mac_addr);
  ble_beacond.ble_adv_par.pDeviceAddress = (uint16_t *)ble_beacond.ble_mac_addr;
  ble_beacond.ble_adv_par.endTrigger.triggerType = TRIG_NEVER;

  rf_ble_cmd_ble_adv_nc.pParams = &ble_beacond.ble_adv_par;
  rf_ble_cmd_ble_adv_nc.pOutput = &ble_beacond.ble_adv_output;

  return RF_BLE_BEACOND_OK;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_config(clock_time_t interval, const char *name)
{
  rf_ble_beacond_result_t res;

  res = RF_BLE_BEACOND_ERROR;

  if(interval > 0) {
    ble_beacond.ble_adv_interval = interval;

    res = RF_BLE_BEACOND_OK;
  } else {
    ble_beacond.ble_adv_interval = BLE_ADV_INTERVAL;
  }

  if(name != NULL) {
    const size_t name_len = strlen(name);

    if((0 < name_len) && (name_len < BLE_ADV_NAME_BUF_LEN)) {
      ble_beacond.adv_name_len = name_len;
      memcpy(ble_beacond.adv_name, name, name_len);

      res = RF_BLE_BEACOND_OK;
    }
  }

  return res;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_start(void)
{
  if(ble_beacond.is_active) {
    return RF_BLE_BEACOND_OK;
  }

  ble_beacond.is_active = true;

  process_start(&ble_beacond_process, NULL);

  return RF_BLE_BEACOND_OK;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_stop(void)
{
  if(!ble_beacond.is_active) {
    return RF_BLE_BEACOND_OK;
  }

  ble_beacond.is_active = false;

  process_exit(&ble_beacond_process);

  return RF_BLE_BEACOND_OK;
}
/*---------------------------------------------------------------------------*/
int8_t
rf_ble_is_active(void)
{
  return (int8_t)ble_beacond.is_active;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_set_tx_power(int8_t dbm)
{
  rf_result_t res;

  if(!tx_power_in_range(dbm, ble_tx_power_table, ble_tx_power_table_size)) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  res = rf_set_tx_power(ble_beacond.rf_handle, ble_tx_power_table, dbm);

  return (res == RF_RESULT_OK)
         ? RF_BLE_BEACOND_OK
         : RF_BLE_BEACOND_ERROR;
}
/*---------------------------------------------------------------------------*/
int8_t
rf_ble_get_tx_power(void)
{
  rf_result_t res;

  int8_t dbm;
  res = rf_get_tx_power(ble_beacond.rf_handle, ble_tx_power_table, &dbm);

  if(res != RF_RESULT_OK) {
    return RF_TxPowerTable_INVALID_DBM;
  }

  return dbm;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ble_beacond_process, ev, data)
{
  size_t len;

  PROCESS_BEGIN();

  while(1) {
    etimer_set(&ble_beacond.ble_adv_et, ble_beacond.ble_adv_interval);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ble_beacond.ble_adv_et) ||
                             (ev == PROCESS_EVENT_EXIT));

    if(ev == PROCESS_EVENT_EXIT) {
      PROCESS_EXIT();
    }

    /* Device info */
    /* Set the adv payload each pass: The device name may have changed */
    len = 0;

    #define append_byte(x)  ble_beacond.tx_buf[len++] = (uint8_t)((x))

    /* 2 bytes */
    append_byte(2);
    append_byte(BLE_ADV_TYPE_FLAGS);
    /* LE general discoverable + BR/EDR not supported */
    append_byte(BLE_ADV_TYPE_FLAGS_GENERAL |
                BLE_ADV_TYPE_FLAGS_BREDR_NOT_SUPPORTED);

    /* 1 + len(name) bytes (excluding zero termination) */
    append_byte(1 + ble_beacond.adv_name_len);
    append_byte(BLE_ADV_TYPE_LOCAL_NAME_COMPLETE);

    memcpy(ble_beacond.tx_buf + len, ble_beacond.adv_name, ble_beacond.adv_name_len);
    len += ble_beacond.adv_name_len;

    #undef append_byte

    /* Send advertisements on all three channels */
    ble_beacond.ble_adv_par.advLen = len;
    ble_beacond.ble_adv_par.pAdvData = ble_beacond.tx_buf;

    ble_sched_beacons(BLE_ADV_CHANNEL_ALL);
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
#else /* RF_CONF_BLE_BEACON_ENABLE */
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_init(void)
{
  return RF_BLE_BEACOND_DISABLED;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_config(clock_time_t interval, const char *name)
{
  (void)interval;
  (void)name;
  return RF_BLE_BEACOND_DISABLED;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_start(void)
{
  return RF_BLE_BEACOND_DISABLED;
}

/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_stop(void)
{
  return RF_BLE_BEACOND_DISABLED;
}
/*---------------------------------------------------------------------------*/
int8_t
rf_ble_is_active(void)
{
  return -1;
}
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_set_tx_power(int8_t power)
{
  (void)power;
  return RF_BLE_BEACOND_DISABLED;
}
/*---------------------------------------------------------------------------*/
int8_t
rf_ble_get_tx_power(void)
{
  return ~(int8_t)(0);
}
/*---------------------------------------------------------------------------*/
#endif /* RF_CONF_BLE_BEACON_ENABLE */
/*---------------------------------------------------------------------------*/
/** @} */
