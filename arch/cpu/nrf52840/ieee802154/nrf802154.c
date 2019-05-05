/*
 * Copyright (C) 2019 University of Pisa
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
 */

/**
 * @ingroup     drivers_nrf52_802154
 * @{
 *
 * @file
 * @brief       Implementation of the 802154 radio driver for nRF52 radios
 *
 * @author      Carlo Vallati <carlo.vallati@unipi.it>
 * @}
 */

#define OUTPUT_POWER_MAX   0
#define OUTPUT_POWER_MIN -25

#include "nrf_drv_common.h"

#include "contiki.h"
#include "sys/clock.h"
#include "sys/rtimer.h"
#include "dev/leds.h"
#include "sys/mutex.h"

#include "net/packetbuf.h"
#include "net/netstack.h"

#define DEBUG 0

#include "net/net-debug.h"
#include "lib/assert.h"

#include <nrf.h>
#include <nrf_802154.h>

#include "ieee802154/ieee-addr.h"

#include "net/mac/tsch/tsch.h"

#include "nrf_802154_const.h"

static volatile bool m_tx_in_progress;
static volatile bool m_rx_in_progress;
static volatile bool m_tx_done;
static volatile bool m_rx_done;
static volatile bool m_cca_status;
static volatile bool m_cca_completed;
static volatile bool tx_on_cca;
static volatile bool tx_ok;
static volatile bool polling_enabled;

#define MAX_MESSAGE_SIZE 125 // Max message size that can be handled by the driver

uint8_t last_lqi;
uint32_t  last_time;
static uint8_t m_message[MAX_MESSAGE_SIZE];
uint8_t len;

linkaddr_t addr; // Current extended address

PROCESS(nrf52_process, "NRF52 driver"); // RX process

// Default values
#define CHANNEL 26
#define POWER 0
#define NRF52_CSMA_ENABLED 0
#define NRF52_AUTOACK_ENABLED 1

#define NRF52_MAX_TX_TIME RTIMER_SECOND / 25 // Long time for active wait, this should never happen
#define NRF52_MAX_CCA_TIME RTIMER_SECOND / 25 // Long time for active wait, this should never happen

static int nrf52_init()
{
	PRINTF("[nrf802154] Radio INIT\n");

	uint8_t p_pan_id[2];
	linkaddr_t linkaddr_node_addr;

	// Take care of endianess for pan-id and ext address
	ieee_addr_init(addr.u8, LINKADDR_SIZE);
	ieee_addr_cpy_to(linkaddr_node_addr.u8, addr.u8, LINKADDR_SIZE);
	pan_id_le(p_pan_id, IEEE802154_PANID);

	nrf_802154_init();

	m_tx_in_progress = 0;
	m_rx_in_progress = 0;
	m_tx_done = 1;
	m_rx_done = 0;
	tx_on_cca = 0;
	tx_ok = 0;

	polling_enabled = 0;

	// Set pan-id and address
	nrf_802154_pan_id_set(p_pan_id);
	nrf_802154_extended_address_set(linkaddr_node_addr.u8);

	// Set params
	nrf_802154_channel_set(CHANNEL);
	nrf_802154_tx_power_set(POWER);
	nrf_802154_auto_ack_set(NRF52_AUTOACK_ENABLED);

	// Initial status receive
	nrf_802154_receive();

	// Trigger RX process
	process_start(&nrf52_process, NULL);

	return 0;
}

static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
	nrf_802154_cca_cfg_t cca_cfg;
	switch(param) {
	case RADIO_PARAM_POWER_MODE:
		if(value == RADIO_POWER_MODE_ON) {
			nrf_802154_receive();
			return RADIO_RESULT_OK;
		}
		if(value == RADIO_POWER_MODE_OFF) {
			nrf_802154_sleep();
			return RADIO_RESULT_OK;
		}

		return RADIO_RESULT_INVALID_VALUE;
	case RADIO_PARAM_CHANNEL:
		if(value < 11 || value > 26) {
			return RADIO_RESULT_INVALID_VALUE;
		}
		nrf_802154_channel_set(value);

		return RADIO_RESULT_OK;
	case RADIO_PARAM_RX_MODE:
		if(value & ~(RADIO_RX_MODE_ADDRESS_FILTER |
				RADIO_RX_MODE_AUTOACK | RADIO_RX_MODE_POLL_MODE)) {
			return RADIO_RESULT_INVALID_VALUE;
		}
		nrf_802154_promiscuous_set((value & RADIO_RX_MODE_ADDRESS_FILTER) != 0);
		nrf_802154_auto_ack_set((value & RADIO_RX_MODE_AUTOACK) != 0);
		polling_enabled = ((value & RADIO_RX_MODE_POLL_MODE) != 0);
		return RADIO_RESULT_OK;
	case RADIO_PARAM_TX_MODE:

		if(value & ~(RADIO_TX_MODE_SEND_ON_CCA)) {
			return RADIO_RESULT_INVALID_VALUE;
		}
		tx_on_cca = ((value & RADIO_TX_MODE_SEND_ON_CCA) != 0);
		return RADIO_RESULT_OK;
	case RADIO_PARAM_TXPOWER:
		if(value < OUTPUT_POWER_MIN || value > OUTPUT_POWER_MAX) {
			return RADIO_RESULT_INVALID_VALUE;
		}
		nrf_802154_tx_power_set(value);
		return RADIO_RESULT_OK;
	case RADIO_PARAM_CCA_THRESHOLD:

		cca_cfg.mode = NRF_RADIO_CCA_MODE_CARRIER_AND_ED;
		cca_cfg.ed_threshold = (uint8_t) value;
		cca_cfg.corr_threshold = 0;
		cca_cfg.corr_limit = 0;

		nrf_802154_cca_cfg_set(&cca_cfg);

		return RADIO_RESULT_OK;
	default:
		return RADIO_RESULT_NOT_SUPPORTED;
	}
}


static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
	nrf_802154_cca_cfg_t cca_cfg;

	if(!value) {
		return RADIO_RESULT_INVALID_VALUE;
	}
	switch(param) {
	case RADIO_PARAM_POWER_MODE:
		if(nrf_802154_state_get() == NRF_802154_STATE_SLEEP)
			*value = RADIO_POWER_MODE_CARRIER_OFF;
		else
			*value = RADIO_POWER_MODE_CARRIER_ON;
		return RADIO_RESULT_OK;
	case RADIO_PARAM_CHANNEL:
		*value = nrf_802154_channel_get();
		return RADIO_RESULT_OK;
	case RADIO_PARAM_RX_MODE:
		*value = 0;
		if(nrf_802154_promiscuous_get()) {
			*value |= RADIO_RX_MODE_ADDRESS_FILTER;
		}
		if(nrf_802154_auto_ack_get()) {
			*value |= RADIO_RX_MODE_AUTOACK;
		}
		if(polling_enabled) {
			*value |= RADIO_RX_MODE_POLL_MODE;
		}

		return RADIO_RESULT_OK;
	case RADIO_PARAM_TX_MODE:
		*value = 0;
		if(tx_on_cca) {
			*value |= RADIO_TX_MODE_SEND_ON_CCA;
		}
		return RADIO_RESULT_OK;
		return RADIO_RESULT_OK;
	case RADIO_PARAM_TXPOWER:
		*value = nrf_802154_tx_power_get();
		return RADIO_RESULT_OK;
	case RADIO_PARAM_CCA_THRESHOLD:

		nrf_802154_cca_cfg_get(&cca_cfg);

		*value = cca_cfg.ed_threshold;
		return RADIO_RESULT_OK;
	case RADIO_PARAM_RSSI:
		/* Return the RSSI value in dBm */
		*value = nrf_802154_rssi_last_get();
		return RADIO_RESULT_OK;
	case RADIO_PARAM_LAST_RSSI:
		/* RSSI of the last packet received */
		*value = nrf_802154_rssi_last_get();
		return RADIO_RESULT_OK;
	case RADIO_PARAM_LAST_LINK_QUALITY:
		/* LQI of the last packet received */
		*value = last_lqi;
		return RADIO_RESULT_OK;
	case RADIO_CONST_CHANNEL_MIN:
		*value = 11;
		return RADIO_RESULT_OK;
	case RADIO_CONST_CHANNEL_MAX:
		*value = 26;
		return RADIO_RESULT_OK;
	case RADIO_CONST_TXPOWER_MIN:
		*value = OUTPUT_POWER_MIN;
		return RADIO_RESULT_OK;
	case RADIO_CONST_TXPOWER_MAX:
		*value = OUTPUT_POWER_MAX;
		return RADIO_RESULT_OK;
	default:
		return RADIO_RESULT_NOT_SUPPORTED;
	}
}

static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{

	if(param == RADIO_PARAM_64BIT_ADDR) {
		if(size != LINKADDR_SIZE || !dest) {
			return RADIO_RESULT_INVALID_VALUE;
		}

		memcpy(dest, addr.u8, LINKADDR_SIZE);

		return RADIO_RESULT_OK;
	}

	if(param == RADIO_PARAM_LAST_PACKET_TIMESTAMP) {
		if(size != sizeof(rtimer_clock_t) || !dest) {
			return RADIO_RESULT_INVALID_VALUE;
		}
		*(rtimer_clock_t *)dest = last_time;
		return RADIO_RESULT_OK;
	}

#if MAC_CONF_WITH_TSCH
	if(param == RADIO_CONST_TSCH_TIMING) {
		if(size != sizeof(uint16_t *) || !dest) {
			return RADIO_RESULT_INVALID_VALUE;
		}
		*(const uint16_t **)dest = tsch_timeslot_timing_us_10000;
		return RADIO_RESULT_OK;
	}
#endif /* MAC_CONF_WITH_TSCH */

	return RADIO_RESULT_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
	linkaddr_t linkaddr_node_addr;

	if(param == RADIO_PARAM_64BIT_ADDR) {
		if(size != LINKADDR_SIZE || !src) {
			return RADIO_RESULT_INVALID_VALUE;
		}

		memcpy(&addr, src ,LINKADDR_SIZE);

		// Take care of endianess for the ext address
		ieee_addr_cpy_to(linkaddr_node_addr.u8, addr.u8, LINKADDR_SIZE);

		nrf_802154_extended_address_set(linkaddr_node_addr.u8);


		return RADIO_RESULT_OK;
	}
	return RADIO_RESULT_NOT_SUPPORTED;
}

static int nrf52_cca(void){
	PRINTF("[nrf802154] CCA\n");

	m_cca_status = 0;
	m_cca_completed = 0;

	nrf_802154_cca();

	RTIMER_BUSYWAIT_UNTIL(m_cca_completed, NRF52_MAX_CCA_TIME);

	PRINTF("[nrf802154] RESULT %u %u\n", m_cca_completed , m_cca_status);

	return m_cca_completed && m_cca_status;
}

static int nrf52_prepare(const void *payload, unsigned short payload_len){
	PRINTF("[nrf802154] Prepare %u\n", payload_len);

	memcpy(m_message, payload, payload_len);

	m_tx_in_progress = 0;
	m_tx_done        = 0;

	return 0;
}

static int nrf52_transmit(unsigned short len){

	PRINTF("[nrf802154] Transmit %u\n", len);

	if(tx_on_cca){ // If needed perform CCA
		if(!nrf52_cca()){
			m_tx_in_progress = 0;
			return RADIO_TX_COLLISION;
		}
	}

	m_tx_in_progress = nrf_802154_transmit(m_message, len, 0);

	if(m_tx_in_progress){
		RTIMER_BUSYWAIT_UNTIL(m_tx_done, NRF52_MAX_TX_TIME);

		if(tx_ok){
			m_tx_in_progress = 0;
			PRINTF("[nrf802154] TX OK\n");
			return RADIO_TX_OK;
		} else {
			PRINTF("[nrf802154] TX FAILED\n");
			m_tx_in_progress = 0;
			return RADIO_TX_NOACK;
		}
	}


	PRINTF("[nrf802154] TX COLLISION\n");
	m_tx_in_progress = 0;
	return RADIO_TX_COLLISION;
}

static int nrf52_send(const void *payload, unsigned short payload_len)
{
	PRINTF("[nrf802154] Send\n");
	nrf52_prepare(payload, payload_len);
	return nrf52_transmit(payload_len);
}

static int nrf52_receiving_packet(void){

	return m_rx_in_progress;

}

static int nrf52_pending_packet(void){
	return m_rx_done;
}

static int nrf52_off(void){

	nrf_802154_sleep();

	return 0;
}

static int nrf52_on(void){

	nrf_802154_receive();

	return 0;
}

static int
nrf52_read(void *buf, unsigned short bufsize)
{

	memcpy((void *)buf, (const void *) m_message, len);

	m_rx_done = 0;

	return len;
}


/**
 * @brief   Export of the netdev interface
 */
const struct radio_driver nrf52840_driver =
{
		nrf52_init, /* Initialize radio. */
		nrf52_prepare, /* Prepare the radio with a packet to be sent. */
		nrf52_transmit, /* Send the packet that has previously been prepared. */
		nrf52_send, /* Prepare & transmit a packet. */
		nrf52_read, /* Upper layer read from buffer (used for ACKs) */
		nrf52_cca, /*Perform a Clear-Channel Assessment (CCA) to find out if there is a packet in the air or not. */
		nrf52_receiving_packet, /* Check if the radio driver is currently receiving a packet. */
		nrf52_pending_packet, /* Check if the radio driver has just received a packet. */
		nrf52_on, /* turn on */
		nrf52_off, /* turn off */
		get_value,
		set_value,
		get_object,
		set_object
};

// RX Process
PROCESS_THREAD(nrf52_process, ev, data)
{
	PROCESS_BEGIN();

	PRINTF("nrf52_process: started\n");

	while(1) {
		PROCESS_YIELD_UNTIL(!polling_enabled && ev == PROCESS_EVENT_POLL);

		if(m_rx_done == 0)
			continue;

		packetbuf_clear();

		nrf52_read(packetbuf_dataptr(), PACKETBUF_SIZE);

		packetbuf_set_datalen(len);

		NETSTACK_MAC.input();

	}

	PROCESS_END();
}

/********         CALLBACKS from NSD          ********/

// RX
void nrf_802154_received(uint8_t * p_data, uint8_t length, int8_t power, uint8_t lqi)
{
	if (length > MAX_MESSAGE_SIZE || m_rx_done == 1 )
	{
		goto exit;
	}

	memcpy(m_message, p_data, length);
	len = length - 2;
	last_lqi = lqi;

	m_rx_done = 1;
	m_rx_in_progress = 0;

	process_poll(&nrf52_process);

	exit:
	nrf_802154_buffer_free(p_data);

	return;
}

// TX OK
void nrf_802154_transmitted(const uint8_t * p_frame, uint8_t * p_ack, uint8_t length, int8_t power, uint8_t lqi)
{

	m_tx_done = 1;
	last_lqi = lqi;

	tx_ok = 1;

	if (p_ack != NULL)
	{
		m_rx_done = 1;
		m_rx_in_progress = 0;

		memcpy(m_message, p_ack, length);
		len = length - 2;

		nrf_802154_buffer_free(p_ack);
	}

}

// TX FAILED
void nrf_802154_transmit_failed(const uint8_t * p_frame,
		nrf_802154_tx_error_t error){

	tx_ok = 0;

	m_tx_done = 1;

}

// CCA Result
void nrf_802154_cca_done(bool channel_free){
	m_cca_status = channel_free;
	m_cca_completed = 1;
}

// RX Started
void nrf_802154_rx_started(void){
	m_rx_in_progress = 1;
	last_time = RTIMER_NOW();
}

void nrf_802154_receive_failed(nrf_802154_rx_error_t error){
	m_rx_in_progress = 0;
}
