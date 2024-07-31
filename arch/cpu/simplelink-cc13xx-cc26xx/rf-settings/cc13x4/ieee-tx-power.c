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
 *        Source file for IEEE-mode TX power tables for LP-EM-CC1354P10-1.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */

#include "contiki.h"
#include "rf/tx-power.h"

/*
 * TX Power table for 2400 MHz, 5 dBm
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY and RF_TxPowerTable_HIGH_PA_ENTRY macro is defined in RF.h.
 * The following arguments are required:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * RF_TxPowerTable_HIGH_PA_ENTRY(bias, ibboost, boost, coefficient, ldoTrim)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
tx_power_table_t txPowerTable_2400_pa5[] =
{
    {-20, RF_TxPowerTable_DEFAULT_PA_ENTRY(7, 3, 0, 0) }, // 0x00C7
    {-18, RF_TxPowerTable_DEFAULT_PA_ENTRY(9, 3, 0, 0) }, // 0x00C9
    {-15, RF_TxPowerTable_DEFAULT_PA_ENTRY(12, 3, 0, 4) }, // 0x08CC
    {-12, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 2, 0, 4) }, // 0x088A
    {-10, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 3, 0, 0) }, // 0x00D2
    {-9, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 2, 0, 7) }, // 0x0E8E
    {-6, RF_TxPowerTable_DEFAULT_PA_ENTRY(19, 2, 0, 11) }, // 0x1693
    {-5, RF_TxPowerTable_DEFAULT_PA_ENTRY(21, 2, 0, 11) }, // 0x1695
    {-3, RF_TxPowerTable_DEFAULT_PA_ENTRY(38, 3, 0, 14) }, // 0x1CE6
    {0, RF_TxPowerTable_DEFAULT_PA_ENTRY(30, 1, 0, 21) }, // 0x2A5E
    {1, RF_TxPowerTable_DEFAULT_PA_ENTRY(35, 1, 0, 25) }, // 0x3263
    {2, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 0, 0, 35) }, // 0x4616
    {3, RF_TxPowerTable_DEFAULT_PA_ENTRY(29, 0, 0, 46) }, // 0x5C1D
    {4, RF_TxPowerTable_DEFAULT_PA_ENTRY(41, 0, 0, 64) }, // 0x8029
    {5, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 0, 0) }, // 0x003F
    RF_TxPowerTable_TERMINATION_ENTRY
};

/*
 * Define symbols for both the TX power table and its size. The TX power
 * table size is with one less entry by excluding the termination entry.
 */
#if RF_MODE == RF_MODE_2_4_GHZ
#define TX_POWER_TABLE  txPowerTable_2400_pa5
tx_power_table_t *const rf_tx_power_table = TX_POWER_TABLE;
const size_t rf_tx_power_table_size = (sizeof(TX_POWER_TABLE) / sizeof(TX_POWER_TABLE[0])) - 1;
#endif /* RF_MODE == RF_MODE_2_4_GHZ */

/* @} */
