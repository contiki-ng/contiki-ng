/*
 * Copyright (c) 2015, Nordic Semiconductor
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
/**
 * \addtogroup nrf52840-ble
 * @{
 *
 * \file
 *         Basic BLE functions.
 * \author
 *         Wojciech Bober <wojciech.bober@nordicsemi.no>
 *         Carlo Vallati <carlo.vallati@unipi.it>
 *
 */
#include <stdbool.h>
#include <stdint.h>
#include "boards.h"
#include "nordic_common.h"
#include "nrf_delay.h"
#include "nrf_sdm.h"
#include "ble_advdata.h"
#include "ble_srv_common.h"
#include "ble_ipsp.h"
#include "app_error.h"
#include "iot_defines.h"
#include "ble-core.h"

#include "nrf_soc.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "iot_common.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define APP_IPSP_TAG                        35                                                      /**< Identifier for L2CAP configuration with the softdevice. */

#define IS_SRVC_CHANGED_CHARACT_PRESENT 1
#define APP_IPSP_ACCEPTOR_PRIO              1                                                       /**< Priority with the SDH on receiving events from the softdevice. */
#define APP_ADV_TIMEOUT                 0                                  /**< Time for which the device must be advertising in non-connectable mode (in seconds). 0 disables timeout. */
#define APP_ADV_ADV_INTERVAL            MSEC_TO_UNITS(333, UNIT_0_625_MS)  /**< The advertising interval. This value can vary between 100ms to 10.24s). */

static ble_gap_adv_params_t m_adv_params; /**< Parameters to be passed to the stack when starting advertising. */

static uint8_t              m_enc_advdata[BLE_GAP_ADV_SET_DATA_SIZE_MAX];                           /**< Buffer for storing an encoded advertising set. */
static uint8_t              m_adv_handle = BLE_GAP_ADV_SET_HANDLE_NOT_SET;                          /**< Advertising handle used to identify an advertising set. */


/**@brief Struct that contains pointers to the encoded advertising data. */
static ble_gap_adv_data_t m_adv_data =
{
    .adv_data =
    {
        .p_data = m_enc_advdata,
        .len    = BLE_GAP_ADV_SET_DATA_SIZE_MAX
    },
    .scan_rsp_data =
    {
        .p_data = NULL,
        .len    = 0

    }
};


static void
ble_evt_dispatch(ble_evt_t const * p_ble_evt, void * p_context);
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialize and enable the BLE stack.
 */
void
ble_stack_init(void)
{
  uint32_t err_code;
  uint32_t     ram_start = 0;
  ble_cfg_t    ble_cfg;

  // Enable BLE stack.
  nrf_sdh_enable_request();

  // Fetch the start address of the application RAM.
  err_code = nrf_sdh_ble_app_ram_start_get(&ram_start);

  if (err_code == NRF_SUCCESS)
  {
      // Configure the maximum number of connections.
      memset(&ble_cfg, 0, sizeof(ble_cfg));
      ble_cfg.gap_cfg.role_count_cfg.periph_role_count  = BLE_IPSP_MAX_CHANNELS;
      ble_cfg.gap_cfg.role_count_cfg.central_role_count = 0;
      ble_cfg.gap_cfg.role_count_cfg.central_sec_count  = 0;
      err_code = sd_ble_cfg_set(BLE_GAP_CFG_ROLE_COUNT, &ble_cfg, ram_start);
  }

  if (err_code == NRF_SUCCESS)
  {
      memset(&ble_cfg, 0, sizeof(ble_cfg));

      // Configure total number of connections.
      ble_cfg.conn_cfg.conn_cfg_tag                     = APP_IPSP_TAG;
      ble_cfg.conn_cfg.params.gap_conn_cfg.conn_count   = BLE_IPSP_MAX_CHANNELS;
      ble_cfg.conn_cfg.params.gap_conn_cfg.event_length = BLE_GAP_EVENT_LENGTH_DEFAULT;
      err_code = sd_ble_cfg_set(BLE_CONN_CFG_GAP, &ble_cfg, ram_start);

  }

  if (err_code ==  NRF_SUCCESS)
  {
      memset(&ble_cfg, 0, sizeof(ble_cfg));

       // Configure the number of custom UUIDS.
      ble_cfg.common_cfg.vs_uuid_cfg.vs_uuid_count = 0;
      err_code = sd_ble_cfg_set(BLE_COMMON_CFG_VS_UUID, &ble_cfg, ram_start);
  }

  if (err_code == NRF_SUCCESS)
  {
      memset(&ble_cfg, 0, sizeof(ble_cfg));

      // Set L2CAP channel configuration

      // @note The TX MPS and RX MPS of initiator and acceptor are not symmetrically set.
      // This will result in effective MPS of 50 in reach direction when using the initiator and
      // acceptor example against each other. In the IPSP, the TX MPS is set to a higher value
      // as Linux which is the border router for 6LoWPAN examples uses default RX MPS of 230
      // bytes. Setting TX MPS of 212 in place of 50 results in better credit and hence bandwidth
      // utilization.
      ble_cfg.conn_cfg.conn_cfg_tag                        = APP_IPSP_TAG;
      ble_cfg.conn_cfg.params.l2cap_conn_cfg.rx_mps        = BLE_IPSP_RX_MPS;
      ble_cfg.conn_cfg.params.l2cap_conn_cfg.rx_queue_size = BLE_IPSP_RX_BUFFER_COUNT;
      ble_cfg.conn_cfg.params.l2cap_conn_cfg.tx_mps        = BLE_IPSP_TX_MPS;
      ble_cfg.conn_cfg.params.l2cap_conn_cfg.tx_queue_size = 1;
      ble_cfg.conn_cfg.params.l2cap_conn_cfg.ch_count      = 1; // One L2CAP channel per link.
      err_code = sd_ble_cfg_set(BLE_CONN_CFG_L2CAP, &ble_cfg, ram_start);
  }

  if (err_code == NRF_SUCCESS)
  {
      memset(&ble_cfg, 0, sizeof(ble_cfg));

      // Set the ATT table size.
      ble_cfg.gatts_cfg.attr_tab_size.attr_tab_size = 256;
      err_code = sd_ble_cfg_set(BLE_GATTS_CFG_ATTR_TAB_SIZE, &ble_cfg, ram_start);
  }


  if (err_code ==  NRF_SUCCESS)
  {
      err_code = nrf_sdh_ble_enable(&ram_start);
  }

  if (err_code ==  NRF_SUCCESS)
  {
	  NRF_SDH_BLE_OBSERVER(m_ble_evt_observer, APP_IPSP_ACCEPTOR_PRIO, ble_evt_dispatch, NULL);
  }

  APP_ERROR_CHECK(err_code);

  // Setup address
  ble_gap_addr_t ble_addr;
  err_code = sd_ble_gap_addr_get(&ble_addr);
  APP_ERROR_CHECK(err_code);

  /*ble_addr.addr[5]   = 0x00;
  ble_addr.addr[4]   = 0x11;
  ble_addr.addr[3]   = 0x22;
  ble_addr.addr[2]   = 0x33;
  ble_addr.addr[1]   = 0x44;
  ble_addr.addr[0]   = 0x55;*/
  ble_addr.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;

  err_code = sd_ble_gap_addr_set(&ble_addr);
  APP_ERROR_CHECK(err_code);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Return device EUI64 MAC address
 * \param addr pointer to a buffer to store the address
 */
void
ble_get_mac(uint8_t addr[8])
{
  uint32_t err_code;
  ble_gap_addr_t ble_addr;

  err_code = sd_ble_gap_addr_get(&ble_addr);
  APP_ERROR_CHECK(err_code);

  IPV6_EUI64_CREATE_FROM_EUI48(addr, ble_addr.addr, ble_addr.addr_type);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialize BLE advertising data.
 * \param name Human readable device name that will be advertised
 */
void
ble_advertising_init(const char *name)
{
  uint32_t err_code;
  ble_advdata_t advdata;
  uint8_t flags = BLE_GAP_ADV_FLAG_BR_EDR_NOT_SUPPORTED;
  ble_gap_conn_sec_mode_t sec_mode;

  BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

  err_code = sd_ble_gap_device_name_set(&sec_mode, (const uint8_t *)name,
                                        strlen(name));
  APP_ERROR_CHECK(err_code);

  ble_uuid_t adv_uuids[] = {{BLE_UUID_IPSP_SERVICE, BLE_UUID_TYPE_BLE}};

  // Build and set advertising data.
  memset(&advdata, 0, sizeof(advdata));

  advdata.name_type = BLE_ADVDATA_FULL_NAME;
  advdata.flags = flags;
  advdata.uuids_complete.uuid_cnt = sizeof(adv_uuids) / sizeof(adv_uuids[0]);
  advdata.uuids_complete.p_uuids = adv_uuids;

  err_code = ble_advdata_encode(&advdata, m_adv_data.adv_data.p_data, &m_adv_data.adv_data.len);
  APP_ERROR_CHECK(err_code);

  // Initialize advertising parameters (used when starting advertising).
  memset(&m_adv_params, 0, sizeof(m_adv_params));

  m_adv_params.properties.type = BLE_GAP_ADV_TYPE_CONNECTABLE_SCANNABLE_UNDIRECTED;
  m_adv_params.p_peer_addr     = NULL;                                              // Undirected advertisement.
  m_adv_params.filter_policy   = BLE_GAP_ADV_FP_ANY;
  m_adv_params.interval        = APP_ADV_ADV_INTERVAL;
  m_adv_params.duration        = APP_ADV_TIMEOUT;

  err_code = sd_ble_gap_adv_set_configure(&m_adv_handle, &m_adv_data, &m_adv_params);
  APP_ERROR_CHECK(err_code);

}
/*---------------------------------------------------------------------------*/
/**
 * \brief Start BLE advertising.
 */
void
ble_advertising_start(void)
{
  uint32_t err_code;

  err_code = sd_ble_gap_adv_start(m_adv_handle, APP_IPSP_TAG);
  APP_ERROR_CHECK(err_code);

  PRINTF("ble-core: advertising started\n");
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Print GAP address.
 * \param addr a pointer to address
 */
void
ble_gap_addr_print(const ble_gap_addr_t *addr)
{
  unsigned int i;
  for(i = 0; i < sizeof(addr->addr); i++) {
    if(i > 0) {
      PRINTF(":");
    }PRINTF("%02x", addr->addr[i]);
  }PRINTF(" (%d)", addr->addr_type);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Function for handling the Application's BLE Stack events.
 * \param[in]   p_ble_evt   Bluetooth stack event.
 */
static void
on_ble_evt(ble_evt_t const *p_ble_evt)
{
  switch(p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
      PRINTF("ble-core: connected [handle:%d, peer: ", p_ble_evt->evt.gap_evt.conn_handle);
      ble_gap_addr_print(&(p_ble_evt->evt.gap_evt.params.connected.peer_addr));
      PRINTF("]\n");
      sd_ble_gap_rssi_start(p_ble_evt->evt.gap_evt.conn_handle,
                            BLE_GAP_RSSI_THRESHOLD_INVALID,
                            0);
      break;

    case BLE_GAP_EVT_DISCONNECTED:
      PRINTF("ble-core: disconnected [handle:%d]\n", p_ble_evt->evt.gap_evt.conn_handle);
      ble_advertising_start();
      break;
    default:
      break;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief SoftDevice BLE event callback.
 * \param[in]   p_ble_evt   Bluetooth stack event.
 */
static void
ble_evt_dispatch(ble_evt_t const *p_ble_evt, void * p_context)
{
  ble_ipsp_evt_handler(p_ble_evt);
  on_ble_evt(p_ble_evt);
}
/*---------------------------------------------------------------------------*/
/** @} */
