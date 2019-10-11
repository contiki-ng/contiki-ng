/*
 * Copyright (c) 2015, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup rf-core-prop
 * @{
 *
 * \file
 * Implementation of the CC13xx prop mode NETSTACK_RADIO driver
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "dev/cc26xx-uart.h"
#include "dev/oscillators.h"
#include "dev/watchdog.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "sys/energest.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "sys/cc.h"
#include "lpm.h"
#include "ti-lib.h"
#include "rf-core/rf-core.h"
#include "rf-core/rf-switch.h"
#include "rf-core/rf-ble.h"
#include "rf-core/prop-mode.h"
#include "rf-core/dot-15-4g.h"
/*---------------------------------------------------------------------------*/
/* RF core and RF HAL API */
#include "hw_rfc_dbell.h"
#include "hw_rfc_pwr.h"
/*---------------------------------------------------------------------------*/
/* RF Core Mailbox API */
#include "driverlib/rf_mailbox.h"
#include "driverlib/rf_common_cmd.h"
#include "driverlib/rf_data_entry.h"
#include "driverlib/rf_prop_mailbox.h"
#include "driverlib/rf_prop_cmd.h"
/*---------------------------------------------------------------------------*/
/* CC13xxware patches */
#include "rf_patches/rf_patch_cpe_genfsk.h"
#include "rf_patches/rf_patch_rfe_genfsk.h"
/*---------------------------------------------------------------------------*/
#include "rf-core/smartrf-settings.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
/* Data entry status field constants */
#define DATA_ENTRY_STATUS_PENDING    0x00 /* Not in use by the Radio CPU */
#define DATA_ENTRY_STATUS_ACTIVE     0x01 /* Open for r/w by the radio CPU */
#define DATA_ENTRY_STATUS_BUSY       0x02 /* Ongoing r/w */
#define DATA_ENTRY_STATUS_FINISHED   0x03 /* Free to use and to free */
#define DATA_ENTRY_STATUS_UNFINISHED 0x04 /* Partial RX entry */
/*---------------------------------------------------------------------------*/
/* Data whitener. 1: Whitener, 0: No whitener */
#ifdef PROP_MODE_CONF_DW
#define PROP_MODE_DW PROP_MODE_CONF_DW
#else
#define PROP_MODE_DW 0
#endif

#ifdef PROP_MODE_CONF_USE_CRC16
#define PROP_MODE_USE_CRC16 PROP_MODE_CONF_USE_CRC16
#else
#define PROP_MODE_USE_CRC16 0
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns the current status of a running Radio Op command
 * \param a A pointer with the buffer used to initiate the command
 * \return The value of the Radio Op buffer's status field
 *
 * This macro can be used to e.g. return the status of a previously
 * initiated background operation, or of an immediate command
 */
#define RF_RADIO_OP_GET_STATUS(a) GET_FIELD_V(a, radioOp, status)
/*---------------------------------------------------------------------------*/
#ifdef PROP_MODE_CONF_RSSI_THRESHOLD
#define PROP_MODE_RSSI_THRESHOLD PROP_MODE_CONF_RSSI_THRESHOLD
#else
#define PROP_MODE_RSSI_THRESHOLD 0xA6
#endif

static int8_t rssi_threshold = PROP_MODE_RSSI_THRESHOLD;
/*---------------------------------------------------------------------------*/
#if MAC_CONF_WITH_TSCH
static volatile rfc_dataEntry_t *packet_being_received;
#endif
/*---------------------------------------------------------------------------*/
static int on(void);
static int off(void);

static bool rf_setup_is_completed = false;

static rfc_propRxOutput_t rx_stats;
/*---------------------------------------------------------------------------*/
/* Defines and variables related to the .15.4g PHY HDR */
#define DOT_4G_PHR_LEN             2

/* PHY HDR bits */
#define DOT_4G_PHR_CRC16  0x10
#define DOT_4G_PHR_DW     0x08

#if PROP_MODE_USE_CRC16
/* CRC16 */
#define DOT_4G_PHR_CRC_BIT DOT_4G_PHR_CRC16
#define CRC_LEN            2
#else
/* CRC32 */
#define DOT_4G_PHR_CRC_BIT 0
#define CRC_LEN            4
#endif

#if PROP_MODE_DW
#define DOT_4G_PHR_DW_BIT DOT_4G_PHR_DW
#else
#define DOT_4G_PHR_DW_BIT 0
#endif
/*---------------------------------------------------------------------------*/
/*
 * The maximum number of bytes this driver can accept from the MAC layer for
 * transmission or will deliver to the MAC layer after reception. Includes
 * the MAC header and payload, but not the CRC.
 *
 * Unlike typical 2.4GHz radio drivers, this driver supports the .15.4g
 * 32-bit CRC option.
 *
 * This radio hardware is perfectly happy to transmit frames longer than 127
 * bytes, which is why it's OK to end up transmitting 125 payload bytes plus
 * a 4-byte CRC.
 *
 * In the future we can change this to support transmission of long frames,
 * for example as per .15.4g, which defines 2047 as the maximum frame size.
 * The size of the TX and RX buffers would need to be adjusted accordingly.
 */
#define MAX_PAYLOAD_LEN 125
/*---------------------------------------------------------------------------*/
/* TX power table for the 431-527MHz band */
#ifdef PROP_MODE_CONF_TX_POWER_431_527
#define PROP_MODE_TX_POWER_431_527 PROP_MODE_CONF_TX_POWER_431_527
#else
#define PROP_MODE_TX_POWER_431_527 prop_mode_tx_power_431_527
#endif
/*---------------------------------------------------------------------------*/
/* TX power table for the 779-930MHz band */
#ifdef PROP_MODE_CONF_TX_POWER_779_930
#define PROP_MODE_TX_POWER_779_930 PROP_MODE_CONF_TX_POWER_779_930
#else
#define PROP_MODE_TX_POWER_779_930 prop_mode_tx_power_779_930
#endif
/*---------------------------------------------------------------------------*/
/* Select power table based on the frequency band */
#if DOT_15_4G_FREQUENCY_BAND_ID==DOT_15_4G_FREQUENCY_BAND_470
#define TX_POWER_DRIVER PROP_MODE_TX_POWER_431_527
#else
#define TX_POWER_DRIVER PROP_MODE_TX_POWER_779_930
#endif
/*---------------------------------------------------------------------------*/
extern const prop_mode_tx_power_config_t TX_POWER_DRIVER[];

/* Max and Min Output Power in dBm */
#define OUTPUT_POWER_MAX     (TX_POWER_DRIVER[0].dbm)
#define OUTPUT_POWER_UNKNOWN 0xFFFF

/* Default TX Power - position in output_power[] */
static const prop_mode_tx_power_config_t *tx_power_current = &TX_POWER_DRIVER[1];
/*---------------------------------------------------------------------------*/
#ifdef PROP_MODE_CONF_LO_DIVIDER
#define PROP_MODE_LO_DIVIDER   PROP_MODE_CONF_LO_DIVIDER
#else
#define PROP_MODE_LO_DIVIDER   0x05
#endif
/*---------------------------------------------------------------------------*/
#ifdef PROP_MODE_CONF_RX_BUF_CNT
#define PROP_MODE_RX_BUF_CNT PROP_MODE_CONF_RX_BUF_CNT
#else
#define PROP_MODE_RX_BUF_CNT 4
#endif
/*---------------------------------------------------------------------------*/
#define DATA_ENTRY_LENSZ_NONE 0
#define DATA_ENTRY_LENSZ_BYTE 1
#define DATA_ENTRY_LENSZ_WORD 2 /* 2 bytes */

/* The size of the metadata (excluding the packet length field) */
#define RX_BUF_METADATA_SIZE \
  (CRC_LEN * RF_CORE_RX_BUF_INCLUDE_CRC \
      + RF_CORE_RX_BUF_INCLUDE_RSSI \
      + RF_CORE_RX_BUF_INCLUDE_CORR \
      + 4 * RF_CORE_RX_BUF_INCLUDE_TIMESTAMP)

/* The offset of the packet length in a rx buffer */
#define RX_BUF_LENGTH_OFFSET sizeof(rfc_dataEntry_t)
/* The offset of the packet data in a rx buffer */
#define RX_BUF_DATA_OFFSET (RX_BUF_LENGTH_OFFSET + DOT_4G_PHR_LEN)

#define ALIGN_TO_4(size)	(((size) + 3) & ~3)

#define RX_BUF_SIZE ALIGN_TO_4(RX_BUF_DATA_OFFSET          \
      + MAX_PAYLOAD_LEN \
      + RX_BUF_METADATA_SIZE)

/*
 * RX buffers.
 * PROP_MODE_RX_BUF_CNT buffers of RX_BUF_SIZE bytes each. The start of each
 * buffer must be 4-byte aligned, therefore RX_BUF_SIZE must divide by 4
 */
static uint8_t rx_buf[PROP_MODE_RX_BUF_CNT][RX_BUF_SIZE] CC_ALIGN(4);

/* The RX Data Queue */
static dataQueue_t rx_data_queue = { 0 };

/* Receive entry pointer to keep track of read items */
volatile static uint8_t *rx_read_entry;
/*---------------------------------------------------------------------------*/
/*
 * Increasing this number causes unicast Tx immediately after broadcast Rx to have
 * negative synchronization errors ("dr" in TSCH logs); decreasing it: the opposite.
 */
#define RAT_TIMESTAMP_OFFSET_SUB_GHZ USEC_TO_RADIO(160 * 6 - 240)
/*---------------------------------------------------------------------------*/
/* The outgoing frame buffer */
#define TX_BUF_PAYLOAD_LEN 180
#define TX_BUF_HDR_LEN       2

static uint8_t tx_buf[TX_BUF_HDR_LEN + TX_BUF_PAYLOAD_LEN] CC_ALIGN(4);
/*---------------------------------------------------------------------------*/
static uint8_t
rx_is_on(void)
{
  if(!rf_core_is_accessible()) {
    return 0;
  }

  return smartrf_settings_cmd_prop_rx_adv.status == RF_CORE_RADIO_OP_STATUS_ACTIVE;
}
/*---------------------------------------------------------------------------*/
static uint8_t
rf_is_on(void)
{
  if (!rf_core_is_accessible()) {
    return 0;
  }

  return rf_setup_is_completed;
}
/*---------------------------------------------------------------------------*/
static uint8_t
transmitting(void)
{
  return smartrf_settings_cmd_prop_tx_adv.status == RF_CORE_RADIO_OP_STATUS_ACTIVE;
}
/*---------------------------------------------------------------------------*/
static radio_value_t
get_rssi(void)
{
  uint32_t cmd_status;
  int8_t rssi;
  uint8_t attempts = 0;
  uint8_t was_off = 0;
  rfc_CMD_GET_RSSI_t cmd;

  /* If we are off, turn on first */
  if(!rf_is_on()) {
    was_off = 1;
    if(on() != RF_CORE_CMD_OK) {
      PRINTF("get_rssi: on() failed\n");
      return RF_CORE_CMD_CCA_REQ_RSSI_UNKNOWN;
    }
  }

  rssi = RF_CORE_CMD_CCA_REQ_RSSI_UNKNOWN;

  while((rssi == RF_CORE_CMD_CCA_REQ_RSSI_UNKNOWN || rssi == 0) && ++attempts < 10) {
    memset(&cmd, 0x00, sizeof(cmd));
    cmd.commandNo = CMD_GET_RSSI;

    if(rf_core_send_cmd((uint32_t)&cmd, &cmd_status) == RF_CORE_CMD_ERROR) {
      PRINTF("get_rssi: CMDSTA=0x%08lx\n", cmd_status);
      break;
    } else {
      /* Current RSSI in bits 23:16 of cmd_status */
      rssi = (cmd_status >> 16) & 0xFF;
    }
  }

  /* If we were off, turn back off */
  if(was_off) {
    off();
  }

  return rssi;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_channel(void)
{
  uint32_t freq_khz;

  freq_khz = smartrf_settings_cmd_fs.frequency * 1000;

  /*
   * For some channels, fractFreq * 1000 / 65536 will return 324.99xx.
   * Casting the result to uint32_t will truncate decimals resulting in the
   * function returning channel - 1 instead of channel. Thus, we do a quick
   * positive integer round up.
   */
  freq_khz += (((smartrf_settings_cmd_fs.fractFreq * 1000) + 65535) / 65536);

  return (freq_khz - DOT_15_4G_CHAN0_FREQUENCY) / DOT_15_4G_CHANNEL_SPACING;
}
/*---------------------------------------------------------------------------*/
static void
set_channel(uint8_t channel)
{
  uint32_t new_freq;
  uint16_t freq, frac;

  new_freq = DOT_15_4G_CHAN0_FREQUENCY + (channel * DOT_15_4G_CHANNEL_SPACING);

  freq = (uint16_t)(new_freq / 1000);
  frac = (new_freq - (freq * 1000)) * 65536 / 1000;

  PRINTF("set_channel: %u = 0x%04x.0x%04x (%lu)\n", channel, freq, frac,
         new_freq);

  smartrf_settings_cmd_prop_radio_div_setup.centerFreq = freq;
  smartrf_settings_cmd_fs.frequency = freq;
  smartrf_settings_cmd_fs.fractFreq = frac;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_tx_power_array_last_element(void)
{
  const prop_mode_tx_power_config_t *array = TX_POWER_DRIVER;
  uint8_t count = 0;

  while(array->tx_power != OUTPUT_POWER_UNKNOWN) {
    count++;
    array++;
  }
  return count - 1;
}
/*---------------------------------------------------------------------------*/
/* Returns the current TX power in dBm */
static radio_value_t
get_tx_power(void)
{
  return tx_power_current->dbm;
}
/*---------------------------------------------------------------------------*/
/*
 * The caller must make sure to send a new CMD_PROP_RADIO_DIV_SETUP to the
 * radio after calling this function.
 */
static void
set_tx_power(radio_value_t power)
{
  int i;

  for(i = get_tx_power_array_last_element(); i >= 0; --i) {
    if(power <= TX_POWER_DRIVER[i].dbm) {
      /*
       * Merely save the value. It will be used in all subsequent usages of
       * CMD_PROP_RADIO_DIV_SETP, including one immediately after this function
       * has returned
       */
      tx_power_current = &TX_POWER_DRIVER[i];

      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
prop_div_radio_setup(void)
{
  uint32_t cmd_status;
  rfc_radioOp_t *cmd = (rfc_radioOp_t *)&smartrf_settings_cmd_prop_radio_div_setup;

  rf_switch_select_path(RF_SWITCH_PATH_SUBGHZ);

  /* Adjust loDivider depending on the selected band */
  smartrf_settings_cmd_prop_radio_div_setup.loDivider = PROP_MODE_LO_DIVIDER;

  /* Update to the correct TX power setting */
  smartrf_settings_cmd_prop_radio_div_setup.txPower = tx_power_current->tx_power;

  /* Adjust RF Front End and Bias based on the board */
  smartrf_settings_cmd_prop_radio_div_setup.config.frontEndMode =
    RF_CORE_PROP_FRONT_END_MODE;
  smartrf_settings_cmd_prop_radio_div_setup.config.biasMode =
    RF_CORE_PROP_BIAS_MODE;

  /* Send Radio setup to RF Core */
  if(rf_core_send_cmd((uint32_t)cmd, &cmd_status) != RF_CORE_CMD_OK) {
    PRINTF("prop_div_radio_setup: DIV_SETUP, CMDSTA=0x%08lx, status=0x%04x\n",
           cmd_status, cmd->status);
    return RF_CORE_CMD_ERROR;
  }

  /* Wait until radio setup is done */
  if(rf_core_wait_cmd_done(cmd) != RF_CORE_CMD_OK) {
    PRINTF("prop_div_radio_setup: DIV_SETUP wait, CMDSTA=0x%08lx,"
           "status=0x%04x\n", cmd_status, cmd->status);
    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
static uint8_t
rf_cmd_prop_rx()
{
  uint32_t cmd_status;
  volatile rfc_CMD_PROP_RX_ADV_t *cmd_rx_adv;
  int ret;

  cmd_rx_adv = (rfc_CMD_PROP_RX_ADV_t *)&smartrf_settings_cmd_prop_rx_adv;
  cmd_rx_adv->status = RF_CORE_RADIO_OP_STATUS_IDLE;

  cmd_rx_adv->rxConf.bIncludeCrc = RF_CORE_RX_BUF_INCLUDE_CRC;
  cmd_rx_adv->rxConf.bAppendRssi = RF_CORE_RX_BUF_INCLUDE_RSSI;
  cmd_rx_adv->rxConf.bAppendTimestamp = RF_CORE_RX_BUF_INCLUDE_TIMESTAMP;
  cmd_rx_adv->rxConf.bAppendStatus = RF_CORE_RX_BUF_INCLUDE_CORR;

  /*
   * Set the max Packet length. This is for the payload only.
   */
  cmd_rx_adv->maxPktLen = RADIO_PHY_OVERHEAD + MAX_PAYLOAD_LEN;

  ret = rf_core_send_cmd((uint32_t)cmd_rx_adv, &cmd_status);

  if(ret != RF_CORE_CMD_OK) {
    PRINTF("rf_cmd_prop_rx: send_cmd ret=%d, CMDSTA=0x%08lx, status=0x%04x\n",
           ret, cmd_status, cmd_rx_adv->status);
    return RF_CORE_CMD_ERROR;
  }

  RTIMER_BUSYWAIT_UNTIL(cmd_rx_adv->status == RF_CORE_RADIO_OP_STATUS_ACTIVE,
                        RF_CORE_ENTER_RX_TIMEOUT);

  /* Wait to enter RX */
  if(cmd_rx_adv->status != RF_CORE_RADIO_OP_STATUS_ACTIVE) {
    PRINTF("rf_cmd_prop_rx: CMDSTA=0x%08lx, status=0x%04x\n",
           cmd_status, cmd_rx_adv->status);
    return RF_CORE_CMD_ERROR;
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
static void
init_rx_buffers(void)
{
  rfc_dataEntry_t *entry;
  int i;

  for(i = 0; i < PROP_MODE_RX_BUF_CNT; i++) {
    entry = (rfc_dataEntry_t *)rx_buf[i];
    entry->status = DATA_ENTRY_STATUS_PENDING;
    entry->config.type = DATA_ENTRY_TYPE_GEN;
    entry->config.lenSz = DATA_ENTRY_LENSZ_WORD;
    entry->length = RX_BUF_SIZE - 8;
    if(i == PROP_MODE_RX_BUF_CNT - 1) {
      entry->pNextEntry = rx_buf[0];
    } else {
      entry->pNextEntry = rx_buf[i + 1];
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
rx_on_prop(void)
{
  int ret;

  if(rx_is_on()) {
    PRINTF("rx_on_prop: We were on. PD=%u, RX=0x%04x\n",
           rf_core_is_accessible(), smartrf_settings_cmd_prop_rx_adv.status);
    return RF_CORE_CMD_OK;
  }

  /* Put CPE in RX using the currently configured parameters */
  ret = rf_cmd_prop_rx();

  if(ret) {
    ENERGEST_ON(ENERGEST_TYPE_LISTEN);
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
static int
rx_off_prop(void)
{
  uint32_t cmd_status;
  int ret;

#if MAC_CONF_WITH_TSCH
  /* Failsafe in case this variable failed to clear as part of normal operation */
  packet_being_received = NULL;
#endif /* MAC_CONF_WITH_TSCH */

  /* If we are off, do nothing */
  if(!rx_is_on()) {
    return RF_CORE_CMD_OK;
  }

  /* Wait for ongoing ACK TX to finish */
  RTIMER_BUSYWAIT_UNTIL(!transmitting(), RF_CORE_TX_FINISH_TIMEOUT);

  /* Send a CMD_ABORT command to RF Core */
  if(rf_core_send_cmd(CMDR_DIR_CMD(CMD_ABORT), &cmd_status) != RF_CORE_CMD_OK) {
    PRINTF("rx_off_prop: CMD_ABORT status=0x%08lx\n", cmd_status);
    /* Continue nonetheless */
  }

  RTIMER_BUSYWAIT_UNTIL(!rx_is_on(), RF_CORE_TURN_OFF_TIMEOUT);

  if(smartrf_settings_cmd_prop_rx_adv.status == PROP_DONE_STOPPED ||
     smartrf_settings_cmd_prop_rx_adv.status == PROP_DONE_ABORT) {
    /* Stopped gracefully */
    ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
    ret = RF_CORE_CMD_OK;
  } else {
    PRINTF("rx_off_prop: status=0x%04x\n",
           smartrf_settings_cmd_prop_rx_adv.status);
    ret = RF_CORE_CMD_ERROR;
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
static uint8_t
request(void)
{
  /*
   * We rely on the RDC layer to turn us on and off. Thus, if we are on we
   * will only allow sleep, standby otherwise
   */
  if(rf_is_on()) {
    return LPM_MODE_SLEEP;
  }

  return LPM_MODE_MAX_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
LPM_MODULE(prop_lpm_module, request, NULL, NULL, LPM_DOMAIN_NONE);
/*---------------------------------------------------------------------------*/
static int
prop_fs(void)
{
  uint32_t cmd_status;
  rfc_radioOp_t *cmd = (rfc_radioOp_t *)&smartrf_settings_cmd_fs;

  /* Send the command to the RF Core */
  if(rf_core_send_cmd((uint32_t)cmd, &cmd_status) != RF_CORE_CMD_OK) {
    PRINTF("prop_fs: CMD_FS, CMDSTA=0x%08lx, status=0x%04x\n",
           cmd_status, cmd->status);
    return RF_CORE_CMD_ERROR;
  }

  /* Wait until the command is done */
  if(rf_core_wait_cmd_done(cmd) != RF_CORE_CMD_OK) {
    PRINTF("prop_fs: CMD_FS wait, CMDSTA=0x%08lx, status=0x%04x\n",
           cmd_status, cmd->status);
    return RF_CORE_CMD_ERROR;
  }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
static void
soft_off_prop(void)
{
  uint32_t cmd_status;
  volatile rfc_radioOp_t *cmd = rf_core_get_last_radio_op();

  if(!rf_core_is_accessible()) {
    return;
  }

  /* Send a CMD_ABORT command to RF Core */
  if(rf_core_send_cmd(CMDR_DIR_CMD(CMD_ABORT), &cmd_status) != RF_CORE_CMD_OK) {
    PRINTF("soft_off_prop: CMD_ABORT status=0x%08lx\n", cmd_status);
    return;
  }

  RTIMER_BUSYWAIT_UNTIL((cmd->status & RF_CORE_RADIO_OP_MASKED_STATUS) !=
                         RF_CORE_RADIO_OP_MASKED_STATUS_RUNNING, RF_CORE_TURN_OFF_TIMEOUT);
}
/*---------------------------------------------------------------------------*/
static uint8_t
soft_on_prop(void)
{
  if(prop_div_radio_setup() != RF_CORE_CMD_OK) {
    PRINTF("soft_on_prop: prop_div_radio_setup() failed\n");
    return RF_CORE_CMD_ERROR;
  }

  if(prop_fs() != RF_CORE_CMD_OK) {
    PRINTF("soft_on_prop: prop_fs() failed\n");
    return RF_CORE_CMD_ERROR;
  }

  return rx_on_prop();
}
/*---------------------------------------------------------------------------*/
static const rf_core_primary_mode_t mode_prop = {
  soft_off_prop,
  soft_on_prop,
  rf_is_on,
  RAT_TIMESTAMP_OFFSET_SUB_GHZ
};
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  lpm_register_module(&prop_lpm_module);

  if(ti_lib_chipinfo_chip_family_is_cc13xx() == false) {
    return RF_CORE_CMD_ERROR;
  }

  /* Initialise RX buffers */
  memset(rx_buf, 0, sizeof(rx_buf));

  /* Set of RF Core data queue. Circular buffer, no last entry */
  rx_data_queue.pCurrEntry = rx_buf[0];
  rx_data_queue.pLastEntry = NULL;

  /* Initialize current read pointer to first element (used in ISR) */
  rx_read_entry = rx_buf[0];

  smartrf_settings_cmd_prop_rx_adv.pQueue = &rx_data_queue;
  smartrf_settings_cmd_prop_rx_adv.pOutput = (uint8_t *)&rx_stats;

  set_channel(IEEE802154_DEFAULT_CHANNEL);

  if(on() != RF_CORE_CMD_OK) {
    PRINTF("init: on() failed\n");
    return RF_CORE_CMD_ERROR;
  }

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  rf_core_primary_mode_register(&mode_prop);

  rf_core_rat_init();

  process_start(&rf_core_process, NULL);

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  if(payload_len > TX_BUF_PAYLOAD_LEN || payload_len > MAX_PAYLOAD_LEN) {
    return RADIO_TX_ERR;
  }

  memcpy(&tx_buf[TX_BUF_HDR_LEN], payload, payload_len);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  int ret;
  uint8_t was_off = 0;
  uint32_t cmd_status;
  volatile rfc_CMD_PROP_TX_ADV_t *cmd_tx_adv;

  /* Length in .15.4g PHY HDR. Includes the CRC but not the HDR itself */
  uint16_t total_length;

  if(transmit_len > MAX_PAYLOAD_LEN) {
    PRINTF("transmit: too long\n");
    return RADIO_TX_ERR;
  }

  if(!rf_is_on()) {
    was_off = 1;
    if(on() != RF_CORE_CMD_OK) {
      PRINTF("transmit: on() failed\n");
      return RADIO_TX_ERR;
    }
  }

  /*
   * Prepare the .15.4g PHY header
   * MS=0, Length MSBits=0, DW and CRC configurable
   * Total length = transmit_len (payload) + CRC length
   *
   * The Radio will flip the bits around, so tx_buf[0] must have the length
   * LSBs (PHR[15:8] and tx_buf[1] will have PHR[7:0]
   */
  total_length = transmit_len + CRC_LEN;

  tx_buf[0] = total_length & 0xFF;
  tx_buf[1] = (total_length >> 8) + DOT_4G_PHR_DW_BIT + DOT_4G_PHR_CRC_BIT;

  /* Prepare the CMD_PROP_TX_ADV command */
  cmd_tx_adv = (rfc_CMD_PROP_TX_ADV_t *)&smartrf_settings_cmd_prop_tx_adv;

  /*
   * pktLen: Total number of bytes in the TX buffer, including the header if
   * one exists, but not including the CRC (which is not present in the buffer)
   */
  cmd_tx_adv->pktLen = transmit_len + DOT_4G_PHR_LEN;
  cmd_tx_adv->pPkt = tx_buf;

  /* Abort RX */
  rx_off_prop();

  /* Enable the LAST_COMMAND_DONE interrupt to wake us up */
  rf_core_cmd_done_en(false);

  ret = rf_core_send_cmd((uint32_t)cmd_tx_adv, &cmd_status);

  if(ret) {
    /* If we enter here, TX actually started */
    ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);

    /* Idle away while the command is running */
    while((cmd_tx_adv->status & RF_CORE_RADIO_OP_MASKED_STATUS)
          == RF_CORE_RADIO_OP_MASKED_STATUS_RUNNING) {
      /* Note: for now sleeping while Tx'ing in polling mode is disabled.
       * To enable it:
       *  1) make the `lpm_sleep()` call here unconditional;
       *  2) change the radio ISR priority to allow radio ISR to interrupt rtimer ISR.
       */
      if(!rf_core_poll_mode) {
        lpm_sleep();
      }
    }

    if(cmd_tx_adv->status == RF_CORE_RADIO_OP_STATUS_PROP_DONE_OK) {
      /* Sent OK */
      ret = RADIO_TX_OK;
    } else {
      /* Operation completed, but frame was not sent */
      PRINTF("transmit: Not Sent OK status=0x%04x\n",
             cmd_tx_adv->status);
      ret = RADIO_TX_ERR;
    }
  } else {
    /* Failure sending the CMD_PROP_TX command */
    PRINTF("transmit: PROP_TX_ERR ret=%d, CMDSTA=0x%08lx, status=0x%04x\n",
           ret, cmd_status, cmd_tx_adv->status);
    ret = RADIO_TX_ERR;
  }

  /*
   * Update ENERGEST state here, before a potential call to off(), which
   * will correctly update it if required.
   */
  ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);

  /*
   * Disable LAST_FG_COMMAND_DONE interrupt. We don't really care about it
   * except when we are transmitting
   */
  rf_core_cmd_done_dis();

  /* Workaround. Set status to IDLE */
  cmd_tx_adv->status = RF_CORE_RADIO_OP_STATUS_IDLE;

  rx_on_prop();

  if(was_off) {
    off();
  }

  return ret;
}
/*---------------------------------------------------------------------------*/
static int
send(const void *payload, unsigned short payload_len)
{
  prepare(payload, payload_len);
  return transmit(payload_len);
}
/*---------------------------------------------------------------------------*/
static void
release_data_entry(void)
{
  rfc_dataEntryGeneral_t *entry = (rfc_dataEntryGeneral_t *)rx_read_entry;
  uint8_t *data_ptr = &entry->data;

  /* Clear the length field (2 bytes) */
  data_ptr[0] = 0;
  data_ptr[1] = 0;

  /* Set status to 0 "Pending" in element */
  entry->status = DATA_ENTRY_STATUS_PENDING;
  rx_read_entry = entry->pNextEntry;

  if(!rx_is_on()) {
    PRINTF("RX was off, re-enabling rx!\n");
    rx_on_prop();
  }

}
/*---------------------------------------------------------------------------*/
static int
read_frame(void *buf, unsigned short buf_len)
{
  rfc_dataEntryGeneral_t *entry = (rfc_dataEntryGeneral_t *)rx_read_entry;
  uint8_t *data_ptr;
  int len = 0;
  uint32_t rat_timestamp;

  int is_found = 0;
  /* Go through all RX buffers and check their status */
  do {
    if(entry->status >= DATA_ENTRY_STATUS_BUSY) {
      is_found = 1;
      break;
    }

    entry = (rfc_dataEntryGeneral_t *)entry->pNextEntry;
  } while(entry != (rfc_dataEntryGeneral_t *)rx_read_entry);

  if(is_found == 0) {
    /* No available data */
    return 0;
  }

  rx_read_entry = (volatile uint8_t *)entry;

  /* wait for entry to become finished */
  rtimer_clock_t t0 = RTIMER_NOW();
  while(entry->status == DATA_ENTRY_STATUS_BUSY
      && RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + RADIO_FRAME_DURATION(MAX_PAYLOAD_LEN)));

#if MAC_CONF_WITH_TSCH
  /* Make sure the flag is reset */
  packet_being_received = NULL;
#endif

  if(entry->status != DATA_ENTRY_STATUS_FINISHED) {
    /* No available data */
    return 0;
  }

  /*
   * First 2 bytes in the data entry are the length.
   * Our data entry consists of:
   *   Payload + RSSI (1 byte) + Timestamp (4 bytes) + Status (1 byte)
   * This length includes all of those.
   */
  data_ptr = &entry->data;
  len = (*(uint16_t *)data_ptr);

  if(len <= RX_BUF_METADATA_SIZE) {
    PRINTF("RF: too short!");

    release_data_entry();
    return 0;
  }

  data_ptr += 2;
  len -= RX_BUF_METADATA_SIZE;

  if(len > buf_len) {
    PRINTF("RF: too long\n");

    release_data_entry();
    return 0;
  }

  memcpy(buf, data_ptr, len);

  /* get the RSSI and status */
  rf_core_last_rssi = (int8_t)data_ptr[len];
  rf_core_last_corr_lqi = data_ptr[len + 5];

  /* get the timestamp */
  memcpy(&rat_timestamp, data_ptr + len + 1, 4);

  rf_core_last_packet_timestamp = rf_core_convert_rat_to_rtimer(rat_timestamp);

  if(!rf_core_poll_mode) {
    /* Not in poll mode: packetbuf should not be accessed in interrupt context.
     * In poll mode, the last packet RSSI and link quality can be obtained through
     * RADIO_PARAM_LAST_RSSI and RADIO_PARAM_LAST_LINK_QUALITY */
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rf_core_last_rssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, rf_core_last_corr_lqi);
  }

  release_data_entry();

  return len;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  uint8_t was_off = 0;
  uint32_t cmd_status;
  int8_t rssi = RF_CORE_CMD_CCA_REQ_RSSI_UNKNOWN;

  /*
   * If we are in the middle of a BLE operation, we got called by ContikiMAC
   * from within an interrupt context. Indicate a clear channel
   */
  if(rf_ble_is_active() == RF_BLE_ACTIVE) {
    return RF_CORE_CCA_CLEAR;
  }

  if(!rf_core_is_accessible()) {
    was_off = 1;
    if(on() != RF_CORE_CMD_OK) {
      PRINTF("channel_clear: on() failed\n");
      if(was_off) {
        off();
      }
      return RF_CORE_CCA_CLEAR;
    }
  } else {
    if(transmitting()) {
      PRINTF("channel_clear: called while in TX\n");
      return RF_CORE_CCA_CLEAR;
    }
  }

  while(rssi == RF_CORE_CMD_CCA_REQ_RSSI_UNKNOWN || rssi == 0) {
    if(rf_core_send_cmd(CMDR_DIR_CMD(CMD_GET_RSSI), &cmd_status)
       != RF_CORE_CMD_OK) {
      break;
    }
    /* Current RSSI in bits 23:16 of cmd_status */
    rssi = (cmd_status >> 16) & 0xFF;
  }

  if(was_off) {
    off();
  }

  if(rssi >= rssi_threshold) {
    return RF_CORE_CCA_BUSY;
  }

  return RF_CORE_CCA_CLEAR;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  if(!rf_is_on()) {
    return 0;
  }

#if MAC_CONF_WITH_TSCH
  /*
   * Under TSCH operation, we rely on "hints" from the MDMSOFT interrupt
   * flag. This flag is set by the radio upon sync word detection, but it is
   * not cleared automatically by hardware. We store state in a variable after
   * first call. The assumption is that the TSCH code will keep calling us
   * until frame reception has completed, at which point we can clear MDMSOFT.
   */
  if(packet_being_received == NULL) {
    /* Look for the modem synchronization word detection interrupt flag.
     * This flag is raised when the synchronization word is received.
     */
    if(HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIFG) & RFC_DBELL_RFHWIFG_MDMSOFT) {
      packet_being_received = (rfc_dataEntry_t *)rx_data_queue.pCurrEntry;
    }
  } else {
    /* After the start of the packet: reset the Rx flag once the packet is finished */
    if(packet_being_received->status >= DATA_ENTRY_FINISHED) {
      /* Clear the modem sync flag */
      ti_lib_rfc_hw_int_clear(RFC_DBELL_RFHWIFG_MDMSOFT);
      packet_being_received = NULL;
    }
  }

  return packet_being_received != NULL;
#else
  /*
   * Under CSMA operation, there is no immediately straightforward logic as to
   * when it's OK to clear the MDMSOFT interrupt flag:
   *
   *   - We cannot re-use the same logic as above, since CSMA may bail out of
   *     frame TX immediately after a single call this function here. In this
   *     scenario, is_receiving_packet would remain equal to one and we would
   *     therefore erroneously signal ongoing RX in subsequent calls to this
   *     function here, even _after_ reception has completed.
   *   - We can neither clear inside read_frame() nor inside the RX frame
   *     interrupt handler (remember, we are not in poll mode under CSMA),
   *     since we risk clearing MDMSOFT after we have seen a sync word for the
   *     _next_ frame. If this happens, this function here would incorrectly
   *     return 0 during RX of this next frame.
   *
   * So to avoid a very convoluted logic of how to handle MDMSOFT, we simply
   * perform a clear channel assessment here: We interpret channel activity
   * as frame reception.
   */

  if(channel_clear() == RF_CORE_CCA_CLEAR) {
    return 0;
  }

  return 1;

#endif
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  int rv = 0;
  volatile rfc_dataEntry_t *entry = (rfc_dataEntry_t *)rx_data_queue.pCurrEntry;

  /* Go through all RX buffers and check their status */
  do {
    if(entry->status >= DATA_ENTRY_STATUS_BUSY) {
      rv = 1;
      if(!rf_core_poll_mode) {
        process_poll(&rf_core_process);
      }
    }

    entry = (rfc_dataEntry_t *)entry->pNextEntry;
  } while(entry != (rfc_dataEntry_t *)rx_data_queue.pCurrEntry);

  /* If we didn't find an entry at status finished, no frames are pending */
  return rv;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  /*
   * If we are in the middle of a BLE operation, we got called by ContikiMAC
   * from within an interrupt context. Abort, but pretend everything is OK.
   */
  if(rf_ble_is_active() == RF_BLE_ACTIVE) {
    return RF_CORE_CMD_OK;
  }

  if(rf_is_on()) {
    PRINTF("on: We were on. PD=%u, RX=0x%04x \n", rf_core_is_accessible(),
           smartrf_settings_cmd_prop_rx_adv.status);
    return RF_CORE_CMD_OK;
  }

  /*
   * Request the HF XOSC as the source for the HF clock. Needed before we can
   * use the FS. This will only request, it will _not_ perform the switch.
   */
  oscillators_request_hf_xosc();

  if(!rf_core_is_accessible()) {
    if(rf_core_power_up() != RF_CORE_CMD_OK) {
      PRINTF("on: rf_core_power_up() failed\n");

      rf_core_power_down();

      return RF_CORE_CMD_ERROR;
    }

    /* Keep track of RF Core mode */
    rf_core_set_modesel();

    /* Apply patches to radio core */
    rf_patch_cpe_genfsk();
    while(!HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG));
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
    rf_patch_rfe_genfsk();

    /* Initialize bus request */
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_CMDR) =
      CMDR_DIR_CMD_1BYTE(CMD_BUS_REQUEST, 1);

    /* set VCOLDO reference */
    ti_lib_rfc_adi3vco_ldo_voltage_mode(true);

    /* Let CC13xxware automatically set a correct value for RTRIM for us */
    ti_lib_rfc_rtrim((rfc_radioOp_t *)&smartrf_settings_cmd_prop_radio_div_setup);

    /* Make sure BUS_REQUEST is done */
    while(!HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG));
    HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFACKIFG) = 0;

    if(rf_core_start_rat() != RF_CORE_CMD_OK) {
      PRINTF("on: rf_core_start_rat() failed\n");

      rf_core_power_down();

      return RF_CORE_CMD_ERROR;
    }
  }

  rf_core_setup_interrupts();

  init_rx_buffers();

  /*
   * Trigger a switch to the XOSC, so that we can subsequently use the RF FS
   * This will block until the XOSC is actually ready, but give how we
   * requested it early on, this won't be too long a wait/
   */
  oscillators_switch_to_hf_xosc();

  if(prop_div_radio_setup() != RF_CORE_CMD_OK) {
    PRINTF("on: prop_div_radio_setup() failed\n");
    rf_core_power_down();
    return RF_CORE_CMD_ERROR;
  }

  if(prop_fs() != RF_CORE_CMD_OK) {
    PRINTF("on: prop_fs() failed\n");
    return RF_CORE_CMD_ERROR;
  }

  if(rx_on_prop() != RF_CORE_CMD_OK) {

    if((rf_core_cmd_status() & RF_CORE_CMDSTA_RESULT_MASK) != RF_CORE_CMDSTA_SCHEDULING_ERR) {
      PRINTF("on: failed with status=0x%08lx\n", rf_core_cmd_status());
      return RF_CORE_CMD_ERROR;
    }

    /* Looks like a command was alredy pending on radio. */
    /* Abort any existing commands */
    uint32_t status;
    if(rf_core_send_cmd(CMDR_DIR_CMD(CMD_ABORT), &status) == RF_CORE_CMD_ERROR) {
      PRINTF("on: CMD_ABORT status=0x%08lx\n", rf_core_cmd_status());
      return RF_CORE_CMD_ERROR;
    }

    /* Retry setup */
    if(soft_on_prop() != RF_CORE_CMD_OK) {
      PRINTF("on: retry with status=0x%08lx\n", rf_core_cmd_status());
      return RF_CORE_CMD_ERROR;
    }
  }
  rf_setup_is_completed = true;
  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  int i;
  rfc_dataEntry_t *entry;

  if(!rf_is_on()) {
    PRINTF("off: We were off. PD=%u, RX=0x%04x \n", rf_core_is_accessible(),
           smartrf_settings_cmd_prop_rx_adv.status);
    return RF_CORE_CMD_OK;
  }

  /*
   * If we are in the middle of a BLE operation, we got called by ContikiMAC
   * from within an interrupt context. Abort, but pretend everything is OK.
   */
  if(rf_ble_is_active() == RF_BLE_ACTIVE) {
    return RF_CORE_CMD_OK;
  }

  rx_off_prop();
  rf_core_power_down();

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

#if !CC2650_FAST_RADIO_STARTUP
  /* Switch HF clock source to the RCOSC to preserve power */
  oscillators_switch_to_hf_rc();
#endif

  /* We pulled the plug, so we need to restore the status manually */
  smartrf_settings_cmd_prop_rx_adv.status = RF_CORE_RADIO_OP_STATUS_IDLE;

  /*
   * Just in case there was an ongoing RX (which started after we begun the
   * shutdown sequence), we don't want to leave the buffer in state == ongoing
   */
  for(i = 0; i < PROP_MODE_RX_BUF_CNT; i++) {
    entry = (rfc_dataEntry_t *)rx_buf[i];
    if(entry->status == DATA_ENTRY_STATUS_BUSY) {
       entry->status = DATA_ENTRY_STATUS_PENDING;
    }
  }

  rf_setup_is_completed = false;

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
/* Enable or disable CCA before sending */
static radio_result_t
set_send_on_cca(uint8_t enable)
{
  if(enable) {
    /* this driver does not have support for CCA on Tx */
    return RADIO_RESULT_NOT_SUPPORTED;
  }
  return RADIO_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  if(!value) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    /* On / off */
    *value = rf_is_on() ? RADIO_POWER_MODE_ON : RADIO_POWER_MODE_OFF;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    *value = (radio_value_t)get_channel();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RX_MODE:
    *value = 0;
    if(rf_core_poll_mode) {
      *value |= RADIO_RX_MODE_POLL_MODE;
    }
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    *value = 0;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    *value = get_tx_power();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CCA_THRESHOLD:
    *value = rssi_threshold;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RSSI:
    *value = get_rssi();

    if(*value == RF_CORE_CMD_CCA_REQ_RSSI_UNKNOWN) {
      return RADIO_RESULT_ERROR;
    } else {
      return RADIO_RESULT_OK;
    }
  case RADIO_CONST_CHANNEL_MIN:
    *value = 0;
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MAX:
    *value = DOT_15_4G_CHANNEL_MAX;
    return RADIO_RESULT_OK;
  case RADIO_CONST_TXPOWER_MIN:
    *value = TX_POWER_DRIVER[get_tx_power_array_last_element()].dbm;
    return RADIO_RESULT_OK;
  case RADIO_CONST_TXPOWER_MAX:
    *value = OUTPUT_POWER_MAX;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_LAST_RSSI:
    *value = rf_core_last_rssi;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_LAST_LINK_QUALITY:
    *value = rf_core_last_corr_lqi;
    return RADIO_RESULT_OK;
  case RADIO_CONST_PHY_OVERHEAD:
    /* 2 header bytes, 2 or 4 bytes CRC */
    *value = (radio_value_t)(DOT_4G_PHR_LEN + CRC_LEN);
    return RADIO_RESULT_OK;
  case RADIO_CONST_BYTE_AIR_TIME:
    *value = (radio_value_t)RADIO_BYTE_AIR_TIME;
    return RADIO_RESULT_OK;
  case RADIO_CONST_DELAY_BEFORE_TX:
    *value = (radio_value_t)RADIO_DELAY_BEFORE_TX;
    return RADIO_RESULT_OK;
  case RADIO_CONST_DELAY_BEFORE_RX:
    *value = (radio_value_t)RADIO_DELAY_BEFORE_RX;
    return RADIO_RESULT_OK;
  case RADIO_CONST_DELAY_BEFORE_DETECT:
    *value = (radio_value_t)RADIO_DELAY_BEFORE_DETECT;
    return RADIO_RESULT_OK;
  case RADIO_CONST_MAX_PAYLOAD_LEN:
    *value = (radio_value_t)MAX_PAYLOAD_LEN;
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  radio_result_t rv = RADIO_RESULT_OK;
  uint8_t old_poll_mode;

  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    if(value == RADIO_POWER_MODE_ON) {
      if(on() != RF_CORE_CMD_OK) {
        PRINTF("set_value: on() failed (1)\n");
        return RADIO_RESULT_ERROR;
      }
      return RADIO_RESULT_OK;
    }
    if(value == RADIO_POWER_MODE_OFF) {
      off();
      return RADIO_RESULT_OK;
    }
    return RADIO_RESULT_INVALID_VALUE;
  case RADIO_PARAM_CHANNEL:
    if(value < 0 ||
       value > DOT_15_4G_CHANNEL_MAX) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    if(get_channel() == (uint8_t)value) {
      /* We already have that very same channel configured.
       * Nothing to do here. */
      return RADIO_RESULT_OK;
    }

    set_channel((uint8_t)value);
    break;

  case RADIO_PARAM_RX_MODE:
    if(value & ~(RADIO_RX_MODE_POLL_MODE)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    old_poll_mode = rf_core_poll_mode;
    rf_core_poll_mode = (value & RADIO_RX_MODE_POLL_MODE) != 0;
    if(rf_core_poll_mode == old_poll_mode) {
      return RADIO_RESULT_OK;
    }
    break;

  case RADIO_PARAM_TX_MODE:
    if(value & ~(RADIO_TX_MODE_SEND_ON_CCA)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    return set_send_on_cca((value & RADIO_TX_MODE_SEND_ON_CCA) != 0);

  case RADIO_PARAM_TXPOWER:
    if(value < TX_POWER_DRIVER[get_tx_power_array_last_element()].dbm ||
       value > OUTPUT_POWER_MAX) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    soft_off_prop();

    set_tx_power(value);

    if(soft_on_prop() != RF_CORE_CMD_OK) {
      PRINTF("set_value: soft_on_prop() failed\n");
      rv = RADIO_RESULT_ERROR;
    }

    return rv;

  case RADIO_PARAM_CCA_THRESHOLD:
    rssi_threshold = (int8_t)value;
    break;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }

  /* If off, the new configuration will be applied the next time radio is started */
  if(!rf_is_on()) {
    return RADIO_RESULT_OK;
  }

  /* If we reach here we had no errors. Apply new settings */
  if(rx_off_prop() != RF_CORE_CMD_OK) {
    PRINTF("set_value: rx_off_prop() failed\n");
    rv = RADIO_RESULT_ERROR;
  }

  /* Restart the radio timer (RAT).
     This causes resynchronization between RAT and RTC: useful for TSCH. */
  if(rf_core_restart_rat() != RF_CORE_CMD_OK) {
    PRINTF("set_value: rf_core_restart_rat() failed\n");
    /* do not set the error */
  } else {
    rf_core_check_rat_overflow();
  }

  if(soft_on_prop() != RF_CORE_CMD_OK) {
    PRINTF("set_value: soft_on_prop() failed\n");
    rv = RADIO_RESULT_ERROR;
  }

  return rv;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  if(param == RADIO_PARAM_LAST_PACKET_TIMESTAMP) {
    if(size != sizeof(rtimer_clock_t) || !dest) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    *(rtimer_clock_t *)dest = rf_core_last_packet_timestamp;

    return RADIO_RESULT_OK;
  }

  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver prop_mode_driver = {
  init,
  prepare,
  transmit,
  send,
  read_frame,
  channel_clear,
  receiving_packet,
  pending_packet,
  on,
  off,
  get_value,
  set_value,
  get_object,
  set_object,
};
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
