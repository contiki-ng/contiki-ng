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
# define PROP_MODE_RF_SETTINGS  PROP_MODE_CONF_RF_SETTINGS
#else
# define PROP_MODE_RF_SETTINGS "prop-settings.h"
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
# define PRINTF(...)
#else
# define PRINTF(...)  printf(__VA_ARGS__)
#endif
/*---------------------------------------------------------------------------*/
/* Configuration for default Prop channel */
#ifdef PROP_MODE_CONF_CHANNEL
#   define PROP_MODE_CHANNEL  PROP_MODE_CONF_CHANNEL
#else
#   define PROP_MODE_CHANNEL  RF_CHANNEL
#endif
/*---------------------------------------------------------------------------*/
/* Data whitener. 1: Whitener, 0: No whitener */
#ifdef PROP_MODE_CONF_DW
# define PROP_MODE_DW  PROP_MODE_CONF_DW
#else
# define PROP_MODE_DW  0
#endif

#ifdef PROP_MODE_CONF_USE_CRC16
# define PROP_MODE_USE_CRC16  PROP_MODE_CONF_USE_CRC16
#else
# define PROP_MODE_USE_CRC16  0
#endif
/*---------------------------------------------------------------------------*/
/* Used for the return value of channel_clear */
#define RF_CCA_CLEAR                       1
#define RF_CCA_BUSY                        0

/* Used as an error return value for get_cca_info */
#define RF_GET_CCA_INFO_ERROR           0xFF

/*
 * Values of the individual bits of the ccaInfo field in CMD_IEEE_CCA_REQ's
 * status struct
 */
#define RF_CMD_CCA_REQ_CCA_STATE_IDLE      0 /* 0b00 */
#define RF_CMD_CCA_REQ_CCA_STATE_BUSY      1 /* 0b01 */
#define RF_CMD_CCA_REQ_CCA_STATE_INVALID   2 /* 0b10 */

#ifdef PROP_MODE_CONF_RSSI_THRESHOLD
# define PROP_MODE_RSSI_THRESHOLD  PROP_MODE_CONF_RSSI_THRESHOLD
#else
# define PROP_MODE_RSSI_THRESHOLD  0xA6
#endif
/*---------------------------------------------------------------------------*/
/* Defines and variables related to the .15.4g PHY HDR */
#define DOT_4G_MAX_FRAME_LEN    2047
#define DOT_4G_PHR_LEN             2

/* PHY HDR bits */
#define DOT_4G_PHR_CRC16  0x10
#define DOT_4G_PHR_DW     0x08

#if PROP_MODE_USE_CRC16
/* CRC16 */
# define DOT_4G_PHR_CRC_BIT DOT_4G_PHR_CRC16
# define CRC_LEN            2
#else
/* CRC32 */
# define DOT_4G_PHR_CRC_BIT 0
# define CRC_LEN            4
#endif /* PROP_MODE_USE_CRC16 */

#if PROP_MODE_DW
# define DOT_4G_PHR_DW_BIT DOT_4G_PHR_DW
#else
 #define DOT_4G_PHR_DW_BIT 0
#endif
/*---------------------------------------------------------------------------*/
/* How long to wait for the RF to enter RX in rf_cmd_ieee_rx */
#define ENTER_RX_WAIT_TIMEOUT (RTIMER_SECOND >> 10)

/* How long to wait for the rx read entry to become ready */
#define TIMEOUT_DATA_ENTRY_BUSY (RTIMER_SECOND / 250)
/*---------------------------------------------------------------------------*/
/* Configuration for TX power table */
#ifdef PROP_MODE_CONF_TX_POWER_TABLE
# define TX_POWER_TABLE  PROP_MODE_CONF_TX_POWER_TABLE
#else
# define TX_POWER_TABLE  propTxPowerTable
#endif
/*---------------------------------------------------------------------------*/
/* TX power table convenience macros */
#define TX_POWER_TABLE_SIZE  ((sizeof(TX_POWER_TABLE) / sizeof(TX_POWER_TABLE[0])) - 1)

#define TX_POWER_MIN  (TX_POWER_TABLE[0].power)
#define TX_POWER_MAX  (TX_POWER_TABLE[TX_POWER_TABLE_SIZE - 1].power)

#define TX_POWER_IN_RANGE(dbm)  (((dbm) >= TX_POWER_MIN) && ((dbm) <= TX_POWER_MAX))
/*---------------------------------------------------------------------------*/
/* TX buf configuration */
#define TX_BUF_HDR_LEN          2
#define TX_BUF_PAYLOAD_LEN      180

#define TX_BUF_SIZE             (TX_BUF_HDR_LEN + TX_BUF_PAYLOAD_LEN)

/* RX buf configuration */
#ifdef PROP_MODE_CONF_RX_BUF_CNT
# define RX_BUF_CNT             PROP_MODE_CONF_RX_BUF_CNT
#else
# define RX_BUF_CNT             4
#endif

#define RX_BUF_SIZE             140
/*---------------------------------------------------------------------------*/
#define DATA_ENTRY_LENSZ_NONE 0 /* 0 bytes */
#define DATA_ENTRY_LENSZ_BYTE 1 /* 1 byte */
#define DATA_ENTRY_LENSZ_WORD 2 /* 2 bytes */

#define DATA_ENTRY_LENSZ      DATA_ENTRY_LENSZ_WORD
typedef uint16_t lensz_t;

#define FRAME_OFFSET          DATA_ENTRY_LENSZ
#define FRAME_SHAVE           2   /* RSSI (1) + Status (1) */
/*---------------------------------------------------------------------------*/
#define MAC_RADIO_RECEIVER_SENSITIVITY_DBM              -110
#define MAC_RADIO_RECEIVER_SATURATION_DBM               10
#define MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY  10
#define MAC_SPEC_ED_MAX                                 0xFF

#define ED_RF_POWER_MIN_DBM   (MAC_RADIO_RECEIVER_SENSITIVITY_DBM + MAC_SPEC_ED_MIN_DBM_ABOVE_RECEIVER_SENSITIVITY)
#define ED_RF_POWER_MAX_DBM   MAC_RADIO_RECEIVER_SATURATION_DBM
/*---------------------------------------------------------------------------*/
typedef rfc_dataEntryGeneral_t data_entry_t;

typedef union {
  data_entry_t data_entry;
  uint8_t      buf[RX_BUF_SIZE];
} rx_buf_t CC_ALIGN(4);

typedef struct {
  /* Outgoing frame buffer */
  uint8_t             tx_buf[TX_BUF_SIZE] CC_ALIGN(4);
  /* Incoming frame buffer */
  rx_buf_t            rx_bufs[RX_BUF_CNT];

  /* RX Data Queue */
  dataQueue_t         rx_data_queue;
  /* RX Statistics struct */
  rfc_propRxOutput_t  rx_stats;
  /* Receive entry pointer to keep track of read items */
  data_entry_t*       rx_read_entry;

  /* RSSI Threshold */
  int8_t              rssi_threshold;

  /* Indicates RF is supposed to be on or off */
  uint8_t             rf_is_on;

  /* RF driver */
  RF_Object           rf_object;
  RF_Handle           rf_handle;
} prop_radio_t;

static prop_radio_t prop_radio;
/*---------------------------------------------------------------------------*/
#define cmd_radio_setup   (*(volatile rfc_CMD_PROP_RADIO_DIV_SETUP_t *)&rf_cmd_prop_radio_div_setup)
#define cmd_fs            (*(volatile rfc_CMD_FS_t *)                  &rf_cmd_prop_fs)
#define cmd_tx            (*(volatile rfc_CMD_PROP_TX_ADV_t *)         &rf_cmd_prop_tx_adv)
#define cmd_rx            (*(volatile rfc_CMD_PROP_RX_ADV_t *)         &rf_cmd_prop_rx_adv)
/*---------------------------------------------------------------------------*/
static CC_INLINE bool tx_active(void) { return cmd_tx.status == ACTIVE; }
static CC_INLINE bool rx_active(void) { return cmd_rx.status == ACTIVE; }
/*---------------------------------------------------------------------------*/
static int on(void);
static int off(void);
/*---------------------------------------------------------------------------*/
static void
cmd_rx_cb(RF_Handle client, RF_CmdHandle command, RF_EventMask events)
{
  /* Unused arguments */
  (void)client;
  (void)command;

  if (events & RF_EventRxEntryDone) {
    process_poll(&rf_process);
  }
}
/*---------------------------------------------------------------------------*/
static cmd_result_t
start_rx(void)
{
  cmd_rx.status = IDLE;
  RF_CmdHandle rx_handle = RF_postCmd(prop_radio.rf_handle, (RF_Op*)&cmd_rx, RF_PriorityNormal,
                                      &cmd_rx_cb, RF_EventRxEntryDone);
  if (rx_handle == RF_ALLOC_ERROR) {
    PRINTF("start_rx: RF_ALLOC_ERROR for cmd_rx\n");
    return CMD_RESULT_ERROR;
  }

  /* Wait to enter RX */
  const rtimer_clock_t t0 = RTIMER_NOW();
  while (!rx_active() &&
      (RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + ENTER_RX_WAIT_TIMEOUT)));

  if (!rx_active()) {
    PRINTF("cmd_rx: status=0x%04x\n", cmd_rx.status);
    return CMD_RESULT_ERROR;
  }

  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static cmd_result_t
stop_rx(void)
{
  /* Abort any ongoing operation. Don't care about the result. */
  RF_cancelCmd(prop_radio.rf_handle, RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);

  if (cmd_rx.status != PROP_DONE_STOPPED &&
      cmd_rx.status != PROP_DONE_ABORT) {
    PRINTF("RF_cmdPropRxAdv cancel: status=0x%04x\n",
           cmd_rx.status);
    return CMD_RESULT_ERROR;
  }

  /* Stopped gracefully */
  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static cmd_result_t
rf_run_setup()
{
  // TODO not the right way to do this.
  RF_runCmd(prop_radio.rf_handle, (RF_Op*)&cmd_radio_setup, RF_PriorityNormal, NULL, 0);
  if (cmd_radio_setup.status != PROP_DONE_OK) {
    return CMD_RESULT_ERROR;
  }

  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_value_t
get_rssi(void)
{
  if (tx_active()) {
    PRINTF("get_rssi: called while in TX\n");
    return RF_GET_RSSI_ERROR_VAL;
  }

  const bool was_off = !rx_active();
  if (was_off && start_rx() == CMD_RESULT_ERROR) {
    PRINTF("get_rssi: unable to start RX\n");
    return RF_GET_RSSI_ERROR_VAL;
  }

  int8_t rssi = RF_GET_RSSI_ERROR_VAL;
  while(rssi == RF_GET_RSSI_ERROR_VAL || rssi == 0) {
    rssi = RF_getRssi(prop_radio.rf_handle);
  }

  if (was_off) {
    stop_rx();
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

  return (freq_khz - DOT_15_4G_CHAN0_FREQUENCY) / DOT_15_4G_CHANNEL_SPACING;
}
/*---------------------------------------------------------------------------*/
static cmd_result_t
set_channel(uint8_t channel)
{
  uint32_t new_freq = DOT_15_4G_CHAN0_FREQUENCY + (channel * DOT_15_4G_CHANNEL_SPACING);

  uint16_t freq = (uint16_t)(new_freq / 1000);
  uint16_t frac = (new_freq - (freq * 1000)) * 65536 / 1000;

  PRINTF("set_channel: %u = 0x%04x.0x%04x (%lu)\n",
         channel, freq, frac, new_freq);

  cmd_radio_setup.centerFreq = freq;
  cmd_fs.frequency = freq;
  cmd_fs.fractFreq = frac;

  // We don't care whether the FS command is successful because subsequent
  // TX and RX commands will tell us indirectly.
  RF_EventMask rf_events = RF_runCmd(prop_radio.rf_handle, (RF_Op*)&cmd_fs,
                                     RF_PriorityNormal, NULL, 0);
  if ((rf_events & (RF_EventCmdDone | RF_EventLastCmdDone)) == 0) {
    PRINTF("set_channel: RF_runCmd failed, events=0x%llx\n", rf_events);
    return CMD_RESULT_ERROR;
  }

  if (cmd_fs.status != DONE_OK) {
    PRINTF("set_channel: cmd_fs failed, status=0x%04x\n", cmd_fs.status);
    return CMD_RESULT_ERROR;
  }

  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
/* Returns the current TX power in dBm */
static radio_value_t
get_tx_power(void)
{
  const RF_TxPowerTable_Value value = RF_getTxPower(prop_radio.rf_handle);
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
  RF_Stat stat = RF_setTxPower(prop_radio.rf_handle, value);

  return (stat == RF_StatSuccess)
    ? RADIO_RESULT_OK
    : RADIO_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
static uint8_t
calculate_lqi(int8_t rssi)
{
  /* Note : Currently the LQI value is simply the energy detect measurement.
   *        A more accurate value could be derived by using the correlation
   *        value along with the RSSI value. */
  rssi = CLAMP(rssi, ED_RF_POWER_MIN_DBM, ED_RF_POWER_MAX_DBM);

  /* Create energy detect measurement by normalizing and scaling RF power level.
   * Note : The division operation below is designed for maximum accuracy and
   *        best granularity.  This is done by grouping the math operations to
   *        compute the entire numerator before doing any division. */
  return (MAC_SPEC_ED_MAX * (rssi - ED_RF_POWER_MIN_DBM)) / (ED_RF_POWER_MAX_DBM - ED_RF_POWER_MIN_DBM);
}
/*---------------------------------------------------------------------------*/
static void
init_rx_buffers(void)
{
  size_t i = 0;
  for (i = 0; i < RX_BUF_CNT; ++i) {
    const data_entry_t data_entry = {
      .status       = DATA_ENTRY_PENDING,
      .config.type  = DATA_ENTRY_TYPE_GEN,
      .config.lenSz = DATA_ENTRY_LENSZ,
      .length       = RX_BUF_SIZE - sizeof(data_entry_t), /* TODO: is this sizeof sound? */
      /* Point to fist entry if this is last entry, else point to next entry */
      .pNextEntry   = (i == (RX_BUF_CNT - 1))
        ? prop_radio.rx_bufs[0].buf
        : prop_radio.rx_bufs[i].buf
    };
    /* Write back data entry struct */
    prop_radio.rx_bufs[i].data_entry = data_entry;
  }
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  int len = MIN(payload_len, TX_BUF_PAYLOAD_LEN);

  memcpy(prop_radio.tx_buf + TX_BUF_HDR_LEN, payload, len);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  int ret = RADIO_TX_OK;

  if (tx_active()) {
    PRINTF("transmit: not allowed while transmitting\n");
    return RADIO_TX_ERR;
  }

  const bool rx_is_on = rx_active();
  if (rx_is_on) {
      stop_rx();
  }

  /* Length in .15.4g PHY HDR. Includes the CRC but not the HDR itself */
  uint16_t total_length = transmit_len + CRC_LEN;
  /* Prepare the .15.4g PHY header
   * MS=0, Length MSBits=0, DW and CRC configurable
   * Total length = transmit_len (payload) + CRC length
   *
   * The Radio will flip the bits around, so tx_buf[0] must have the length
   * LSBs (PHR[15:8] and tx_buf[1] will have PHR[7:0] */
  prop_radio.tx_buf[0] = total_length & 0xFF;
  prop_radio.tx_buf[1] = (total_length >> 8) + DOT_4G_PHR_DW_BIT + DOT_4G_PHR_CRC_BIT;

  /* pktLen: Total number of bytes in the TX buffer, including the header if
   * one exists, but not including the CRC (which is not present in the buffer) */
  cmd_tx.pktLen = transmit_len + DOT_4G_PHR_LEN;
  cmd_tx.pPkt = prop_radio.tx_buf;

  RF_CmdHandle tx_handle = RF_postCmd(prop_radio.rf_handle, (RF_Op*)&cmd_tx, RF_PriorityNormal, NULL, 0);
  if (tx_handle == RF_ALLOC_ERROR) {
    /* Failure sending the CMD_PROP_TX command */
    PRINTF("transmit: unable to allocate RF command handle\n");
    return RADIO_TX_ERR;
  }

  ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);

  // watchdog_periodic();

  /* Idle away while the command is running */
  RF_EventMask rf_events = RF_pendCmd(prop_radio.rf_handle, tx_handle, RF_EventLastCmdDone);

  if ((rf_events & (RF_EventCmdDone | RF_EventLastCmdDone)) == 0) {
    PRINTF("transmit: RF_pendCmd failed, events=0x%llx\n", rf_events);
    ret = RADIO_TX_ERR;
  }

  else if (cmd_tx.status != PROP_DONE_OK) {
    /* Operation completed, but frame was not sent */
    PRINTF("transmit: Not Sent OK status=0x%04x\n",
           cmd_tx.status);
    ret = RADIO_TX_ERR;
  }

  ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);

  /* Workaround. Set status to IDLE */
  cmd_tx.status = IDLE;

  if (rx_is_on) {
    start_rx();
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
  data_entry_t *const data_entry = prop_radio.rx_read_entry;
  uint8_t *const frame_ptr = (uint8_t*)&data_entry->data;

  // Clear the Length byte(s) and set status to 0: "Pending"
  *(lensz_t*)frame_ptr = 0;
  data_entry->status = DATA_ENTRY_PENDING;
  prop_radio.rx_read_entry = (data_entry_t*)data_entry->pNextEntry;
}
/*---------------------------------------------------------------------------*/
static int
read(void *buf, unsigned short buf_len)
{
  volatile data_entry_t *data_entry = prop_radio.rx_read_entry;

  const rtimer_clock_t t0 = RTIMER_NOW();
  // Only wait if the Radio timer is accessing the entry
  while ((data_entry->status == DATA_ENTRY_BUSY) &&
          RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + TIMEOUT_DATA_ENTRY_BUSY));

  if (data_entry->status != DATA_ENTRY_FINISHED) {
    // No available data
    return 0;
  }

  /* First 2 bytes in the data entry are the length.
   * Data frame is on the following format:
   *    Length (2) + Payload (N) + RSSI (1) + Status (1)
   * Data frame DOES NOT contain the following:
   *    no Header/PHY bytes
   *    no appended Received CRC bytes
   *    no Timestamp bytes
   * +---------+---------+--------+--------+
   * | 2 bytes | N bytes | 1 byte | 1 byte |
   * +---------+---------+--------+--------+
   * | Length  | Payload | RSSI   | Status |
   * +---------+---------+--------+--------+
   * Length bytes equal total length of entire frame excluding itself,
   * i.e.: Length = N + RSSI (1) + Status (1)
   *              = N + 2
   *            N = Length - 2 */

  uint8_t *const frame_ptr = (uint8_t*)&data_entry->data;
  const lensz_t frame_len = *(lensz_t*)frame_ptr;

  /* Sanity check that Frame is at least Frame Shave bytes long */
  if (frame_len < FRAME_SHAVE) {
    PRINTF("read: frame too short len=%d\n", frame_len);
    release_data_entry();
    return 0;
  }

  const uint8_t *payload_ptr = frame_ptr + sizeof(lensz_t);
  const unsigned short payload_len = (unsigned short)(frame_len - FRAME_SHAVE);

  /* Sanity check that Payload fits in Buffer */
  if (payload_len > buf_len) {
    PRINTF("read: payload too large for buffer len=%d buf_len=%d\n", payload_len, buf_len);
    release_data_entry();
    return 0;
  }

  memcpy(buf, payload_ptr, payload_len);

  /* RSSI stored after payload */
  const int8_t rssi = (int8_t)payload_ptr[payload_len];
  /* LQI calculated from RSSI */
  const uint8_t lqi = calculate_lqi(rssi);

  packetbuf_set_attr(PACKETBUF_ATTR_RSSI,         (packetbuf_attr_t)rssi);
  packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, (packetbuf_attr_t)lqi);

  release_data_entry();
  return (int)payload_len;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  if (tx_active()) {
    PRINTF("channel_clear: called while in TX\n");
    return RF_CCA_CLEAR;
  }

  const bool rx_was_off = !rx_active();
  if (rx_was_off) {
    start_rx();
  }

  int8_t rssi = RF_GET_RSSI_ERROR_VAL;
  while (rssi == RF_GET_RSSI_ERROR_VAL || rssi == 0) {
    rssi = RF_getRssi(prop_radio.rf_handle);
  }

  if (rx_was_off) {
    stop_rx();
  }

  if(rssi >= prop_radio.rssi_threshold) {
    return RF_CCA_BUSY;
  }

  return RF_CCA_CLEAR;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  if (!rx_active()) {
    return 0;
  }

  if (channel_clear() == RF_CCA_CLEAR) {
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  const rfc_dataEntry_t *const first_entry = (rfc_dataEntry_t *)prop_radio.rx_data_queue.pCurrEntry;
  volatile const rfc_dataEntry_t *entry = first_entry;

  int rv = 0;

  /* Go through RX Circular buffer and check their status */
  do {
    const uint8_t status = entry->status;
    if ((status == DATA_ENTRY_FINISHED) ||
        (status == DATA_ENTRY_BUSY)) {
      rv += 1;
    }

    entry = (rfc_dataEntry_t *)entry->pNextEntry;
  } while (entry != first_entry);

  if (rv > 0) {
    process_poll(&rf_process);
  }

  /* If we didn't find an entry at status finished, no frames are pending */
  return rv;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  if (prop_radio.rf_is_on) {
    PRINTF("on: Radio already on\n");
    return CMD_RESULT_OK;
  }

  /* Reset all RF command statuses */
  cmd_fs.status = IDLE;
  cmd_tx.status = IDLE;
  cmd_rx.status = IDLE;

  init_rx_buffers();

  const int rx_ok = start_rx();
  if (!rx_ok) {
    off();
    return CMD_RESULT_ERROR;
  }

  prop_radio.rf_is_on = true;
  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  if (!prop_radio.rf_is_on) {
    PRINTF("off: Radio already off\n");
    return CMD_RESULT_OK;
  }

  // Force abort of any ongoing RF operation.
  RF_flushCmd(prop_radio.rf_handle, RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);

  // Trigger a manual power-down
  RF_yield(prop_radio.rf_handle);

  /* We pulled the plug, so we need to restore the status manually */
  cmd_fs.status = IDLE;
  cmd_tx.status = IDLE;
  cmd_rx.status = IDLE;

  // Reset RX buffers if there was an ongoing RX
  size_t i;
  for (i = 0; i < RX_BUF_CNT; ++i) {
    data_entry_t *entry = &prop_radio.rx_bufs[i].data_entry;
    if (entry->status == DATA_ENTRY_BUSY) {
      entry->status = DATA_ENTRY_PENDING;
    }
  }

  prop_radio.rf_is_on = false;
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
    *value = (prop_radio.rf_is_on)
        ? RADIO_POWER_MODE_ON
        : RADIO_POWER_MODE_OFF;

    return RADIO_RESULT_OK;

  case RADIO_PARAM_CHANNEL:
    *value = (radio_value_t)get_channel();
    return RADIO_RESULT_OK;

  case RADIO_PARAM_TXPOWER:
    *value = get_tx_power();
    return RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    *value = prop_radio.rssi_threshold;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RSSI:
    *value = get_rssi();
    return (*value == RF_GET_RSSI_ERROR_VAL)
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

  case RADIO_PARAM_TXPOWER:
    return set_tx_power(value);

  case RADIO_PARAM_RX_MODE:
    return RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    prop_radio.rssi_threshold = (int8_t)value;
    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }

  /* If we reach here we had no errors. Apply new settings */
  if (rx_active()) {
    stop_rx();
    // TODO fix this
    if (rf_run_setup() != CMD_RESULT_OK) {
      return RADIO_RESULT_ERROR;
    }
    start_rx();
  } else if (tx_active()) {
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
init(void)
{
  /* Zero initalize TX and RX buffers */
  memset(prop_radio.tx_buf,  0x0, sizeof(prop_radio.tx_buf));
  memset(prop_radio.rx_bufs, 0x0, sizeof(prop_radio.rx_bufs));

  /* Circular buffer, no last entry */
  prop_radio.rx_data_queue.pCurrEntry = prop_radio.rx_bufs[0].buf;
  prop_radio.rx_data_queue.pLastEntry = NULL;

  /* Initialize current read pointer to first element (used in ISR) */
  prop_radio.rx_read_entry = &prop_radio.rx_bufs[0].data_entry;

  /* Set configured RSSI threshold */
  prop_radio.rssi_threshold = PROP_MODE_RSSI_THRESHOLD;

  /* RX is off */
  prop_radio.rf_is_on = false;

  /* Configure RX command */
  cmd_rx.maxPktLen = DOT_4G_MAX_FRAME_LEN - cmd_rx.lenOffset;
  cmd_rx.pQueue = &prop_radio.rx_data_queue;
  cmd_rx.pOutput = (uint8_t *)&prop_radio.rx_stats;

  /* Init RF params and specify non-default params */
  RF_Params       rf_params;
  RF_Params_init(&rf_params);
  rf_params.nInactivityTimeout = 2000; /* 2 ms  */

  /* Open RF Driver */
  prop_radio.rf_handle = RF_open(&prop_radio.rf_object, &rf_prop_mode,
                                 (RF_RadioSetup*)&cmd_radio_setup, &rf_params);
  if (prop_radio.rf_handle == NULL) {
    return CMD_RESULT_ERROR;
  }

  set_channel(PROP_MODE_CHANNEL);

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  /* Start RF process */
  process_start(&rf_process, NULL);

  return CMD_RESULT_OK;
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
 */
