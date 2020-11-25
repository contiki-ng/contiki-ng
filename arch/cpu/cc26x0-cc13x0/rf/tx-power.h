/*
 * uip.h
 *
 *  Created on: 17 ���. 2020 �.
 *      Author: alexrayne
 *-------------------------------------------------------------
 *      wraper of rc-core/tx-power.h for compatibility with simplelink rf-settings/xxx-power.c
 */

#ifndef RFTX_POWER_H_
#define RFTX_POWER_H_

#undef  RF_TX_POWER_TABLE_STYLE
#define RF_TX_POWER_TABLE_STYLE RF_TX_POWER_TABLE_SIMPLELINK


#include "rf-core/tx-power.h"
/*---------------------------------------------------------------------------*/
// #include <ti/drivers/rf/RF.h>
#ifndef RF_TxPowerTable_MIN_DBM
/* this is a part of <ti/drivers/rf/RF.h>*/

/**
 * @name TX Power Table defines
 * @{
 */

/**
 * Refers to the the minimum available power in dBm when accessing a power
 * table.
 *
 * \sa #RF_TxPowerTable_findValue()
 */
#define RF_TxPowerTable_MIN_DBM   -128

/**
 * Refers to the the maximum available power in dBm when accessing a power
 * table.
 *
 * \sa #RF_TxPowerTable_findValue()
 */
#define RF_TxPowerTable_MAX_DBM   126

/**
 * Refers to an invalid power level in a TX power table.
 *
 * \sa #RF_TxPowerTable_findPowerLevel()
 */
#define RF_TxPowerTable_INVALID_DBM   127

/**
 * Refers to an invalid power value in a TX power table.
 *
 * This is the raw value part of a TX power configuration. In order to check
 * whether a given power configuration is valid, do:
 *
 * @code
 * RF_TxPowerTable_Value value = ...;
 * if (value.rawValue == RF_TxPowerTable_INVALID_VALUE) {
 *     // error, value not valid
 * }
 * @endcode
 *
 * A TX power table is always terminated by an invalid power configuration.
 *
 * \sa #RF_getTxPower(), RF_TxPowerTable_findValue
 */
#define RF_TxPowerTable_INVALID_VALUE 0xFFFF

/**
 * Marks the last entry in a TX power table.
 *
 * In order to use #RF_TxPowerTable_findValue() and #RF_TxPowerTable_findPowerLevel(),
 * every power table must be terminated by a %RF_TxPowerTable_TERMINATION_ENTRY:
 *
 * @code
 * RF_TxPowerTable_Entry txPowerTable[] =
 * {
 *     { 20,  RF_TxPowerTable_HIGH_PA_ENTRY(1, 2, 3) },
 *     // ... ,
 *     RF_TxPowerTable_TERMINATION_ENTRY
 * };
 * @endcode
 */
#define RF_TxPowerTable_TERMINATION_ENTRY { 127, RF_TxPowerTable_INVALID_VALUE}

/**
 * Creates a TX power table entry for the default PA.
 *
 * The values for \a bias, \a gain, \a boost and \a coefficient are usually measured by Texas Instruments
 * for a specific front-end configuration. They can then be obtained from SmartRFStudio.
 */
#define RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost, coefficient) \
        ( ((bias) << 0) | ((gain) << 6) | ((boost) << 8) | ((coefficient) << 9) )

/**
 * Creates a TX power table entry for the High-power PA.
 *
 * The values for \a bias, \a ibboost, \a boost, \a coefficient and \a ldoTrim are usually measured by Texas Instruments
 * for a specific front-end configuration. They can then be obtained from SmartRFStudio.
 */
/*
#define RF_TxPowerTable_HIGH_PA_ENTRY(bias, ibboost, boost, coefficient, ldotrim) \
        ( ((bias) << 0) | ((ibboost) << 6) | ((boost) << 8) | ((coefficient) << 9) | ((ldotrim) << 16) )
*/
// not support High PA yet. use simplelink target for it.
#define RF_TxPowerTable_HIGH_PA_ENTRY(bias, ibboost, boost, coefficient, ldotrim)   RF_TxPowerTable_INVALID_VALUE


/** @} */

#endif //RF_TxPowerTable_MIN_DBM
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

#endif /* RFTX_POWER_H_ */
