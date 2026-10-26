/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
/*
 * LP-CC1312R7 (RF design CC1312-R7EM-XD7793_B0), 770-930 MHz, from TI's
 * radioconfig pa_tables/cc13x2_cc26x2_r7/pasettings.json, SimpleLink
 * CC13xx/CC26xx SDK 7.10.01.24. The trailing comment is TI's raw value.
 */
tx_power_table_t rf_prop_tx_power_table_cc1312r7[] =
{
  {-20, RF_TxPowerTable_DEFAULT_PA_ENTRY(0, 3, 0, 2) }, /* 0x04C0 */
  {-15, RF_TxPowerTable_DEFAULT_PA_ENTRY(1, 3, 0, 3) }, /* 0x06C1 */
  {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(2, 3, 0, 5) }, /* 0x0AC2 */
  {-5, RF_TxPowerTable_DEFAULT_PA_ENTRY(4, 3, 0, 5) }, /* 0x0AC4 */
  {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(8, 3, 0, 8) }, /* 0x10C8 */
  {1, RF_TxPowerTable_DEFAULT_PA_ENTRY(9, 3, 0, 9) }, /* 0x12C9 */
  {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 3, 0, 9) }, /* 0x12CA */
  {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 10) }, /* 0x14CB */
  {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 3, 0, 11) }, /* 0x16CD */
  {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 0, 14) }, /* 0x1CCE */
  {6, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 3, 0, 16) }, /* 0x20D1 */
  {7, RF_TxPowerTable_DEFAULT_PA_ENTRY(20, 3, 0, 19) }, /* 0x26D4 */
  {8, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 3, 0, 22) }, /* 0x2CD8 */
  {9, RF_TxPowerTable_DEFAULT_PA_ENTRY(28, 3, 0, 31) }, /* 0x3EDC */
  {10, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 2, 0, 31) }, /* 0x3E92 */
  {11, RF_TxPowerTable_DEFAULT_PA_ENTRY(26, 2, 0, 51) }, /* 0x669A */
  /* Boost bit set, but TI's table asks for CCFG_FORCE_VDDR_HH only at 14 dBm. */
  {12, RF_TxPowerTable_DEFAULT_PA_ENTRY(30, 1, 1, 68) }, /* 0x895E */
  // The original PA value (12.5 dBm) has been rounded to an integer value.
  {13, RF_TxPowerTable_DEFAULT_PA_ENTRY(36, 0, 0, 89) }, /* 0xB224 */
#if RF_TXPOWER_BOOST_MODE
  // This setting requires CCFG_FORCE_VDDR_HH = 1.
  {14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 0) }, /* 0x013F */
#endif /* RF_TXPOWER_BOOST_MODE */
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
#if defined(DEVICE_CC1312R7)
#define TX_POWER_TABLE  rf_prop_tx_power_table_cc1312r7

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
