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
 * \defgroup cc13xx-cc26xx-rf-settings RF settings for CC13xx/CC26xx
 *
 * @{
 *
 * \file
 *        Header file of RF settings for CC13xx/CC26xx.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef NETSTACK_SETTINGS_H_
#define NETSTACK_SETTINGS_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
/*---------------------------------------------------------------------------*/
/* Netstack RF command configuration */

#if SUPPORTS_PROP_MODE
#include "prop-settings.h"
#endif

#if SUPPORTS_IEEE_MODE
#include "ieee-settings.h"
#endif

/* Prop-mode RF settings */
#if (RF_MODE == RF_MODE_SUB_1_GHZ)

#define netstack_mode             rf_prop_mode
#define netstack_cmd_radio_setup  rf_cmd_prop_radio_div_setup
#define netstack_cmd_fs           rf_cmd_prop_fs
#define netstack_cmd_tx           rf_cmd_prop_tx_adv
#define netstack_cmd_rx           rf_cmd_prop_rx_adv

/* IEEE-mode RF settings */
#elif (RF_MODE == RF_MODE_2_4_GHZ)

#define netstack_mode             rf_ieee_mode
#define netstack_cmd_radio_setup  rf_cmd_ieee_radio_setup
#define netstack_cmd_fs           rf_cmd_ieee_fs
#define netstack_cmd_tx           rf_cmd_ieee_tx
#define netstack_cmd_rx           rf_cmd_ieee_rx

#endif /* RF_MODE */
/*---------------------------------------------------------------------------*/
/* BLE Beacon RF command configuration */
#if SUPPORTS_BLE_BEACON

#include "ble-settings.h"

/* CC13x0/CC26x0 devices */
#if (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X0_CC26X0)

#define ble_mode                rf_ble_mode
#define ble_cmd_radio_setup     rf_ble_cmd_radio_setup
#define ble_adv_par             rf_ble_adv_par
#define ble_cmd_adv_nc          rf_ble_cmd_ble_adv_nc

/* CC13x2/CC26x2 devices */
#elif (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X2_CC26X2)

#define ble_mode                rf_ble_mode
#define ble_cmd_radio_setup     rf_ble_cmd_ble5_radio_setup
#define ble_adv_par             rf_ble_adv_par
#define ble_cmd_adv_nc          rf_ble_cmd_ble5_adv_nc

#endif /* DeviceFamily_PARENT */

#endif /* SUPPORTS_BLE_BEACON */
/*---------------------------------------------------------------------------*/
#endif /* NETSTACK_SETTINGS_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */