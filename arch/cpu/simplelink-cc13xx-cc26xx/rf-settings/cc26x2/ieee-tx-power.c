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
 *        Source file for IEEE-mode TX power tables for CC26x2.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "rf/tx-power.h"
/*---------------------------------------------------------------------------*/
/*
 * TX Power table for CC2652R
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t rf_ieee_tx_power_table_cc2652r[] =
{
  { -21, RF_TxPowerTable_DEFAULT_PA_ENTRY(7,  3, 0,   3) },
  { -18, RF_TxPowerTable_DEFAULT_PA_ENTRY(9,  3, 0,   3) },
  { -15, RF_TxPowerTable_DEFAULT_PA_ENTRY(12, 2, 0, 100) },
  { -12, RF_TxPowerTable_DEFAULT_PA_ENTRY(40, 2, 0,   8) },
  { -10, RF_TxPowerTable_DEFAULT_PA_ENTRY(12, 2, 0,  11) },
  {  -9, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 2, 0,   5) },
  {  -6, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 1, 0,  16) },
  {  -5, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 1, 0,  17) },
  {  -3, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 1, 0,  20) },
  {   0, RF_TxPowerTable_DEFAULT_PA_ENTRY(25, 1, 0,  26) },
  {   1, RF_TxPowerTable_DEFAULT_PA_ENTRY(28, 1, 0,  28) },
  {   2, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 0, 0,  34) },
  {   3, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 0, 0,  42) },
  {   4, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 0, 0,  54) },
  {   5, RF_TxPowerTable_DEFAULT_PA_ENTRY(30, 0, 0,  74) },
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
tx_power_table_t rf_ieee_tx_power_table_empty[] =
{
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
/* Only define the symbols if Prop-mode is selected */
#if (RF_MODE == RF_MODE_2_4_GHZ)
/*---------------------------------------------------------------------------*/
/* TX power table, based on which board is used. */
#if defined(DEVICE_CC2652R)
#define TX_POWER_TABLE  rf_ieee_tx_power_table_cc2652r

#else
#define TX_POWER_TABLE  rf_ieee_tx_power_table_empty
#endif

/*
 * Define symbols for both the TX power table and its size. The TX power
 * table size is with one less entry by excluding the termination entry.
 */
tx_power_table_t *const rf_tx_power_table = TX_POWER_TABLE;
const size_t rf_tx_power_table_size = (sizeof(TX_POWER_TABLE) / sizeof(TX_POWER_TABLE[0])) - 1;
/*---------------------------------------------------------------------------*/
#endif /* RF_MODE */
/*---------------------------------------------------------------------------*/
