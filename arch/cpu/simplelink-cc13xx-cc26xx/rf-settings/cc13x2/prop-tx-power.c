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
 *        Source file for TX power tables for CC13x2.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "rf/tx-power.h"
/*---------------------------------------------------------------------------*/
/*
 * TX Power table for CC1312R
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t rf_prop_tx_power_table_cc1312r[] =
{
  {-20, RF_TxPowerTable_DEFAULT_PA_ENTRY(0, 3, 0, 2) },
  {-15, RF_TxPowerTable_DEFAULT_PA_ENTRY(1, 3, 0, 3) },
  {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(2, 3, 0, 5) },
  {-5, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 3, 0, 5) },
  {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 3, 0, 8) },
  {1, RF_TxPowerTable_DEFAULT_PA_ENTRY(9, 3, 0, 9) },
  {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 3, 0, 9) },
  {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 10) },
  {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 3, 0, 11) },
  {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 0, 14) },
  {6, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 3, 0, 16) },
  {7, RF_TxPowerTable_DEFAULT_PA_ENTRY(20, 3, 0, 19) },
  {8, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 3, 0, 22) },
  {9, RF_TxPowerTable_DEFAULT_PA_ENTRY(28, 3, 0, 31) },
  {10, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 2, 0, 31) },
  {11, RF_TxPowerTable_DEFAULT_PA_ENTRY(26, 2, 0, 51) },
  {12, RF_TxPowerTable_DEFAULT_PA_ENTRY(16, 0, 0, 82) },
  // The original PA value (12.5 dBm) has been rounded to an integer value.
  {13, RF_TxPowerTable_DEFAULT_PA_ENTRY(36, 0, 0, 89) },
#if RF_TXPOWER_BOOST_MODE
  // This setting requires CCFG_FORCE_VDDR_HH = 1.
  {14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 0) },
#endif /* RF_TXPOWER_BOOST_MODE */
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
/*
 * TX Power table for CC1352R
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t rf_prop_tx_power_table_cc1352r[] =
{
  {-20, RF_TxPowerTable_DEFAULT_PA_ENTRY(0, 3, 0, 2) },
  {-15, RF_TxPowerTable_DEFAULT_PA_ENTRY(1, 3, 0, 3) },
  {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(2, 3, 0, 5) },
  {-5, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 3, 0, 5) },
  {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 3, 0, 8) },
  {1, RF_TxPowerTable_DEFAULT_PA_ENTRY(9, 3, 0, 9) },
  {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 3, 0, 9) },
  {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 10) },
  {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 3, 0, 11) },
  {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 0, 14) },
  {6, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 3, 0, 16) },
  {7, RF_TxPowerTable_DEFAULT_PA_ENTRY(20, 3, 0, 19) },
  {8, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 3, 0, 22) },
  {9, RF_TxPowerTable_DEFAULT_PA_ENTRY(28, 3, 0, 31) },
  {10, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 2, 0, 31) },
  {11, RF_TxPowerTable_DEFAULT_PA_ENTRY(26, 2, 0, 51) },
  {12, RF_TxPowerTable_DEFAULT_PA_ENTRY(16, 0, 0, 82) },
  // The original PA value (12.5 dBm) has been rounded to an integer value.
  {13, RF_TxPowerTable_DEFAULT_PA_ENTRY(36, 0, 0, 89) },
#if RF_TXPOWER_BOOST_MODE
  // This setting requires CCFG_FORCE_VDDR_HH = 1.
  {14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 0) },
#endif /* RF_TXPOWER_BOOST_MODE */
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
/*
 * TX Power table for CC1352P with default PA
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t rf_prop_tx_power_table_cc1352p[] =
{
  {-20, RF_TxPowerTable_DEFAULT_PA_ENTRY(0, 3, 0, 2) },
  {-15, RF_TxPowerTable_DEFAULT_PA_ENTRY(1, 3, 0, 3) },
  {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(2, 3, 0, 5) },
  {-5, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 3, 0, 5) },
  {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 3, 0, 8) },
  {1, RF_TxPowerTable_DEFAULT_PA_ENTRY(9, 3, 0, 9) },
  {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 3, 0, 9) },
  {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 10) },
  {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 3, 0, 11) },
  {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 0, 14) },
  {6, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 3, 0, 16) },
  {7, RF_TxPowerTable_DEFAULT_PA_ENTRY(20, 3, 0, 19) },
  {8, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 3, 0, 22) },
  {9, RF_TxPowerTable_DEFAULT_PA_ENTRY(28, 3, 0, 31) },
  {10, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 2, 0, 31) },
  {11, RF_TxPowerTable_DEFAULT_PA_ENTRY(26, 2, 0, 51) },
  {12, RF_TxPowerTable_DEFAULT_PA_ENTRY(16, 0, 0, 82) },
  // The original PA value (12.5 dBm) has been rounded to an integer value.
  {13, RF_TxPowerTable_DEFAULT_PA_ENTRY(36, 0, 0, 89) },
#if RF_TXPOWER_BOOST_MODE
  // This setting requires CCFG_FORCE_VDDR_HH = 1.
  {14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 0) },
#endif /* RF_TXPOWER_BOOST_MODE */
  {15, RF_TxPowerTable_HIGH_PA_ENTRY(18, 0, 0, 36, 0) },
  {16, RF_TxPowerTable_HIGH_PA_ENTRY(24, 0, 0, 43, 0) },
  {17, RF_TxPowerTable_HIGH_PA_ENTRY(28, 0, 0, 51, 2) },
  {18, RF_TxPowerTable_HIGH_PA_ENTRY(34, 0, 0, 64, 4) },
  {19, RF_TxPowerTable_HIGH_PA_ENTRY(15, 3, 0, 36, 4) },
  {20, RF_TxPowerTable_HIGH_PA_ENTRY(18, 3, 0, 71, 27) },
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
#if defined(DEVICE_CC1312R)
#define TX_POWER_TABLE  rf_prop_tx_power_table_cc1312r

#elif defined(DEVICE_CC1352R)
#define TX_POWER_TABLE  rf_prop_tx_power_table_cc1352r

#elif defined(DEVICE_CC1352P)
#define TX_POWER_TABLE  rf_prop_tx_power_table_cc1352p

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
