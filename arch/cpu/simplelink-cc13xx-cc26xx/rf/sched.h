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
 * \defgroup cc13xx-cc26xx-rf-sched RF Scheduler for CC13xx/CC26xx
 *
 * @{
 *
 * \file
 *        Header file of the CC13xx/CC26xx RF scheduler.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef RF_SCHED_H_
#define RF_SCHED_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/process.h"
/*---------------------------------------------------------------------------*/
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
PROCESS_NAME(rf_sched_process);
/*---------------------------------------------------------------------------*/
typedef enum {
  RF_RESULT_OK = 0,
  RF_RESULT_ERROR,
} rf_result_t;
/*---------------------------------------------------------------------------*/
typedef enum {
    BLE_ADV_CHANNEL_37 = (1 << 0),
    BLE_ADV_CHANNEL_38 = (1 << 1),
    BLE_ADV_CHANNEL_39 = (1 << 2),

    BLE_ADV_CHANNEL_ALL = (BLE_ADV_CHANNEL_37 |
                           BLE_ADV_CHANNEL_38 |
                           BLE_ADV_CHANNEL_39),
} ble_adv_channel_t;
/*---------------------------------------------------------------------------*/
/**
 * \name Common RF scheduler functionality.
 *
 * @{
 */
rf_result_t rf_yield(void);
rf_result_t rf_restart_rat(void);
rf_result_t rf_set_tx_power(RF_Handle handle, RF_TxPowerTable_Entry *table, int8_t dbm);
rf_result_t rf_get_tx_power(RF_Handle handle, RF_TxPowerTable_Entry *table, int8_t *dbm);
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name Nestack Radio scheduler functionality.
 *
 * Either for Prop-mode or IEEE-mode Radio driver.
 *
 * @{
 */
RF_Handle   netstack_open(RF_Params *params);
rf_result_t netstack_sched_fs(void);
rf_result_t netstack_sched_ieee_tx(uint16_t payload_length, bool ack_request);
rf_result_t netstack_sched_prop_tx(uint16_t payload_length);
rf_result_t netstack_sched_rx(bool start);
rf_result_t netstack_stop_rx(void);
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name BLE Radio scheduler functionality.
 *
 * Only for the BLE Beacon Daemon.
 *
 * @{
 */
RF_Handle   ble_open(RF_Params *params);
rf_result_t ble_sched_beacons(uint8_t bm_adv_channel);
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* RF_SCHED_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
