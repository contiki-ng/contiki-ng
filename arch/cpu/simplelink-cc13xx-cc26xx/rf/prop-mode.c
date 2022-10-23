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
#include DeviceFamily_constructPath(inc/hw_rfc_dbell.h)
#include DeviceFamily_constructPath(driverlib/rfc.h)
/*---------------------------------------------------------------------------*/
/* Platform RF dev */
#include "rf/rf.h"
#include "rf/dot-15-4g.h"
#include "rf/sched.h"
#include "rf/data-queue.h"
#include "rf/tx-power.h"
#include "rf/settings.h"
#include "rf/rat.h"
#include "rf/radio-mode.h"
/*---------------------------------------------------------------------------*/
#include <inttypes.h>
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
#if MAC_CONF_WITH_TSCH
static volatile uint8_t is_receiving_packet;
#endif
/*---------------------------------------------------------------------------*/
/* Defines and variables related to the .15.4g PHY HDR */
#define DOT_4G_PHR_NUM_BYTES    2
#define DOT_4G_LEN_OFFSET       0xFC
#define DOT_4G_SYNCWORD         0x0055904E

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
/* How long to wait for the RF to enter RX in rf_cmd_ieee_rx */
#define TIMEOUT_ENTER_RX_WAIT   (RTIMER_SECOND >> 10)

/*---------------------------------------------------------------------------*/
/*
 * Offset of the end of SFD when compared to the radio HW-generated timestamp.
 */
#define RAT_TIMESTAMP_OFFSET    USEC_TO_RAT(RADIO_PHY_HEADER_LEN * RADIO_BYTE_AIR_TIME - 270)
/*---------------------------------------------------------------------------*/
/* TX buf configuration */
#define TX_BUF_HDR_LEN          2
#define TX_BUF_PAYLOAD_LEN      180

#define TX_BUF_SIZE             (TX_BUF_HDR_LEN + TX_BUF_PAYLOAD_LEN)
/*---------------------------------------------------------------------------*/
/* Size of the Length field in Data Entry, two bytes in this case */
typedef uint16_t lensz_t;

#define FRAME_OFFSET            sizeof(lensz_t)
#define FRAME_SHAVE             6   /**< RSSI (1) +  Timestamp (4) + Status (1) */
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

  /* RX Statistics struct */
  rx_output_t rx_stats;

  /* RSSI Threshold */
  int8_t rssi_threshold;
  uint16_t channel;

  /* Indicates RF is supposed to be on or off */
  uint8_t rf_is_on;
  /* Enable/disable CCA before sending */
  bool send_on_cca;

  /* Last RX operation stats */
  struct {
    int8_t rssi;
    uint8_t corr_lqi;
    uint32_t timestamp;
  } last;
} prop_radio_t;

static prop_radio_t prop_radio;

/*---------------------------------------------------------------------------*/
/* Convenience macros for more succinct access of RF commands */
#define cmd_radio_setup     rf_cmd_prop_radio_div_setup
#define cmd_fs              rf_cmd_prop_fs
#define cmd_tx              rf_cmd_prop_tx_adv
#define cmd_rx              rf_cmd_prop_rx_adv

/* Convenience macros for volatile access with the RF commands */
#define v_cmd_radio_setup   CC_ACCESS_NOW(rfc_CMD_PROP_RADIO_DIV_SETUP_t, rf_cmd_prop_radio_div_setup)
#define v_cmd_fs            CC_ACCESS_NOW(rfc_CMD_FS_t,                   rf_cmd_prop_fs)
#define v_cmd_tx            CC_ACCESS_NOW(rfc_CMD_PROP_TX_ADV_t,          rf_cmd_prop_tx_adv)
#define v_cmd_rx            CC_ACCESS_NOW(rfc_CMD_PROP_RX_ADV_t,          rf_cmd_prop_rx_adv)
/*---------------------------------------------------------------------------*/
static inline bool
tx_is_active(void)
{
  return v_cmd_tx.status == ACTIVE;
}
/*---------------------------------------------------------------------------*/
static inline bool
rx_is_active(void)
{
  return v_cmd_rx.status == ACTIVE;
}
/*---------------------------------------------------------------------------*/
static int channel_clear(void);
static int on(void);
static int off(void);
static rf_result_t set_channel_force(uint16_t channel);
/*---------------------------------------------------------------------------*/
static void
init_rf_params(void)
{
  cmd_radio_setup.config.frontEndMode = RF_SUB_1_GHZ_FRONT_END_MODE;
  cmd_radio_setup.config.biasMode = RF_SUB_1_GHZ_BIAS_MODE;
  cmd_radio_setup.centerFreq = PROP_MODE_CENTER_FREQ;
  cmd_radio_setup.loDivider = PROP_MODE_LO_DIVIDER;

  cmd_tx.numHdrBits = DOT_4G_PHR_NUM_BYTES * 8;
  cmd_tx.syncWord = DOT_4G_SYNCWORD;

  cmd_rx.syncWord0 = DOT_4G_SYNCWORD;
  cmd_rx.syncWord1 = 0x00000000;
  cmd_rx.maxPktLen = RADIO_PHY_OVERHEAD + MAX_PAYLOAD_LEN;
  cmd_rx.hdrConf.numHdrBits = DOT_4G_PHR_NUM_BYTES * 8;
  cmd_rx.lenOffset = DOT_4G_LEN_OFFSET;
  cmd_rx.pQueue = data_queue_init(sizeof(lensz_t));
  cmd_rx.pOutput = (uint8_t *)&prop_radio.rx_stats;
}
/*---------------------------------------------------------------------------*/
static int8_t
get_rssi(void)
{
  rf_result_t res;
  bool stop_rx = false;
  int8_t rssi = RF_GET_RSSI_ERROR_VAL;

  /* RX is required to be running in order to do a RSSI measurement */
  if(!rx_is_active()) {
    /* If RX is not pending, i.e. soon to be running, schedule the RX command */
    if(v_cmd_rx.status != PENDING) {
      res = netstack_sched_rx(false);
      if(res != RF_RESULT_OK) {
        LOG_ERR("RSSI measurement failed to schedule RX\n");
        return res;
      }

      /* We only stop RX if we had to schedule it */
      stop_rx = true;
    }

    /* Make sure RX is running before we continue, unless we timeout and fail */
    RTIMER_BUSYWAIT_UNTIL(rx_is_active(), TIMEOUT_ENTER_RX_WAIT);

    if(!rx_is_active()) {
      LOG_ERR("RSSI measurement failed to turn on RX, RX status=0x%04X\n", v_cmd_rx.status);
      return RF_RESULT_ERROR;
    }
  }

  /* Perform the RSSI measurement */
  rssi = RF_getRssi(prop_radio.rf_handle);

  if(stop_rx) {
    netstack_stop_rx();
  }

  return rssi;
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_channel(void)
{
  uint32_t freq_khz = v_cmd_fs.frequency * 1000;

  /*
   * For some channels, fractFreq * 1000 / 65536 will return 324.99xx.
   * Casting the result to uint32_t will truncate decimals resulting in the
   * function returning channel - 1 instead of channel. Thus, we do a quick
   * positive integer round up.
   */
  freq_khz += (((v_cmd_fs.fractFreq * 1000) + 65535) / 65536);

  return (uint8_t)((freq_khz - DOT_15_4G_CHAN0_FREQ) / DOT_15_4G_FREQ_SPACING);
}
/*---------------------------------------------------------------------------*/
static rf_result_t
set_channel(uint16_t channel)
{
  if(!dot_15_4g_chan_in_range(channel)) {
    LOG_WARN("Supplied hannel %d is illegal, defaults to %d\n",
             (int)channel, DOT_15_4G_DEFAULT_CHAN);
    channel = DOT_15_4G_DEFAULT_CHAN;
  }

  if(channel == prop_radio.channel) {
    /* We are already calibrated to this channel */
    return RF_RESULT_OK;
  }

  return set_channel_force(channel);
}
/*---------------------------------------------------------------------------*/
/* Sets the given channel without checking if it is a valid channel number. */
static rf_result_t
set_channel_force(uint16_t channel)
{
  rf_result_t res;

  if(prop_radio.rf_is_on) {
    /* Force RAT and RTC resync */
    rf_restart_rat();
  }

  const uint32_t new_freq = dot_15_4g_freq(channel);
  const uint16_t freq = (uint16_t)(new_freq / 1000);
  const uint16_t frac = (uint16_t)(((new_freq - (freq * 1000)) * 0x10000) / 1000);

  LOG_DBG("Set channel to %d, frequency 0x%04X.0x%04X (%" PRIu32 ")\n",
          (int)channel, freq, frac, new_freq);

  v_cmd_fs.frequency = freq;
  v_cmd_fs.fractFreq = frac;

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
static void
set_send_on_cca(bool enable)
{
  prop_radio.send_on_cca = enable;
}
/*---------------------------------------------------------------------------*/
static int
prepare(const void *payload, unsigned short payload_len)
{
  if(payload_len > TX_BUF_PAYLOAD_LEN || payload_len > MAX_PAYLOAD_LEN) {
    return RADIO_TX_ERR;
  }

  memcpy(prop_radio.tx_buf + TX_BUF_HDR_LEN, payload, payload_len);
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

  if(tx_is_active()) {
    LOG_ERR("A transmission is already active\n");
    return RADIO_TX_ERR;
  }

  if(prop_radio.send_on_cca && !channel_clear()) {
    LOG_WARN("Channel is not clear for transmission\n");
    return RADIO_TX_COLLISION;
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
  v_cmd_tx.pktLen = transmit_len + DOT_4G_PHR_NUM_BYTES;
  v_cmd_tx.pPkt = prop_radio.tx_buf;

  res = netstack_sched_prop_tx(transmit_len);

  if(res != RF_RESULT_OK) {
    LOG_WARN("Channel is not clear for transmission\n");
    return RADIO_TX_ERR;
  }

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

  /* Only wait if the Radio is accessing the entry */
  const rtimer_clock_t t0 = RTIMER_NOW();
  while((data_entry->status == DATA_ENTRY_BUSY) &&
         RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + RADIO_FRAME_DURATION(MAX_PAYLOAD_LEN)));

#if MAC_CONF_WITH_TSCH
  /* Make sure the flag is reset */
  is_receiving_packet = 0;
#endif

  if(data_entry->status != DATA_ENTRY_FINISHED) {
    /* No available data */
    return 0;
  }

  /*
   * lensz bytes (2) in the data entry are the length of the received frame.
   * Data frame is on the following format:
   *    Length (2) + Payload (N) + RSSI (1) + Timestamp (4) + Status (1)
   * Data frame DOES NOT contain the following:
   *    no Header/PHY bytes
   *    no appended Received CRC bytes
   * Visual representation of frame format:
   *
   *  +---------+---------+--------+-----------+--------+
   *  | 2 bytes | N bytes | 1 byte | 4 bytes   | 1 byte |
   *  +---------+---------+--------+-----------+--------+
   *  | Length  | Payload | RSSI   | Timestamp | Status |
   *  +---------+---------+--------+-----------+--------+
   *
   * Length bytes equal total length of entire frame excluding itself,
   *       Length = N + RSSI (1) + Timestamp (4) +  Status (1)
   *              = N + 6
   *            N = Length - 6
   */
  uint8_t *const frame_ptr = (uint8_t *)&data_entry->data;
  const lensz_t frame_len = *(lensz_t *)frame_ptr;

  /* Sanity check that Frame is at least Frame Shave bytes long */
  if(frame_len < FRAME_SHAVE) {
    LOG_ERR("Received frame is too short, len=%d\n", frame_len);

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
  prop_radio.last.rssi = (int8_t)payload_ptr[payload_len];
  /* LQI calculated from RSSI */
  prop_radio.last.corr_lqi = calculate_lqi(prop_radio.last.rssi);
  /* Timestamp stored RSSI (1) bytes after payload. */
  uint32_t rat_ticks;
  memcpy(&rat_ticks, payload_ptr + payload_len + 1, 4);

  /* Correct timestamp so that it refers to the end of the SFD */
  prop_radio.last.timestamp = rat_to_timestamp(rat_ticks, RAT_TIMESTAMP_OFFSET);

  if(!prop_radio.poll_mode) {
    /* Not in poll mode: packetbuf should not be accessed in interrupt context. */
    /* In poll mode, the last packet RSSI and link quality can be obtained through */
    /* RADIO_PARAM_LAST_RSSI and RADIO_PARAM_LAST_LINK_QUALITY */
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, (packetbuf_attr_t)prop_radio.last.rssi);
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, (packetbuf_attr_t)prop_radio.last.corr_lqi);
  }

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
  if(!prop_radio.rf_is_on) {
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
  if(!is_receiving_packet) {
    /* Look for the modem synchronization word detection interrupt flag.
     * This flag is raised when the synchronization word is received.
     */
    if(HWREG(RFC_DBELL_BASE + RFC_DBELL_O_RFHWIFG) & RFC_DBELL_RFHWIFG_MDMSOFT) {
      is_receiving_packet = 1;
    }
  } else {
    /* After the start of the packet: reset the Rx flag once the channel gets clear */
    is_receiving_packet = (cca_request() == CCA_STATE_BUSY);
    if(!is_receiving_packet) {
      /* Clear the modem sync flag */
      RFCHwIntClear(RFC_DBELL_RFHWIFG_MDMSOFT);
    }
  }

  return is_receiving_packet;
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

  if(cca_request() == CCA_STATE_BUSY) {
    return 1;
  }

  return 0;

#endif
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

  if(num_pending > 0 && !prop_radio.poll_mode) {
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

  /* RX mode */
  case RADIO_PARAM_RX_MODE:
    *value = 0;
    if(prop_radio.poll_mode) {
      *value |= (radio_value_t)RADIO_RX_MODE_POLL_MODE;
    }
    return RADIO_RESULT_OK;

  /* TX mode */
  case RADIO_PARAM_TX_MODE:
    *value = 0;
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

  case RADIO_PARAM_LAST_RSSI:
     *value = prop_radio.last.rssi;
     return RADIO_RESULT_OK;

  case RADIO_PARAM_LAST_LINK_QUALITY:
     *value = prop_radio.last.corr_lqi;
     return RADIO_RESULT_OK;

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

  /* RX Mode */
  case RADIO_PARAM_RX_MODE:
    if(value & ~(RADIO_RX_MODE_POLL_MODE)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    const bool old_poll_mode = prop_radio.poll_mode;
    prop_radio.poll_mode = (value & RADIO_RX_MODE_POLL_MODE) != 0;
    if(old_poll_mode == prop_radio.poll_mode) {
      return RADIO_RESULT_OK;
    }
    if(!prop_radio.rf_is_on) {
      return RADIO_RESULT_OK;
    }

    netstack_stop_rx();
    res = netstack_sched_rx(false);
    return (res == RF_RESULT_OK)
           ? RADIO_RESULT_OK
           : RADIO_RESULT_ERROR;

  /* TX Mode */
  case RADIO_PARAM_TX_MODE:
    if(value & ~(RADIO_TX_MODE_SEND_ON_CCA)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_send_on_cca((value & RADIO_TX_MODE_SEND_ON_CCA) != 0);
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
  if(!dest) {
    return RADIO_RESULT_INVALID_VALUE;
  }

  switch(param) {
  /* Last packet timestamp */
  case RADIO_PARAM_LAST_PACKET_TIMESTAMP:
    if(size != sizeof(rtimer_clock_t)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    *(rtimer_clock_t *)dest = prop_radio.last.timestamp;

    return RADIO_RESULT_OK;

  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
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
  RF_Params rf_params;
  RF_TxPowerTable_Value tx_power_value;
  RF_Stat rf_stat;

  prop_radio.rx_is_active = rx_is_active;

  radio_mode = (simplelink_radio_mode_t *)&prop_radio;

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
  RF_Params_init(&rf_params);
  rf_params.nInactivityTimeout = RF_CONF_INACTIVITY_TIMEOUT;

  /* Open RF Driver */
  prop_radio.rf_handle = netstack_open(&rf_params);

  if(prop_radio.rf_handle == NULL) {
    LOG_ERR("Unable to open RF driver during initialization\n");
    return RF_RESULT_ERROR;
  }

  set_channel_force(IEEE802154_DEFAULT_CHANNEL);

  tx_power_value = RF_TxPowerTable_findValue(rf_tx_power_table, RF_TXPOWER_DBM);
  if(tx_power_value.rawValue != RF_TxPowerTable_INVALID_VALUE) {
    rf_stat = RF_setTxPower(prop_radio.rf_handle, tx_power_value);
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
