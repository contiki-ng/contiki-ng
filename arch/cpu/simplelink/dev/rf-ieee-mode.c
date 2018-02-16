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
#include <driverlib/rf_mailbox.h>
#include <driverlib/rf_common_cmd.h>
#include <driverlib/rf_data_entry.h>
#include <driverlib/rf_ieee_cmd.h>
#include <driverlib/rf_ieee_mailbox.h>
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
/* RF settings */
#ifdef IEEE_MODE_CONF_RF_SETTINGS
#   define IEEE_MODE_RF_SETTINGS  IEEE_MODE_CONF_RF_SETTINGS
#   undef IEEE_MODE_CONF_RF_SETTINGS
#else
#   define IEEE_MODE_RF_SETTINGS "rf-settings/rf-ieee-settings.h"
#endif

#include IEEE_MODE_RF_SETTINGS
/*---------------------------------------------------------------------------*/
/* Simplelink Platform RF dev */
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
/* Configuration parameters */

/* Configuration to enable/disable auto ACKs in IEEE mode */
#ifdef IEEE_MODE_CONF_AUTOACK
#   define IEEE_MODE_AUTOACK  IEEE_MODE_CONF_AUTOACK
#   undef IEEE_MODE_CONF_AUTOACK
#else
#   define IEEE_MODE_AUTOACK  1
#endif /* IEEE_MODE_CONF_AUTOACK */

/* Configuration to enable/disable frame filtering in IEEE mode */
#ifdef IEEE_MODE_CONF_PROMISCOUS
#   define IEEE_MODE_PROMISCOUS  IEEE_MODE_CONF_PROMISCOUS
#   undef IEEE_MODE_CONF_PROMISCOUS
#else
#   define IEEE_MODE_PROMISCOUS  0
#endif /* IEEE_MODE_CONF_PROMISCOUS */

/* Configuration to set the RSSI threshold */
#ifdef IEEE_MODE_CONF_RSSI_THRESHOLD
#   define IEEE_MODE_RSSI_THRESHOLD  IEEE_MODE_CONF_RSSI_THRESHOLD
#   undef IEEE_MODE_CONF_RSSI_THRESHOLD
#else
#   define IEEE_MODE_RSSI_THRESHOLD  0xA6
#endif /* IEEE_MODE_CONF_RSSI_THRESHOLD */

/* Configuration for default IEEE channel */
#ifdef IEEE_MODE_CONF_CHANNEL
#   define IEEE_MODE_CHANNEL  IEEE_MODE_CONF_CHANNEL
#   undef IEEE_MODE_CONF_CHANNEL
#else
#   define IEEE_MODE_CHANNEL  RF_CORE_CHANNEL
#endif

/* Configuration for TX power table */
#ifdef TX_POWER_CONF_DRIVER
#   define TX_POWER_DRIVER  TX_POWER_CONF_DRIVER
#   undef TX_POWER_CONF_DRIVER
#else
#   define TX_POWER_DRIVER  RF_ieeeTxPower
#endif

#ifdef TX_POWER_CONF_COUNT
#   define TX_POWER_COUNT  TX_POWER_CONF_COUNT
#   undef TX_POWER_CONF_COUNT
#else
#   define TX_POWER_COUNT  RF_ieeeTxPowerLen
#endif
/*---------------------------------------------------------------------------*/
/* TX power convenience macros */
static RF_TxPower * const g_pTxPower = TX_POWER_DRIVER;

#define TX_POWER_MAX  (g_pTxPower[0])
#define TX_POWER_MIN  (g_pTxPower[(TX_POWER_COUNT) - 1])

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

#define IEEE_MODE_CHAN_IN_RANGE(ch)  (((ch) >= IEEE_MODE_CHAN_MIN) && ((ch) <= IEEE_MODE_CHAN_MAX))

/* Sanity check of default IEEE channel */
#if !IEEE_MODE_CHAN_IN_RANGE(IEEE_MODE_CHANNEL)
#   error "Default IEEE channel IEEE_MODE_CHANNEL is outside allowed channel range"
#endif
/*---------------------------------------------------------------------------*/
/* Timeout constants */

/* How long to wait for an ongoing ACK TX to finish before starting frame TX */
#define TIMEOUT_TX_WAIT          (RTIMER_SECOND >> 11)

/* How long to wait for the RF to enter RX in RF_cmdIeeeRx */
#define TIMEOUT_ENTER_RX_WAIT    (RTIMER_SECOND >> 10)

/* How long to wait for the RF to react on CMD_ABORT: around 1 msec */
#define TIMEOUT_RF_TURN_OFF_WAIT (RTIMER_SECOND >> 10)

/* How long to wait for the RF to finish TX of a packet or an ACK */
#define TIMEOUT_TX_FINISH_WAIT   (RTIMER_SECOND >> 7)

/* How long to wait for the rx read entry to become ready */
#define TIMEOUT_DATA_ENTRY_BUSY (RTIMER_SECOND / 250)
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF driver object */
static RF_Object g_rfObj;
static RF_Handle g_rfHandle;

/* RF Core command pointers */
static volatile rfc_CMD_RADIO_SETUP_t *g_vpCmdRadioSetup = &RF_cmdRadioSetup;
static volatile rfc_CMD_FS_t          *g_vpCmdFs         = &RF_cmdFs;
static volatile rfc_CMD_IEEE_TX_t     *g_vpCmdTx         = &RF_cmdIeeeTx;
static volatile rfc_CMD_IEEE_RX_t     *g_vpCmdRx         = &RF_cmdIeeeRx;

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
/* Receive entry pointer to keep track of read items */
volatile static uint8_t *g_pRxReadEntry;

/* Constants for receive buffers */
#define DATA_ENTRY_LENSZ_NONE  0
#define DATA_ENTRY_LENSZ_BYTE  1
#define DATA_ENTRY_LENSZ_WORD  2 /* 2 bytes */

#define RX_BUF_ENTRIES  4
#define RX_BUF_SIZE     144

/* Receive buffer entries with room for 1 IEEE 802.15.4 frame in each */
typedef union {
    rfc_dataEntry_t dataEntry;
    uint8_t         buf[RX_BUF_SIZE] CC_ALIGN(4);
} RxBuf;
static RxBuf g_rxBufs[RX_BUF_ENTRIES];
/*---------------------------------------------------------------------------*/
/* RAT overflow upkeep */
static struct ctimer g_ratOverflowTimer;
static rtimer_clock_t g_lastRatOverflow;
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

/* Current RX channel */
static volatile uint8_t g_currChannel;

/* Current TX power */
static volatile RF_TxPower *g_pCurrTxPower;

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
    POWER_STATE_ON,
    POWER_STATE_OFF,
    POWER_STATE_RESTART,
} PowerState;
/*---------------------------------------------------------------------------*/
/* Forward declarations of static functions */
static int on(void);
static int off(void);
static int set_rx(const PowerState);
static int channel_clear(void);
static void check_rat_overflow(void);
static bool rf_is_on(void);
static uint32_t rat_to_timestamp(const uint32_t);
/*---------------------------------------------------------------------------*/
static void
synth_error_cb(RF_Handle h, RF_CmdHandle ch, RF_EventMask e)
{
  if ((ch == RF_ERROR_CMDFS_SYNTH_PROG) &&
      (g_vpCmdFs->status == ERROR_SYNTH_PROG)) {
    // See SWRA521: Synth failed to calibrate, CMD_FS must be repeated
    RF_postCmd(g_rfHandle, (RF_Op*)g_vpCmdFs, RF_PriorityNormal, synth_error_cb, 0);
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
  g_rxDataQueue.pCurrEntry = g_rxBufs[0].buf;
  g_rxDataQueue.pLastEntry = NULL;
  // Set current read pointer to first element
  g_pRxReadEntry = g_rxDataQueue.pCurrEntry;
}
/*---------------------------------------------------------------------------*/
static void
init_rf_params(void)
{
    g_vpCmdRx->pRxQ = &g_rxDataQueue;
    g_vpCmdRx->pOutput = &g_rxStats;

#if IEEE_MODE_PROMISCOUS
    g_vpCmdRx->frameFiltOpt.frameFiltEn = 0;
#else
    g_vpCmdRx->frameFiltOpt.frameFiltEn = 1;
#endif

#if IEEE_MODE_AUTOACK
    g_vpCmdRx->frameFiltOpt.autoAckEn = 1;
#else
    g_vpCmdRx->frameFiltOpt.autoAckEn = 0;
#endif

    g_vpCmdRx->ccaRssiThr = IEEE_MODE_RSSI_THRESHOLD;

    // Initialize address filter command
    g_cmdModFilt.commandNo = CMD_IEEE_MOD_FILT;
    memcpy(&g_cmdModFilt.newFrameFiltOpt, &RF_cmdIeeeRx.frameFiltOpt, sizeof(RF_cmdIeeeRx.frameFiltOpt));
    memcpy(&g_cmdModFilt.newFrameTypes,   &RF_cmdIeeeRx.frameTypes,   sizeof(RF_cmdIeeeRx.frameTypes));
}
/*---------------------------------------------------------------------------*/
static void
init_rx_buffers(void)
{
#define getEntry(n) (&(g_rxBufs[(n)].dataEntry))

  rfc_dataEntry_t *entry = NULL;
  const uint16_t length = sizeof(g_rxBufs[0].buf) - 8;

  size_t i;
  for (i = 0; i < (size_t)(RX_BUF_ENTRIES - 1); ++i) {
      entry = getEntry(i);
      entry->pNextEntry = (uint8_t*)getEntry(i + 1);
      entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
      entry->length = length;
  }

  entry = getEntry(RX_BUF_ENTRIES - 1);
  entry->pNextEntry = (uint8_t*)getEntry(0);
  entry->config.lenSz = DATA_ENTRY_LENSZ_BYTE;
  entry->length = length;

#undef getEntry
}
/*---------------------------------------------------------------------------*/
static void
set_channel(uint8_t channel)
{
  if (!IEEE_MODE_CHAN_IN_RANGE(channel)) {
    PRINTF("set_channel: illegal channel %d, defaults to %d\n",
           channel, IEEE_MODE_CHANNEL);
    channel = IEEE_MODE_CHANNEL;
  }
  if (channel == g_currChannel) {
    // We are already calibrated to this channel
      return;
  }
  g_currChannel = channel;
  // freq = freq_base + freq_spacing * (channel - channel_min)
  const uint32_t newFreq = (uint32_t)IEEE_MODE_FREQ_BASE +
          (uint32_t)IEEE_MODE_FREQ_SPACING * ((uint32_t)channel - (uint32_t)IEEE_MODE_CHAN_MIN);
  const uint16_t freq = (uint16_t)(newFreq / 1000);
  const uint16_t frac = (uint16_t)((newFreq - (freq * 1000)) * 65536 / 1000);

  PRINTF("set_channel: %d = 0x%04X.0x%04X (%lu)\n",
         channel, freq, frac, newFreq);

  g_vpCmdFs->frequency = freq;
  g_vpCmdFs->fractFreq = frac;

  // Start FS command asynchronously. We don't care when it is finished
  RF_postCmd(g_rfHandle, (RF_Op*)g_vpCmdFs, RF_PriorityNormal, synth_error_cb, 0);
  if (g_vpCmdRx->status == ACTIVE) {
      set_rx(POWER_STATE_RESTART);
  }
}
/*---------------------------------------------------------------------------*/
static int
set_tx_power(const radio_value_t dbm)
{
  g_pCurrTxPower = NULL;
  if (dbm > TX_POWER_MAX.dbm) {
    g_pCurrTxPower = &TX_POWER_MAX;
  } else {
    size_t i;
    for (i = 0; g_pTxPower[i + 1].power != TX_POWER_UNKNOWN; ++i) {
      if (dbm > g_pTxPower[i + 1].dbm) {
        break;
      }
    }
    g_pCurrTxPower = &g_pTxPower[i + 1];
  }

  if (!g_pCurrTxPower) {
    return CMD_RESULT_ERROR;
  }

  rfc_CMD_SET_TX_POWER_t cmdSetTxPower = {
    .commandNo = CMD_SET_TX_POWER,
    .txPower = g_pCurrTxPower->power,
  };

  const RF_Stat stat = RF_runImmediateCmd(g_rfHandle, (uint32_t*)&cmdSetTxPower);
  if (stat != RF_StatCmdDoneSuccess) {
    PRINTF("set_tx_power: stat=0x%02X\n", stat);
    return CMD_RESULT_ERROR;
  }
  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static radio_value_t
get_tx_power(void)
{
  return (g_pCurrTxPower)
    ? g_pCurrTxPower->power
    : (radio_value_t)TX_POWER_UNKNOWN;
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
  const bool was_off = rf_is_on();
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
      g_lastRatOverflow = RTIMER_NOW();
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

  // If the timestamp is in the 4th quarter and the last oveflow was recently,
  // assume that the timestamp refers to the time before the overflow
  if(ratTimestamp > (uint32_t)(RAT_RANGE * 3 / 4)) {
    if(RTIMER_CLOCK_LT(RTIMER_NOW(),
                       g_lastRatOverflow + RAT_OVERFLOW_PERIOD_SECONDS * RTIMER_SECOND / 4)) {
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
  // Disable automatic power-down just to not interfere with stack timing
  params.nInactivityTimeout = 0;

  init_rf_params();
  init_data_queue();

  g_rfHandle = RF_open(&g_rfObj, &RF_ieeeMode, (RF_RadioSetup*)g_vpCmdRadioSetup, &params);
  assert(g_rfHandle != NULL);

  set_channel(IEEE_MODE_CHANNEL);

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  // Start RAT overflow upkeep
  check_rat_overflow();
  ctimer_set(&g_ratOverflowTimer, RAT_OVERFLOW_PERIOD_SECONDS * CLOCK_SECOND / 2,
             rat_overflow_cb, NULL);

  process_start(&RF_coreProcess, NULL);

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
  if (state == POWER_STATE_OFF || state == POWER_STATE_RESTART) {
    const uint16_t status = g_vpCmdRx->status;
    if (status != IDLE && status != IEEE_DONE_OK && status != IEEE_DONE_STOPPED) {
      const uint8_t stopGracefully = 1;
      const RF_Stat stat = RF_cancelCmd(g_rfHandle, g_cmdRxHandle,
                                        stopGracefully);
      if (stat != RF_StatSuccess)
      {
        PRINTF("set_rx(off): unable to cancel RX state=0x%02X\n", stat);
        return CMD_RESULT_ERROR;
      }
    }
  }
  if (state == POWER_STATE_ON || state == POWER_STATE_RESTART) {
    if (g_vpCmdRx->status == ACTIVE) {
      PRINTF("set_rx(on): already in RX\n");
      return CMD_RESULT_OK;
    }

    RF_ScheduleCmdParams schedParams = {
      .endTime = 0,
      .priority = RF_PriorityNormal,
      .bIeeeBgCmd = true,
    };

    g_vpCmdRx->status = IDLE;
    g_cmdRxHandle = RF_scheduleCmd(g_rfHandle, (RF_Op*)g_vpCmdRx, &schedParams, NULL, 0);
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
  g_vpCmdTx->payloadLen = (uint8_t)transmit_len;
  g_vpCmdTx->pPayload = &g_txBuf[TX_BUF_HDR_LEN];
  g_vpCmdTx->startTime = 0;
  g_vpCmdTx->startTrigger.triggerType = TRIG_NOW;

  RF_ScheduleCmdParams schedParams = {
    .endTime = 0,
    .priority = RF_PriorityNormal,
    .bIeeeBgCmd = false,
  };

  // As IEEE_TX is a FG command, the TX operation will be executed
  // either way if RX is running or not
  g_vpCmdTx->status = IDLE;
  g_cmdTxHandle = RF_scheduleCmd(g_rfHandle, (RF_Op*)g_vpCmdTx, &schedParams, NULL, 0);
  if ((g_cmdTxHandle == RF_ALLOC_ERROR) || (g_cmdTxHandle == RF_SCHEDULE_CMD_ERROR)) {
      // Failure sending the CMD_IEEE_TX command
      PRINTF("transmit: failed to allocate TX command cmdHandle=%d, status=%04x\n",
             g_cmdTxHandle, g_vpCmdTx->status);
      return RADIO_TX_ERR;
  }

  ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);

  // Wait until TX operation finishes
  RF_EventMask events = RF_pendCmd(g_rfHandle, g_cmdTxHandle, 0);
  if ((events & (RF_EventFGCmdDone | RF_EventLastFGCmdDone)) == 0) {
    PRINTF("transmit: TX command error events=0x%08llx, status=0x%04x\n",
           events, g_vpCmdTx->status);
    return RADIO_TX_ERR;
  }

  return RADIO_TX_OK;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  const bool was_rx = (g_vpCmdRx->status == ACTIVE);

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
  rfc_dataEntryGeneral_t *pEntry = (rfc_dataEntryGeneral_t *)g_pRxReadEntry;

  // Clear the length byte and set status to 0: "Pending"
  pEntry->length = 0;
  pEntry->status = DATA_ENTRY_PENDING;
  // Set next entry
  g_pRxReadEntry = pEntry->pNextEntry;
}
/*---------------------------------------------------------------------------*/
static int
read_frame(void *buf, unsigned short buf_len)
{
  volatile rfc_dataEntryGeneral_t *pEntry = (rfc_dataEntryGeneral_t *)g_pRxReadEntry;

  const rtimer_clock_t t0 = RTIMER_NOW();
  // Only wait if the Radio timer is accessing the entry
  while ((pEntry->status == DATA_ENTRY_BUSY) &&
          RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + TIMEOUT_DATA_ENTRY_BUSY));

  if (pEntry->status != DATA_ENTRY_FINISHED) {
    // No available data
    return 0;
  }

  // FIXME: something is wrong here about length constraints
  if (pEntry->length < 4) {
    PRINTF("read_frame: frame too short \n");
    release_data_entry();
    return 0;
  }

  const int frame_len = pEntry->length - 8;

  if (frame_len > buf_len) {
    PRINTF("read_frame: frame larger than buffer\n");
    release_data_entry();
    return 0;
  }

  const uint8_t *pData = (uint8_t *)&pEntry->data;

  memcpy(buf, pData, frame_len);

  g_lastRssi = (int8_t)(pData[frame_len + 2]);
  g_lastCorrLqi = (uint8_t)(pData[frame_len + 3]) & STATUS_CORRELATION;

  uint32_t ratTimestamp;
  memcpy(&ratTimestamp, pData + frame_len + 4, sizeof(ratTimestamp));
  g_lastTimestamp = rat_to_timestamp(ratTimestamp);

  if (!g_bPollMode) {
    // Not in poll mode: packetbuf should not be accessed in interrupt context.
    // In poll mode, the last packet RSSI and link quality can be obtained through
    // RADIO_PARAM_LAST_RSSI and RADIO_PARAM_LAST_LINK_QUALITY
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, g_lastRssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, g_lastCorrLqi);
  }

  release_data_entry();

  return frame_len;
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
  const bool was_rx = (g_vpCmdRx->status == ACTIVE);
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
  if (g_vpCmdRx->status != ACTIVE) {
    PRINTF("receiving_packet: not in RX\n");
    return 0;
  }

  rfc_CMD_IEEE_CCA_REQ_t RF_cmdIeeeCaaReq;
  memset(&RF_cmdIeeeCaaReq, 0x0, sizeof(rfc_CMD_IEEE_CCA_REQ_t));
  RF_cmdIeeeCaaReq.commandNo = CMD_IEEE_CCA_REQ;

  const RF_Stat stat = RF_runImmediateCmd(g_rfHandle, (uint32_t*)&RF_cmdIeeeCaaReq);
  if (stat != RF_StatCmdDoneSuccess) {
    PRINTF("receiving_packet: CCA request failed stat=0x%02X\n", stat);
    return 0;
  }

  // If the radio is transmitting an ACK or is suspended for running a TX operation,
  // ccaEnergy, ccaCorr and ccaSync are all busy (1)
  if ((RF_cmdIeeeCaaReq.ccaInfo.ccaEnergy == 1) &&
      (RF_cmdIeeeCaaReq.ccaInfo.ccaCorr == 1) &&
      (RF_cmdIeeeCaaReq.ccaInfo.ccaSync == 1)) {
    PRINTF("receiving_packet: we were TXing\n");
    return 0;
  }

  // We are on and not in TX, then we are in RX if a CCA sync has been seen,
  // i.e. ccaSync is busy (1)
  return (RF_cmdIeeeCaaReq.ccaInfo.ccaSync == 1);
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  const rfc_dataEntry_t *const pStartEntry = (rfc_dataEntry_t *)g_rxDataQueue.pCurrEntry;
  volatile const rfc_dataEntry_t *pCurrEntry = pStartEntry;

  // Check all RX buffers and check their statuses, stopping when looping the circular buffer
  int bIsPending = 0;
  do {
    const uint8_t status = pCurrEntry->status;
    if ((status == DATA_ENTRY_FINISHED) ||
        (status == DATA_ENTRY_BUSY)) {
      bIsPending = 1;
      if (!g_bPollMode) {
        process_poll(&RF_coreProcess);
      }
    }

    pCurrEntry = (rfc_dataEntry_t *)pCurrEntry->pNextEntry;
  } while (pCurrEntry != pStartEntry);

  // If we didn't find an entry at status finished or busy, no frames are pending
  return bIsPending;
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
  set_rx(POWER_STATE_OFF);
  RF_yield(g_rfHandle);

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

  // Reset RX buffers if there was an ongoing RX
  size_t i;
  for (i = 0; i < RX_BUF_ENTRIES; ++i) {
    rfc_dataEntry_t *entry = &(g_rxBufs[i].dataEntry);
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
    *value = rf_is_on() ? RADIO_POWER_MODE_ON : RADIO_POWER_MODE_OFF;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_CHANNEL:
    *value = (radio_value_t)g_currChannel;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_PAN_ID:
    *value = (radio_value_t)g_vpCmdRx->localPanID;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_16BIT_ADDR:
    *value = (radio_value_t)g_vpCmdRx->localShortAddr;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RX_MODE:
    *value = 0;
    if (g_vpCmdRx->frameFiltOpt.frameFiltEn) {
      *value |= (radio_value_t)RADIO_RX_MODE_ADDRESS_FILTER;
    }
    if (g_vpCmdRx->frameFiltOpt.autoAckEn) {
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
    return (*value == TX_POWER_UNKNOWN)
      ? RADIO_RESULT_ERROR
      : RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    *value = g_vpCmdRx->ccaRssiThr;
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
    *value = (radio_value_t)(TX_POWER_MIN.dbm);
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MAX:
    *value = (radio_value_t)(TX_POWER_MAX.dbm);
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
  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    switch (value) {
    case RADIO_POWER_MODE_ON:
      if (on() != CMD_RESULT_OK) {
        PRINTF("set_value: on() failed (1)\n");
        return RADIO_RESULT_ERROR;
      }
      return RADIO_RESULT_OK;

    case RADIO_POWER_MODE_OFF:
      off();
      return RADIO_RESULT_OK;

    default:
      return RADIO_RESULT_INVALID_VALUE;
    }

  case RADIO_PARAM_CHANNEL:
    if (!IEEE_MODE_CHAN_IN_RANGE(value)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_channel((uint8_t)value);
    return RADIO_RESULT_OK;

  case RADIO_PARAM_PAN_ID:
    g_vpCmdRx->localPanID = (uint16_t)value;
    if (rf_is_on() && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
      PRINTF("failed to restart RX");
      return RADIO_RESULT_ERROR;
    }
    return RADIO_RESULT_OK;

  case RADIO_PARAM_16BIT_ADDR:
    g_vpCmdRx->localShortAddr = (uint16_t)value;
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

    g_vpCmdRx->frameFiltOpt.frameFiltEn = (value & RADIO_RX_MODE_ADDRESS_FILTER) != 0;
    g_vpCmdRx->frameFiltOpt.frameFiltStop = 1;
    g_vpCmdRx->frameFiltOpt.autoAckEn = (value & RADIO_RX_MODE_AUTOACK) != 0;
    g_vpCmdRx->frameFiltOpt.slottedAckEn = 0;
    g_vpCmdRx->frameFiltOpt.autoPendEn = 0;
    g_vpCmdRx->frameFiltOpt.defaultPend = 0;
    g_vpCmdRx->frameFiltOpt.bPendDataReqOnly = 0;
    g_vpCmdRx->frameFiltOpt.bPanCoord = 0;
    g_vpCmdRx->frameFiltOpt.bStrictLenFilter = 0;

    const bool bOldPollMode = g_bPollMode;
    g_bPollMode = (value & RADIO_RX_MODE_POLL_MODE) != 0;
    if (g_bPollMode == bOldPollMode) {
      // Do not turn the radio off and on, just send an update command
      memcpy(&g_cmdModFilt.newFrameFiltOpt, &RF_cmdIeeeRx.frameFiltOpt, sizeof(RF_cmdIeeeRx.frameFiltOpt));
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
    if(value < TX_POWER_MIN.dbm || value > TX_POWER_MAX.dbm) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    return (set_tx_power(value) != CMD_RESULT_OK)
      ? RADIO_RESULT_ERROR
      : RADIO_RESULT_OK;

  case RADIO_PARAM_CCA_THRESHOLD:
    g_vpCmdRx->ccaRssiThr = (int8_t)value;
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
    const size_t srcSize = sizeof(g_vpCmdRx->localExtAddr);
    if(size != srcSize) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    const uint8_t *pSrc = (const uint8_t *)&g_vpCmdRx->localExtAddr;
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
    const size_t destSize = sizeof(g_vpCmdRx->localExtAddr);
    if (size != destSize) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    const uint8_t *pSrc = (const uint8_t *)src;
    uint8_t *pDest = (uint8_t *)&g_vpCmdRx->localExtAddr;
    for (size_t i = 0; i < destSize; ++i) {
      pDest[i] = pSrc[destSize - 1 - i];
    }

    const bool is_rx = (g_vpCmdRx->status == ACTIVE);
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
const struct radio_driver ieee_mode_driver = {
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
 * @}
 */
