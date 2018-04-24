/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "contiki.h"

#include "sys/cooja_mt.h"
#include "lib/simEnvChange.h"

#include "net/packetbuf.h"
#include "net/netstack.h"
#include "sys/energest.h"

#include "dev/radio.h"
#include "dev/cooja-radio.h"

#include "sys/log.h"
#define LOG_MODULE "cooja-radio"
#define LOG_LEVEL LOG_LEVEL_NONE

/*
 * The maximum number of bytes this driver can accept from the MAC layer for
 * transmission or will deliver to the MAC layer after reception. Includes
 * the MAC header and payload, but not the FCS.
 */
#ifdef COOJA_RADIO_CONF_BUFSIZE
#define COOJA_RADIO_BUFSIZE COOJA_RADIO_CONF_BUFSIZE
#else
#define COOJA_RADIO_BUFSIZE 125
#endif

#define MIN_CHANNEL 11
#define MAX_CHANNEL 26
#define CCA_SS_THRESHOLD -95

/* The radio driver can provide Cooja these values.
 * But at present, Cooja ignores and overrides them.
 * */
enum {
    /*
     * Tmote Sky (with CC2420 radio) gives value -100dB
     * CC1310 gives about -110dB
    */
    RSSI_NO_SIGNAL = -110,

    /*
     * Tmote Sky (with CC2420 radio) gives value 105
     * CC1310 gives about 100
    */
    LQI_NO_SIGNAL  = 100,
};

/* COOJA */
int simReceiving = 0;
char simInDataBuffer[RADIO_HEADER_LEN + RADIO_MAX_PAYLOAD];
int simReceivedBytes = 0;
int simPendingIncomingFrame = 0;
rtimer_clock_t simLastPacketTimestamp = 0;
int simTransmitting;
char simOutDataBuffer[RADIO_HEADER_LEN + RADIO_MAX_PAYLOAD];
int simIsSequence = 0;
int simStopSequence = 0;
int simPendingOutgoingFrame = 0;
char simRadioHWOn = 1;
int simSignalStrength       = RSSI_NO_SIGNAL;
int simLastSignalStrength   = RSSI_NO_SIGNAL;
char simPower = 100;
int simRadioChannel = MAX_CHANNEL;
int simLQI      = LQI_NO_SIGNAL;
int simLastLQI  = LQI_NO_SIGNAL;
int simShrSearching = RADIO_SHR_SEARCH_EN;
int simShrInterrupt = 0;
int simFifopInterrupt = 0;
char simFifopThreshold = RADIO_MAX_PAYLOAD;
int simTxdoneInterrupt = 0;
int simRxdoneInterrupt = 0;
int simAutoCrc = 1;
int simShallEnterRxAfterTx = 0;
int simSequencePointer = 0;



static const void *pending_data;

/* If we are in the polling mode, poll_mode is 1; otherwise 0 */
static int poll_mode = 0; /* default 0, disabled */
static int auto_ack = 0; /* AUTO_ACK is not supported; always 0 */
static int addr_filter = 0; /* ADDRESS_FILTER is not supported; always 0 */
static int send_on_cca = (COOJA_TRANSMIT_ON_CCA != 0);
static volatile radio_shr_callback_t shr_callback;
static volatile radio_fifop_callback_t fifop_callback;
static volatile radio_txdone_callback_t txdone_callback;
static volatile uint8_t read_bytes;
static volatile uint8_t sequence_pos;

PROCESS(cooja_radio_process, "cooja radio process");
/*---------------------------------------------------------------------------*/
static void
set_send_on_cca(uint8_t enable)
{
  send_on_cca = enable;
}
/*---------------------------------------------------------------------------*/
static void
set_frame_filtering(int enable)
{
  addr_filter = enable;
}
/*---------------------------------------------------------------------------*/
static void
set_auto_ack(int enable)
{
  auto_ack = enable;
}
/*---------------------------------------------------------------------------*/
static void
set_poll_mode(int enable)
{
  poll_mode = enable;
}
/*---------------------------------------------------------------------------*/
void
radio_set_channel(int channel)
{
  simRadioChannel = channel;
}
/*---------------------------------------------------------------------------*/
static void
set_shr_search(int enable)
{
  simShrSearching = enable;
}
/*---------------------------------------------------------------------------*/
void
radio_set_txpower(unsigned char power)
{
  /* 1 - 100: Number indicating output power */
  simPower = power;
}
/*---------------------------------------------------------------------------*/
int
radio_signal_strength_last(void)
{
  return simLastSignalStrength;
}
/*---------------------------------------------------------------------------*/
int
radio_signal_strength_current(void)
{
  return simSignalStrength;
}
/*---------------------------------------------------------------------------*/
int
radio_LQI(void)
{
  return simLQI;
}
/*---------------------------------------------------------------------------*/
static
int radio_lqi_last(void)
{
  return simLastLQI;
}
/*---------------------------------------------------------------------------*/
static int
radio_on(void)
{
  ENERGEST_ON(ENERGEST_TYPE_LISTEN);
  simRadioHWOn = 1;
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
radio_off(void)
{
  ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
  ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
  simRadioHWOn = 0;
  return 1;
}
/*---------------------------------------------------------------------------*/
void
cooja_radio_check(void)
{
  if(!simRadioHWOn) {
    simPendingIncomingFrame = 0;
  } else if(simReceiving) {
    simLastSignalStrength = simSignalStrength;
    simLastLQI = simLQI;
  }

  /* acknowledge interrupts */
  if(simShrInterrupt) {
    simShrInterrupt = 0;
    if(shr_callback) {
      shr_callback();
    }
  }
  if(simFifopInterrupt) {
    simFifopInterrupt = 0;
    if(fifop_callback) {
      fifop_callback();
    }
  }
  if(simRxdoneInterrupt) {
    simRxdoneInterrupt = 0;
    if(fifop_callback) {
      fifop_callback();
    }
    process_poll(&cooja_radio_process);
  }
  if(simTxdoneInterrupt) {
    simTxdoneInterrupt = 0;
    if(txdone_callback) {
      txdone_callback();
    }
  }
}
/*---------------------------------------------------------------------------*/
static int
radio_read(void *buf, unsigned short bufsize)
{
  if(!simPendingIncomingFrame) {
    return 0;
  }
  simPendingIncomingFrame = 0; /* rx flush */
  if(bufsize < simInDataBuffer[0]) {
    return 0;
  }
  memcpy(buf, simInDataBuffer + 1, simInDataBuffer[0]);
  if(!poll_mode) {
    packetbuf_set_attr(PACKETBUF_ATTR_RSSI, radio_signal_strength_last());
    packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, radio_lqi_last() );
  }
  return simInDataBuffer[0];
}
/*---------------------------------------------------------------------------*/
static int
channel_clear(void)
{
  if(simSignalStrength > CCA_SS_THRESHOLD) {
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
radio_send(const void *payload, unsigned short payload_len)
{
  int result;

  if(payload_len > COOJA_RADIO_BUFSIZE) {
    return RADIO_TX_ERR;
  }
  if(payload_len == 0) {
    return RADIO_TX_ERR;
  }
  if(simPendingOutgoingFrame) {
    return RADIO_TX_ERR;
  }

  simShallEnterRxAfterTx = simRadioHWOn;
  if(simRadioHWOn) {
    ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);
  } else {
    /* Turn on radio temporarily */
    simRadioHWOn = 1;
    ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
  }

#if COOJA_SIMULATE_TURNAROUND
  simProcessRunValue = 1;
  cooja_mt_yield();
  if(payload_len > 3) {
    simProcessRunValue = 1;
    cooja_mt_yield();
  }
#endif /* COOJA_SIMULATE_TURNAROUND */

  /* Transmit on CCA */
  if(COOJA_TRANSMIT_ON_CCA && send_on_cca && !channel_clear()) {
    result = RADIO_TX_COLLISION;
  } else {
    /* Copy packet data to temporary storage */
    simOutDataBuffer[0] = payload_len;
    memcpy(simOutDataBuffer + 1, payload, payload_len);
    simPendingOutgoingFrame = 1;

    /* Transmit */
    do {
      simProcessRunValue = 1;
      cooja_mt_yield();
    } while(simTransmitting);

    result = RADIO_TX_OK;
  }

  if(simShallEnterRxAfterTx) {
    ENERGEST_SWITCH(ENERGEST_TYPE_TRANSMIT, ENERGEST_TYPE_LISTEN);
  } else {
    ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
  }

  return result;
}
/*---------------------------------------------------------------------------*/
static int
prepare_packet(const void *data, unsigned short len)
{
  if(len > COOJA_RADIO_BUFSIZE) {
    return RADIO_TX_ERR;
  }
  pending_data = data;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
transmit_packet(unsigned short len)
{
  int ret = RADIO_TX_ERR;
  if(pending_data != NULL) {
    ret = radio_send(pending_data, len);
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
static int
receiving_packet(void)
{
  return simReceiving;
}
/*---------------------------------------------------------------------------*/
static int
pending_packet(void)
{
  return !simReceiving && simPendingIncomingFrame;
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(cooja_radio_process, ev, data)
{
  int len;

  PROCESS_BEGIN();

  while(1) {
    PROCESS_YIELD_UNTIL(ev == PROCESS_EVENT_POLL);
    if(poll_mode) {
      continue;
    }

    packetbuf_clear();
    len = radio_read(packetbuf_dataptr(), PACKETBUF_SIZE);
    if(len > 0) {
      packetbuf_set_datalen(len);
      NETSTACK_MAC.input();
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
static int
init(void)
{
  process_start(&cooja_radio_process, NULL);
  return 1;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
  switch(param) {
  case RADIO_PARAM_RX_MODE:
    *value = 0;
    if(addr_filter) {
      *value |= RADIO_RX_MODE_ADDRESS_FILTER;
    }
    if(auto_ack) {
      *value |= RADIO_RX_MODE_AUTOACK;
    }
    if(poll_mode) {
      *value |= RADIO_RX_MODE_POLL_MODE;
    }
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    *value = 0;
    if(send_on_cca) {
      *value |= RADIO_TX_MODE_SEND_ON_CCA;
    }
    return RADIO_RESULT_OK;
  case RADIO_PARAM_LAST_RSSI:
    *value = radio_signal_strength_last();
    return RADIO_RESULT_OK;

  case RADIO_PARAM_LAST_LINK_QUALITY:
    *value = radio_lqi_last();
    return RADIO_RESULT_OK;

  case RADIO_PARAM_RSSI:
    *value = radio_signal_strength_current();
    return RADIO_RESULT_OK;

  case RADIO_CONST_MAX_PAYLOAD_LEN:
    *value = (radio_value_t)COOJA_RADIO_BUFSIZE;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    *value = simRadioChannel;
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MIN:
    *value = MIN_CHANNEL;
    return RADIO_RESULT_OK;
  case RADIO_CONST_CHANNEL_MAX:
    *value = MAX_CHANNEL;
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
  case RADIO_PARAM_RX_MODE:
    if(value & ~(RADIO_RX_MODE_ADDRESS_FILTER |
        RADIO_RX_MODE_AUTOACK | RADIO_RX_MODE_POLL_MODE)) {
      return RADIO_RESULT_INVALID_VALUE;
    }

    /* Only disabling is acceptable for RADIO_RX_MODE_ADDRESS_FILTER */
    if ((value & RADIO_RX_MODE_ADDRESS_FILTER) != 0) {
      return RADIO_RESULT_NOT_SUPPORTED;
    }
    set_frame_filtering((value & RADIO_RX_MODE_ADDRESS_FILTER) != 0);

    /* Only disabling is acceptable for RADIO_RX_MODE_AUTOACK */
    if ((value & RADIO_RX_MODE_ADDRESS_FILTER) != 0) {
      return RADIO_RESULT_NOT_SUPPORTED;
    }
    set_auto_ack((value & RADIO_RX_MODE_AUTOACK) != 0);

    set_poll_mode((value & RADIO_RX_MODE_POLL_MODE) != 0);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    if(value & ~(RADIO_TX_MODE_SEND_ON_CCA)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_send_on_cca((value & RADIO_TX_MODE_SEND_ON_CCA) != 0);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    /* With channel value < 0 Cooja matches any channels:
     *  - send packets on a negative channel -> to any receiver's channels.
     *  - receive on a negative channel <- get packets from any sender's channels.
     * So, negative channel are useful for wide-band noise generation.
     * Or for wide-band sniffing.
     * */
    radio_set_channel(value);
    return RADIO_RESULT_OK;
  case RADIO_PARAM_SHR_SEARCH:
    if((value != RADIO_SHR_SEARCH_EN) && (value != RADIO_SHR_SEARCH_DIS)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    set_shr_search(value);
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}
/*---------------------------------------------------------------------------*/
static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
  if(param == RADIO_PARAM_LAST_PACKET_TIMESTAMP) {
    if(size != sizeof(rtimer_clock_t) || !dest) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    *(rtimer_clock_t *)dest = (rtimer_clock_t)simLastPacketTimestamp;
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
static void
async_enter(void)
{
  radio_off();
  poll_mode = 1;
  simAutoCrc = 0;
}
/*---------------------------------------------------------------------------*/
static void
async_prepare(uint8_t *length_then_payload)
{
  memcpy(simOutDataBuffer, length_then_payload, length_then_payload[0] + 1);
  simIsSequence = 0;
}
/*---------------------------------------------------------------------------*/
static void
async_reprepare(uint8_t offset, uint8_t *patch, uint8_t patch_len)
{
  memcpy(simOutDataBuffer + RADIO_HEADER_LEN + offset, patch, patch_len);
}
/*---------------------------------------------------------------------------*/
static void
async_transmit(bool shall_enter_rx_after_tx)
{
  simShallEnterRxAfterTx = shall_enter_rx_after_tx;
  simRadioHWOn = 1;
  simPendingOutgoingFrame = 1;
  ENERGEST_SWITCH(ENERGEST_TYPE_LISTEN, ENERGEST_TYPE_TRANSMIT);
}
/*---------------------------------------------------------------------------*/
static void
async_on(void)
{
  if(simRadioHWOn) {
    LOG_WARN("already on\n");
    return;
  }
  ENERGEST_ON(ENERGEST_TYPE_LISTEN);
  simRadioHWOn = 1;
}
/*---------------------------------------------------------------------------*/
static void
async_off(void)
{
  if(!simRadioHWOn) {
    LOG_WARN("already off\n");
    return;
  }
  radio_off();
}
/*---------------------------------------------------------------------------*/
static void
async_set_shr_callback(radio_shr_callback_t cb)
{
  shr_callback = cb;
}
/*---------------------------------------------------------------------------*/
static void
async_set_fifop_callback(radio_fifop_callback_t cb, uint8_t threshold)
{
  fifop_callback = cb;
  simFifopThreshold = threshold;
}
/*---------------------------------------------------------------------------*/
static void
async_set_txdone_callback(radio_txdone_callback_t cb)
{
  txdone_callback = cb;
}
/*---------------------------------------------------------------------------*/
static uint8_t
async_read_phy_header(void)
{
  while(!simReceivedBytes) {
    simProcessRunValue = 1;
    cooja_mt_yield();
  }
  read_bytes = 0;
  return simInDataBuffer[0];
}
/*---------------------------------------------------------------------------*/
static bool
async_read_payload(uint8_t *buf, uint8_t bytes)
{
  if(radio_remaining_payload_bytes() < bytes) {
    return false;
  }
  while(simReceivedBytes - RADIO_HEADER_LEN - read_bytes < bytes) {
    simProcessRunValue = 1;
    cooja_mt_yield();
  }
  memcpy(buf, simInDataBuffer + RADIO_HEADER_LEN + read_bytes, bytes);
  read_bytes += bytes;
  return true;
}
/*---------------------------------------------------------------------------*/
static uint8_t
async_read_payload_bytes(void)
{
  return read_bytes;
}
/*---------------------------------------------------------------------------*/
static void
async_prepare_sequence(uint8_t *sequence, uint8_t sequence_len)
{
  memcpy(simOutDataBuffer,
      sequence + RADIO_SHR_LEN,
      sequence_len - RADIO_SHR_LEN);
  sequence_pos = sequence_len - RADIO_SHR_LEN;
  simIsSequence = 1;
}
/*---------------------------------------------------------------------------*/
static void
async_append_to_sequence(uint8_t *appendix, uint8_t appendix_len)
{
  uint_fast8_t i;

  for(i = 0; i < appendix_len; i++) {
    simOutDataBuffer[0x7F & sequence_pos++] = appendix[i];
  }
}
/*---------------------------------------------------------------------------*/
static void
async_transmit_sequence(void)
{
  async_transmit(false);
}
/*---------------------------------------------------------------------------*/
static void
async_finish_sequence(void)
{
  simStopSequence = 1;
  while(simStopSequence) {
    simProcessRunValue = 1;
    cooja_mt_yield();
  }
  ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
}
/*---------------------------------------------------------------------------*/
const struct radio_driver cooja_radio_driver =
{
    init,
    prepare_packet,
    transmit_packet,
    radio_send,
    radio_read,
    channel_clear,
    receiving_packet,
    pending_packet,
    radio_on,
    radio_off,
    get_value,
    set_value,
    get_object,
    set_object,
    async_enter,
    async_prepare,
    async_reprepare,
    async_transmit,
    async_on,
    async_off,
    async_set_shr_callback,
    async_set_fifop_callback,
    async_set_txdone_callback,
    async_read_phy_header,
    async_read_payload,
    async_read_payload_bytes,
    async_prepare_sequence,
    async_append_to_sequence,
    async_transmit_sequence,
    async_finish_sequence
};
/*---------------------------------------------------------------------------*/
