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
/*---------------------------------------------------------------------------*/
#ifndef NETSTACK_SETTINGS_H_
#define NETSTACK_SETTINGS_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <ti/devices/DeviceFamily.h>
/*---------------------------------------------------------------------------*/
#ifdef RF_CORE_CONF_PROP_RF_SETTINGS
#   define RF_CORE_PROP_RF_SETTINGS  RF_CORE_CONF_PROP_RF_SETTINGS
#else
#   define RF_CORE_PROP_RF_SETTINGS "prop-settings.h"
#endif

#include RF_CORE_PROP_RF_SETTINGS

#ifdef RF_CORE_CONF_IEEE_RF_SETTINGS
#   define RF_CORE_IEEE_RF_SETTINGS  RF_CORE_CONF_IEEE_RF_SETTINGS
#else
#   define RF_CORE_IEEE_RF_SETTINGS "ieee-settings.h"
#endif

#include RF_CORE_IEEE_RF_SETTINGS
/*---------------------------------------------------------------------------*/
/* Prop-mode RF settings */
#if (RF_CORE_CONF_MODE == RF_CORE_MODE_SUB_1_GHZ)

#define netstack_mode             rf_prop_mode
#define netstack_cmd_radio_setup  rf_cmd_prop_radio_div_setup
#define netstack_cmd_fs           rf_cmd_prop_fs
#define netstack_cmd_tx           rf_cmd_prop_tx_adv
#define netstack_cmd_rx           rf_cmd_prop_rx_adv
/*---------------------------------------------------------------------------*/
/* IEEE-mode RF settings */
#elif (RF_CORE_CONF_MODE == RF_CORE_MODE_2_4_GHZ)

#define netstack_mode             rf_ieee_mode
#define netstack_cmd_radio_setup  rf_cmd_ieee_radio_setup
#define netstack_cmd_fs           rf_cmd_ieee_fs
#define netstack_cmd_tx           rf_cmd_ieee_tx
#define netstack_cmd_rx           rf_cmd_ieee_rx
/*---------------------------------------------------------------------------*/
#else
# error "Unsupported RF_CORE_MODE"
#endif
/*---------------------------------------------------------------------------*/
#include "ble-settings.h"

#if (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X0_CC26X0)

#define ble_mode                rf_ble_mode
#define ble_cmd_radio_setup     rf_ble_cmd_radio_setup
#define ble_adv_par             rf_ble_adv_par
#define ble_cmd_beacon          rf_ble_cmd_ble_adv_nc

#elif (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X2_CC26X2)

#define ble_mode                rf_ble_mode
#define ble_cmd_radio_setup     rf_cmd_ble5_radio_setup
#define ble_adv_par             rf_ble5_adv_aux_par
#define ble_cmd_beacon          rf_cmd_ble5_adv_aux

#else
# error "Unsupported DeviceFamily_PARENT for BLE settings"
#endif
/*---------------------------------------------------------------------------*/
#endif /* NETSTACK_SETTINGS_H_ */
/*---------------------------------------------------------------------------*/
