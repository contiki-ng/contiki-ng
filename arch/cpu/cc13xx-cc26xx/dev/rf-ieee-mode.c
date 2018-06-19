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
/* rf_ieee_cmd and rf_ieee_mailbox included by RF settings because of the
 * discrepancy between CC13x0 and CC13x2 IEEE support. CC13x0 doesn't provide
 * RFCore definitions of IEEE commandos, and are therefore included locally
 * from the Contiki build system. CC13x2 includes these normally from driverlib.
 * This is taken care of RF settings. */

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
#define TX_POWER_MIN  (TX_POWER_TABLE[0].power)
#define TX_POWER_MAX  (TX_POWER_TABLE[TX_POWER_TABLE_SIZE - 1].power)

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

/* How long to wait for the rx read entry to become ready */
#define TIMEOUT_DATA_ENTRY_BUSY (RTIMER_SECOND / 250)
/*---------------------------------------------------------------------------*/
#define RAT_RANGE             (~(uint32_t)0)
#define RAT_ONE_QUARTER       (RAT_RANGE / (uint32_t)4)
#define RAT_THREE_QUARTERS    ((RAT_RANGE * (uint32_t)3) / (uint32_t)4)

/* XXX: don't know what exactly is this, looks like the time to TX 3 octets */
#define RAT_TIMESTAMP_OFFSET  -(USEC_TO_RADIO(32 * 3) - 1) /* -95.75 usec */
/*---------------------------------------------------------------------------*/
#define STATUS_CORRELATION   0x3f  /* bits 0-5 */
#define STATUS_REJECT_FRAME  0x40  /* bit 6 */
#define STATUS_CRC_FAIL      0x80  /* bit 7 */
/*---------------------------------------------------------------------------*/
/* TX buf configuration */
#define TX_BUF_HDR_LEN      2
#define TX_BUF_PAYLOAD_LEN  180

#define TX_BUF_SIZE         (TX_BUF_HDR_LEN + TX_BUF_PAYLOAD_LEN)

/* RX buf configuration */
#ifdef IEEE_MODE_CONF_RX_BUF_CNT
# define RX_BUF_CNT             IEEE_MODE_CONF_RX_BUF_CNT
#else
# define RX_BUF_CNT             4
#endif

#define RX_BUF_SIZE     144
/*---------------------------------------------------------------------------*/
/* Size of the Length representation in Data Entry, one byte in this case */
typedef uint8_t               lensz_t;

#define FRAME_OFFSET          sizeof(lensz_t)
#define FRAME_SHAVE           8   /* FCS (2) + RSSI (1) + Status (1) + Timestamp (4) */
/*---------------------------------------------------------------------------*/
/* Used for checking result of CCA_REQ command */
typedef enum {
  CCA_STATE_IDLE    = 0,
  CCA_STATE_BUSY    = 1,
  CCA_STATE_INVALID = 2
} cca_state_t;
/*---------------------------------------------------------------------------*/
typedef enum {
    POWER_STATE_ON      = (1 << 0),
    POWER_STATE_OFF     = (1 << 1),
    POWER_STATE_RESTART = POWER_STATE_ON | POWER_STATE_OFF,
} PowerState;
/*---------------------------------------------------------------------------*/
/* RF Core typedefs */
typedef dataQueue_t             data_queue_t;
typedef rfc_dataEntryGeneral_t  data_entry_t;
typedef rfc_ieeeRxOutput_t      rx_output_t;
typedef rfc_CMD_IEEE_MOD_FILT_t cmd_mod_filt_t;
typedef rfc_CMD_IEEE_CCA_REQ_t  cmd_cca_req_t;

/* Receive buffer entries with room for 1 IEEE 802.15.4 frame in each */
typedef union {
  data_entry_t data_entry;
  uint8_t      buf[RX_BUF_SIZE];
} rx_buf_t CC_ALIGN(4);

typedef struct {
  /* Outgoing frame buffer */
  uint8_t             tx_buf[TX_BUF_SIZE] CC_ALIGN(4);
  /* Ingoing frame buffers */
  rx_buf_t            rx_bufs[RX_BUF_CNT];

  /* RX Data Queue */
  data_queue_t        rx_data_queue;
  /* RF Statistics struct */
  rx_output_t         rx_stats;
  /* Receive entry pointer to keep track of read items */
  data_entry_t*       rx_read_entry;

  /* Indicates RF is supposed to be on or off */
  bool                rf_is_on;
  /* Enable/disable CCA before sending */
  bool                send_on_cca;
  /* Are we currently in poll mode? */
  bool                poll_mode;

  /* Last RX operation stats */
  struct {
    int8_t            rssi;
    uint8_t           corr_lqi;
    uint32_t          timestamp;
  } last;

  /* RAT Overflow Upkeep */
  struct {
    struct ctimer     overflow_timer;
    rtimer_clock_t    last_overflow;
    volatile uint32_t overflow_count;
  } rat;

  /* RF driver */
  RF_Object           rf_object;
  RF_Handle           rf_handle;
} ieee_radio_t;

static ieee_radio_t ieee_radio;

/* Global RF Core commands */
static cmd_mod_filt_t cmd_mod_filt;
/*---------------------------------------------------------------------------*/
/* RF Command volatile objects */
#define cmd_radio_setup  (*(volatile rfc_CMD_RADIO_SETUP_t*)&rf_cmd_ieee_radio_setup)
#define cmd_fs           (*(volatile rfc_CMD_FS_t*)         &rf_cmd_ieee_fs)
#define cmd_tx           (*(volatile rfc_CMD_IEEE_TX_t*)    &rf_cmd_ieee_tx)
#define cmd_rx           (*(volatile rfc_CMD_IEEE_RX_t*)    &rf_cmd_ieee_rx)
/*---------------------------------------------------------------------------*/
static CC_INLINE bool tx_active(void) { return cmd_tx.status == ACTIVE; }
static CC_INLINE bool rx_active(void) { return cmd_rx.status == ACTIVE; }
/*---------------------------------------------------------------------------*/
/* Forward declarations of static functions */
static int set_rx(const PowerState);
static void check_rat_overflow(void);
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
rx_cb(RF_Handle client, RF_CmdHandle command, RF_EventMask events)
{
  /* Unused arguments */
  (void)client;
  (void)command;

  if (events & RF_EventRxOk) {
    process_poll(&rf_process);
  }
}
/*---------------------------------------------------------------------------*/
static void
rat_overflow_cb(void *arg)
{
  check_rat_overflow();
  /* Check next time after half of the RAT interval */
  const clock_time_t two_quarters = (2 * RAT_ONE_QUARTER * CLOCK_SECOND) / RAT_SECOND;
  ctimer_set(&ieee_radio.rat.overflow_timer, two_quarters, rat_overflow_cb, NULL);
}
/*---------------------------------------------------------------------------*/
static void
init_data_queue(void)
{
  /* Initialize RF core data queue, circular buffer */
  ieee_radio.rx_data_queue.pCurrEntry = ieee_radio.rx_bufs[0].buf;
  ieee_radio.rx_data_queue.pLastEntry = NULL;
  /* Set current read pointer to first element */
  ieee_radio.rx_read_entry = &ieee_radio.rx_bufs[0].data_entry;
}
/*---------------------------------------------------------------------------*/
static void
init_rf_params(void)
{
    cmd_rx.channel = IEEE_MODE_CHANNEL;

    cmd_rx.pRxQ    = &ieee_radio.rx_data_queue;
    cmd_rx.pOutput = &ieee_radio.rx_stats;

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

    /* Initialize address filter command */
    cmd_mod_filt.commandNo = CMD_IEEE_MOD_FILT;
    memcpy(&(cmd_mod_filt.newFrameFiltOpt), &(rf_cmd_ieee_rx.frameFiltOpt), sizeof(rf_cmd_ieee_rx.frameFiltOpt));
    memcpy(&(cmd_mod_filt.newFrameTypes),   &(rf_cmd_ieee_rx.frameTypes),   sizeof(rf_cmd_ieee_rx.frameTypes));
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
      .config.lenSz = sizeof(lensz_t),
      .length       = RX_BUF_SIZE - sizeof(data_entry_t), /* TODO: is this sizeof sound? */
      /* Point to fist entry if this is last entry, else point to next entry */
      .pNextEntry   = (i == (RX_BUF_CNT - 1))
        ? ieee_radio.rx_bufs[0].buf
        : ieee_radio.rx_bufs[i].buf
    };
    ieee_radio.rx_bufs[i].data_entry = data_entry;
  }
}
/*---------------------------------------------------------------------------*/
static cmd_result_t
set_channel(uint32_t channel)
{
  if (!IEEE_MODE_CHAN_IN_RANGE(channel)) {
    PRINTF("set_channel: illegal channel %d, defaults to %d\n",
           (int)channel, IEEE_MODE_CHANNEL);
    channel = IEEE_MODE_CHANNEL;
  }
  if (channel == cmd_rx.channel) {
    /* We are already calibrated to this channel */
      return true;
  }

  cmd_rx.channel = 0;

  /* freq = freq_base + freq_spacing * (channel - channel_min) */
  const uint32_t new_freq = IEEE_MODE_FREQ_BASE + IEEE_MODE_FREQ_SPACING * (channel - IEEE_MODE_CHAN_MIN);
  const uint32_t freq = new_freq / 1000;
  const uint32_t frac = ((new_freq - (freq * 1000)) * 65536) / 1000;

  PRINTF("set_channel: %d = 0x%04X.0x%04X (%lu)\n",
         (int)channel, (uint16_t)freq, (uint16_t)frac, new_freq);

  cmd_fs.frequency = (uint16_t)freq;
  cmd_fs.fractFreq = (uint16_t)frac;

  const bool was_on = rx_active();

  if (was_on) {
    RF_flushCmd(ieee_radio.rf_handle, RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);
  }

  /* Start FS command asynchronously. We don't care when it is finished */
  RF_EventMask events = 0;
  uint8_t tries = 0;
  bool cmd_ok = false;
  do {
    events = RF_runCmd(ieee_radio.rf_handle, (RF_Op*)&cmd_fs, RF_PriorityNormal, NULL, 0);
    cmd_ok = ((events & RF_EventLastCmdDone) != 0)
          && (cmd_fs.status == DONE_OK);
  } while (!cmd_ok && (tries++ < 3));

  if (!cmd_ok) {
    return false;
  }

  cmd_rx.channel = channel;

  if (was_on) {
      set_rx(POWER_STATE_ON);
  }

  return true;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_tx_power(const radio_value_t dBm)
{
  if (!TX_POWER_IN_RANGE(dBm)) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  const RF_TxPowerTable_Value tx_power_table_value = RF_TxPowerTable_findValue(TX_POWER_TABLE, (int8_t)dBm);
  const RF_Stat stat = RF_setTxPower(ieee_radio.rf_handle, tx_power_table_value);

  return (stat == RF_StatSuccess)
    ? RADIO_RESULT_OK
    : RADIO_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
static radio_value_t
get_tx_power(void)
{
  const RF_TxPowerTable_Value value = RF_getTxPower(ieee_radio.rf_handle);
  return (radio_value_t)RF_TxPowerTable_findPowerLevel(TX_POWER_TABLE, value);
}
/*---------------------------------------------------------------------------*/
static void
set_send_on_cca(bool enable)
{
  ieee_radio.send_on_cca = enable;
}
/*---------------------------------------------------------------------------*/
static void
check_rat_overflow(void)
{
  const bool was_off = !rx_active();

  if (was_off) {
    RF_runDirectCmd(ieee_radio.rf_handle, CMD_NOP);
  }

  const uint32_t current_value = RF_getCurrentTime();

  static bool initial_iteration = true;
  static uint32_t last_value;

  if (initial_iteration) {
    /* First time checking overflow will only store the current value */
    initial_iteration = false;
  } else {
    /* Overflow happens in the last quarter of the RAT range */
    if ((current_value + RAT_ONE_QUARTER) < last_value) {
      /* Overflow detected */
      ieee_radio.rat.last_overflow = RTIMER_NOW();
      ieee_radio.rat.overflow_count += 1;
    }
  }

  last_value = current_value;

  if (was_off) {
    RF_yield(ieee_radio.rf_handle);
  }
}
/*---------------------------------------------------------------------------*/
static uint32_t
rat_to_timestamp(const uint32_t rat_ticks)
{
  check_rat_overflow();

  uint64_t adjusted_overflow_count = ieee_radio.rat.overflow_count;

  /* If the timestamp is in the 4th quarter and the last overflow was recently,
   * assume that the timestamp refers to the time before the overflow */
  if (rat_ticks > RAT_THREE_QUARTERS) {
    const rtimer_clock_t one_quarter = (RAT_ONE_QUARTER * RTIMER_SECOND) / RAT_SECOND;
    if (RTIMER_CLOCK_LT(RTIMER_NOW(), ieee_radio.rat.last_overflow + one_quarter)) {
      adjusted_overflow_count -= 1;
    }
  }

  /* Add the overflowed time to the timestamp */
  const uint64_t rat_ticks_adjusted = (uint64_t)rat_ticks + (uint64_t)RAT_RANGE * adjusted_overflow_count;

  /* Correct timestamp so that it refers to the end of the SFD and convert to RTIMER */
  return RAT_TO_RTIMER(rat_ticks_adjusted + RAT_TIMESTAMP_OFFSET);
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  if (ieee_radio.rf_handle) {
    PRINTF("init: Radio already initialized\n");
    return CMD_RESULT_OK;
  }

  /* RX is off */
  ieee_radio.rf_is_on = false;

  init_rf_params();
  init_data_queue();

  /* Init RF params and specify non-default params */
  RF_Params rf_params;
  RF_Params_init(&rf_params);
  rf_params.nInactivityTimeout = 2000; /* 2 ms */

  ieee_radio.rf_handle = RF_open(&ieee_radio.rf_object, &rf_ieee_mode,
                                 (RF_RadioSetup*)&cmd_radio_setup, &rf_params);
  if (ieee_radio.rf_handle == NULL) {
    PRINTF("init: unable to open RF driver\n");
    return CMD_RESULT_ERROR;
  }

  set_channel(IEEE_MODE_CHANNEL);

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  /* Start RAT overflow upkeep */
  check_rat_overflow();
  clock_time_t two_quarters = (2 * RAT_ONE_QUARTER * CLOCK_SECOND) / RAT_SECOND;
  ctimer_set(&ieee_radio.rat.overflow_timer, two_quarters, rat_overflow_cb, NULL);

  /* Start RF process */
  process_start(&rf_process, NULL);

  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  const size_t len = MIN((size_t)payload_len,
                         (size_t)TX_BUF_PAYLOAD_LEN);
  memcpy(&ieee_radio.tx_buf[TX_BUF_HDR_LEN], payload, len);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
set_rx(const PowerState state)
{
  if (state & POWER_STATE_OFF) {
    /* Stop RX gracefully, don't care about the result */
    RF_cancelCmd(ieee_radio.rf_handle, RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);
  }

  if (state & POWER_STATE_ON) {
    if (cmd_rx.status == ACTIVE) {
      PRINTF("set_rx(on): already in RX\n");
      return CMD_RESULT_OK;
    }

    RF_ScheduleCmdParams sched_params;
    RF_ScheduleCmdParams_init(&sched_params);

    cmd_rx.status = IDLE;
    RF_CmdHandle rx_handle = RF_scheduleCmd(ieee_radio.rf_handle, (RF_Op*)&cmd_rx, &sched_params, rx_cb,
                                            RF_EventRxOk | RF_EventRxBufFull | RF_EventRxEntryDone);
    if ((rx_handle == RF_ALLOC_ERROR) || (rx_handle == RF_SCHEDULE_CMD_ERROR)) {
      PRINTF("transmit: unable to schedule RX command cmd_handle=%d\n", rx_handle);
      return CMD_RESULT_ERROR;
    }
  }

  return CMD_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
transmit_aux(unsigned short transmit_len)
{
  /* Configure TX command */
  cmd_tx.payloadLen = (uint8_t)transmit_len;
  cmd_tx.pPayload = &ieee_radio.tx_buf[TX_BUF_HDR_LEN];

  RF_ScheduleCmdParams sched_params;
  RF_ScheduleCmdParams_init(&sched_params);

  /* As IEEE_TX is a FG command, the TX operation will be executed
   * either way if RX is running or not */
  cmd_tx.status = IDLE;
  RF_CmdHandle tx_handle = RF_scheduleCmd(ieee_radio.rf_handle, (RF_Op*)&cmd_tx, &sched_params,
                                          NULL, 0);
  if ((tx_handle == RF_ALLOC_ERROR) || (tx_handle == RF_SCHEDULE_CMD_ERROR)) {
      /* Failure sending the CMD_IEEE_TX command */
      PRINTF("transmit: unable to schedule TX command cmd_handle=%d, status=%04x\n",
             tx_handle, cmd_tx.status);
      return RADIO_TX_ERR;
  }

  ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);

  /* Wait until TX operation finishes */
  RF_EventMask tx_events = RF_pendCmd(ieee_radio.rf_handle, tx_handle, 0);
  if ((tx_events & (RF_EventFGCmdDone | RF_EventLastFGCmdDone)) == 0) {
    PRINTF("transmit: TX command pend error events=0x%08llx, status=0x%04x\n",
           tx_events, cmd_tx.status);
    return RADIO_TX_ERR;
  }

  return RADIO_TX_OK;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  if (ieee_radio.send_on_cca && channel_clear() != 1) {
    PRINTF("transmit: channel wasn't clear\n");
    return RADIO_TX_COLLISION;
  }

  const int rv = transmit_aux(transmit_len);
  ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);

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
  data_entry_t *const data_entry = ieee_radio.rx_read_entry;
  uint8_t *const frame_ptr = (uint8_t*)&data_entry->data;

  /* Clear the length byte and set status to 0: "Pending" */
  *(lensz_t*)frame_ptr = 0;
  data_entry->status = DATA_ENTRY_PENDING;
  ieee_radio.rx_read_entry = (data_entry_t*)data_entry->pNextEntry;
}
/*---------------------------------------------------------------------------*/
static int
read(void *buf, unsigned short buf_len)
{
  volatile data_entry_t *data_entry = ieee_radio.rx_read_entry;

  const rtimer_clock_t t0 = RTIMER_NOW();
  /* Only wait if the Radio timer is accessing the entry */
  while ((data_entry->status == DATA_ENTRY_BUSY) &&
          RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + TIMEOUT_DATA_ENTRY_BUSY));

  if (data_entry->status != DATA_ENTRY_FINISHED) {
    /* No available data */
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
  ieee_radio.last.rssi = (int8_t)payload_ptr[payload_len + 2];
  /* LQI retrieved from Status byte, FCS (2) + RSSI (1) bytes after payload */
  ieee_radio.last.corr_lqi = ((uint8_t)payload_ptr[payload_len + 3]) & STATUS_CORRELATION;
  /* Timestamp stored FCS (2) + RSSI (1) + Status (1) bytes after payload */
  const uint32_t rat_ticks = *(uint32_t*)(payload_ptr + payload_len + 4);
  ieee_radio.last.timestamp = rat_to_timestamp(rat_ticks);

  if (!ieee_radio.poll_mode) {
    /* Not in poll mode: packetbuf should not be accessed in interrupt context. */
    /* In poll mode, the last packet RSSI and link quality can be obtained through */
    /* RADIO_PARAM_LAST_RSSI and RADIO_PARAM_LAST_LINK_QUALITY */
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI,         (packetbuf_attr_t)ieee_radio.last.rssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, (packetbuf_attr_t)ieee_radio.last.corr_lqi);
  }

  release_data_entry();
  return (int)payload_len;
}
/*---------------------------------------------------------------------------*/
static cmd_result_t
cca_request(cmd_cca_req_t *cmd_cca_req)
{
  const bool rx_is_off = !rx_active();

  if (rx_is_off && set_rx(POWER_STATE_ON) != CMD_RESULT_OK) {
    return CMD_RESULT_ERROR;
  }

  const RF_Stat stat = RF_runImmediateCmd(ieee_radio.rf_handle, (uint32_t*)&cmd_cca_req);

  if (rx_is_off) {
    set_rx(POWER_STATE_OFF);
  }

  return (stat == RF_StatCmdDoneSuccess)
    ? CMD_RESULT_OK
    : CMD_RESULT_ERROR;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  cmd_cca_req_t cmd_cca_req;
  memset(&cmd_cca_req, 0x0, sizeof(cmd_cca_req_t));
  cmd_cca_req.commandNo = CMD_IEEE_CCA_REQ;

  if (cca_request(&cmd_cca_req) != CMD_RESULT_OK) {
    return 0;
  }

  /* Channel is clear if CCA state is IDLE */
  return (cmd_cca_req.ccaInfo.ccaState == CCA_STATE_IDLE);
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  cmd_cca_req_t cmd_cca_req;
  memset(&cmd_cca_req, 0x0, sizeof(cmd_cca_req_t));
  cmd_cca_req.commandNo = CMD_IEEE_CCA_REQ;

  if (cca_request(&cmd_cca_req) != CMD_RESULT_OK) {
    return 0;
  }

  /* If we are transmitting (can only be an ACK here), we are not receiving */
  if ((cmd_cca_req.ccaInfo.ccaEnergy == CCA_STATE_BUSY) &&
      (cmd_cca_req.ccaInfo.ccaCorr   == CCA_STATE_BUSY) &&
      (cmd_cca_req.ccaInfo.ccaSync   == CCA_STATE_BUSY)) {
    PRINTF("receiving_packet: we were TXing ACK\n");
    return 0;
  }

  /* We are receiving a packet if a CCA sync has been seen, i.e. ccaSync is busy (1) */
  return (cmd_cca_req.ccaInfo.ccaSync == CCA_STATE_BUSY);
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  const data_entry_t *const read_entry = ieee_radio.rx_read_entry;
  volatile const data_entry_t *curr_entry = read_entry;

  int num_pending = 0;

  /* Go through RX Circular buffer and check their status */
  do {
    const uint8_t status = curr_entry->status;
    if ((status == DATA_ENTRY_FINISHED) ||
        (status == DATA_ENTRY_BUSY)) {
      num_pending += 1;
    }

    /* Stop when we have looped the circular buffer */
    curr_entry = (data_entry_t *)curr_entry->pNextEntry;
  } while (curr_entry != read_entry);

  if ((num_pending > 0) && !ieee_radio.poll_mode) {
    process_poll(&rf_process);
  }

  /* If we didn't find an entry at status finished or busy, no frames are pending */
  return num_pending;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  if (ieee_radio.rf_is_on) {
    PRINTF("on: Radio already on\n");
    return CMD_RESULT_OK;
  }

  /* Reset all RF command statuses */
  cmd_fs.status = IDLE;
  cmd_tx.status = IDLE;
  cmd_rx.status = IDLE;

  init_rx_buffers();

  const int rx_ok = set_rx(POWER_STATE_ON);
  if (!rx_ok) {
    off();
    return CMD_RESULT_ERROR;
  }

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  ieee_radio.rf_is_on = true;
  return rx_ok;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  if (!ieee_radio.rf_is_on) {
    PRINTF("off: Radio already off\n");
    return CMD_RESULT_OK;
  }

  /* Force abort of any ongoing RF operation */
  RF_flushCmd(ieee_radio.rf_handle, RF_CMDHANDLE_FLUSH_ALL, RF_ABORT_GRACEFULLY);
  /* Trigger a manual power-down */
  RF_yield(ieee_radio.rf_handle);

  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);

  /* Reset RX buffers if there was an ongoing RX */
  size_t i;
  for (i = 0; i < RX_BUF_CNT; ++i) {
    data_entry_t *entry = &ieee_radio.rx_bufs[i].data_entry;
    if (entry->status == DATA_ENTRY_BUSY) {
      entry->status = DATA_ENTRY_PENDING;
    }
  }

  ieee_radio.rf_is_on = false;
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
    *value = (ieee_radio.rf_is_on)
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
    if (ieee_radio.poll_mode) {
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
    *value = RF_getRssi(ieee_radio.rf_handle);
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
    *value = (radio_value_t)TX_POWER_MIN;
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MAX:
    *value = (radio_value_t)TX_POWER_MAX;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_LAST_RSSI:
    *value = (radio_value_t)ieee_radio.last.rssi;
    return RADIO_RESULT_OK;

  case RADIO_PARAM_LAST_LINK_QUALITY:
    *value = (radio_value_t)ieee_radio.last.corr_lqi;
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
    if (ieee_radio.rf_is_on && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
      PRINTF("failed to restart RX");
      return RADIO_RESULT_ERROR;
    }
    return RADIO_RESULT_OK;

  case RADIO_PARAM_16BIT_ADDR:
    cmd_rx.localShortAddr = (uint16_t)value;
    if (ieee_radio.rf_is_on && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
      PRINTF("failed to restart RX");
      return RADIO_RESULT_ERROR;
    }
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RX_MODE: {
    if (value & ~(RADIO_RX_MODE_ADDRESS_FILTER |
                  RADIO_RX_MODE_AUTOACK | RADIO_RX_MODE_POLL_MODE)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    cmd_rx.frameFiltOpt.frameFiltEn      = (value & RADIO_RX_MODE_ADDRESS_FILTER) != 0;
    cmd_rx.frameFiltOpt.frameFiltStop    = 1;
    cmd_rx.frameFiltOpt.autoAckEn        = (value & RADIO_RX_MODE_AUTOACK) != 0;
    cmd_rx.frameFiltOpt.slottedAckEn     = 0;
    cmd_rx.frameFiltOpt.autoPendEn       = 0;
    cmd_rx.frameFiltOpt.defaultPend      = 0;
    cmd_rx.frameFiltOpt.bPendDataReqOnly = 0;
    cmd_rx.frameFiltOpt.bPanCoord        = 0;
    cmd_rx.frameFiltOpt.bStrictLenFilter = 0;

    const bool old_poll_mode = ieee_radio.poll_mode;
    ieee_radio.poll_mode = (value & RADIO_RX_MODE_POLL_MODE) != 0;
    if (old_poll_mode == ieee_radio.poll_mode) {
      /* Do not turn the radio off and on, just send an update command */
      memcpy(&cmd_mod_filt.newFrameFiltOpt, &(rf_cmd_ieee_rx.frameFiltOpt), sizeof(rf_cmd_ieee_rx.frameFiltOpt));
      const RF_Stat stat = RF_runImmediateCmd(ieee_radio.rf_handle, (uint32_t*)&cmd_mod_filt);
      if (stat != RF_StatCmdDoneSuccess) {
        PRINTF("setting address filter failed: stat=0x%02X\n", stat);
        return RADIO_RESULT_ERROR;
      }
      return RADIO_RESULT_OK;
    }
    if (ieee_radio.rf_is_on && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
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
    return set_tx_power(value);

  case RADIO_PARAM_CCA_THRESHOLD:
    cmd_rx.ccaRssiThr = (int8_t)value;
    if (ieee_radio.rf_is_on && set_rx(POWER_STATE_RESTART) != CMD_RESULT_OK) {
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

    *(rtimer_clock_t *)dest = ieee_radio.last.timestamp;

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
