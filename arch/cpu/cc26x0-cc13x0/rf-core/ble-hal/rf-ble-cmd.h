/*
 * Copyright (c) 2017, Graz University of Technology
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
 * \file
 *    BLE commands for the TI CC26xx BLE radio.
 *    These functions are specific to the TI CC26xx and cannot be
 *    reused by other BLE radios.
 *
 * \author
 *    Michael Spoerk <michael.spoerk@tugraz.at>
 */
/*---------------------------------------------------------------------------*/

#ifndef RF_BLE_CMD_H_
#define RF_BLE_CMD_H_

#include "os/dev/ble-hal.h"
#include "../../ble-addr.h"
#include "rf_common_cmd.h"

#define RF_BLE_CMD_OK    1
#define RF_BLE_CMD_ERROR 0

/*---------------------------------------------------------------------------*/
/**
 * \brief Sends a BLE radio command to the radio
 * \param cmd A pointer to the command to be send
 * \return RF_CORE_CMD_OK or RF_CORE_CMD_ERROR
 */
unsigned short rf_ble_cmd_send(uint8_t *cmd);

/*---------------------------------------------------------------------------*/
/**
 * \brief Waits for a running BLE radio command to be finished
 * \param cmd A pointer to the running command
 * \return RF_CORE_CMD_OK or RF_CORE_CMD_ERROR
 */
unsigned short rf_ble_cmd_wait(uint8_t *cmd);

/*---------------------------------------------------------------------------*/
/**
 * \brief Initializes the radio core to be used as a BLE radio
 * \return RF_CORE_CMD_OK or RF_CORE_CMD_ERROR
 */
unsigned short rf_ble_cmd_setup_ble_mode(void);

/*---------------------------------------------------------------------------*/
/**
 * \brief Creates a BLE radio command structure that enables
 *      BLE advertisement when sent to the radio core
 * \param command A pointer to command that is created
 * \param channel The BLE advertisement channel used for advertisement
 * \param param   A pointer to the radio command parameters
 * \param output  A pointer to the radio command output
 */
void rf_ble_cmd_create_adv_cmd(uint8_t *command, uint8_t channel,
                               uint8_t *param, uint8_t *output);

/*---------------------------------------------------------------------------*/
/**
 * \brief Creates BLE radio command parameters that are used to enable
 *      BLE advertisement on the radio core
 * \param param	    A pointer to parameter structure that is created
 * \param rx_queue  A pointer to the RX queue that is used
 * \param adv_data_len
 *          The length of the advertisement data
 * \param adv_data  A pointer to the advertisement data that is advertised
 * \param scan_resp_data_len
 *          The length of the scan response data
 * \param scan_resp_data
 *          A pointer to the scan response data
 * \param own_addr_type
 *          Either BLE_ADDR_TYPE_PUBLIC or BLE_ADDR_TYPE_RANDOM
 * \param own_addr A pointer to the device address that is used as own address
 */
void rf_ble_cmd_create_adv_params(uint8_t *param, dataQueue_t *rx_queue,
                                  uint8_t adv_data_len, uint8_t *adv_data,
                                  uint8_t scan_resp_data_len, uint8_t *scan_resp_data,
                                  ble_addr_type_t own_addr_type, uint8_t *own_addr);

/*---------------------------------------------------------------------------*/
/**
 * \brief Creates a BLE radio command structure that sets up
 *      BLE initiation event when sent to the radio core
 * \param cmd     A pointer to command that is created
 * \param channel The BLE data channel used for the connection event
 * \param params   A pointer to the radio command parameters
 * \param output  A pointer to the radio command output
 * \param start_time
 *          The time in rf_core_ticks when the connection event will start
 */
void rf_ble_cmd_create_initiator_cmd(uint8_t *cmd, uint8_t channel, uint8_t *params,
                                     uint8_t *output, uint32_t start_time);

/*---------------------------------------------------------------------------*/
/**
 * \brief Creates BLE radio command parameters that are used to set up
 *      BLE initiation event on the radio core
 * \param param      A pointer to parameter structure that is created
 * \param rx_queue  A pointer to the RX queue that is used
 * \param initiator_window
 *          T
 * \param own_addr_type
 *          Either BLE_ADDR_TYPE_PUBLIC or BLE_ADDR_TYPE_RANDOM
 * \param own_addr A pointer to the device address that is used as own address
 * \param peer_addr_type
 *          Either BLE_ADDR_TYPE_PUBLIC or BLE_ADDR_TYPE_RANDOM
 * \param peer_addr A pointer to the device address that is used as peer address
 * \param connect_time
            The first possible start time of the first connection event
 * \param conn_req_data  A pointer to the connect request data
 */
void rf_ble_cmd_create_initiator_params(uint8_t *param, dataQueue_t *rx_queue,
                                        uint32_t initiator_window,
                                        ble_addr_type_t own_addr_type, uint8_t *own_addr,
                                        ble_addr_type_t peer_addr_type, uint8_t *peer_addr,
                                        uint32_t connect_time,
                                        uint8_t *conn_req_data);

/*---------------------------------------------------------------------------*/
/**
 * \brief Creates a BLE radio command structure that sets up a single
 *      BLE connection event when sent to the radio core
 * \param cmd     A pointer to command that is created
 * \param channel The BLE data channel used for the connection event
 * \param param   A pointer to the radio command parameters
 * \param output  A pointer to the radio command output
 * \param start_time
 *          The time in rf_core_ticks when the connection event will start
 */
void rf_ble_cmd_create_slave_cmd(uint8_t *cmd, uint8_t channel, uint8_t *param,
                                 uint8_t *output, uint32_t start_time);

/*---------------------------------------------------------------------------*/
/**
 * \brief Creates BLE radio command parameters that are used to setup a single
 *      BLE connection event on the radio core
 * \param param	    A pointer to parameter structure that is created
 * \param rx_queue  A pointer to the RX queue that is used
 * \param tx_queue  A pointer to the TX queue that is used
 * \param access_address
 *          The access address of the used BLE connection
 * \param crc_init_0
 *          Part of the initialization of the CRC checksum
 * \param crc_init_1
 *          Part of the initialization of the CRC checksum
 * \param crc_init_2
 *          Part of the initialization of the CRC checksum
 * \param win_size  The window size parameter of the BLE connection event
 * \param window_widening
 *          The window widening parameter used for this connection event
 * \param first_packet
 *          1 for the first packet of the BLE connection so that the
 *          connection is properly initialized
 */
void rf_ble_cmd_create_slave_params(uint8_t *param, dataQueue_t *rx_queue,
                                    dataQueue_t *tx_queue, uint32_t access_address,
                                    uint8_t crc_init_0, uint8_t crc_init_1,
                                    uint8_t crc_init_2, uint32_t win_size,
                                    uint32_t window_widening, uint8_t first_packet);

/*---------------------------------------------------------------------------*/
/**
 * \brief Creates a BLE radio command structure that sets up
 *      BLE connection event when sent to the radio core
 * \param cmd     A pointer to command that is created
 * \param channel The BLE data channel used for the connection event
 * \param params   A pointer to the radio command parameters
 * \param output  A pointer to the radio command output
 * \param start_time
 *          The time in rf_core_ticks when the connection event will start
 */
void rf_ble_cmd_create_master_cmd(uint8_t *cmd, uint8_t channel, uint8_t *params,
                                  uint8_t *output, uint32_t start_time);

/*---------------------------------------------------------------------------*/
/**
 * \brief Creates BLE radio command parameters that are used to set up
 *      BLE connection event on the radio core
 * \param params      A pointer to parameter structure that is created
 * \param rx_queue  A pointer to the RX queue that is used
 * \param tx_queue  A pointer to the TX queue that is used
 * \param access_address
 *          The access address of the used BLE connection
 * \param crc_init_0
 *          Part of the initialization of the CRC checksum
 * \param crc_init_1
 *          Part of the initialization of the CRC checksum
 * \param crc_init_2
 *          Part of the initialization of the CRC checksum
 * \param first_packet
 *          1 for the first packet of the BLE connection so that the
 *          connection is properly initialized
 */
void rf_ble_cmd_create_master_params(uint8_t *params, dataQueue_t *rx_queue,
                                     dataQueue_t *tx_queue, uint32_t access_address,
                                     uint8_t crc_init_0, uint8_t crc_init_1,
                                     uint8_t crc_init_2, uint8_t first_packet);

/*---------------------------------------------------------------------------*/
/**
 * \brief Adds a data buffer to a BLE transmission queue
 * \param q	    A pointer to BLE transmission queue where the buffer
 *        should be added
 * \param e	    A pointer to the data buffer that is added
 */
unsigned short rf_ble_cmd_add_data_queue_entry(dataQueue_t *q, uint8_t *e);

#endif /* RF_BLE_CMD_H_ */
