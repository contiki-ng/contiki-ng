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
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \defgroup cc13xx-cc26xx-rf-tx-power TX power functioanlity for CC13xx/CC26xx
 *
 * @{
 *
 * \file
 *        Header file of TX power functionality of CC13xx/CC26xx.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef TX_POWER_H_
#define TX_POWER_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#define RF_TXPOWER_HIGH_PA      RF_CONF_TXPOWER_HIGH_PA
#define RF_TXPOWER_BOOST_MODE   RF_CONF_TXPOWER_BOOST_MODE
/*---------------------------------------------------------------------------*/
typedef RF_TxPowerTable_Entry tx_power_table_t;
/*---------------------------------------------------------------------------*/
/**
 * \name Extern declarations of TX Power Table variables.
 *
 * @{
 */

/* Nestack RF TX power table */
extern tx_power_table_t *const rf_tx_power_table;
extern const size_t rf_tx_power_table_size;

/* BLE Beacon RF TX power table */
extern tx_power_table_t *const ble_tx_power_table;
extern const size_t ble_tx_power_table_size;
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \name TX power table convenience functions.
 *
 * @{
 */

static inline int8_t
tx_power_min(tx_power_table_t *table)
{
  return table[0].power;
}
static inline int8_t
tx_power_max(tx_power_table_t *table, size_t size)
{
  return table[size - 1].power;
}
static inline bool
tx_power_in_range(int8_t dbm, tx_power_table_t *table, size_t size)
{
  return (dbm >= tx_power_min(table)) &&
         (dbm <= tx_power_max(table, size));
}
/** @} */
/*---------------------------------------------------------------------------*/
#endif /* TX_POWER_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
