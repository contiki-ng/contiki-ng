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
/*---------------------------------------------------------------------------*/
/* RF Core Mailbox API */
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_data_entry.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_mailbox.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
/* RF settings */
#ifdef PROP_MODE_CONF_RF_SETTINGS
#   define PROP_MODE_RF_SETTINGS  PROP_MODE_CONF_RF_SETTINGS
#else
#   define PROP_MODE_RF_SETTINGS "prop-settings.h"
#endif

#include PROP_MODE_RF_SETTINGS
/*---------------------------------------------------------------------------*/
/* Platform RF dev */
#include "rf-common.h"
#include "dot-15-4g.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
/*---------------------------------------------------------------------------*/
#ifdef NDEBUG
#   define PRINTF(...)
#else
#   define PRINTF(...)  printf(__VA_ARGS__)
#endif
/*---------------------------------------------------------------------------*/
/* Data whitener. 1: Whitener, 0: No whitener */
#ifdef PROP_MODE_CONF_DW
#   define PROP_MODE_DW  PROP_MODE_CONF_DW
#else
#   define PROP_MODE_DW  0
#endif

#ifdef PROP_MODE_CONF_USE_CRC16
#   define PROP_MODE_USE_CRC16  PROP_MODE_CONF_USE_CRC16
#else
#   define PROP_MODE_USE_CRC16  0
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
/* Configuration for TX power table */
#ifdef PROP_MODE_CONF_TX_POWER_TABLE
#   define TX_POWER_TABLE  PROP_MODE_CONF_TX_POWER_TABLE
#else
#   define TX_POWER_TABLE  propTxPowerTable
#endif
/*---------------------------------------------------------------------------*/
/* TX power table convenience macros */
#define TX_POWER_TABLE_SIZE  ((sizeof(TX_POWER_TABLE) / sizeof(TX_POWER_TABLE[0])) - 1)

#define TX_POWER_MIN  (TX_POWER_TABLE[0].power)
#define TX_POWER_MAX  (TX_POWER_TABLE[TX_POWER_TABLE_SIZE - 1].power)

#define TX_POWER_IN_RANGE(dbm)  (((dbm) >= TX_POWER_MIN) && ((dbm) <= TX_POWER_MAX))
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
#define cmd_radio_setup   ((volatile rfc_CMD_PROP_RADIO_DIV_SETUP_t *)&RF_cmdPropRadioDivSetup)
#define cmd_fs            ((volatile rfc_CMD_FS_t *)&RF_cmdPropFs)
#define cmd_tx            ((volatile rfc_CMD_PROP_TX_ADV_t *)&RF_cmdPropTxAdv)
#define cmd_rx            ((volatile rfc_CMD_PROP_RX_ADV_t *)&RF_cmdPropRxAdv)
/*---------------------------------------------------------------------------*/
/* RF driver */
static RF_Object rfObject;
static RF_Handle rfHandle;
/*---------------------------------------------------------------------------*/
static CC_INLINE bool rf_is_transmitting(void) { return cmd_tx->status == ACTIVE; }
static CC_INLINE bool rf_is_receiving(void) { return cmd_rx->status == ACTIVE; }
static CC_INLINE bool rf_is_on(void) { return rf_is_transmitting() || rf_is_receiving(); }
/*---------------------------------------------------------------------------*/
static void
rf_rx_callback(RF_Handle client, RF_CmdHandle command, RF_EventMask events)
{
    if (events & RF_EventRxEntryDone) {
        process_poll(&RF_coreProcess);
    }
}
/*---------------------------------------------------------------------------*/
static CmdResult
rf_start_rx()
{
    cmd_rx->status = IDLE;

    /*
    * Set the max Packet length. This is for the payload only, therefore
    * 2047 - length offset
    */
    cmd_rx->maxPktLen = DOT_4G_MAX_FRAME_LEN - cmd_rx->lenOffset;

    RF_CmdHandle rxCmdHandle = RF_postCmd(rfHandle, (RF_Op*)cmd_rx, RF_PriorityNormal,
                                          &rf_rx_callback, RF_EventRxEntryDone);
    if (rxCmdHandle == RF_ALLOC_ERROR) {
        return CMD_RESULT_ERROR;
    }

    /* Wait to enter RX */
    const rtimer_clock_t t0 = RTIMER_NOW();
    while (!rf_is_receiving() &&
        (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + ENTER_RX_WAIT_TIMEOUT)));

    if (!rf_is_receiving()) {
        PRINTF("RF_cmdPropRxAdv: handle=0x%08lx, status=0x%04x\n",
               (unsigned long)rxCmdHandle, cmd_rx->status);
        rf_switch_off();
        return CMD_RESULT_ERROR;
    }

    return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static CmdResult
rf_stop_rx(void)
{
    /* If we are off, do nothing */
    if (!rf_is_receiving()) {
        return CMD_RESULT_OK;
    }

    /* Abort any ongoing operation. Don't care about the result. */
    RF_cancelCmd(rfHandle, RF_CMDHANDLE_FLUSH_ALL, 1);

    /* Todo: maybe do a RF_pendCmd() to synchronize with command execution. */

    if(cmd_rx->status != PROP_DONE_STOPPED &&
       cmd_rx->status != PROP_DONE_ABORT) {
        PRINTF("RF_cmdPropRxAdv cancel: status=0x%04x\n",
               cmd_rx->status);
        return CMD_RESULT_ERROR;
    }

    /* Stopped gracefully */
    ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
    return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static CmdResult
rf_run_setup()
{
    RF_runCmd(rfHandle, (RF_Op*)cmd_radio_setup, RF_PriorityNormal, NULL, 0);
    if (cmd_radio_setup->status != PROP_DONE_OK) {
        return CMD_RESULT_ERROR;
    }

    return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_value_t
get_rssi(void)
{
    if (rf_is_transmitting()) {
        PRINTF("get_rssi: called while in TX\n");
        return RF_GET_RSSI_ERROR_VAL;
    }

    const bool was_off = !rf_is_receiving();
    if (was_off && rf_start_rx() == CMD_RESULT_ERROR) {
        PRINTF("get_rssi: unable to start RX\n");
        return RF_GET_RSSI_ERROR_VAL;
    }

    int8_t rssi = RF_GET_RSSI_ERROR_VAL;
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

  freq_khz = cmd_fs->frequency * 1000;

  /*
   * For some channels, fractFreq * 1000 / 65536 will return 324.99xx.
   * Casting the result to uint32_t will truncate decimals resulting in the
   * function returning channel - 1 instead of channel. Thus, we do a quick
   * positive integer round up.
   */
  freq_khz += (((cmd_fs->fractFreq * 1000) + 65535) / 65536);

  return (freq_khz - DOT_15_4G_CHAN0_FREQUENCY) / DOT_15_4G_CHANNEL_SPACING;
}
/*---------------------------------------------------------------------------*/
static void
set_channel(uint8_t channel)
{
    uint32_t new_freq = DOT_15_4G_CHAN0_FREQUENCY + (channel * DOT_15_4G_CHANNEL_SPACING);

    uint16_t freq = (uint16_t)(new_freq / 1000);
    uint16_t frac = (new_freq - (freq * 1000)) * 65536 / 1000;

    PRINTF("set_channel: %u = 0x%04x.0x%04x (%lu)\n",
           channel, freq, frac, new_freq);

    cmd_radio_setup->centerFreq = freq;
    cmd_fs->frequency = freq;
    cmd_fs->fractFreq = frac;

    // Todo: Need to re-run setup command when deviation from previous frequency
    // is too large
    // rf_run_setup();

    // We don't care whether the FS command is successful because subsequent
    // TX and RX commands will tell us indirectly.
    RF_postCmd(rfHandle, (RF_Op*)cmd_fs, RF_PriorityNormal, NULL, 0);
}
/*---------------------------------------------------------------------------*/
/* Returns the current TX power in dBm */
static radio_value_t
get_tx_power(void)
{
  const RF_TxPowerTable_Value value  = RF_getTxPower(rfHandle);
  return (radio_value_t)RF_TxPowerTable_findPowerLevel(TX_POWER_TABLE, value);
}
/*---------------------------------------------------------------------------*/
/*
 * The caller must make sure to send a new CMD_PROP_RADIO_DIV_SETUP to the
 * radio after calling this function.
 */
static radio_result_t
set_tx_power(const radio_value_t power)
{
  if (!TX_POWER_IN_RANGE(power)) {
    return RADIO_RESULT_INVALID_VALUE;
  }
  const RF_TxPowerTable_Value value = RF_TxPowerTable_findValue(TX_POWER_TABLE, power);
  RF_Stat stat = RF_setTxPower(rfHandle, value);

  return (stat == RF_StatSuccess)
    ? RADIO_RESULT_OK
    : RADIO_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
static void
init_rx_buffers(void)
{
  rfc_dataEntry_t *entry;
  int i;

  for(i = 0; i < PROP_MODE_RX_BUF_CNT; i++) {
    entry = (rfc_dataEntry_t *)rx_buf[i];
    entry->status = DATA_ENTRY_PENDING;
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

    if (rf_is_transmitting()) {
        PRINTF("transmit: not allowed while transmitting\n");
        return RADIO_TX_ERR;
    } else if (rf_is_receiving()) {
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

    /*
    * pktLen: Total number of bytes in the TX buffer, including the header if
    * one exists, but not including the CRC (which is not present in the buffer)
    */
    cmd_tx->pktLen = transmit_len + DOT_4G_PHR_LEN;
    cmd_tx->pPkt = tx_buf;

    // TODO: Register callback
    RF_runCmd(rfHandle, (RF_Op*)cmd_tx, RF_PriorityNormal, NULL, 0);
//    if (txHandle == RF_ALLOC_ERROR)
//    {
//        /* Failure sending the CMD_PROP_TX command */
//        PRINTF("transmit: PROP_TX_ERR ret=%d, CMDSTA=0x%08lx, status=0x%04x\n",
//            ret, cmd_status, cmd_tx_adv->status);
//        return RADIO_TX_ERR;
//    }
//
//    ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
//
//    // watchdog_periodic();
//
//    /* Idle away while the command is running */
//    RF_pendCmd(rfHandle, txHandle, RF_EventLastCmdDone);

    if(cmd_tx->status == PROP_DONE_OK) {
      /* Sent OK */
      ret = RADIO_TX_OK;
    } else {
      /* Operation completed, but frame was not sent */
      PRINTF("transmit: Not Sent OK status=0x%04x\n",
             cmd_tx->status);
      ret = RADIO_TX_ERR;
    }

    ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);

    /* Workaround. Set status to IDLE */
    cmd_tx->status = IDLE;

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

    if(entry->status == DATA_ENTRY_FINISHED) {

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
        entry->status = DATA_ENTRY_PENDING;
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

    if (rf_is_transmitting()) {
        PRINTF("channel_clear: called while in TX\n");
        return RF_CCA_CLEAR;
    } else if (!rf_is_receiving()) {
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
    if(!rf_is_receiving()) {
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
        if(entry->status == DATA_ENTRY_FINISHED) {
            rv += 1;
            process_poll(&RF_coreProcess);
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
//    return CMD_RESULT_OK;
//  }

    // Force abort of any ongoing RF operation.
    RF_cancelCmd(rfHandle, RF_CMDHANDLE_FLUSH_ALL, 0);

    // Trigger a manual power-down
    RF_yield(rfHandle);

    /* We pulled the plug, so we need to restore the status manually */
    cmd_rx->status = IDLE;

    return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  if (!value) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch (param) {
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
    return (*value == RF_CMD_CCA_REQ_RSSI_UNKNOWN)
      ? RADIO_RESULT_ERROR
      : RADIO_RESULT_OK;

  case RADIO_CONST_CHANNEL_MIN:
    *value = 0;
    return RADIO_RESULT_OK;

  case RADIO_CONST_CHANNEL_MAX:
    *value = DOT_15_4G_CHANNEL_MAX;
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MIN:
    *value = (radio_value_t)TX_POWER_MIN;
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MAX:
    *value = (radio_value_t)TX_POWER_MAX;
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
    return set_tx_power(value);

  case RADIO_PARAM_RX_MODE:
    return RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    rssi_threshold = (int8_t)value;
    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }

  /* If we reach here we had no errors. Apply new settings */
  if (rf_is_receiving()) {
      rf_stop_rx();
      if (rf_run_setup() != CMD_RESULT_OK) {
          return RADIO_RESULT_ERROR;
      }
      rf_start_rx();
  } else if (rf_is_transmitting()) {
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
    // Disable automatic power-down just to not interfere with stack timing
    params.nInactivityTimeout = 0;

    rfHandle = RF_open(&rfObject, &RF_propMode, (RF_RadioSetup*)cmd_radio_setup, &params);
    assert(rfHandle != NULL);

    /* Initialise RX buffers */
    memset(rx_buf, 0, sizeof(rx_buf));

    /* Set of RF Core data queue. Circular buffer, no last entry */
    rx_data_queue.pCurrEntry = rx_buf[0];
    rx_data_queue.pLastEntry = NULL;

    /* Initialize current read pointer to first element (used in ISR) */
    rx_read_entry = rx_buf[0];

    cmd_rx->pQueue = &rx_data_queue;
    cmd_rx->pOutput = (uint8_t *)&rx_stats;

    set_channel(RF_CORE_CHANNEL);

    ENERGEST_ON(ENERGEST_TYPE_LISTEN);

    process_start(&RF_coreProcess, NULL);

    return CMD_RESULT_OK;
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
