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
 * \addtogroup cc13xx-cc26xx-rf-tx-power
 * @{
 *
 * \file
 *        Source file for BLE Beacon TX power tables for CC13x0.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "rf/tx-power.h"
/*---------------------------------------------------------------------------*/
/*
 * TX Power table for CC1350
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t rf_ble_tx_power_table_cc1350[] =
{
  { -21, RF_TxPowerTable_DEFAULT_PA_ENTRY( 8, 3, 1,  6) },
  { -18, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 1,  6) },
  { -15, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 1, 10) },
  { -12, RF_TxPowerTable_DEFAULT_PA_ENTRY(20, 3, 1, 12) },
  {  -9, RF_TxPowerTable_DEFAULT_PA_ENTRY(26, 3, 1, 14) },
  {  -6, RF_TxPowerTable_DEFAULT_PA_ENTRY(35, 3, 1, 18) },
  {  -3, RF_TxPowerTable_DEFAULT_PA_ENTRY(47, 3, 1, 22) },
  {   0, RF_TxPowerTable_DEFAULT_PA_ENTRY(29, 0, 1, 45) },
  {   1, RF_TxPowerTable_DEFAULT_PA_ENTRY(33, 0, 1, 49) },
  {   2, RF_TxPowerTable_DEFAULT_PA_ENTRY(38, 0, 1, 55) },
  {   3, RF_TxPowerTable_DEFAULT_PA_ENTRY(44, 0, 1, 63) },
  {   4, RF_TxPowerTable_DEFAULT_PA_ENTRY(52, 0, 1, 59) },
  {   5, RF_TxPowerTable_DEFAULT_PA_ENTRY(60, 0, 1, 47) },
#if RF_TXPOWER_BOOST_MODE
  /* This setting requires CCFG_FORCE_VDDR_HH = 1. */
  {   6, RF_TxPowerTable_DEFAULT_PA_ENTRY(38, 0, 1, 49) },
  /* This setting requires CCFG_FORCE_VDDR_HH = 1. */
  {   7, RF_TxPowerTable_DEFAULT_PA_ENTRY(46, 0, 1, 59) },
  /* This setting requires CCFG_FORCE_VDDR_HH = 1. */
  {   8, RF_TxPowerTable_DEFAULT_PA_ENTRY(55, 0, 1, 51) },
  /* This setting requires CCFG_FORCE_VDDR_HH = 1. */
  {   9, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 30) },
#endif
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
tx_power_table_t rf_ble_tx_power_table_empty[] =
{
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
/* TX power table, based on which board is used. */
#if defined(DEVICE_CC1350) || defined(DEVICE_CC1350_4)
#define TX_POWER_TABLE  rf_ble_tx_power_table_cc1350

#else
#define TX_POWER_TABLE  rf_ble_tx_power_table_empty
#endif

/*
 * Define symbols for both the TX power table and its size. The TX power
 * table size is with one less entry by excluding the termination entry.
 */
tx_power_table_t *const ble_tx_power_table = TX_POWER_TABLE;
const size_t ble_tx_power_table_size = (sizeof(TX_POWER_TABLE) / sizeof(TX_POWER_TABLE[0])) - 1;
/*---------------------------------------------------------------------------*/
