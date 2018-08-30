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
/* BLE Advertisement channels. Not to be changed by the user. */
typedef enum {
  BLE_ADV_CHANNEL_37 = (1 << 0),
  BLE_ADV_CHANNEL_38 = (1 << 1),
  BLE_ADV_CHANNEL_39 = (1 << 2),

  BLE_ADV_CHANNEL_ALL = (BLE_ADV_CHANNEL_37 |
                         BLE_ADV_CHANNEL_38 |
                         BLE_ADV_CHANNEL_39),
} ble_adv_channel_t;

#define BLE_ADV_CHANNEL_MIN         37
#define BLE_ADV_CHANNEL_MAX         39
/*---------------------------------------------------------------------------*/
/* Maximum BLE advertisement size. Not to be changed by the user. */
#define BLE_ADV_MAX_SIZE            31
/*---------------------------------------------------------------------------*/
/* BLE Intervals: Send a burst of advertisements every BLE_ADV_INTERVAL secs */
#define BLE_ADV_INTERVAL            (CLOCK_SECOND * 5)
#define BLE_ADV_DUTY_CYCLE          (CLOCK_SECOND / 10)
#define BLE_ADV_MESSAGES            10

/* BLE Advertisement-related macros */
#define BLE_ADV_TYPE_DEVINFO        0x01
#define BLE_ADV_TYPE_NAME           0x09
#define BLE_ADV_TYPE_MANUFACTURER   0xFF
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
} ble_beacond_t;

static ble_beacond_t ble_beacond;
/*---------------------------------------------------------------------------*/
PROCESS(ble_beacond_process, "RF BLE Beacon Daemon Process");
/*---------------------------------------------------------------------------*/
rf_ble_beacond_result_t
rf_ble_beacond_init(void)
{
  ble_adv_par.pDeviceAddress = (uint16_t *)ble_addr_ptr();

  RF_Params rf_params;
  RF_Params_init(&rf_params);

  /* Should immediately turn off radio if possible */
  rf_params.nInactivityTimeout = 0;

  ble_beacond.rf_handle = ble_open(&rf_params);

  if(ble_beacond.rf_handle == NULL) {
    return RF_BLE_BEACOND_ERROR;
  }

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
  }

  if(name != NULL) {
    const size_t name_len = strlen(name);

    if((name_len == 0) || (name_len >= BLE_ADV_NAME_BUF_LEN)) {
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
static rf_ble_beacond_result_t
ble_beacon_burst(uint8_t channels_bm, uint8_t *data, uint8_t len)
{
  rf_result_t res;

  uint8_t channel;
  for(channel = BLE_ADV_CHANNEL_MIN; channel <= BLE_ADV_CHANNEL_MAX; ++channel) {
    const uint8_t channel_bv = (1 << (channel - BLE_ADV_CHANNEL_MIN));
    if((channel_bv & channels_bm) == 0) {
      continue;
    }

    ble_adv_par.advLen = len;
    ble_adv_par.pAdvData = data;

    ble_cmd_beacon.channel = channel;

    res = ble_sched_beacon(NULL, 0);

    if(res != RF_RESULT_OK) {
      return RF_BLE_BEACOND_ERROR;
    }
  }

  return RF_BLE_BEACOND_OK;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(ble_beacond_process, ev, data)
{
  static size_t i;
  static size_t len;

  PROCESS_BEGIN();

  while(1) {
    etimer_set(&ble_beacond.ble_adv_et, ble_beacond.ble_adv_interval);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ble_beacond.ble_adv_et) ||
                             (ev == PROCESS_EVENT_EXIT));

    if(ev == PROCESS_EVENT_EXIT) {
      PROCESS_EXIT();
    }

    /* Set the adv payload each pass: The device name may have changed */
    len = 0;

    /* Device info */
    ble_beacond.tx_buf[len++] = (uint8_t)0x02;          /* 2 bytes */
    ble_beacond.tx_buf[len++] = (uint8_t)BLE_ADV_TYPE_DEVINFO;
    ble_beacond.tx_buf[len++] = (uint8_t)0x1A;          /* LE general discoverable + BR/EDR */
    ble_beacond.tx_buf[len++] = (uint8_t)ble_beacond.adv_name_len;
    ble_beacond.tx_buf[len++] = (uint8_t)BLE_ADV_TYPE_NAME;

    memcpy(ble_beacond.tx_buf + len, ble_beacond.adv_name, ble_beacond.adv_name_len);
    len += ble_beacond.adv_name_len;

    /*
     * Send BLE_ADV_MESSAGES beacon bursts. Each burst on all three
     * channels, with a BLE_ADV_DUTY_CYCLE interval between bursts
     */
    ble_beacon_burst(BLE_ADV_CHANNEL_ALL, ble_beacond.tx_buf, len);
    for(i = 1; i < BLE_ADV_MESSAGES; ++i) {
      etimer_set(&ble_beacond.ble_adv_et, BLE_ADV_DUTY_CYCLE);
      PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ble_beacond.ble_adv_et));

      ble_beacon_burst(BLE_ADV_CHANNEL_ALL, ble_beacond.tx_buf, len);
    }
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
