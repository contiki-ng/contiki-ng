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
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "sys/energest.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "sys/cc.h"
#include "dev/watchdog.h"
#include "proprietary-rf.h"
#include "rf-core.h"
#include "dot-15-4g.h"
/*---------------------------------------------------------------------------*/
/* RF core and RF HAL API */
#include <inc/hw_rfc_dbell.h>
#include <inc/hw_rfc_pwr.h>
/*---------------------------------------------------------------------------*/
/* RF Core Mailbox API */
#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_prop_mailbox.h>
#include <driverlib/rf_prop_cmd.h>
#include <rf-settings/proprietary-rf-settings.h>
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
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
/* Special value returned by CMD_IEEE_CCA_REQ when an RSSI is not available */
#define RF_CMD_CCA_REQ_RSSI_UNKNOWN     -128

/* Used for the return value of channel_clear */
#define RF_CCA_CLEAR                       1
#define RF_CCA_BUSY                        0

/* Used as an error return value for get_cca_info */
#define RF_GET_CCA_INFO_ERROR           0xFF

/*
 * Values of the individual bits of the ccaInfo field in CMD_IEEE_CCA_REQ's
 * status struct
 */
#define RF_CMD_CCA_REQ_CCA_STATE_IDLE      0 /* 00 */
#define RF_CMD_CCA_REQ_CCA_STATE_BUSY      1 /* 01 */
#define RF_CMD_CCA_REQ_CCA_STATE_INVALID   2 /* 10 */

#ifdef PROP_MODE_CONF_RSSI_THRESHOLD
#define PROP_MODE_RSSI_THRESHOLD PROP_MODE_CONF_RSSI_THRESHOLD
#else
#define PROP_MODE_RSSI_THRESHOLD 0xA6
#endif

static int8_t rssi_threshold = PROP_MODE_RSSI_THRESHOLD;
/*---------------------------------------------------------------------------*/
static int rf_switch_on(void);
static int rf_switch_off(void);

static rfc_propRxOutput_t rx_stats;
/*---------------------------------------------------------------------------*/
/* Defines and variables related to the .15.4g PHY HDR */
#define DOT_4G_MAX_FRAME_LEN    2047
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
/* How long to wait for an ongoing ACK TX to finish before starting frame TX */
#define TX_WAIT_TIMEOUT       (RTIMER_SECOND >> 11)

/* How long to wait for the RF to enter RX in rf_cmd_ieee_rx */
#define ENTER_RX_WAIT_TIMEOUT (RTIMER_SECOND >> 10)
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

/*
 * RX buffers.
 * PROP_MODE_RX_BUF_CNT buffers of RX_BUF_SIZE bytes each. The start of each
 * buffer must be 4-byte aligned, therefore RX_BUF_SIZE must divide by 4
 */
#define RX_BUF_SIZE 140
static uint8_t rx_buf[PROP_MODE_RX_BUF_CNT][RX_BUF_SIZE] CC_ALIGN(4);

/* The RX Data Queue */
static dataQueue_t rx_data_queue = { 0 };

/* Receive entry pointer to keep track of read items */
volatile static uint8_t *rx_read_entry;
/*---------------------------------------------------------------------------*/
/* The outgoing frame buffer */
#define TX_BUF_PAYLOAD_LEN 180
#define TX_BUF_HDR_LEN       2

static uint8_t tx_buf[TX_BUF_HDR_LEN + TX_BUF_PAYLOAD_LEN] CC_ALIGN(4);
/*---------------------------------------------------------------------------*/
/* RF driver */
static RF_Object rfObject;
static RF_Handle rfHandle;
static RF_CmdHandle rxCmdHandle = RF_ALLOC_ERROR;
/*---------------------------------------------------------------------------*/
PROCESS(rf_core_process, "CC13xx / CC26xx RF driver");

static uint8_t
rf_transmitting(void)
{
    return smartrf_settings_cmd_prop_tx_adv.status == ACTIVE;
}
/*---------------------------------------------------------------------------*/
static uint8_t
rf_receiving(void)
{
    return smartrf_settings_cmd_prop_rx_adv.status == ACTIVE;
}
/*---------------------------------------------------------------------------*/
static uint8_t
rf_is_on(void)
{
    return rf_receiving() | rf_transmitting();
}
/*---------------------------------------------------------------------------*/
static void
rf_rx_callback(RF_Handle client, RF_CmdHandle command, RF_EventMask events)
{
    if (events & RF_EventRxEntryDone) {
        process_poll(&rf_core_process);
    }
}
/*---------------------------------------------------------------------------*/
static uint8_t
rf_start_rx()
{
    rtimer_clock_t t0;
    volatile rfc_CMD_PROP_RX_ADV_t *cmd_rx_adv;

    cmd_rx_adv = (rfc_CMD_PROP_RX_ADV_t *)&smartrf_settings_cmd_prop_rx_adv;
    cmd_rx_adv->status = IDLE;

    /*
    * Set the max Packet length. This is for the payload only, therefore
    * 2047 - length offset
    */
    cmd_rx_adv->maxPktLen = DOT_4G_MAX_FRAME_LEN - cmd_rx_adv->lenOffset;

    rxCmdHandle = RF_postCmd(rfHandle, (RF_Op*)&smartrf_settings_cmd_prop_rx_adv, RF_PriorityNormal, &rf_rx_callback, RF_EventRxEntryDone);
    if (rxCmdHandle == RF_ALLOC_ERROR) {
        return RF_CORE_CMD_ERROR;
    }

    t0 = RTIMER_NOW();

    while(cmd_rx_adv->status != ACTIVE &&
        (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + ENTER_RX_WAIT_TIMEOUT)));

    /* Wait to enter RX */
    if(cmd_rx_adv->status != ACTIVE) {
        PRINTF("rf_cmd_prop_rx: CMDSTA=0x%08lx, status=0x%04x\n",
               md_status, cmd_rx_adv->status);
        rf_switch_off();
        return RF_CORE_CMD_ERROR;
    }

  return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
static int
rf_stop_rx(void)
{
    int ret;

    /* If we are off, do nothing */
    if(!rf_receiving()) {
        return RF_CORE_CMD_OK;
    }

    /* Abort any ongoing operation. Don't care about the result. */
    RF_cancelCmd(rfHandle, RF_CMDHANDLE_FLUSH_ALL, 1);

    /* Todo: maybe do a RF_pendCmd() to synchronise with command execution. */

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
}/*---------------------------------------------------------------------------*/
static uint8_t
rf_run_setup()
{
    RF_runCmd(rfHandle, (RF_Op*)&smartrf_settings_cmd_prop_radio_div_setup, RF_PriorityNormal, NULL, 0);
    if (((volatile RF_Op*)&smartrf_settings_cmd_prop_radio_div_setup)->status != PROP_DONE_OK) {
        return RF_CORE_CMD_ERROR;
    }

    return RF_CORE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
static radio_value_t
get_rssi(void)
{
    int8_t rssi = RF_GET_RSSI_ERROR_VAL;
    uint8_t was_off = 0;

    if (rf_transmitting()) {
        PRINTF("channel_clear: called while in TX\n");
        return RF_CCA_CLEAR;
    } else if (!rf_receiving()) {
        was_off = 1;
        rf_start_rx();
    }

    while(rssi == RF_GET_RSSI_ERROR_VAL || rssi == 0) {
        rssi = RF_getRssi(rfHandle);
    }

    if(was_off) {
        rf_switch_off();
    }

    return rssi;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_channel(void)
{
  uint32_t freq_khz;

  freq_khz = smartrf_settings_cmd_prop_fs.frequency * 1000;

  /*
   * For some channels, fractFreq * 1000 / 65536 will return 324.99xx.
   * Casting the result to uint32_t will truncate decimals resulting in the
   * function returning channel - 1 instead of channel. Thus, we do a quick
   * positive integer round up.
   */
  freq_khz += (((smartrf_settings_cmd_prop_fs.fractFreq * 1000) + 65535) / 65536);

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
    smartrf_settings_cmd_prop_fs.frequency = freq;
    smartrf_settings_cmd_prop_fs.fractFreq = frac;

    // Todo: Need to re-run setup command when deviation from previous frequency
    // is too large
    // rf_run_setup();

    // We don't care whether the FS command is successful because subsequent
    // TX and RX commands will tell us indirectly.
    RF_postCmd(rfHandle, (RF_Op*)&smartrf_settings_cmd_prop_fs, RF_PriorityNormal, NULL, 0);
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
    entry->pNextEntry = rx_buf[i + 1];
  }

  ((rfc_dataEntry_t *)rx_buf[PROP_MODE_RX_BUF_CNT - 1])->pNextEntry = rx_buf[0];
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
    int len = MIN(payload_len, TX_BUF_PAYLOAD_LEN);

    memcpy(&tx_buf[TX_BUF_HDR_LEN], payload, len);
    return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
    int ret;
    uint8_t was_off = 0;
    volatile rfc_CMD_PROP_TX_ADV_t *cmd_tx_adv;

    if (rf_transmitting()) {
        PRINTF("transmit: not allowed while transmitting\n");
        return RADIO_TX_ERR;
    } else if (rf_receiving()) {
        rf_stop_rx();
    } else {
        was_off = 1;
    }

    /* Length in .15.4g PHY HDR. Includes the CRC but not the HDR itself */
    uint16_t total_length;

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

    // TODO: Register callback
    RF_CmdHandle txHandle = RF_postCmd(rfHandle, (RF_Op*)&cmd_tx_adv, RF_PriorityNormal, NULL, 0);
    if (txHandle == RF_ALLOC_ERROR)
    {
        /* Failure sending the CMD_PROP_TX command */
        PRINTF("transmit: PROP_TX_ERR ret=%d, CMDSTA=0x%08lx, status=0x%04x\n",
            ret, cmd_status, cmd_tx_adv->status);
        return RADIO_TX_ERR;
    }

    ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

    // watchdog_periodic();

    /* Idle away while the command is running */
    RF_pendCmd(rfHandle, txHandle, 0);

    if(cmd_tx_adv->status == PROP_DONE_OK) {
      /* Sent OK */
      ret = RADIO_TX_OK;
    } else {
      /* Operation completed, but frame was not sent */
      PRINTF("transmit: Not Sent OK status=0x%04x\n",
             cmd_tx_adv->status);
      ret = RADIO_TX_ERR;
    }

    ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);

    /* Workaround. Set status to IDLE */
    cmd_tx_adv->status = IDLE;

    if (was_off) {
        RF_yield(rfHandle);
    } else {
        rf_start_rx();
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
static int
read_frame(void *buf, unsigned short buf_len)
{
    rfc_dataEntryGeneral_t *entry = (rfc_dataEntryGeneral_t *)rx_read_entry;
    uint8_t *data_ptr = &entry->data;
    int len = 0;

    if(entry->status == DATA_ENTRY_STATUS_FINISHED) {

        /*
         * First 2 bytes in the data entry are the length.
         * Our data entry consists of: Payload + RSSI (1 byte) + Status (1 byte)
         * This length includes all of those.
         */
        len = (*(uint16_t *)data_ptr);
        data_ptr += 2;
        len -= 2;

        if(len > 0) {
            if(len <= buf_len) {
                memcpy(buf, data_ptr, len);
            }

            packetbuf_set_attr(PACKETBUF_ATTR_RSSI, (int8_t)data_ptr[len]);
            packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, 0x7F);
        }

        /* Move read entry pointer to next entry */
        rx_read_entry = entry->pNextEntry;
        entry->status = DATA_ENTRY_STATUS_PENDING;
    }

    return len;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
    uint8_t was_off = 0;
    int8_t rssi = RF_CMD_CCA_REQ_RSSI_UNKNOWN;

//  /*
//   * If we are in the middle of a BLE operation, we got called by ContikiMAC
//   * from within an interrupt context. Indicate a clear channel
//   */
//  if(rf_ble_is_active() == RF_BLE_ACTIVE) {
//    return RF_CCA_CLEAR;
//  }

    if (rf_transmitting()) {
        PRINTF("channel_clear: called while in TX\n");
        return RF_CCA_CLEAR;
    } else if (!rf_receiving()) {
        was_off = 1;
        rf_start_rx();
    }

    while(rssi == RF_CMD_CCA_REQ_RSSI_UNKNOWN || rssi == 0) {
        rssi = RF_getRssi(rfHandle);
    }

    if(was_off) {
        rf_switch_off();
    }

    if(rssi >= rssi_threshold) {
        return RF_CCA_BUSY;
    }

    return RF_CCA_CLEAR;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
    if(!rf_receiving()) {
        return 0;
    }

    if(channel_clear() == RF_CCA_CLEAR) {
        return 0;
    }

    return 1;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
    int rv = 0;
    volatile rfc_dataEntry_t *entry = (rfc_dataEntry_t *)rx_data_queue.pCurrEntry;

    /* Go through all RX buffers and check their status */
    do {
        if(entry->status == DATA_ENTRY_STATUS_FINISHED) {
            rv += 1;
            process_poll(&rf_core_process);
        }

        entry = (rfc_dataEntry_t *)entry->pNextEntry;
    } while(entry != (rfc_dataEntry_t *)rx_data_queue.pCurrEntry);

    /* If we didn't find an entry at status finished, no frames are pending */
    return rv;
}
/*---------------------------------------------------------------------------*/
static int
rf_switch_on(void)
{
    init_rx_buffers();
    return rf_start_rx();
}
/*---------------------------------------------------------------------------*/
static int
rf_switch_off(void)
{
//  /*
//   * If we are in the middle of a BLE operation, we got called by ContikiMAC
//   * from within an interrupt context. Abort, but pretend everything is OK.
//   */
//  if(rf_ble_is_active() == RF_BLE_ACTIVE) {
//    return RF_CORE_CMD_OK;
//  }

    // Force abort of any ongoing RF operation.
    RF_cancelCmd(rfHandle, RF_CMDHANDLE_FLUSH_ALL, 0);

    // Trigger a manual power-down
    RF_yield(rfHandle);

    /* We pulled the plug, so we need to restore the status manually */
    smartrf_settings_cmd_prop_rx_adv.status = IDLE;

    return RF_CORE_CMD_OK;
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
  case RADIO_PARAM_TXPOWER:
    *value = get_tx_power();
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CCA_THRESHOLD:
    *value = rssi_threshold;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RSSI:
    *value = get_rssi();

    if(*value == RF_CMD_CCA_REQ_RSSI_UNKNOWN) {
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
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    if(value == RADIO_POWER_MODE_ON) {
      // Powering on happens implicitly
      return RADIO_RESULT_OK;
    }
    if(value == RADIO_POWER_MODE_OFF) {
      rf_switch_off();
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
  case RADIO_PARAM_TXPOWER:
    if(value < TX_POWER_DRIVER[get_tx_power_array_last_element()].dbm ||
       value > OUTPUT_POWER_MAX) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_tx_power(value);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RX_MODE:
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CCA_THRESHOLD:
    rssi_threshold = (int8_t)value;
    return RADIO_RESULT_OK;
    break;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }

  /* If we reach here we had no errors. Apply new settings */
  if (rf_receiving()) {
      rf_stop_rx();
      if (rf_run_setup() != RF_CORE_CMD_OK) {
          return RADIO_RESULT_ERROR;
      }
      rf_start_rx();
  } else if (rf_transmitting()) {
      // Should not happen. TX is always synchronous and blocking.
      // Todo: maybe remove completely here.
      PRINTF("set_value: cannot apply new value while transmitting. \n");
      return RADIO_RESULT_ERROR;
  } else {
      // was powered off. Nothing to do. New values will be
      // applied automatically on next power-up.
  }

  return RADIO_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
    return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
    return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static int
rf_init(void)
{
    RF_Params params;
    RF_Params_init(&params);
    params.nInactivityTimeout = 0; // disable automatic power-down
                                   // just to not interfere with stack timing

    rfHandle = RF_open(&rfObject, &RF_prop, (RF_RadioSetup*)&smartrf_settings_cmd_prop_radio_div_setup, &params);
    assert(rfHandle != NULL);

    /* Initialise RX buffers */
    memset(rx_buf, 0, sizeof(rx_buf));

    /* Set of RF Core data queue. Circular buffer, no last entry */
    rx_data_queue.pCurrEntry = rx_buf[0];
    rx_data_queue.pLastEntry = NULL;

    /* Initialize current read pointer to first element (used in ISR) */
    rx_read_entry = rx_buf[0];

    smartrf_settings_cmd_prop_rx_adv.pQueue = &rx_data_queue;
    smartrf_settings_cmd_prop_rx_adv.pOutput = (uint8_t *)&rx_stats;

    set_channel(RF_CORE_CHANNEL);

    ENERGEST_ON(ENERGEST_TYPE_LISTEN);

    process_start(&rf_core_process, NULL);

    return 1;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(rf_core_process, ev, data)
{
  int len;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    do {
      watchdog_periodic();
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
const struct radio_driver prop_mode_driver = {
  rf_init,
  prepare,
  transmit,
  send,
  read_frame,
  channel_clear,
  receiving_packet,
  pending_packet,
  rf_switch_on,
  rf_switch_off,
  get_value,
  set_value,
  get_object,
  set_object,
};
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
