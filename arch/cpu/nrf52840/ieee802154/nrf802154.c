/*
 * Copyright (C) 2019 Freie Universität Berlin
 *               2019 HAW Hamburg
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_nrf52_802154
 * @{
 *
 * @file
 * @brief       Implementation of the radio driver for nRF52 radios
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Dimitri Nahm <dimitri.nahm@haw-hamburg.de>
 * @author      Semjon Kerner <semjon.kerner@fu-berlin.de>
 * @}
 */

// TODO Check values!
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

#define DEBUG 1
#include "net/net-debug.h"

#include "lib/assert.h"

#include <nrf.h>
#include <nrf_802154.h>

static int nrf52_init()
{
	PRINTF("[nrf802154] Radio INIT\n");
	nrf_802154_init();
    return 0;
}

static int nrf52_send(const void *payload, unsigned short payload_len)
{
    PRINTF("[nrf802154] Send\n");
    return 0;
}

static int nrf52_recv( void *buf, unsigned short bufsize)
{
	PRINTF("[nrf802154] Recv\n");


    return 0;
}


static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{

  switch(param) {
  case RADIO_PARAM_POWER_MODE:
    if(value == RADIO_POWER_MODE_ON) {
      // TODO TURN RADIO ON?
      return RADIO_RESULT_OK;
    }
    if(value == RADIO_POWER_MODE_OFF) {
      // TODO TURN RADIO OFF?
      return RADIO_RESULT_OK;
    }
    if(value == RADIO_POWER_MODE_CARRIER_ON ||
       value == RADIO_POWER_MODE_CARRIER_OFF) {
      //set_test_mode((value == RADIO_POWER_MODE_CARRIER_ON), 0); // TODO
      return RADIO_RESULT_OK;
    }
    return RADIO_RESULT_INVALID_VALUE;
  case RADIO_PARAM_CHANNEL:
    if(value < 11 || value > 26) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    //cc2420_set_channel(value); // TODO
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RX_MODE:
	  // TODO
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    if(value & ~(RADIO_TX_MODE_SEND_ON_CCA)) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    // set_send_on_cca((value & RADIO_TX_MODE_SEND_ON_CCA) != 0); // TODO
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    if(value < OUTPUT_POWER_MIN || value > OUTPUT_POWER_MAX) {
      return RADIO_RESULT_INVALID_VALUE;
    }
    // TODO

    return RADIO_RESULT_OK;
  case RADIO_PARAM_CCA_THRESHOLD:
    // cc2420_set_cca_threshold(value - RSSI_OFFSET); // TODO
    return RADIO_RESULT_OK;
  default:
    return RADIO_RESULT_NOT_SUPPORTED;
  }
}


static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{

  if(!value) {
    return RADIO_RESULT_INVALID_VALUE;
  }
  switch(param) {
  case RADIO_PARAM_POWER_MODE:
	*value = RADIO_POWER_MODE_CARRIER_ON; // TODO
    return RADIO_RESULT_OK;
  case RADIO_PARAM_CHANNEL:
    *value = 0;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RX_MODE:
    *value = 0;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TX_MODE:
    *value = 0;
    return RADIO_RESULT_OK;
  case RADIO_PARAM_TXPOWER:
    *value = 0;

    return RADIO_RESULT_OK;
  case RADIO_PARAM_CCA_THRESHOLD:
    *value = 0; // TODO
    return RADIO_RESULT_OK;
  case RADIO_PARAM_RSSI:
    /* Return the RSSI value in dBm */
    *value = 0; // TODO
    return RADIO_RESULT_OK;
  case RADIO_PARAM_LAST_RSSI:
    /* RSSI of the last packet received */
    *value = 0; // TODO SET RSSI VALUE
    return RADIO_RESULT_OK;
  case RADIO_PARAM_LAST_LINK_QUALITY:
    /* LQI of the last packet received */
    *value = 0; // TODO SET LQI VALUE
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


static int nrf52_prepare(const void *payload, unsigned short payload_len){
	PRINTF("[nrf802154] Prepare\n");
    return 0;
}

static int nrf52_transmit(unsigned short len){
	PRINTF("[nrf802154] Transmit\n");
	return 0;
}

static int nrf52_cca(void){
	PRINTF("[nrf802154] CCA\n");
	return 0;
}

static int nrf52_receiving_packet(void){
	PRINTF("[nrf802154] Receiving\n");
	return 0;
}

static int nrf52_pending_packet(void){
	PRINTF("[nrf802154] Pending\n");
	return 0;
}

static int nrf52_off(void){
	PRINTF("[nrf802154] Off\n");
    nrf_802154_sleep();
    nrf_802154_deinit();
	return 0;
}

static int nrf52_on(void){
	PRINTF("[nrf802154] On\n");
    nrf_802154_init();
	return 0;
}


/**
 * @brief   Export of the netdev interface
 */
const struct radio_driver nrf52840_driver =
  {
    nrf52_init,
    nrf52_prepare,
	nrf52_transmit,
	nrf52_send,
	nrf52_recv,
	nrf52_cca,
	nrf52_receiving_packet,
	nrf52_pending_packet,
	nrf52_on,
    nrf52_off,
    get_value,
    set_value,
    NULL,
    NULL
};


