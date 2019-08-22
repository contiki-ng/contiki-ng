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
 *        Source file for Prop-mode TX power tables for CC13x0.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "rf/tx-power.h"
/*---------------------------------------------------------------------------*/
/*
 * TX Power table for CC1310
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t rf_prop_tx_power_table_cc1310[] =
{
  {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(0, 3, 0, 4) },
  {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(1, 1, 0, 0) },
  {1, RF_TxPowerTable_DEFAULT_PA_ENTRY(3, 3, 0, 8) },
  {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(2, 1, 0, 8) },
  {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 3, 0, 10) },
  {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(5, 3, 0, 12) },
  {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(6, 3, 0, 12) },
  {6, RF_TxPowerTable_DEFAULT_PA_ENTRY(7, 3, 0, 14) },
  {7, RF_TxPowerTable_DEFAULT_PA_ENTRY(9, 3, 0, 16) },
  {8, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 18) },
  {9, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 3, 0, 22) },
  {10, RF_TxPowerTable_DEFAULT_PA_ENTRY(19, 3, 0, 28) },
  {11, RF_TxPowerTable_DEFAULT_PA_ENTRY(26, 3, 0, 40) },
  {12, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 0, 0, 92) },
  // The original PA value (12.5 dBm) has been rounded to an integer value.
  {13, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 0, 83) },
#if RF_CONF_TXPOWER_BOOST_MODE
  // This setting requires CCFG_FORCE_VDDR_HH = 1.
  {14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 83) },
#endif
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
/*
 * TX Power table for CC1350
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t rf_prop_tx_power_table_cc1350[] =
{
  {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(0, 3, 0, 2) },
  {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(3, 3, 0, 9) },
  {1, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 3, 0, 11) },
  {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(5, 3, 0, 12) },
  {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(6, 3, 0, 14) },
  {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 1, 0, 12) },
  {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 3, 0, 16) },
  {6, RF_TxPowerTable_DEFAULT_PA_ENTRY(9, 3, 0, 18) },
  {7, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 21) },
  {8, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 0, 25) },
  {9, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 3, 0, 32) },
  {10, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 3, 0, 44) },
  {11, RF_TxPowerTable_DEFAULT_PA_ENTRY(37, 3, 0, 72) },
  {12, RF_TxPowerTable_DEFAULT_PA_ENTRY(43, 0, 0, 94) },
#if RF_TXPOWER_BOOST_MODE
  // This setting requires CCFG_FORCE_VDDR_HH = 1.
  {14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 85) },
#endif
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
/*
 * TX Power table for CC1350_433
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t rf_prop_tx_power_table_cc1350_4[] =
{
  {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(0, 3, 0, 2) },
  {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(1, 3, 0, 7) },
  {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(1, 3, 0, 9) },
  {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(2, 3, 0, 11) },
  {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(2, 3, 0, 12) },
  {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(3, 3, 0, 16) },
  {6, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 3, 0, 18) },
  {7, RF_TxPowerTable_DEFAULT_PA_ENTRY(5, 3, 0, 21) },
  {8, RF_TxPowerTable_DEFAULT_PA_ENTRY(6, 3, 0, 23) },
  {9, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 3, 0, 28) },
  {10, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 35) },
  {11, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 1, 0, 39) },
  {12, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 1, 0, 60) },
  {13, RF_TxPowerTable_DEFAULT_PA_ENTRY(15, 0, 0, 108) },
  // The original PA value (13.7 dBm) has been rounded to an integer value.
  {14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 0, 92) },
#if RF_CONF_TXPOWER_BOOST_MODE
  // This setting requires CCFG_FORCE_VDDR_HH = 1.
  {15, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 72) },
#endif
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
tx_power_table_t rf_prop_tx_power_table_empty[] =
{
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
/* Only define the symbols if Prop-mode is selected */
#if (RF_MODE == RF_MODE_SUB_1_GHZ)
/*---------------------------------------------------------------------------*/
/* TX power table, based on which board is used. */
#if defined(DEVICE_CC1310)
#define TX_POWER_TABLE  rf_prop_tx_power_table_cc1310

#elif defined(DEVICE_CC1350)
#define TX_POWER_TABLE  rf_prop_tx_power_table_cc1350

#elif defined(DEVICE_CC1350_4)
#define TX_POWER_TABLE  rf_prop_tx_power_table_cc1350_4

#else
#define TX_POWER_TABLE  rf_prop_tx_power_table_empty
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
