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
 * \defgroup cc13xx-cc26xx-rf-ble CC13xx/CC26xx BLE Beacon Daemon
 *
 * @{
 *
 * \file
 *        Header file for the CC13xx/CC26xx BLE Beacon Daemon.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef RF_BLE_BEACOND_H_
#define RF_BLE_BEACOND_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
typedef enum {
  RF_BLE_BEACOND_OK,
  RF_BLE_BEACOND_ERROR,
  RF_BLE_BEACOND_DISABLED,
} rf_ble_beacond_result_t;
/*---------------------------------------------------------------------------*/
/**
 * \brief Initialize the BLE advertisement/beacon daemon
 */
rf_ble_beacond_result_t rf_ble_beacond_init(void);

/**
 * \brief Set the device name to use with the BLE advertisement/beacon daemon
 * \param interval The interval (ticks) between two consecutive beacon bursts
 * \param name The device name to advertise
 *
 * If name is NULL it will be ignored. If interval==0 it will be ignored. Thus,
 * this function can be used to configure a single parameter at a time if so
 * desired.
 */
rf_ble_beacond_result_t rf_ble_beacond_config(clock_time_t interval, const char *name);

/**
 * \brief  Start the BLE advertisement/beacon daemon
 * \return RF_CORE_CMD_OK: Success, RF_CORE_CMD_ERROR: Failure
 *
 * Before calling this function, the name to advertise must first be set by
 * calling rf_ble_beacond_config(). Otherwise, this function will return an
 * error.
 */
rf_ble_beacond_result_t rf_ble_beacond_start(void);

/**
 * \brief Stop the BLE advertisement/beacon daemon
 */
rf_ble_beacond_result_t rf_ble_beacond_stop(void);

/**
 * \brief Check whether the BLE beacond is currently active
 * \retval  1 BLE daemon is active
 * \retval  0 BLE daemon is inactive
 * \retval -1 BLE daemon is disabled
 */
int8_t rf_ble_is_active(void);

/**
 * \brief Set TX power for BLE advertisements
 * \param dbm The 'at least' TX power in dBm, values avove 5 dBm will be ignored
 *
 * Set TX power to 'at least' power dBm.
 * This works with a lookup table. If the value of 'power' does not exist in
 * the lookup table, TXPOWER will be set to the immediately higher available
 * value
 */
rf_ble_beacond_result_t rf_ble_set_tx_power(int8_t dbm);

/**
 * \brief   Get TX power for BLE advertisements
 * \return  The TX power for BLE advertisements
 */
int8_t rf_ble_get_tx_power(void);
/*---------------------------------------------------------------------------*/
#endif /* RF_BLE_BEACOND_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
