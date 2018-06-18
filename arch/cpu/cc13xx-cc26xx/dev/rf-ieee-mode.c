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
 * \addtogroup rf-core
 * @{
 *
 * \defgroup rf-core-ieee CC13xx/CC26xx IEEE mode driver
 *
 * @{
 *
 * \file
 * Implementation of the CC13xx/CC26xx IEEE mode NETSTACK_RADIO driver
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "net/packetbuf.h"
#include "net/linkaddr.h"
#include "net/netstack.h"
#include "sys/energest.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "sys/ctimer.h"
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
/* RF driver and RF Core API */
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_data_entry.h)
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
// rf_ieee_cmd and rf_ieee_mailbox included by RF settings because of the
// discrepancy between CC13x0 and CC13x2 IEEE support. CC13x0 doesn't provide
// RFCore definitions of IEEE commandos, and are therefore included locally
// from the Contiki build system. CC13x2 includes these normally from driverlib.
// This is taken care of RF settings.

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
/* SimpleLink Platform RF dev */
#include "rf-common.h"
#include "dot-15-4g.h"
/*---------------------------------------------------------------------------*/
/* RF settings */
#ifdef IEEE_MODE_CONF_RF_SETTINGS
#   define IEEE_MODE_RF_SETTINGS  IEEE_MODE_CONF_RF_SETTINGS
#else
#   define IEEE_MODE_RF_SETTINGS "ieee-settings.h"
#endif

#include IEEE_MODE_RF_SETTINGS
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RF"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
#ifdef NDEBUG
#   define PRINTF(...)
#else
#   define PRINTF(...)  printf(__VA_ARGS__)
#endif
/*---------------------------------------------------------------------------*/
/* Configuration parameters */

/* Configuration to enable/disable auto ACKs in IEEE mode */
#ifdef IEEE_MODE_CONF_AUTOACK
#   define IEEE_MODE_AUTOACK  IEEE_MODE_CONF_AUTOACK
#else
#   define IEEE_MODE_AUTOACK  1
#endif /* IEEE_MODE_CONF_AUTOACK */

/* Configuration to enable/disable frame filtering in IEEE mode */
#ifdef IEEE_MODE_CONF_PROMISCOUS
#   define IEEE_MODE_PROMISCOUS  IEEE_MODE_CONF_PROMISCOUS
#else
#   define IEEE_MODE_PROMISCOUS  0
#endif /* IEEE_MODE_CONF_PROMISCOUS */

/* Configuration to set the RSSI threshold */
#ifdef IEEE_MODE_CONF_RSSI_THRESHOLD
#   define IEEE_MODE_RSSI_THRESHOLD  IEEE_MODE_CONF_RSSI_THRESHOLD
#else
#   define IEEE_MODE_RSSI_THRESHOLD  0xA6
#endif /* IEEE_MODE_CONF_RSSI_THRESHOLD */

/* Configuration for default IEEE channel */
#ifdef IEEE_MODE_CONF_CHANNEL
#   define IEEE_MODE_CHANNEL  IEEE_MODE_CONF_CHANNEL
#else
#   define IEEE_MODE_CHANNEL  RF_CHANNEL
#endif

/* Configuration for TX power table */
#ifdef IEEE_MODE_CONF_TX_POWER_TABLE
#   define TX_POWER_TABLE  IEEE_MODE_CONF_TX_POWER_TABLE
#else
#   define TX_POWER_TABLE  rf_ieee_tx_power_table
#endif

#ifdef IEEE_MODE_CONF_TX_POWER_TABLE_SIZE
#   define TX_POWER_TABLE_SIZE  IEEE_MODE_CONF_TX_POWER_TABLE_SIZE
#else
#   define TX_POWER_TABLE_SIZE  RF_IEEE_TX_POWER_TABLE_SIZE
#endif
/*---------------------------------------------------------------------------*/
/* TX power table convenience macros */
#define TX_POWER_MIN  (TX_POWER_TABLE[0])
#define TX_POWER_MAX  (TX_POWER_TABLE[TX_POWER_TABLE_SIZE - 1])

#define TX_POWER_IN_RANGE(dbm)  (((dbm) >= TX_POWER_MIN) && ((dbm) <= TX_POWER_MAX))
/*---------------------------------------------------------------------------*/
/* IEEE channel-to-frequency conversion constants */
/* Channel frequency base: 2.405 GHz */
/* Channel frequency spacing: 5 MHz */
/* Channel range: 11 - 26 */
#define IEEE_MODE_FREQ_BASE     2405000
#define IEEE_MODE_FREQ_SPACING  5000
#define IEEE_MODE_CHAN_MIN      11
#define IEEE_MODE_CHAN_MAX      26

#define IEEE_MODE_CHAN_IN_RANGE(ch)  ((IEEE_MODE_CHAN_MIN <= (ch)) && ((ch) <= IEEE_MODE_CHAN_MAX))

/* Sanity check of default IEEE channel */
#if !IEEE_MODE_CHAN_IN_RANGE(IEEE_MODE_CHANNEL)
#   error "Default IEEE channel IEEE_MODE_CHANNEL is outside allowed channel range"
#endif
/*---------------------------------------------------------------------------*/
/* Timeout constants */

/* How long to wait for an ongoing ACK TX to finish before starting frame TX */
#define TIMEOUT_TX_WAIT          (RTIMER_SECOND >> 11)

/* How long to wait for the RF to enter RX in cmd_rx */
#define TIMEOUT_ENTER_RX_WAIT    (RTIMER_SECOND >> 10)

/* How long to wait for the RF to react on CMD_ABORT: around 1 msec */
#define TIMEOUT_RF_TURN_OFF_WAIT (RTIMER_SECOND >> 10)

/* How long to wait for the RF to finish TX of a packet or an ACK */
#define TIMEOUT_TX_FINISH_WAIT   (RTIMER_SECOND >> 7)

/* How long to wait for the rx read entry to become ready */
#define TIMEOUT_DATA_ENTRY_BUSY (RTIMER_SECOND / 250)
/*---------------------------------------------------------------------------*/
#define CCA_STATE_IDLE      0
#define CCA_STATE_BUSY      1
#define CCA_STATE_INVALID   2
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF driver object */
static RF_Object g_rfObj;
static RF_Handle g_rfHandle;

/* RF Core command pointers */
#define cmd_radio_setup  (*(volatile rfc_CMD_RADIO_SETUP_t*)&rf_cmd_ieee_radio_setup)
#define cmd_fs           (*(volatile rfc_CMD_FS_t*)         &rf_cmd_ieee_fs)
#define cmd_tx           (*(volatile rfc_CMD_IEEE_TX_t*)    &rf_cmd_ieee_tx)
#define cmd_rx           (*(volatile rfc_CMD_IEEE_RX_t*)    &rf_cmd_ieee_rx)

/* RF command handles */
static RF_CmdHandle g_cmdTxHandle;
static RF_CmdHandle g_cmdRxHandle;

/* Global RF Core commands */
static rfc_CMD_IEEE_MOD_FILT_t g_cmdModFilt;

/* RF stats data structure */
static rfc_ieeeRxOutput_t g_rxStats;
/*---------------------------------------------------------------------------*/
/* The outgoing frame buffer */
#define TX_BUF_PAYLOAD_LEN  180
#define TX_BUF_HDR_LEN      2

static uint8_t g_txBuf[TX_BUF_HDR_LEN + TX_BUF_PAYLOAD_LEN] CC_ALIGN(4);
/*---------------------------------------------------------------------------*/
/* RX Data Queue */
static dataQueue_t g_rxDataQueue;
/*---------------------------------------------------------------------------*/
/* Constants for receive buffers */
#define DATA_ENTRY_LENSZ_NONE  0
#define DATA_ENTRY_LENSZ_BYTE  1
#define DATA_ENTRY_LENSZ_WORD  2 /* 2 bytes */

#define DATA_ENTRY_LENSZ      DATA_ENTRY_LENSZ_BYTE
typedef uint8_t lensz_t;

#define FRAME_OFFSET          DATA_ENTRY_LENSZ
#define FRAME_SHAVE           8   /* FCS (2) + RSSI (1) + Status (1) + Timestamp (4) */
/*---------------------------------------------------------------------------*/
/* RX buf configuration */
#ifdef IEEE_MODE_CONF_RX_BUF_CNT
# define RX_BUF_CNT             IEEE_MODE_CONF_RX_BUF_CNT
#else
# define RX_BUF_CNT             4
#endif

#define RX_BUF_SIZE     144

/* Receive buffer entries with room for 1 IEEE 802.15.4 frame in each */
typedef rfc_dataEntryGeneral_t data_entry_t;

typedef union {
  data_entry_t data_entry;
  uint8_t      buf[RX_BUF_SIZE];
} rx_buf_t CC_ALIGN(4);

static rx_buf_t rx_bufs[RX_BUF_CNT];

/* Receive entry pointer to keep track of read items */
static data_entry_t *rx_read_entry;
/*---------------------------------------------------------------------------*/
/* RAT overflow upkeep */
static struct ctimer g_ratOverflowTimer;
static rtimer_clock_t g_ratLastOverflow;
static volatile uint32_t g_ratOverflowCount;

#define RAT_RANGE     (~(uint32_t)0)
#define RAT_OVERFLOW_PERIOD_SECONDS  (RAT_RANGE / (uint32_t)RADIO_TIMER_SECOND)
/* XXX: don't know what exactly is this, looks like the time to TX 3 octets */
#define RAT_TIMESTAMP_OFFSET  -(USEC_TO_RADIO(32 * 3) - 1) /* -95.75 usec */
/*---------------------------------------------------------------------------*/
#define STATUS_CORRELATION   0x3f  // bits 0-5
#define STATUS_REJECT_FRAME  0x40  // bit 6
#define STATUS_CRC_FAIL      0x80  // bit 7
/*---------------------------------------------------------------------------*/
#define CHANNEL_CLEAR_ERROR  -1
/*---------------------------------------------------------------------------*/
/* Global state */

/* Are we currently in poll mode? */
static volatile bool g_bPollMode = false;

/* Enable/disable CCA before sending */
static volatile bool g_bSendOnCca = false;

/* Last RX operation stats */
static volatile int8_t g_lastRssi;
static volatile uint8_t g_lastCorrLqi;
static volatile uint32_t g_lastTimestamp;
/*---------------------------------------------------------------------------*/
typedef enum {
    POWER_STATE_ON      = (1 << 0),
    POWER_STATE_OFF     = (1 << 1),
    POWER_STATE_RESTART = POWER_STATE_ON | POWER_STATE_OFF,
} PowerState;
/*---------------------------------------------------------------------------*/
/* Forward declarations of static functions */
static int set_rx(const PowerState);
static void check_rat_overflow(void);
static bool rf_is_on(void);
static uint32_t rat_to_timestamp(const uint32_t);
/*---------------------------------------------------------------------------*/
/* Forward declarations of Radio driver functions */
static int init(void);
static int prepare(const void*, unsigned short);
static int transmit(unsigned short);
static int send(const void*, unsigned short);
static int read(void*, unsigned short);
static int channel_clear(void);
static int receiving_packet(void);
static int pending_packet(void);
static int on(void);
static int off(void);
static radio_result_t get_value(radio_param_t, radio_value_t*);
static radio_result_t set_value(radio_param_t, radio_value_t);
static radio_result_t get_object(radio_param_t, void*, size_t);
static radio_result_t set_object(radio_param_t, const void*, size_t);
/*---------------------------------------------------------------------------*/
/* Radio driver object */
const struct radio_driver ieee_mode_driver = {
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
static void
rx_cb(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
  if (e & RF_EventRxOk) {
    process_poll(&rf_process);
  }
}
/*---------------------------------------------------------------------------*/
static void
rf_error_cb(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
  // See SWRZ062B: Synth failed to calibrate, CMD_FS must be repeated
  if ((ch == RF_ERROR_CMDFS_SYNTH_PROG) &&
      (cmd_fs.status == ERROR_SYNTH_PROG)) {
    // Call CMD_FS async, a synth error will trigger rf_error_cb once more
    const uint8_t stop_gracefully = 1;
    RF_flushCmd(g_rfHandle, RF_CMDHANDLE_FLUSH_ALL, stop_gracefully);
    RF_postCmd(g_rfHandle, (RF_Op*)&cmd_fs, RF_PriorityNormal, NULL, 0);
  }
}
/*---------------------------------------------------------------------------*/
static void
rat_overflow_cb(void *arg)
{
  check_rat_overflow();
  // Check next time after half of the RAT interval
  ctimer_set(&g_ratOverflowTimer, RAT_OVERFLOW_PERIOD_SECONDS * CLOCK_SECOND / 2,
             rat_overflow_cb, NULL);
}
/*---------------------------------------------------------------------------*/
static void
init_data_queue(void)
{
  // Initialize RF core data queue, circular buffer
  g_rxDataQueue.pCurrEntry = rx_bufs[0].buf;
  g_rxDataQueue.pLastEntry = NULL;
  // Set current read pointer to first element
  rx_read_entry = &rx_bufs[0].data_entry;
}
/*---------------------------------------------------------------------------*/
static void
init_rf_params(void)
{
    cmd_rx.channel = IEEE_MODE_CHANNEL;

    cmd_rx.pRxQ = &g_rxDataQueue;
    cmd_rx.pOutput = &g_rxStats;

#if IEEE_MODE_PROMISCOUS
    cmd_rx.frameFiltOpt.frameFiltEn = 0;
#else
    cmd_rx.frameFiltOpt.frameFiltEn = 1;
#endif

#if IEEE_MODE_AUTOACK
    cmd_rx.frameFiltOpt.autoAckEn = 1;
#else
    cmd_rx.frameFiltOpt.autoAckEn = 0;
#endif

    cmd_rx.ccaRssiThr = IEEE_MODE_RSSI_THRESHOLD;

    // Initialize address filter command
    g_cmdModFilt.commandNo = CMD_IEEE_MOD_FILT;
    memcpy(&(g_cmdModFilt.newFrameFiltOpt), &(rf_cmd_ieee_rx.frameFiltOpt), sizeof(rf_cmd_ieee_rx.frameFiltOpt));
    memcpy(&(g_cmdModFilt.newFrameTypes),   &(rf_cmd_ieee_rx.frameTypes),   sizeof(rf_cmd_ieee_rx.frameTypes));
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
      .length       = RX_BUF_SIZE - sizeof(data_entry_t), /* TODO: is  this sizeof sound? */
      /* Point to fist entry if this is last entry, else point to next entry */
      .pNextEntry   = (i == (RX_BUF_CNT - 1))
        ? rx_bufs[0].buf
        : rx_bufs[i].buf
    };
    rx_bufs[i].data_entry = data_entry;
  }
}
/*---------------------------------------------------------------------------*/
static bool
set_channel(uint8_t channel)
{
  if (!IEEE_MODE_CHAN_IN_RANGE(channel)) {
    PRINTF("set_channel: illegal channel %d, defaults to %d\n",
           channel, IEEE_MODE_CHANNEL);
    channel = IEEE_MODE_CHANNEL;
  }
  if (channel == cmd_rx.channel) {
    // We are already calibrated to this channel
      return true;
  }

  cmd_rx.channel = 0;

  // freq = freq_base + freq_spacing * (channel - channel_min)
  const uint32_t newFreq = (uint32_t)(IEEE_MODE_FREQ_BASE + IEEE_MODE_FREQ_SPACING * ((uint32_t)channel - IEEE_MODE_CHAN_MIN));
  const uint32_t freq = newFreq / 1000;
  const uint32_t frac = (newFreq - (freq * 1000)) * 65536 / 1000;

  PRINTF("set_channel: %d = 0x%04X.0x%04X (%lu)\n",
         channel, (uint16_t)freq, (uint16_t)frac, newFreq);

  cmd_fs.frequency = (uint16_t)freq;
  cmd_fs.fractFreq = (uint16_t)frac;

  const bool rx_active = (cmd_rx.status == ACTIVE);

  if (rx_active) {
    const uint8_t stop_gracefully = 1;
    RF_flushCmd(g_rfHandle, RF_CMDHANDLE_FLUSH_ALL, stop_gracefully);
  }

  // Start FS command asynchronously. We don't care when it is finished
  RF_EventMask events = 0;
  uint8_t tries = 0;
  bool cmd_ok = false;
  do {
    events = RF_runCmd(g_rfHandle, (RF_Op*)&cmd_fs, RF_PriorityNormal, NULL, 0);
    cmd_ok = ((events & RF_EventLastCmdDone) != 0)
          && (cmd_fs.status == DONE_OK);
  } while (!cmd_ok && (tries++ < 3));

  if (!cmd_ok) {
    return false;
  }

  cmd_rx.channel = channel;

  if (rx_active) {
      set_rx(POWER_STATE_ON);
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static int
set_tx_power(const radio_value_t dbm)
{
  const RF_TxPowerTable_Value tx_power_table_value = RF_TxPowerTable_findValue(TX_POWER_TABLE, (int8_t)dbm);
  const RF_Stat stat = RF_setTxPower(g_rfHandle, tx_power_table_value);

  if (stat != RF_StatSuccess) {
    PRINTF("set_tx_power: stat=0x%02X\n", stat);
    return CMD_RESULT_ERROR;
  }
  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_value_t
get_tx_power(void)
{
  const RF_TxPowerTable_Value tx_power_table_value = RF_getTxPower(g_rfHandle);
  const int8_t dbm = RF_TxPowerTable_findPowerLevel(TX_POWER_TABLE, tx_power_table_value);

  if (dbm == RF_TxPowerTable_INVALID_DBM) {
    PRINTF("get_tx_power: invalid dbm received=%d\n", dbm);
  }

  return (radio_value_t)dbm;
}
/*---------------------------------------------------------------------------*/
static void
set_send_on_cca(bool enable)
{
  g_bSendOnCca = enable;
}
/*---------------------------------------------------------------------------*/
static void
check_rat_overflow(void)
{
  const bool was_off = !rf_is_on();
  if (was_off) {
    RF_runDirectCmd(g_rfHandle, CMD_NOP);
  }
  const uint32_t currentValue = RF_getCurrentTime();

  static uint32_t lastValue;
  static bool bFirstTime = true;
  if (bFirstTime) {
    // First time checking overflow will only store the current value
    bFirstTime = false;
  } else {
    // Overflow happens in the last quarter of the RAT range
    if (currentValue + RAT_RANGE / 4 < lastValue) {
      // Overflow detected
      g_ratLastOverflow = RTIMER_NOW();
      g_ratOverflowCount += 1;
    }
  }

  lastValue = currentValue;

  if (was_off) {
    off();
  }
}
/*---------------------------------------------------------------------------*/
static uint32_t
rat_to_timestamp(const uint32_t ratTimestamp)
{
  check_rat_overflow();

  uint64_t adjustedOverflowCount = g_ratOverflowCount;

  // If the timestamp is in the 4th quarter and the last overflow was recently,
  // assume that the timestamp refers to the time before the overflow
  if(ratTimestamp > (uint32_t)(RAT_RANGE * 3 / 4)) {
    if(RTIMER_CLOCK_LT(RTIMER_NOW(),
                       g_ratLastOverflow + RAT_OVERFLOW_PERIOD_SECONDS * RTIMER_SECOND / 4)) {
      adjustedOverflowCount -= 1;
    }
  }

  // Add the overflowed time to the timestamp
  const uint64_t ratTimestamp64 = (uint64_t)ratTimestamp + (uint64_t)RAT_RANGE * adjustedOverflowCount;

  // Correct timestamp so that it refers to the end of the SFD and convert to RTIMER
  return RADIO_TO_RTIMER(ratTimestamp64 + RAT_TIMESTAMP_OFFSET);
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  RF_Params params;
  RF_Params_init(&params);
  params.pErrCb = rf_error_cb;

  init_rf_params();
  init_data_queue();

  g_rfHandle = RF_open(&g_rfObj, &rf_ieee_mode, (RF_RadioSetup*)&cmd_radio_setup, &params);
  assert(g_rfHandle != NULL);

  set_channel(IEEE_MODE_CHANNEL);

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  // Start RAT overflow upkeep
  check_rat_overflow();
  ctimer_set(&g_ratOverflowTimer, RAT_OVERFLOW_PERIOD_SECONDS * CLOCK_SECOND / 2,
             rat_overflow_cb, NULL);

  process_start(&rf_process, NULL);

  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  const size_t len = MIN((size_t)payload_len,
                         (size_t)TX_BUF_PAYLOAD_LEN);
  memcpy(&g_txBuf[TX_BUF_HDR_LEN], payload, len);
  return 0;
}
/*---------------------------------------------------------------------------*/
static bool
rf_is_on(void)
{
  RF_InfoVal infoVal;
  memset(&infoVal, 0x0, sizeof(RF_InfoVal));
  const RF_Stat stat = RF_getInfo(g_rfHandle, RF_GET_RADIO_STATE, &infoVal);
  return (stat == RF_StatSuccess)
    ? infoVal.bRadioState  // 0: Radio OFF, 1: Radio ON
    : false;
}
/*---------------------------------------------------------------------------*/
static int
set_rx(const PowerState state)
{
  if (state & POWER_STATE_OFF) {
    // Stop RX gracefully, don't care about the result
    const uint8_t stop_gracefully = 1;
    RF_cancelCmd(g_rfHandle, g_cmdRxHandle, stop_gracefully);
  }

  if (state & POWER_STATE_ON) {
    if (cmd_rx.status == ACTIVE) {
      PRINTF("set_rx(on): already in RX\n");
      return CMD_RESULT_OK;
    }

    RF_ScheduleCmdParams schedParams;
    RF_ScheduleCmdParams_init(&schedParams);

    cmd_rx.status = IDLE;
    g_cmdRxHandle = RF_scheduleCmd(g_rfHandle, (RF_Op*)&cmd_rx, &schedParams, rx_cb,
                                   RF_EventRxOk | RF_EventRxBufFull | RF_EventRxEntryDone);
    if ((g_cmdRxHandle == RF_ALLOC_ERROR) || (g_cmdRxHandle == RF_SCHEDULE_CMD_ERROR)) {
      PRINTF("transmit: unable to allocate RX command\n");
      return CMD_RESULT_ERROR;
    }
  }

  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
transmit_aux(unsigned short transmit_len)
{
  // Configure TX command
  cmd_tx.payloadLen = (uint8_t)transmit_len;
  cmd_tx.pPayload = &g_txBuf[TX_BUF_HDR_LEN];

  RF_ScheduleCmdParams schedParams;
  RF_ScheduleCmdParams_init(&schedParams);

  // As IEEE_TX is a FG command, the TX operation will be executed
  // either way if RX is running or not
  cmd_tx.status = IDLE;
  g_cmdTxHandle = RF_scheduleCmd(g_rfHandle, (RF_Op*)&cmd_tx, &schedParams, NULL, 0);
  if ((g_cmdTxHandle == RF_ALLOC_ERROR) || (g_cmdTxHandle == RF_SCHEDULE_CMD_ERROR)) {
      // Failure sending the CMD_IEEE_TX command
      PRINTF("transmit: failed to allocate TX command cmdHandle=%d, status=%04x\n",
             g_cmdTxHandle, cmd_tx.status);
      return RADIO_TX_ERR;
  }

  ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);

  // Wait until TX operation finishes
  RF_EventMask events = RF_pendCmd(g_rfHandle, g_cmdTxHandle, 0);
  if ((events & (RF_EventFGCmdDone | RF_EventLastFGCmdDone)) == 0) {
    PRINTF("transmit: TX command error events=0x%08llx, status=0x%04x\n",
           events, cmd_tx.status);
    return RADIO_TX_ERR;
  }

  return RADIO_TX_OK;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  const bool was_rx = (cmd_rx.status == ACTIVE);

  if (g_bSendOnCca && channel_clear() != 1) {
    PRINTF("transmit: channel wasn't clear\n");
    return RADIO_TX_COLLISION;
  }

  const int rv = transmit_aux(transmit_len);
  ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);

  if (!was_rx) {
    off();
  }
  return rv;
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
  data_entry_t *const data_entry = rx_read_entry;
  uint8_t *const frame_ptr = (uint8_t*)&data_entry->data;


  // Clear the length byte and set status to 0: "Pending"
  *(lensz_t*)frame_ptr = 0;
  data_entry->status = DATA_ENTRY_PENDING;
  rx_read_entry = (data_entry_t*)data_entry->pNextEntry;
}
/*---------------------------------------------------------------------------*/
static int
read(void *buf, unsigned short buf_len)
{
  volatile data_entry_t *data_entry = rx_read_entry;

  const rtimer_clock_t t0 = RTIMER_NOW();
  // Only wait if the Radio timer is accessing the entry
  while ((data_entry->status == DATA_ENTRY_BUSY) &&
          RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + TIMEOUT_DATA_ENTRY_BUSY));

  if (data_entry->status != DATA_ENTRY_FINISHED) {
    // No available data
    return 0;
  }

  /* First byte in the data entry is the length.
   * Data frame is on the following format:
   *    Length (1) + Payload (N) + FCS (2) + RSSI (1) + Status (1) + Timestamp (4)
   * Data frame DOES NOT contain the following:
   *    no PHY Header bytes
   *    no Source Index bytes
   * +--------+---------+---------+--------+--------+-----------+
   * | 1 byte | N bytes | 2 bytes | 1 byte | 1 byte | 4 bytes   |
   * +--------+---------+---------+--------+--------+-----------+
   * | Length | Payload | FCS     | RSSI   | Status | Timestamp |
   * +--------+---------+---------+--------+--------+-----------+
   * Length bytes equal total length of entire frame excluding itself,
   * i.e.: Length = N + FCS (2) + RSSI (1) + Status (1) + Timestamp (4)
   *              = N + 8
   *            N = Length - 8 */

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

  /* RSSI stored FCS (2) bytes after payload */
  g_lastRssi = (int8_t)payload_ptr[payload_len + 2];
  /* LQI retrieved from Status byte, FCS (2) + RSSI (1) bytes after payload */
  g_lastCorrLqi = ((uint8_t)payload_ptr[payload_len + 3]) & STATUS_CORRELATION;
  /* Timestamp stored FCS (2) + RSSI (1) + Status (1) bytes after payload */
  const uint32_t ratTimestamp = *(uint32_t*)(payload_ptr + payload_len + 4);
  g_lastTimestamp = rat_to_timestamp(ratTimestamp);

  if (!g_bPollMode) {
    // Not in poll mode: packetbuf should not be accessed in interrupt context.
    // In poll mode, the last packet RSSI and link quality can be obtained through
    // RADIO_PARAM_LAST_RSSI and RADIO_PARAM_LAST_LINK_QUALITY
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI,         (packetbuf_attr_t)g_lastRssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, (packetbuf_attr_t)g_lastCorrLqi);
  }

  release_data_entry();
  return (int)payload_len;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear_aux(void)
{
  rfc_CMD_IEEE_CCA_REQ_t RF_cmdIeeeCaaReq;
  memset(&RF_cmdIeeeCaaReq, 0x0, sizeof(rfc_CMD_IEEE_CCA_REQ_t));
  RF_cmdIeeeCaaReq.commandNo = CMD_IEEE_CCA_REQ;

  const RF_Stat stat = RF_runImmediateCmd(g_rfHandle, (uint32_t*)&RF_cmdIeeeCaaReq);
  if (stat != RF_StatCmdDoneSuccess) {
    PRINTF("channel_clear: CCA request failed stat=0x%02X\n", stat);
    return CMD_RESULT_ERROR;
  }

  // Channel is clear if CCA state is idle (0) or invalid (2), i.e. not busy (1)
  return (RF_cmdIeeeCaaReq.ccaInfo.ccaState != 1);
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  const bool was_rx = (cmd_rx.status == ACTIVE);
  if (!was_rx && set_rx(POWER_STATE_ON) != CMD_RESULT_OK) {
    PRINTF("channel_clear: unable to start RX\n");
    return CHANNEL_CLEAR_ERROR;
  }

  const int rv = channel_clear_aux();

  if (!was_rx) {
    set_rx(POWER_STATE_OFF);
  }
  return rv;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  // If we are not in RX, we are not receiving
  if (cmd_rx.status != ACTIVE) {
    PRINTF("receiving_packet: not in RX\n");
    return 0;
  }

  rfc_CMD_IEEE_CCA_REQ_t cca_req;
  memset(&cca_req, 0x0, sizeof(rfc_CMD_IEEE_CCA_REQ_t));
  cca_req.commandNo = CMD_IEEE_CCA_REQ;

  const RF_Stat stat = RF_runImmediateCmd(g_rfHandle, (uint32_t*)&cca_req);
  if (stat != RF_StatCmdDoneSuccess) {
    PRINTF("receiving_packet: CCA request failed stat=0x%02X\n", stat);
    return 0;
  }

  // We are in RX if a CCA sync has been seen, i.e. ccaSync is busy (1)
  return (cca_req.ccaInfo.ccaSync == CCA_STATE_BUSY);
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  const rfc_dataEntry_t *const pStartEntry = (rfc_dataEntry_t *)g_rxDataQueue.pCurrEntry;
  volatile const rfc_dataEntry_t *pCurrEntry = pStartEntry;

  int rv = 0;

  // Check all RX buffers and check their statuses, stopping when looping the circular buffer
  do {
    const uint8_t status = pCurrEntry->status;
    if ((status == DATA_ENTRY_FINISHED) ||
        (status == DATA_ENTRY_BUSY)) {
      rv += 1;
    }

    pCurrEntry = (rfc_dataEntry_t *)pCurrEntry->pNextEntry;
  } while (pCurrEntry != pStartEntry);

  if ((rv > 0) && !g_bPollMode) {
    process_poll(&rf_process);
  }

  // If we didn't find an entry at status finished or busy, no frames are pending
  return rv;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  init_rx_buffers();

  const int rv = set_rx(POWER_STATE_ON);

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  return rv;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  const uint8_t stopGracefully = 1;
  RF_flushCmd(g_rfHandle, RF_CMDHANDLE_FLUSH_ALL, stopGracefully);
  RF_yield(g_rfHandle);

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

  // Reset RX buffers if there was an ongoing RX
  size_t i;
  for (i = 0; i < RX_BUF_CNT; ++i) {
    data_entry_t *entry = &rx_bufs[i].data_entry;
    if (entry->status == DATA_ENTRY_BUSY) {
      entry->status = DATA_ENTRY_PENDING;
    }
  }

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
    *value = rf_is_on()
      ? RADIO_POWER_MODE_ON
      : RADIO_POWER_MODE_OFF;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_CHANNEL:
    *value = (radio_value_t)cmd_rx.channel;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_PAN_ID:
    *value = (radio_value_t)cmd_rx.localPanID;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_16BIT_ADDR:
    *value = (radio_value_t)cmd_rx.localShortAddr;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RX_MODE:
    *value = 0;
    if (cmd_rx.frameFiltOpt.frameFiltEn) {
      *value |= (radio_value_t)RADIO_RX_MODE_ADDRESS_FILTER;
    }
    if (cmd_rx.frameFiltOpt.autoAckEn) {
      *value |= (radio_value_t)RADIO_RX_MODE_AUTOACK;
    }
    if (g_bPollMode) {
      *value |= (radio_value_t)RADIO_RX_MODE_POLL_MODE;
    }
    return RADIO_RESULT_OK;

  case RADIO_PARAM_TX_MODE:
    *value = 0;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_TXPOWER:
    *value = get_tx_power();
    return (*value == RF_TxPowerTable_INVALID_DBM)
      ? RADIO_RESULT_ERROR
      : RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    *value = cmd_rx.ccaRssiThr;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RSSI:
    *value = RF_getRssi(g_rfHandle);
    return (*value == RF_GET_RSSI_ERROR_VAL)
      ? RADIO_RESULT_ERROR
      : RADIO_RESULT_OK;

  case RADIO_CONST_CHANNEL_MIN:
    *value = (radio_value_t)IEEE_MODE_CHAN_MIN;
    return RADIO_RESULT_OK;

  case RADIO_CONST_CHANNEL_MAX:
    *value = (radio_value_t)IEEE_MODE_CHAN_MAX;
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MIN:
    *value = (radio_value_t)(TX_POWER_MIN.power);
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MAX:
    *value = (radio_value_t)(TX_POWER_MAX.power);
    return RADIO_RESULT_OK;

  case RADIO_PARAM_LAST_RSSI:
    *value = (radio_value_t)g_lastRssi;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_LAST_LINK_QUALITY:
    *value = (radio_value_t)g_lastCorrLqi;
    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
  switch (param) {
  case RADIO_PARAM_POWER_MODE:
    if (value == RADIO_POWER_MODE_ON) {
      if (on() != CMD_RESULT_OK) {
        PRINTF("set_value: on() failed (1)\n");
        return RADIO_RESULT_ERROR;
      }
      return RADIO_RESULT_OK;
    } else if (value == RADIO_POWER_MODE_OFF) {
      off();
      return RADIO_RESULT_OK;
    }

    return RADIO_RESULT_INVALID_VALUE;

  case RADIO_PARAM_CHANNEL:
    if (!IEEE_MODE_CHAN_IN_RANGE(value)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_channel((uint8_t)value);
    return RADIO_RESULT_OK;

  case RADIO_PARAM_PAN_ID:
    cmd_rx.localPanID = (uint16_t)value;
    if (rf_is_on() && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
      PRINTF("failed to restart RX");
      return RADIO_RESULT_ERROR;
    }
    return RADIO_RESULT_OK;

  case RADIO_PARAM_16BIT_ADDR:
    cmd_rx.localShortAddr = (uint16_t)value;
    if (rf_is_on() && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
      PRINTF("failed to restart RX");
      return RADIO_RESULT_ERROR;
    }
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RX_MODE: {
    if (value & ~(RADIO_RX_MODE_ADDRESS_FILTER |
                  RADIO_RX_MODE_AUTOACK | RADIO_RX_MODE_POLL_MODE)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    cmd_rx.frameFiltOpt.frameFiltEn = (value & RADIO_RX_MODE_ADDRESS_FILTER) != 0;
    cmd_rx.frameFiltOpt.frameFiltStop = 1;
    cmd_rx.frameFiltOpt.autoAckEn = (value & RADIO_RX_MODE_AUTOACK) != 0;
    cmd_rx.frameFiltOpt.slottedAckEn = 0;
    cmd_rx.frameFiltOpt.autoPendEn = 0;
    cmd_rx.frameFiltOpt.defaultPend = 0;
    cmd_rx.frameFiltOpt.bPendDataReqOnly = 0;
    cmd_rx.frameFiltOpt.bPanCoord = 0;
    cmd_rx.frameFiltOpt.bStrictLenFilter = 0;

    const bool bOldPollMode = g_bPollMode;
    g_bPollMode = (value & RADIO_RX_MODE_POLL_MODE) != 0;
    if (g_bPollMode == bOldPollMode) {
      // Do not turn the radio off and on, just send an update command
      memcpy(&g_cmdModFilt.newFrameFiltOpt, &(rf_cmd_ieee_rx.frameFiltOpt), sizeof(rf_cmd_ieee_rx.frameFiltOpt));
      const RF_Stat stat = RF_runImmediateCmd(g_rfHandle, (uint32_t*)&g_cmdModFilt);
      if (stat != RF_StatCmdDoneSuccess) {
        PRINTF("setting address filter failed: stat=0x%02X\n", stat);
        return RADIO_RESULT_ERROR;
      }
      return RADIO_RESULT_OK;
    }
    if (rf_is_on() && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
      PRINTF("failed to restart RX");
      return RADIO_RESULT_ERROR;
    }
    return RADIO_RESULT_OK;
  }

  case RADIO_PARAM_TX_MODE:
    if(value & ~(RADIO_TX_MODE_SEND_ON_CCA)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_send_on_cca((value & RADIO_TX_MODE_SEND_ON_CCA) != 0);
    return RADIO_RESULT_OK;

  case RADIO_PARAM_TXPOWER:
    if(value < TX_POWER_MIN.power || value > TX_POWER_MAX.power) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    return (set_tx_power(value) != CMD_RESULT_OK)
      ? RADIO_RESULT_ERROR
      : RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    cmd_rx.ccaRssiThr = (int8_t)value;
    if (rf_is_on() && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
      PRINTF("failed to restart RX");
      return RADIO_RESULT_ERROR;
    }
    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  if (!dest) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch (param) {
  case RADIO_PARAM_64BIT_ADDR: {
    const size_t srcSize = sizeof(cmd_rx.localExtAddr);
    if(size != srcSize) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    const uint8_t *pSrc = (uint8_t *)&(cmd_rx.localExtAddr);
    uint8_t *pDest = dest;
    for(size_t i = 0; i < srcSize; ++i) {
      pDest[i] = pSrc[srcSize - 1 - i];
    }

    return RADIO_RESULT_OK;
  }
  case RADIO_PARAM_LAST_PACKET_TIMESTAMP:
    if(size != sizeof(rtimer_clock_t)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    *(rtimer_clock_t *)dest = g_lastTimestamp;

    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
  if (!src) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch (param) {
  case RADIO_PARAM_64BIT_ADDR: {
    const size_t destSize = sizeof(cmd_rx.localExtAddr);
    if (size != destSize) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    const uint8_t *pSrc = (const uint8_t *)src;
    uint8_t *pDest = (uint8_t *)&(cmd_rx.localExtAddr);
    for (size_t i = 0; i < destSize; ++i) {
      pDest[i] = pSrc[destSize - 1 - i];
    }

    const bool is_rx = (cmd_rx.status == ACTIVE);
    if (is_rx && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
      return RADIO_RESULT_ERROR;
    }
    return RADIO_RESULT_OK;
  }
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
