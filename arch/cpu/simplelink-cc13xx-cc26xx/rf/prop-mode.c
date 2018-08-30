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
 * \addtogroup cc13xx-cc26xx-rf
 * @{
 *
 * \defgroup cc13xx-cc26xx-rf-prop Prop-mode driver for CC13xx/CC26xx
 *
 * @{
 *
 * \file
 *        Implementation of the CC13xx/CC26xx prop-mode NETSTACK_RADIO driver.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
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
/* Platform RF dev */
#include "rf/dot-15-4g.h"
#include "rf/sched.h"
#include "rf/data-queue.h"
#include "rf/tx-power.h"
#include "rf/settings.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Radio"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
#undef CLAMP
#define CLAMP(v, vmin, vmax)  (MAX(MIN(v, vmax), vmin))
/*---------------------------------------------------------------------------*/
/* Configuration parameters */
#define PROP_MODE_DYN_WHITENER        PROP_MODE_CONF_DW
#define PROP_MODE_USE_CRC16           PROP_MODE_CONF_USE_CRC16
#define PROP_MODE_CENTER_FREQ         PROP_MODE_CONF_CENTER_FREQ
#define PROP_MODE_LO_DIVIDER          PROP_MODE_CONF_LO_DIVIDER
#define PROP_MODE_CCA_RSSI_THRESHOLD  PROP_MODE_CONF_CCA_RSSI_THRESHOLD
/*---------------------------------------------------------------------------*/
/* Used for checking result of CCA_REQ command */
typedef enum {
  CCA_STATE_IDLE = 0,
  CCA_STATE_BUSY = 1,
  CCA_STATE_INVALID = 2
} cca_state_t;
/*---------------------------------------------------------------------------*/
/* Defines and variables related to the .15.4g PHY HDR */
#define DOT_4G_MAX_FRAME_LEN    2047
#define DOT_4G_PHR_LEN          2

/* PHY HDR bits */
#define DOT_4G_PHR_CRC16        0x10
#define DOT_4G_PHR_DW           0x08

#if PROP_MODE_USE_CRC16
/* CRC16 */
#define DOT_4G_PHR_CRC_BIT      DOT_4G_PHR_CRC16
#define CRC_LEN                 2
#else
/* CRC32 */
#define DOT_4G_PHR_CRC_BIT      0
#define CRC_LEN                 4
#endif /* PROP_MODE_USE_CRC16 */

#if PROP_MODE_DYN_WHITENER
#define DOT_4G_PHR_DW_BIT       DOT_4G_PHR_DW
#else
#define DOT_4G_PHR_DW_BIT       0
#endif
/*---------------------------------------------------------------------------*/
/* How long to wait for the RF to enter RX in rf_cmd_ieee_rx */
#define TIMEOUT_ENTER_RX_WAIT   (RTIMER_SECOND >> 10)

/* How long to wait for the rx read entry to become ready */
#define TIMEOUT_DATA_ENTRY_BUSY (RTIMER_SECOND / 250)
/*---------------------------------------------------------------------------*/
/* TX buf configuration */
#define TX_BUF_HDR_LEN          2
#define TX_BUF_PAYLOAD_LEN      180

#define TX_BUF_SIZE             (TX_BUF_HDR_LEN + TX_BUF_PAYLOAD_LEN)
/*---------------------------------------------------------------------------*/
/* Size of the Length field in Data Entry, two bytes in this case */
typedef uint16_t lensz_t;

#define FRAME_OFFSET            sizeof(lensz_t)
#define FRAME_SHAVE             2   /**< RSSI (1) + Status (1) */
/*---------------------------------------------------------------------------*/
/* Constants used when calculating the LQI from the RSSI */
#define RX_SENSITIVITY_DBM                -110
#define RX_SATURATION_DBM                 10
#define ED_MIN_DBM_ABOVE_RX_SENSITIVITY   10
#define ED_MAX                            0xFF

#define ED_RF_POWER_MIN_DBM               (RX_SENSITIVITY_DBM + ED_MIN_DBM_ABOVE_RX_SENSITIVITY)
#define ED_RF_POWER_MAX_DBM               RX_SATURATION_DBM
/*---------------------------------------------------------------------------*/
/* RF Core typedefs */
typedef rfc_propRxOutput_t rx_output_t;

typedef struct {
  /* Outgoing frame buffer */
  uint8_t tx_buf[TX_BUF_SIZE] CC_ALIGN(4);

  /* RX Statistics struct */
  rx_output_t rx_stats;

  /* RSSI Threshold */
  int8_t rssi_threshold;
  uint16_t channel;

  /* Indicates RF is supposed to be on or off */
  uint8_t rf_is_on;

  /* RF driver */
  RF_Handle rf_handle;
} prop_radio_t;

static prop_radio_t prop_radio;
/*---------------------------------------------------------------------------*/
/* Convenience macros for volatile access with the RF commands */
#define cmd_radio_setup   (*(volatile rfc_CMD_PROP_RADIO_DIV_SETUP_t *)&rf_cmd_prop_radio_div_setup)
#define cmd_fs            (*(volatile rfc_CMD_FS_t *)                  &rf_cmd_prop_fs)
#define cmd_tx            (*(volatile rfc_CMD_PROP_TX_ADV_t *)         &rf_cmd_prop_tx_adv)
#define cmd_rx            (*(volatile rfc_CMD_PROP_RX_ADV_t *)         &rf_cmd_prop_rx_adv)
/*---------------------------------------------------------------------------*/
static inline bool
tx_is_active(void)
{
  return cmd_tx.status == ACTIVE;
}
/*---------------------------------------------------------------------------*/
static inline bool
rx_is_active(void)
{
  return cmd_rx.status == ACTIVE;
}
/*---------------------------------------------------------------------------*/
static int on(void);
static int off(void);
/*---------------------------------------------------------------------------*/
static void
init_rf_params(void)
{
  cmd_radio_setup.centerFreq = PROP_MODE_CENTER_FREQ;
  cmd_radio_setup.loDivider = PROP_MODE_LO_DIVIDER;

  data_queue_t *data_queue = data_queue_init(sizeof(lensz_t));

  cmd_rx.maxPktLen = DOT_4G_MAX_FRAME_LEN - cmd_rx.lenOffset;
  cmd_rx.pQueue = data_queue;
  cmd_rx.pOutput = (uint8_t *)&prop_radio.rx_stats;
}
/*---------------------------------------------------------------------------*/
static int8_t
get_rssi(void)
{
  rf_result_t res;

  const bool rx_is_idle = !rx_is_active();

  if(rx_is_idle) {
    res = netstack_sched_rx(false);
    if(res != RF_RESULT_OK) {
      return RF_GET_RSSI_ERROR_VAL;
    }
  }

  const rtimer_clock_t t0 = RTIMER_NOW();
  while((cmd_rx.status != ACTIVE) &&
        RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + TIMEOUT_ENTER_RX_WAIT)) ;

  int8_t rssi = RF_GET_RSSI_ERROR_VAL;
  if(rx_is_active()) {
    rssi = RF_getRssi(prop_radio.rf_handle);
  }

  if(rx_is_idle) {
    netstack_stop_rx();
  }

  return rssi;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_channel(void)
{
  uint32_t freq_khz = cmd_fs.frequency * 1000;

  /*
   * For some channels, fractFreq * 1000 / 65536 will return 324.99xx.
   * Casting the result to uint32_t will truncate decimals resulting in the
   * function returning channel - 1 instead of channel. Thus, we do a quick
   * positive integer round up.
   */
  freq_khz += (((cmd_fs.fractFreq * 1000) + 65535) / 65536);

  return (uint8_t)((freq_khz - DOT_15_4G_CHAN0_FREQ) / DOT_15_4G_FREQ_SPACING);
}
/*---------------------------------------------------------------------------*/
static rf_result_t
set_channel(uint16_t channel)
{
  rf_result_t res;

  if(!dot_15_4g_chan_in_range(channel)) {
    LOG_WARN("Supplied hannel %d is illegal, defaults to %d\n",
             (int)channel, DOT_15_4G_DEFAULT_CHAN);
    channel = DOT_15_4G_DEFAULT_CHAN;
  }

  if(channel == prop_radio.channel) {
    /* We are already calibrated to this channel */
    return RF_RESULT_OK;
  }

  const uint32_t new_freq = dot_15_4g_freq(channel);
  const uint16_t freq = (uint16_t)(new_freq / 1000);
  const uint16_t frac = (uint16_t)(((new_freq - (freq * 1000)) * 0x10000) / 1000);

  LOG_DBG("Set channel to %d, frequency 0x%04X.0x%04X (%lu)\n",
          (int)channel, freq, frac, new_freq);

  cmd_fs.frequency = freq;
  cmd_fs.fractFreq = frac;

  res = netstack_sched_fs();

  if(res != RF_RESULT_OK) {
    return res;
  }

  prop_radio.channel = channel;
  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static uint8_t
calculate_lqi(int8_t rssi)
{
  /*
   * Note : Currently the LQI value is simply the energy detect measurement.
   *        A more accurate value could be derived by using the correlation
   *        value along with the RSSI value.
   */
  rssi = CLAMP(rssi, ED_RF_POWER_MIN_DBM, ED_RF_POWER_MAX_DBM);

  /*
   * Create energy detect measurement by normalizing and scaling RF power level.
   * Note : The division operation below is designed for maximum accuracy and
   *        best granularity. This is done by grouping the math operations to
   *        compute the entire numerator before doing any division.
   */
  return (ED_MAX * (rssi - ED_RF_POWER_MIN_DBM)) / (ED_RF_POWER_MAX_DBM - ED_RF_POWER_MIN_DBM);
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  const size_t len = MIN((size_t)payload_len,
                         (size_t)TX_BUF_PAYLOAD_LEN);

  memcpy(prop_radio.tx_buf + TX_BUF_HDR_LEN, payload, len);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  rf_result_t res;

  if(tx_is_active()) {
    LOG_ERR("A transmission is already active\n");
    return RADIO_TX_ERR;
  }

  /* Length in .15.4g PHY HDR. Includes the CRC but not the HDR itself */
  const uint16_t total_length = transmit_len + CRC_LEN;
  /*
   * Prepare the .15.4g PHY header
   * MS=0, Length MSBits=0, DW and CRC configurable
   * Total length = transmit_len (payload) + CRC length
   *
   * The Radio will flip the bits around, so tx_buf[0] must have the length
   * LSBs (PHR[15:8] and tx_buf[1] will have PHR[7:0]
   */
  prop_radio.tx_buf[0] = ((total_length >> 0) & 0xFF);
  prop_radio.tx_buf[1] = ((total_length >> 8) & 0xFF) + DOT_4G_PHR_DW_BIT + DOT_4G_PHR_CRC_BIT;

  /* pktLen: Total number of bytes in the TX buffer, including the header if
   * one exists, but not including the CRC (which is not present in the buffer) */
  cmd_tx.pktLen = transmit_len + DOT_4G_PHR_LEN;
  cmd_tx.pPkt = prop_radio.tx_buf;

  res = netstack_sched_prop_tx();

  return (res == RF_RESULT_OK)
         ? RADIO_TX_OK
         : RADIO_TX_ERR;
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
read(void *buf, unsigned short buf_len)
{
  volatile data_entry_t *data_entry = data_queue_current_entry();

  const rtimer_clock_t t0 = RTIMER_NOW();
  /* Only wait if the Radio is accessing the entry */
  while((data_entry->status == DATA_ENTRY_BUSY) &&
        RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + TIMEOUT_DATA_ENTRY_BUSY)) ;

  if(data_entry->status != DATA_ENTRY_FINISHED) {
    /* No available data */
    return -1;
  }

  /*
   * lensz bytes (2) in the data entry are the length of the received frame.
   * Data frame is on the following format:
   *    Length (2) + Payload (N) + RSSI (1) + Status (1)
   * Data frame DOES NOT contain the following:
   *    no Header/PHY bytes
   *    no appended Received CRC bytes
   *    no Timestamp bytes
   * Visual representation of frame format:
   *
   *  +---------+---------+--------+--------+
   *  | 2 bytes | N bytes | 1 byte | 1 byte |
   *  +---------+---------+--------+--------+
   *  | Length  | Payload | RSSI   | Status |
   *  +---------+---------+--------+--------+
   *
   * Length bytes equal total length of entire frame excluding itself,
   *       Length = N + RSSI (1) + Status (1)
   *              = N + 2
   *            N = Length - 2
   */
  uint8_t *const frame_ptr = (uint8_t *)&data_entry->data;
  const lensz_t frame_len = *(lensz_t *)frame_ptr;

  /* Sanity check that Frame is at least Frame Shave bytes long */
  if(frame_len < FRAME_SHAVE) {
    LOG_ERR("Received rame is too short, len=%d\n", frame_len);

    data_queue_release_entry();
    return 0;
  }

  const uint8_t *payload_ptr = frame_ptr + sizeof(lensz_t);
  const unsigned short payload_len = (unsigned short)(frame_len - FRAME_SHAVE);

  /* Sanity check that Payload fits in Buffer */
  if(payload_len > buf_len) {
    LOG_ERR("Payload of received frame is too large for local buffer, len=%d buf_len=%d\n",
            payload_len, buf_len);

    data_queue_release_entry();
    return 0;
  }

  memcpy(buf, payload_ptr, payload_len);

  /* RSSI stored after payload */
  const int8_t rssi = (int8_t)payload_ptr[payload_len];
  /* LQI calculated from RSSI */
  const uint8_t lqi = calculate_lqi(rssi);

  packetbuf_set_attr(PACKETBUF_ATTR_RSSI, (packetbuf_attr_t)rssi);
  packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, (packetbuf_attr_t)lqi);

  data_queue_release_entry();
  return (int)payload_len;
}
/*---------------------------------------------------------------------------*/
static uint8_t
cca_request(void)
{
  const int8_t rssi = get_rssi();

  if(rssi == RF_GET_RSSI_ERROR_VAL) {
    return CCA_STATE_INVALID;
  }

  return (rssi < prop_radio.rssi_threshold)
         ? CCA_STATE_IDLE
         : CCA_STATE_BUSY;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  if(tx_is_active()) {
    LOG_ERR("Channel clear called while in TX\n");
    return 0;
  }

  const uint8_t cca_state = cca_request();

  /* Channel is clear if CCA state is IDLE */
  return cca_state == CCA_STATE_IDLE;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  if(!rx_is_active()) {
    return 0;
  }

  const uint8_t cca_state = cca_request();

  return cca_state == CCA_STATE_BUSY;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  const data_entry_t *const read_entry = data_queue_current_entry();
  volatile const data_entry_t *curr_entry = read_entry;

  int num_pending = 0;

  /* Go through RX Circular buffer and check their status */
  do {
    const uint8_t status = curr_entry->status;
    if((status == DATA_ENTRY_FINISHED) ||
       (status == DATA_ENTRY_BUSY)) {
      num_pending += 1;
    }

    /* Stop when we have looped the circular buffer */
    curr_entry = (data_entry_t *)curr_entry->pNextEntry;
  } while(curr_entry != read_entry);

  if(num_pending > 0) {
    process_poll(&rf_sched_process);
  }

  /* If we didn't find an entry at status finished, no frames are pending */
  return num_pending;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  rf_result_t res;

  if(prop_radio.rf_is_on) {
    LOG_WARN("Radio is already on\n");
    return RF_RESULT_OK;
  }

  data_queue_reset();

  res = netstack_sched_rx(true);

  if(res != RF_RESULT_OK) {
    return RF_RESULT_ERROR;
  }

  prop_radio.rf_is_on = true;
  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  if(!prop_radio.rf_is_on) {
    LOG_WARN("Radio is already off\n");
    return RF_RESULT_OK;
  }

  rf_yield();

  prop_radio.rf_is_on = false;
  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  rf_result_t res;

  if(!value) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    /* On / off */
    *value = (prop_radio.rf_is_on)
      ? RADIO_POWER_MODE_ON
      : RADIO_POWER_MODE_OFF;

    return RADIO_RESULT_OK;

  case RADIO_PARAM_CHANNEL:
    *value = (radio_value_t)get_channel();
    return RADIO_RESULT_OK;

  case RADIO_PARAM_TXPOWER:
    res = rf_get_tx_power(prop_radio.rf_handle, rf_tx_power_table, (int8_t *)&value);
    return ((res == RF_RESULT_OK) &&
            (*value != RF_TxPowerTable_INVALID_DBM))
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  case RADIO_PARAM_CCA_THRESHOLD:
    *value = prop_radio.rssi_threshold;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RSSI:
    *value = get_rssi();
    return (*value == RF_GET_RSSI_ERROR_VAL)
           ? RADIO_RESULT_ERROR
           : RADIO_RESULT_OK;

  case RADIO_CONST_CHANNEL_MIN:
    *value = DOT_15_4G_CHAN_MIN;
    return RADIO_RESULT_OK;

  case RADIO_CONST_CHANNEL_MAX:
    *value = DOT_15_4G_CHAN_MAX;
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MIN:
    *value = (radio_value_t)tx_power_min(rf_tx_power_table);
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MAX:
    *value = (radio_value_t)tx_power_max(rf_tx_power_table, rf_tx_power_table_size);
    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  rf_result_t res;

  switch(param) {
  case RADIO_PARAM_POWER_MODE:

    if(value == RADIO_POWER_MODE_ON) {
      return (on() == RF_RESULT_OK)
             ? RADIO_RESULT_OK
             : RADIO_RESULT_ERROR;
    } else if(value == RADIO_POWER_MODE_OFF) {
      off();
      return RADIO_RESULT_OK;
    }

    return RADIO_RESULT_INVALID_VALUE;

  case RADIO_PARAM_CHANNEL:
    res = set_channel((uint16_t)value);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  case RADIO_PARAM_TXPOWER:
    if(!tx_power_in_range((int8_t)value, rf_tx_power_table, rf_tx_power_table_size)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    res = rf_set_tx_power(prop_radio.rf_handle, rf_tx_power_table, (int8_t)value);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  case RADIO_PARAM_RX_MODE:
    return RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    prop_radio.rssi_threshold = (int8_t)value;
    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
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
init(void)
{
  if(prop_radio.rf_handle) {
    LOG_WARN("Radio is already initialized\n");
    return RF_RESULT_OK;
  }

  /* RX is off */
  prop_radio.rf_is_on = false;

  /* Set configured RSSI threshold */
  prop_radio.rssi_threshold = PROP_MODE_CCA_RSSI_THRESHOLD;

  init_rf_params();

  /* Init RF params and specify non-default params */
  RF_Params rf_params;
  RF_Params_init(&rf_params);
  rf_params.nInactivityTimeout = RF_CONF_INACTIVITY_TIMEOUT;

  /* Open RF Driver */
  prop_radio.rf_handle = netstack_open(&rf_params);

  if(prop_radio.rf_handle == NULL) {
    LOG_ERR("Unable to open RF driver during initialization\n");
    return RF_RESULT_ERROR;
  }

  set_channel(IEEE802154_DEFAULT_CHANNEL);

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  /* Start RF process */
  process_start(&rf_sched_process, NULL);

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
const struct radio_driver prop_mode_driver = {
  init,
  prepare,
  transmit,
  send,
  read,
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
 * @}
 */
