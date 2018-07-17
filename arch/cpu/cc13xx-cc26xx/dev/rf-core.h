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
 * \addtogroup simplelink
 * @{
 *
 * \defgroup rf-common Common functionality fpr the CC13xx/CC26xx RF
 *
 * @{
 *
 * \file
 * Header file of common CC13xx/CC26xx RF functionality
 */
/*---------------------------------------------------------------------------*/
#ifndef RF_CORE_H_
#define RF_CORE_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/process.h"

#include "rf-ble-beacond.h"
/*---------------------------------------------------------------------------*/
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#define RF_MODE_SUB_1_GHZ      (1 << 0)
#define RF_MODE_2_4_GHZ        (1 << 1)

#define RF_MODE_BM             ( RF_MODE_SUB_1_GHZ \
                               | RF_MODE_2_4_GHZ \
                               )
/*---------------------------------------------------------------------------*/
PROCESS_NAME(rf_core_process);
/*---------------------------------------------------------------------------*/
typedef enum {
    RF_RESULT_OK = 0,
    RF_RESULT_ERROR,
} rf_result_t;
/*---------------------------------------------------------------------------*/
/* Common */
rf_result_t rf_yield(void);

rf_result_t rf_set_tx_power(RF_Handle handle, RF_TxPowerTable_Entry *table, int8_t dbm);
rf_result_t rf_get_tx_power(RF_Handle handle, RF_TxPowerTable_Entry *table, int8_t *dbm);
/*---------------------------------------------------------------------------*/
/* Netstack radio: IEEE-mode or prop-mode */
RF_Handle   netstack_open(RF_Params *params);

rf_result_t netstack_sched_fs(void);
rf_result_t netstack_sched_ieee_tx(bool recieve_ack);
rf_result_t netstack_sched_prop_tx(void);
rf_result_t netstack_sched_rx(bool start);
rf_result_t netstack_stop_rx(void);
/*---------------------------------------------------------------------------*/
/* BLE radio: BLE Beacon Daemon */
RF_Handle   ble_open(RF_Params *params);

rf_result_t ble_sched_beacon(RF_Callback cb, RF_EventMask bm_event);
/*---------------------------------------------------------------------------*/
#endif /* RF_CORE_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
