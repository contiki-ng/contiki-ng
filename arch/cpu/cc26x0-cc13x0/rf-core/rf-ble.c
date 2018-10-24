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
#include "dev/oscillators.h"
#include "rf-core/rf-core.h"
#include "rf-core/rf-switch.h"
#include "rf-core/rf-ble.h"
#include "driverlib/rf_ble_cmd.h"
#include "driverlib/rf_common_cmd.h"
#include "ti-lib.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
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
static unsigned char ble_params_buf[32] CC_ALIGN(4);
static uint8_t ble_mode_on = RF_BLE_IDLE;
static struct etimer ble_adv_et;
static uint8_t payload[BLE_ADV_PAYLOAD_BUF_LEN];
static int p = 0;
static int i;
/*---------------------------------------------------------------------------*/
/* BLE beacond config */
static struct ble_beacond_config {
  clock_time_t interval;
  char adv_name[BLE_ADV_NAME_BUF_LEN];
} beacond_config = { .interval = BLE_ADV_INTERVAL };
/*---------------------------------------------------------------------------*/
#ifdef RF_BLE_CONF_BOARD_OVERRIDES
#define RF_BLE_BOARD_OVERRIDES RF_BLE_CONF_BOARD_OVERRIDES
#else
#define RF_BLE_BOARD_OVERRIDES
#endif
/*---------------------------------------------------------------------------*/
/* BLE overrides */
static uint32_t ble_overrides[] = {
  0x00364038, /* Synth: Set RTRIM (POTAILRESTRIM) to 6 */
  0x000784A3, /* Synth: Set FREF = 3.43 MHz (24 MHz / 7) */
  0xA47E0583, /* Synth: Set loop bandwidth after lock to 80 kHz (K2) */
  0xEAE00603, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, LSB) */
  0x00010623, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, MSB) */
  0x00456088, /* Adjust AGC reference level */
  RF_BLE_BOARD_OVERRIDES
  0xFFFFFFFF, /* End of override list */
};
/*---------------------------------------------------------------------------*/
/* TX Power dBm lookup table - values from SmartRF Studio */
typedef struct output_config {
  radio_value_t dbm;
  uint16_t tx_power; /* Value for the CMD_RADIO_SETUP.txPower field */
} output_config_t;

static const output_config_t output_power[] = {
  { 5, 0x9330 },
  { 4, 0x9324 },
  { 3, 0x5a1c },
  { 2, 0x4e18 },
  { 1, 0x4214 },
  { 0, 0x3161 },
  { -3, 0x2558 },
  { -6, 0x1d52 },
  { -9, 0x194e },
  { -12, 0x144b },
  { -15, 0x0ccb },
  { -18, 0x0cc9 },
  { -21, 0x0cc7 },
};

#define OUTPUT_CONFIG_COUNT (sizeof(output_power) / sizeof(output_config_t))

/* Max and Min Output Power in dBm */
#define OUTPUT_POWER_MIN     (output_power[OUTPUT_CONFIG_COUNT - 1].dbm)
#define OUTPUT_POWER_MAX     (output_power[0].dbm)
#define OUTPUT_POWER_UNKNOWN 0xFFFF

/* Default TX Power - position in output_power[] */
static const output_config_t *tx_power_current = &output_power[0];
/*---------------------------------------------------------------------------*/
PROCESS(rf_ble_beacon_process, "CC13xx / CC26xx RF BLE Beacon Process");
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
/* Returns the current TX power in dBm */
radio_value_t
rf_ble_get_tx_power(void)
{
  return tx_power_current->dbm;
}
/*---------------------------------------------------------------------------*/
/*
 * Set TX power to 'at least' power dBm
 * This works with a lookup table. If the value of 'power' does not exist in
 * the lookup table, TXPOWER will be set to the immediately higher available
 * value
 */
void
rf_ble_set_tx_power(radio_value_t power)
{
  int i;

  /* First, find the correct setting and save it */
  for(i = OUTPUT_CONFIG_COUNT - 1; i >= 0; --i) {
    if(power <= output_power[i].dbm) {
      tx_power_current = &output_power[i];
      break;
    }
  }
}
/*---------------------------------------------------------------------------*/
void
rf_ble_beacond_config(clock_time_t interval, const char *name)
{
  if(RF_BLE_ENABLED == 0) {
    return;
  }

  if(name != NULL) {
    if(strlen(name) == 0 || strlen(name) >= BLE_ADV_NAME_BUF_LEN) {
      return;
    }

    memset(beacond_config.adv_name, 0, BLE_ADV_NAME_BUF_LEN);
    memcpy(beacond_config.adv_name, name, strlen(name));
  }

  if(interval != 0) {
    beacond_config.interval = interval;
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_ble_beacond_start()
{
  if(RF_BLE_ENABLED == 0) {
    return RF_CORE_CMD_ERROR;
  }

  if(ti_lib_chipinfo_supports_ble() == false) {
    return RF_CORE_CMD_ERROR;
  }

  if(beacond_config.adv_name[0] == 0) {
    return RF_CORE_CMD_ERROR;
  }

  ble_mode_on = RF_BLE_IDLE;

  process_start(&rf_ble_beacon_process, NULL);

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
uint8_t
rf_ble_is_active()
{
  return ble_mode_on;
}
/*---------------------------------------------------------------------------*/
void
rf_ble_beacond_stop()
{
  process_exit(&rf_ble_beacon_process);
}
static uint8_t
rf_radio_setup()
{
  uint32_t cmd_status;
  rfc_CMD_RADIO_SETUP_t cmd;

  rf_switch_select_path(RF_SWITCH_PATH_2_4GHZ);

  /* Create radio setup command */
  rf_core_init_radio_op((rfc_radioOp_t *)&cmd, sizeof(cmd), CMD_RADIO_SETUP);

  cmd.txPower = tx_power_current->tx_power;
  cmd.pRegOverride = ble_overrides;
  cmd.config.frontEndMode = RF_CORE_RADIO_SETUP_FRONT_END_MODE;
  cmd.config.biasMode = RF_CORE_RADIO_SETUP_BIAS_MODE;
  cmd.mode = 0;

  /* Send Radio setup to RF Core */
  if(rf_core_send_cmd((uint32_t)&cmd, &cmd_status) != RF_CORE_CMD_OK) {
    PRINTF("rf_radio_setup: CMDSTA=0x%08lx, status=0x%04x\n",
           cmd_status, cmd.status);
    return RF_CORE_CMD_ERROR;
  }

  /* Wait until radio setup is done */
  if(rf_core_wait_cmd_done(&cmd) != RF_CORE_CMD_OK) {
    PRINTF("rf_radio_setup: wait, CMDSTA=0x%08lx, status=0x%04x\n",
           cmd_status, cmd.status);
    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
void
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
  interrupts_disabled = ti_lib_int_master_disable();
  ble_mode_on = RF_BLE_ACTIVE;
  if(!interrupts_disabled) {
    ti_lib_int_master_enable();
  }

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

    interrupts_disabled = ti_lib_int_master_disable();

    ble_mode_on = RF_BLE_IDLE;

    if(!interrupts_disabled) {
      ti_lib_int_master_enable();
    }
  }
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rf_ble_beacon_process, ev, data)
{
  PROCESS_BEGIN();

  while(1) {
    etimer_set(&ble_adv_et, beacond_config.interval);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ble_adv_et) || ev == PROCESS_EVENT_EXIT);

    if(ev == PROCESS_EVENT_EXIT) {
      PROCESS_EXIT();
    }

    /* Set the adv payload each pass: The device name may have changed */
    p = 0;

    /* device info */
    memset(payload, 0, BLE_ADV_PAYLOAD_BUF_LEN);
    payload[p++] = 0x02;          /* 2 bytes */
    payload[p++] = BLE_ADV_TYPE_DEVINFO;
    payload[p++] = 0x1a;          /* LE general discoverable + BR/EDR */
    payload[p++] = 1 + strlen(beacond_config.adv_name);
    payload[p++] = BLE_ADV_TYPE_NAME;
    memcpy(&payload[p], beacond_config.adv_name,
           strlen(beacond_config.adv_name));
    p += strlen(beacond_config.adv_name);

    /*
     * Send BLE_ADV_MESSAGES beacon bursts. Each burst on all three
     * channels, with a BLE_ADV_DUTY_CYCLE interval between bursts
     */
    for(i = 0; i < BLE_ADV_MESSAGES; i++) {

      rf_ble_beacon_single(BLE_ADV_CHANNEL_ALL, payload, p);

      etimer_set(&ble_adv_et, BLE_ADV_DUTY_CYCLE);

      /* Wait unless this is the last burst */
      if(i < BLE_ADV_MESSAGES - 1) {
        PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&ble_adv_et));
      }
    }
  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
