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
 * \defgroup cc13xx-cc26xx-rf-ieee IEEE-mode driver for CC13xx/CC26xx
 *
 * @{
 *
 * \file
 *        Implementation of the CC13xx/CC26xx IEEE-mode NETSTACK_RADIO driver.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
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
/*
 * rf_ieee_cmd.h and rf_ieee_mailbox.h are included by RF settings because a
 * discrepancy between CC13x0 and CC13x2 IEEE support. CC13x0 doesn't provide
 * RFCore definitions of IEEE commands, and are therefore included locally
 * from the Contiki build system. CC13x2 includes these normally from driverlib.
 */
#if defined(DeviceFamily_CC13X0)
#include "driverlib/rf_ieee_mailbox.h"
#else
#include DeviceFamily_constructPath(driverlib/rf_ieee_mailbox.h)
#endif

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
/* SimpleLink Platform RF dev */
#include "rf/rf.h"
#include "rf/data-queue.h"
#include "rf/dot-15-4g.h"
#include "rf/sched.h"
#include "rf/settings.h"
#include "rf/tx-power.h"
#include "rf/rat.h"
#include "rf/radio-mode.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Radio"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
/* Configuration parameters */
#define IEEE_MODE_AUTOACK             IEEE_MODE_CONF_AUTOACK
#define IEEE_MODE_PROMISCOUS          IEEE_MODE_CONF_PROMISCOUS
#define IEEE_MODE_CCA_RSSI_THRESHOLD  IEEE_MODE_CONF_CCA_RSSI_THRESHOLD
/*---------------------------------------------------------------------------*/
/* Timeout constants */

/* How long to wait for the rx read entry to become ready */
#define TIMEOUT_DATA_ENTRY_BUSY (RTIMER_SECOND / 200)

/* How long to wait for RX to become active after scheduled */
#define TIMEOUT_ENTER_RX_WAIT   (RTIMER_SECOND >> 10)
/*---------------------------------------------------------------------------*/
/* XXX: don't know what exactly is this */
#define RAT_TIMESTAMP_OFFSET USEC_TO_RAT(-50)
/*---------------------------------------------------------------------------*/
#define STATUS_CORRELATION   0x3f  /* bits 0-5 */
#define STATUS_REJECT_FRAME  0x40  /* bit 6 */
#define STATUS_CRC_FAIL      0x80  /* bit 7 */
/*---------------------------------------------------------------------------*/
/*
 * The number of bytes appended at the end of an outgoing frame as a footer
 * Currently fixed at 2 bytes for IEEE 802.15.4 compliance.
 */
#define CHECKSUM_LEN 2

/*
 * The maximum number of bytes this driver can accept from the MAC layer for
 * transmission or will deliver to the MAC layer after reception. Includes
 * the MAC header and payload, but not the FCS.
 */
#define MAX_PAYLOAD_LEN (127 - CHECKSUM_LEN)
/*---------------------------------------------------------------------------*/
#define FRAME_FCF_OFFSET     0
#define FRAME_SEQNUM_OFFSET  2

#define FRAME_ACK_REQUEST    0x20  /* bit 5 */

/* TX buf configuration */
#define TX_BUF_SIZE         180
/*---------------------------------------------------------------------------*/
/* Size of the Length representation in Data Entry, one byte in this case */
typedef uint8_t lensz_t;

#define FRAME_OFFSET          sizeof(lensz_t)
#define FRAME_SHAVE           8   /* FCS (2) + RSSI (1) + Status (1) + Timestamp (4) */
/*---------------------------------------------------------------------------*/
/* Used for checking result of CCA_REQ command */
typedef enum {
  CCA_STATE_IDLE = 0,
  CCA_STATE_BUSY = 1,
  CCA_STATE_INVALID = 2
} cca_state_t;
/*---------------------------------------------------------------------------*/
/* RF Core typedefs */
typedef rfc_ieeeRxOutput_t rx_output_t;
typedef rfc_CMD_IEEE_MOD_FILT_t cmd_mod_filt_t;
typedef rfc_CMD_IEEE_CCA_REQ_t cmd_cca_req_t;

typedef struct {
  /* RF driver */
  RF_Handle rf_handle;

  /* Are we currently in poll mode? */
  bool poll_mode;

  /* RAT Overflow Upkeep */
  struct {
    struct ctimer overflow_timer;
    rtimer_clock_t last_overflow;
    volatile uint32_t overflow_count;
  } rat;

  bool (* rx_is_active)(void);

  /* Outgoing frame buffer */
  uint8_t tx_buf[TX_BUF_SIZE] CC_ALIGN(4);

  /* RF Statistics struct */
  rx_output_t rx_stats;

  /* Indicates RF is supposed to be on or off */
  bool rf_is_on;
  /* Enable/disable CCA before sending */
  bool send_on_cca;

  /* Last RX operation stats */
  struct {
    int8_t rssi;
    uint8_t corr_lqi;
    uint32_t timestamp;
  } last;
} ieee_radio_t;

static ieee_radio_t ieee_radio;

/* Global RF Core commands */
static cmd_mod_filt_t cmd_mod_filt;
/*---------------------------------------------------------------------------*/
/* Convenience macros for more succinct access of RF commands */
#define cmd_radio_setup     rf_cmd_ieee_radio_setup
#define cmd_fs              rf_cmd_ieee_fs
#define cmd_tx              rf_cmd_ieee_tx
#define cmd_rx              rf_cmd_ieee_rx
#define cmd_rx_ack          rf_cmd_ieee_rx_ack

/* Convenience macros for volatile access with the RF commands */
#define v_cmd_radio_setup   CC_ACCESS_NOW(rfc_CMD_RADIO_SETUP_t, rf_cmd_ieee_radio_setup)
#define v_cmd_fs            CC_ACCESS_NOW(rfc_CMD_FS_t,          rf_cmd_ieee_fs)
#define v_cmd_tx            CC_ACCESS_NOW(rfc_CMD_IEEE_TX_t,     rf_cmd_ieee_tx)
#define v_cmd_rx            CC_ACCESS_NOW(rfc_CMD_IEEE_RX_t,     rf_cmd_ieee_rx)
#define v_cmd_rx_ack        CC_ACCESS_NOW(rfc_CMD_IEEE_RX_ACK_t, rf_cmd_ieee_rx_ack)
/*---------------------------------------------------------------------------*/
static inline bool
rx_is_active(void)
{
  return v_cmd_rx.status == ACTIVE;
}
/*---------------------------------------------------------------------------*/
/* Forward declarations of Radio driver functions */
static int init(void);
static int prepare(const void *, unsigned short);
static int transmit(unsigned short);
static int send(const void *, unsigned short);
static int read(void *, unsigned short);
static int channel_clear(void);
static int receiving_packet(void);
static int pending_packet(void);
static int on(void);
static int off(void);
static radio_result_t get_value(radio_param_t, radio_value_t *);
static radio_result_t set_value(radio_param_t, radio_value_t);
static radio_result_t get_object(radio_param_t, void *, size_t);
static radio_result_t set_object(radio_param_t, const void *, size_t);
/*---------------------------------------------------------------------------*/
static void
init_rf_params(void)
{
  cmd_radio_setup.config.frontEndMode = RF_2_4_GHZ_FRONT_END_MODE;
  cmd_radio_setup.config.biasMode = RF_2_4_GHZ_BIAS_MODE;

  cmd_rx.pRxQ = data_queue_init(sizeof(lensz_t));
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

  cmd_rx.ccaRssiThr = IEEE_MODE_CCA_RSSI_THRESHOLD;

  cmd_tx.pNextOp = (RF_Op *)&cmd_rx_ack;
  cmd_tx.condition.rule = COND_NEVER; /* Initially ACK turned off */

  /*
   * ACK packet is transmitted 192 us after the end of the received packet,
   * takes 352 us for ACK transmission, total of 546 us of expected time to
   * recieve ACK in ideal conditions. 700 us endTime for CMD_IEEE_RX_ACK
   * should give some margins.
   * The ACK frame consists of 6 bytes of SHR/PDR and 5 bytes of PSDU, total
   * of 11 bytes. 11 bytes x 32 us/byte equals 352 us of ACK transmission time.
   */
  cmd_rx_ack.startTrigger.triggerType = TRIG_NOW;
  cmd_rx_ack.endTrigger.triggerType = TRIG_REL_START;
  cmd_rx_ack.endTime = RF_convertUsToRatTicks(700);

  /* Initialize address filter command */
  cmd_mod_filt.commandNo = CMD_IEEE_MOD_FILT;
  memcpy(&(cmd_mod_filt.newFrameFiltOpt), &(cmd_rx.frameFiltOpt), sizeof(cmd_rx.frameFiltOpt));
  memcpy(&(cmd_mod_filt.newFrameTypes), &(cmd_rx.frameTypes), sizeof(cmd_rx.frameTypes));
}
/*---------------------------------------------------------------------------*/
static rf_result_t
set_channel(uint8_t channel)
{
  if(!dot_15_4g_chan_in_range(channel)) {
    LOG_WARN("Supplied hannel %d is illegal, defaults to %d\n",
             (int)channel, DOT_15_4G_DEFAULT_CHAN);
    channel = DOT_15_4G_DEFAULT_CHAN;
  }

  /*
   * cmd_rx.channel is initialized to 0, causing any initial call to
   * set_channel() to cause a synth calibration, since channel must be in
   * range 11-26.
   */
  if(channel == v_cmd_rx.channel) {
    /* We are already calibrated to this channel */
    return true;
  }

  if(ieee_radio.rf_is_on) {
    /* Force RAT and RTC resync */
    rf_restart_rat();
  }

  v_cmd_rx.channel = channel;

  const uint32_t new_freq = dot_15_4g_freq(channel);
  const uint16_t freq = (uint16_t)(new_freq / 1000);
  const uint16_t frac = (uint16_t)(((new_freq - (freq * 1000)) * 0x10000) / 1000);

  LOG_DBG("Set channel to %d, frequency 0x%04X.0x%04X (%lu)\n",
          (int)channel, freq, frac, new_freq);

  v_cmd_fs.frequency = freq;
  v_cmd_fs.fractFreq = frac;

  return netstack_sched_fs();
}
/*---------------------------------------------------------------------------*/
static void
set_send_on_cca(bool enable)
{
  ieee_radio.send_on_cca = enable;
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  RF_Params rf_params;
  RF_TxPowerTable_Value tx_power_value;
  RF_Stat rf_stat;

  ieee_radio.rx_is_active = rx_is_active;
  radio_mode = (simplelink_radio_mode_t *)&ieee_radio;

  if(ieee_radio.rf_handle) {
    LOG_WARN("Radio already initialized\n");
    return RF_RESULT_OK;
  }

  /* RX is off */
  ieee_radio.rf_is_on = false;

  init_rf_params();

  /* Init RF params and specify non-default params */
  RF_Params_init(&rf_params);
  rf_params.nInactivityTimeout = RF_CONF_INACTIVITY_TIMEOUT;

  ieee_radio.rf_handle = netstack_open(&rf_params);

  if(ieee_radio.rf_handle == NULL) {
    LOG_ERR("Unable to open RF driver\n");
    return RF_RESULT_ERROR;
  }

  set_channel(DOT_15_4G_DEFAULT_CHAN);

  tx_power_value = RF_TxPowerTable_findValue(rf_tx_power_table, RF_TXPOWER_DBM);
  if(tx_power_value.rawValue != RF_TxPowerTable_INVALID_VALUE) {
    rf_stat = RF_setTxPower(ieee_radio.rf_handle, tx_power_value);
    if(rf_stat == RF_StatSuccess) {
      LOG_INFO("TX power configured to %d dBm\n", RF_TXPOWER_DBM);
    } else {
      LOG_WARN("Setting TX power to %d dBm failed, stat=0x%02X", RF_TXPOWER_DBM, rf_stat);
    }
  } else {
    LOG_WARN("Unable to find TX power %d dBm in the TX power table\n", RF_TXPOWER_DBM);
  }

  ENERGEST_ON(ENERGEST_TYPE_LISTEN);

  /* Start RAT overflow upkeep */
  rat_init();

  /* Start RF process */
  process_start(&rf_sched_process, NULL);

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  if(payload_len > TX_BUF_SIZE || payload_len > MAX_PAYLOAD_LEN) {
    return RADIO_TX_ERR;
  }
  memcpy(ieee_radio.tx_buf, payload, payload_len);
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit(unsigned short transmit_len)
{
  rf_result_t res;

  if(transmit_len > MAX_PAYLOAD_LEN) {
    LOG_ERR("Too long\n");
    return RADIO_TX_ERR;
  }

  if(ieee_radio.send_on_cca && channel_clear() != 1) {
    LOG_WARN("Channel is not clear for transmission\n");
    return RADIO_TX_COLLISION;
  }

  /*
   * Are we expecting ACK? The ACK Request flag is in the first Frame
   * Control Field byte, that is the first byte in the frame.
   */
  bool ack_request;
  if(!ieee_radio.poll_mode &&
      (ieee_radio.tx_buf[FRAME_FCF_OFFSET] & FRAME_ACK_REQUEST)) {
    ack_request = true;
  } else {
    ack_request = false;
  }
  if(ack_request) {
    /* Yes, turn on chaining */
    v_cmd_tx.condition.rule = COND_STOP_ON_FALSE;

    /* Reset CMD_IEEE_RX_ACK command */
    v_cmd_rx_ack.status = IDLE;
    /* Sequence number is the third byte in the frame */
    v_cmd_rx_ack.seqNo = ieee_radio.tx_buf[FRAME_SEQNUM_OFFSET];
  } else {
    /* No, turn off chaining */
    v_cmd_tx.condition.rule = COND_NEVER;
  }

  /* Configure TX command */
  v_cmd_tx.payloadLen = (uint8_t)transmit_len;
  v_cmd_tx.pPayload = ieee_radio.tx_buf;

  res = netstack_sched_ieee_tx(transmit_len, ack_request);

  if(res != RF_RESULT_OK) {
    return RADIO_TX_ERR;
  }

  if(ack_request) {
    switch(v_cmd_rx_ack.status) {
    /* CMD_IEEE_RX_ACK timed out, i.e. never received ACK */
    case IEEE_DONE_TIMEOUT: return RADIO_TX_NOACK;
    /* An ACK was received with either pending data bit set or cleared */
    case IEEE_DONE_ACK:     /* fallthrough */
    case IEEE_DONE_ACKPEND: return RADIO_TX_OK;
    /* Any other statuses are errors */
    default:                return RADIO_TX_ERR;
    }
  }

  /* No ACK expected, TX OK */
  return RADIO_TX_OK;
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
  /* Only wait if the Radio timer is accessing the entry */
  while((data_entry->status == DATA_ENTRY_BUSY) &&
        RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + TIMEOUT_DATA_ENTRY_BUSY)) ;

  if(data_entry->status != DATA_ENTRY_FINISHED) {
    /* No available data */
    return 0;
  }

  /*
   * lensz bytes (1) in the data entry are the length of the received frame.
   * Data frame is on the following format:
   *    Length (1) + Payload (N) + FCS (2) + RSSI (1) + Status (1) + Timestamp (4)
   * Data frame DOES NOT contain the following:
   *    no PHY Header bytes
   *    no Source Index bytes
   * Visual representation of frame format:
   *
   *  +--------+---------+---------+--------+--------+-----------+
   *  | 1 byte | N bytes | 2 bytes | 1 byte | 1 byte | 4 bytes   |
   *  +--------+---------+---------+--------+--------+-----------+
   *  | Length | Payload | FCS     | RSSI   | Status | Timestamp |
   *  +--------+---------+---------+--------+--------+-----------+
   *
   * Length bytes equal total length of entire frame excluding itself,
   *       Length = N + FCS (2) + RSSI (1) + Status (1) + Timestamp (4)
   *       Length = N + 8
   *            N = Length - 8
   */
  uint8_t *const frame_ptr = (uint8_t *)&data_entry->data;
  const lensz_t frame_len = *(lensz_t *)frame_ptr;

  /* Sanity check that Frame is at least Frame Shave bytes long */
  if(frame_len < FRAME_SHAVE) {
    LOG_ERR("Received frame too short, len=%d\n", frame_len);

    data_queue_release_entry();
    return 0;
  }

  const uint8_t *payload_ptr = frame_ptr + sizeof(lensz_t);
  const unsigned short payload_len = (unsigned short)(frame_len - FRAME_SHAVE);

  /* Sanity check that Payload fits in buffer. */
  if(payload_len > buf_len) {
    LOG_ERR("MAC payload too large for buffer, len=%d buf_len=%d\n",
            payload_len, buf_len);

    data_queue_release_entry();
    return 0;
  }

  memcpy(buf, payload_ptr, payload_len);

  /* RSSI stored FCS (2) bytes after payload. */
  ieee_radio.last.rssi = (int8_t)payload_ptr[payload_len + 2];
  /* LQI retrieved from Status byte, FCS (2) + RSSI (1) bytes after payload. */
  ieee_radio.last.corr_lqi = (uint8_t)(payload_ptr[payload_len + 3] & STATUS_CORRELATION);
  /* Timestamp stored FCS (2) + RSSI (1) + Status (1) bytes after payload. */
  const uint32_t rat_ticks = *(uint32_t *)(payload_ptr + payload_len + 4);
  /* Correct timestamp so that it refers to the end of the SFD */
  ieee_radio.last.timestamp = rat_to_timestamp(rat_ticks, RAT_TIMESTAMP_OFFSET);

  if(!ieee_radio.poll_mode) {
    /* Not in poll mode: packetbuf should not be accessed in interrupt context. */
    /* In poll mode, the last packet RSSI and link quality can be obtained through */
    /* RADIO_PARAM_LAST_RSSI and RADIO_PARAM_LAST_LINK_QUALITY */
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, (packetbuf_attr_t)ieee_radio.last.rssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, (packetbuf_attr_t)ieee_radio.last.corr_lqi);
  }

  data_queue_release_entry();
  return (int)payload_len;
}
/*---------------------------------------------------------------------------*/
static rf_result_t
cca_request(cmd_cca_req_t *cmd_cca_req)
{
  RF_Stat stat = RF_StatRadioInactiveError;
  rf_result_t res;
  bool stop_rx = false;

  /* RX is required to be running in order to do a CCA request */
  if(!rx_is_active()) {
    /* If RX is not pending, i.e. soon to be running, schedule the RX command */
    if(v_cmd_rx.status != PENDING) {
      res = netstack_sched_rx(false);
      if(res != RF_RESULT_OK) {
        LOG_ERR("CCA request failed to schedule RX\n");
        return res;
      }

      /* We only stop RX if we had to schedule it */
      stop_rx = true;
    }

    /* Make sure RX is running before we continue, unless we timeout and fail */
    RTIMER_BUSYWAIT_UNTIL(rx_is_active(), TIMEOUT_ENTER_RX_WAIT);

    if(!rx_is_active()) {
      LOG_ERR("CCA request failed to turn on RX, RX status=0x%04X\n", v_cmd_rx.status);
      return RF_RESULT_ERROR;
    }
  }

  /* Perform the CCA request */
  do {
    memset(cmd_cca_req, 0x00, sizeof(cmd_cca_req_t));
    cmd_cca_req->commandNo = CMD_IEEE_CCA_REQ;
    cmd_cca_req->ccaInfo.ccaState = CCA_STATE_INVALID;

    stat = RF_runImmediateCmd(ieee_radio.rf_handle, (uint32_t *)cmd_cca_req);

    if(stat != RF_StatCmdDoneSuccess) {
      LOG_ERR("CCA request command failed, stat=0x%02X\n", stat);
      if(stop_rx) {
        netstack_stop_rx();
      }
      return RF_RESULT_ERROR;
    }
  } while(cmd_cca_req->ccaInfo.ccaState == CCA_STATE_INVALID);

  if(stop_rx) {
    netstack_stop_rx();
  }

  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  cmd_cca_req_t cmd_cca_req;

  if(cca_request(&cmd_cca_req) != RF_RESULT_OK) {
    return 0;
  }

  /* Channel is clear if CCA state is IDLE */
  return cmd_cca_req.ccaInfo.ccaState == CCA_STATE_IDLE;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  cmd_cca_req_t cmd_cca_req;

  if(cca_request(&cmd_cca_req) != RF_RESULT_OK) {
    return 0;
  }

  /* If we are transmitting (can only be an ACK here), we are not receiving */
  if((cmd_cca_req.ccaInfo.ccaEnergy == CCA_STATE_BUSY) &&
     (cmd_cca_req.ccaInfo.ccaCorr == CCA_STATE_BUSY) &&
     (cmd_cca_req.ccaInfo.ccaSync == CCA_STATE_BUSY)) {
    LOG_WARN("We are TXing ACK, therefore not receiving packets\n");
    return 0;
  }

  /* We are receiving a packet if a CCA sync has been seen, i.e. ccaSync is busy (1) */
  return cmd_cca_req.ccaInfo.ccaSync == CCA_STATE_BUSY;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  const data_entry_t *const read_entry = data_queue_current_entry();
  volatile const data_entry_t *curr_entry = read_entry;

  int num_pending = 0;

  /* Go through RX Circular buffer and check each data entry status */
  do {
    const uint8_t status = curr_entry->status;
    if((status == DATA_ENTRY_FINISHED) ||
       (status == DATA_ENTRY_BUSY)) {
      num_pending += 1;
    }

    /* Stop when we have looped the circular buffer */
    curr_entry = (data_entry_t *)curr_entry->pNextEntry;
  } while(curr_entry != read_entry);

  if(num_pending > 0 && !ieee_radio.poll_mode) {
    process_poll(&rf_sched_process);
  }

  /* If we didn't find an entry at status finished or busy, no frames are pending */
  return num_pending;
}
/*---------------------------------------------------------------------------*/
static int
on(void)
{
  rf_result_t res;

  if(ieee_radio.rf_is_on) {
    LOG_WARN("Radio is already on\n");
    return RF_RESULT_OK;
  }

  data_queue_reset();

  res = netstack_sched_rx(true);

  if(res != RF_RESULT_OK) {
    return RF_RESULT_ERROR;
  }

  ieee_radio.rf_is_on = true;
  return RF_RESULT_OK;
}
/*---------------------------------------------------------------------------*/
static int
off(void)
{
  if(!ieee_radio.rf_is_on) {
    LOG_WARN("Radio is already off\n");
    return RF_RESULT_OK;
  }

  rf_yield();

  ieee_radio.rf_is_on = false;
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

  /* Power Mode */
  case RADIO_PARAM_POWER_MODE:
    *value = (ieee_radio.rf_is_on)
      ? RADIO_POWER_MODE_ON
      : RADIO_POWER_MODE_OFF;
    return RADIO_RESULT_OK;

  /* Channel */
  case RADIO_PARAM_CHANNEL:
    *value = (radio_value_t)v_cmd_rx.channel;
    return RADIO_RESULT_OK;

  /* PAN ID */
  case RADIO_PARAM_PAN_ID:
    *value = (radio_value_t)v_cmd_rx.localPanID;
    return RADIO_RESULT_OK;

  /* 16-bit address */
  case RADIO_PARAM_16BIT_ADDR:
    *value = (radio_value_t)v_cmd_rx.localShortAddr;
    return RADIO_RESULT_OK;

  /* RX mode */
  case RADIO_PARAM_RX_MODE:
    *value = 0;
    if(v_cmd_rx.frameFiltOpt.frameFiltEn) {
      *value |= (radio_value_t)RADIO_RX_MODE_ADDRESS_FILTER;
    }
    if(v_cmd_rx.frameFiltOpt.autoAckEn) {
      *value |= (radio_value_t)RADIO_RX_MODE_AUTOACK;
    }
    if(ieee_radio.poll_mode) {
      *value |= (radio_value_t)RADIO_RX_MODE_POLL_MODE;
    }
    return RADIO_RESULT_OK;

  /* TX mode */
  case RADIO_PARAM_TX_MODE:
    *value = 0;
    return RADIO_RESULT_OK;

  /* TX power */
  case RADIO_PARAM_TXPOWER:
    res = rf_get_tx_power(ieee_radio.rf_handle, rf_tx_power_table, (int8_t *)&value);
    return ((res == RF_RESULT_OK) &&
            (*value != RF_TxPowerTable_INVALID_DBM))
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  /* CCA threshold */
  case RADIO_PARAM_CCA_THRESHOLD:
    *value = v_cmd_rx.ccaRssiThr;
    return RADIO_RESULT_OK;

  /* RSSI */
  case RADIO_PARAM_RSSI:
    *value = RF_getRssi(ieee_radio.rf_handle);
    return (*value == RF_GET_RSSI_ERROR_VAL)
           ? RADIO_RESULT_ERROR
           : RADIO_RESULT_OK;

  /* Channel min */
  case RADIO_CONST_CHANNEL_MIN:
    *value = (radio_value_t)DOT_15_4G_CHAN_MIN;
    return RADIO_RESULT_OK;

  /* Channel max */
  case RADIO_CONST_CHANNEL_MAX:
    *value = (radio_value_t)DOT_15_4G_CHAN_MAX;
    return RADIO_RESULT_OK;

  case RADIO_CONST_TXPOWER_MIN:
    *value = (radio_value_t)tx_power_min(rf_tx_power_table);
    return RADIO_RESULT_OK;

  /* TX power max */
  case RADIO_CONST_TXPOWER_MAX:
    *value = (radio_value_t)tx_power_max(rf_tx_power_table, rf_tx_power_table_size);
    return RADIO_RESULT_OK;

  /* Last RSSI */
  case RADIO_PARAM_LAST_RSSI:
    *value = (radio_value_t)ieee_radio.last.rssi;
    return RADIO_RESULT_OK;

  /* Last link quality */
  case RADIO_PARAM_LAST_LINK_QUALITY:
    *value = (radio_value_t)ieee_radio.last.corr_lqi;
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
  rf_result_t res;

  switch(param) {

  /* Power Mode */
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

  /* Channel */
  case RADIO_PARAM_CHANNEL:
    if(!dot_15_4g_chan_in_range(value)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_channel((uint8_t)value);
    return RADIO_RESULT_OK;

  /* PAN ID */
  case RADIO_PARAM_PAN_ID:
    v_cmd_rx.localPanID = (uint16_t)value;
    if(!ieee_radio.rf_is_on) {
      return RADIO_RESULT_OK;
    }

    netstack_stop_rx();
    res = netstack_sched_rx(false);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  /* 16bit address */
  case RADIO_PARAM_16BIT_ADDR:
    v_cmd_rx.localShortAddr = (uint16_t)value;
    if(!ieee_radio.rf_is_on) {
      return RADIO_RESULT_OK;
    }

    netstack_stop_rx();
    res = netstack_sched_rx(false);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  /* RX Mode */
  case RADIO_PARAM_RX_MODE: {
    if(value & ~(RADIO_RX_MODE_ADDRESS_FILTER |
                 RADIO_RX_MODE_AUTOACK |
                 RADIO_RX_MODE_POLL_MODE)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    v_cmd_rx.frameFiltOpt.frameFiltEn = (value & RADIO_RX_MODE_ADDRESS_FILTER) != 0;
    v_cmd_rx.frameFiltOpt.frameFiltStop = 1;
    v_cmd_rx.frameFiltOpt.autoAckEn = (value & RADIO_RX_MODE_AUTOACK) != 0;
    v_cmd_rx.frameFiltOpt.slottedAckEn = 0;
    v_cmd_rx.frameFiltOpt.autoPendEn = 0;
    v_cmd_rx.frameFiltOpt.defaultPend = 0;
    v_cmd_rx.frameFiltOpt.bPendDataReqOnly = 0;
    v_cmd_rx.frameFiltOpt.bPanCoord = 0;
    v_cmd_rx.frameFiltOpt.bStrictLenFilter = 0;

    const bool old_poll_mode = ieee_radio.poll_mode;
    ieee_radio.poll_mode = (value & RADIO_RX_MODE_POLL_MODE) != 0;
    if(old_poll_mode == ieee_radio.poll_mode) {
      /* Do not turn the radio off and on, just send an update command */
      memcpy(&cmd_mod_filt.newFrameFiltOpt, &(cmd_rx.frameFiltOpt), sizeof(cmd_rx.frameFiltOpt));
      const RF_Stat stat = RF_runImmediateCmd(ieee_radio.rf_handle, (uint32_t *)&cmd_mod_filt);
      if(stat != RF_StatCmdDoneSuccess) {
        LOG_ERR("Setting address filter failed, stat=0x%02X\n", stat);
        return RADIO_RESULT_ERROR;
      }
      return RADIO_RESULT_OK;
    }
    if(!ieee_radio.rf_is_on) {
      return RADIO_RESULT_OK;
    }

    netstack_stop_rx();
    res = netstack_sched_rx(false);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;
  }

  /* TX Mode */
  case RADIO_PARAM_TX_MODE:
    if(value & ~(RADIO_TX_MODE_SEND_ON_CCA)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_send_on_cca((value & RADIO_TX_MODE_SEND_ON_CCA) != 0);
    return RADIO_RESULT_OK;

  /* TX Power */
  case RADIO_PARAM_TXPOWER:
    if(!tx_power_in_range((int8_t)value, rf_tx_power_table, rf_tx_power_table_size)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    res = rf_set_tx_power(ieee_radio.rf_handle, rf_tx_power_table, (int8_t)value);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  /* CCA Threshold */
  case RADIO_PARAM_CCA_THRESHOLD:
    v_cmd_rx.ccaRssiThr = (int8_t)value;
    if(!ieee_radio.rf_is_on) {
      return RADIO_RESULT_OK;
    }

    netstack_stop_rx();
    res = netstack_sched_rx(false);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  if(!dest) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch(param) {
  /* 64bit address */
  case RADIO_PARAM_64BIT_ADDR: {
    const size_t srcSize = sizeof(v_cmd_rx.localExtAddr);
    if(size != srcSize) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    const uint8_t *pSrc = (uint8_t *)&(v_cmd_rx.localExtAddr);
    uint8_t *pDest = dest;
    for(size_t i = 0; i < srcSize; ++i) {
      pDest[i] = pSrc[srcSize - 1 - i];
    }

    return RADIO_RESULT_OK;
  }
  /* Last packet timestamp */
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
  rf_result_t res;

  if(!src) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch(param) {
  /* 64-bit address */
  case RADIO_PARAM_64BIT_ADDR: {
    const size_t destSize = sizeof(v_cmd_rx.localExtAddr);
    if(size != destSize) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    const uint8_t *pSrc = (const uint8_t *)src;
    volatile uint8_t *pDest = (uint8_t *)&(v_cmd_rx.localExtAddr);
    for(size_t i = 0; i < destSize; ++i) {
      pDest[i] = pSrc[destSize - 1 - i];
    }

    if(!rx_is_active()) {
      return RADIO_RESULT_OK;
    }

    netstack_stop_rx();
    res = netstack_sched_rx(false);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;
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
