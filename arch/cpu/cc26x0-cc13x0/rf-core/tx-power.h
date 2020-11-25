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
 *        Motivated by simplelink one.
 * \author
 *        alexraynepe196 <alexraynepe196@gmail.com>
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef RFC_TX_POWER_H_
#define RFC_TX_POWER_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "dev/radio.h"
/*---------------------------------------------------------------------------*/
#define RF_TXPOWER_DBM          RF_CONF_TXPOWER_DBM
#define RF_BLE_TXPOWER_DBM      RF_CONF_BLE_TXPOWER_DBM
#define RF_TXPOWER_BOOST_MODE   RF_CONF_TXPOWER_BOOST_MODE
/*---------------------------------------------------------------------------*/
struct tx_power_table_t {
  radio_value_t dbm;
  uint16_t tx_power; /* Value for the xxx_DIV_RADIO_SETUP.txPower field */
};
typedef struct tx_power_table_t tx_power_table_t;

/*---------------------------------------------------------------------------*/
/*
 * styles for power tables:
 * */

//< old style provided by smartrf-settings.c - this tables lists by falling dbm value
#define RF_TX_POWER_TABLE_OLDSTYLE      0

//< simplelink provided tables style - this tables lists by rising dbm
//  when not defined TX_POWER_CONF_PROP_DRIVER, use rf_tx_power_table, provided
//  by simplelink-xxx platform settings
#define RF_TX_POWER_TABLE_SIMPLELINK    1

#ifndef RF_TX_POWER_TABLE_STYLE
#define RF_TX_POWER_TABLE_STYLE         RF_TX_POWER_TABLE_OLDSTYLE
#endif

/*---------------------------------------------------------------------------*/
// smartrf-settings tables bor RFband:

/* TX power table for the 431-527MHz band */
#ifdef PROP_MODE_CONF_TX_POWER_431_527
#define PROP_MODE_TX_POWER_431_527 PROP_MODE_CONF_TX_POWER_431_527
#else
#define PROP_MODE_TX_POWER_431_527 prop_mode_tx_power_431_527
#endif
/*---------------------------------------------------------------------------*/
/* TX power table for the 779-930MHz band */
#ifdef PROP_MODE_CONF_TX_POWER_779_930
#define PROP_MODE_TX_POWER_779_930 PROP_MODE_CONF_TX_POWER_779_930
#else
#define PROP_MODE_TX_POWER_779_930 prop_mode_tx_power_779_930
#endif

/*
 * this defines PropMode TX power table. If not defined, uses default table
 *  provided by smartrf-settings for specified RadioBand
 * TX_POWER_CONF_PROP_DRIVER -
 * */
//#define TX_POWER_CONF_PROP_DRIVER

//==============================================================================

/**
 * \name TX power table convenience functions.
 *
 * @{
 */

const tx_power_table_t* rfc_tx_power_last_element(const tx_power_table_t *table);

#if RF_TX_POWER_TABLE_STYLE == RF_TX_POWER_TABLE_OLDSTYLE

static inline
int8_t rfc_tx_power_max(const tx_power_table_t *table)
{
  return table[0].dbm;
}

int8_t rfc_tx_power_min(const tx_power_table_t *table);

#else // RF_TX_POWER_TABLE_SIMPLELINK

static inline int8_t
rfc_tx_power_min(const tx_power_table_t *table)
{
  return table[0].dbm;
}

int8_t rfc_tx_power_max(const tx_power_table_t *table);

#endif  //#if RF_TX_POWER_TABLE_STYLE

bool rfc_tx_power_in_range(int8_t dbm, const tx_power_table_t *table);

const tx_power_table_t* rfc_tx_power_eval_power_code(int8_t dbm, const tx_power_table_t *table);

/** @} */
/*---------------------------------------------------------------------------*/
#endif /* TX_POWER_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
