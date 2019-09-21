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
/*
 * Parameter summary
 * Adv. Address: 010203040506
 * Adv. Data: dummy
 * BLE Channel: 17
 * Extended Header: 09 09 010203040506 babe
 * Frequency: 2440 MHz
 * PDU Payload length:: 30
 * TX Power: 5 dBm (requires define CCFG_FORCE_VDDR_HH = 0 in ccfg
 *                  see CC13xx/CC26xx Technical Reference Manual)
 * Whitening: true
 */
/*---------------------------------------------------------------------------*/
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_ble_cmd.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_multi_protocol.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "ble-settings.h"
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF Mode Object */
RF_Mode rf_ble_mode =
{
  .rfMode = RF_MODE_AUTO,
  .cpePatchFxn = &rf_patch_cpe_multi_protocol,
  .mcePatchFxn = 0,
  .rfePatchFxn = 0,
};
/*---------------------------------------------------------------------------*/
#if defined(DEVICE_CC1352R)

/* Overrides for CMD_BLE5_RADIO_SETUP */
uint32_t rf_ble_overrides_common[] CC_ALIGN(4) =
{
  // override_ble5_setup_override_common.xml
  // DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0x3 (DITHER_EN=0 and IPEAK=3).
  (uint32_t)0x00F388D3,
  // Bluetooth 5: Set pilot tone length to 20 us Common
  HW_REG_OVERRIDE(0x6024,0x2E20),
  // Bluetooth 5: Compensate for reduced pilot tone length
  (uint32_t)0x01280263,
  // Bluetooth 5: Default to no CTE.
  HW_REG_OVERRIDE(0x5328,0x0000),
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP */
uint32_t rf_ble_overrides_1mbps[] CC_ALIGN(4) =
{
  // override_ble5_setup_override_1mbps.xml
  // Bluetooth 5: Set pilot tone length to 20 us
  HW_REG_OVERRIDE(0x5320,0x03C0),
  // Bluetooth 5: Compensate syncTimeadjust
  (uint32_t)0x015302A3,
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP */
uint32_t rf_ble_overrides_2mbps[] CC_ALIGN(4) =
{
  // override_ble5_setup_override_2mbps.xml
  // Bluetooth 5: Set pilot tone length to 20 us
  HW_REG_OVERRIDE(0x5320,0x03C0),
  // Bluetooth 5: Compensate syncTimeAdjust
  (uint32_t)0x00F102A3,
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP */
uint32_t rf_ble_overrides_coded[] CC_ALIGN(4) =
{
  // override_ble5_setup_override_coded.xml
  // Bluetooth 5: Set pilot tone length to 20 us
  HW_REG_OVERRIDE(0x5320,0x03C0),
  // Bluetooth 5: Compensate syncTimeadjust
  (uint32_t)0x07A902A3,
  // Rx: Set AGC reference level to 0x1B (default: 0x2E)
  HW_REG_OVERRIDE(0x609C,0x001B),
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* CMD_BLE5_RADIO_SETUP: Bluetooth 5 Radio Setup Command for all PHYs */
rfc_CMD_BLE5_RADIO_SETUP_t rf_ble_cmd_radio_setup =
{
  .commandNo = CMD_BLE5_RADIO_SETUP,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .defaultPhy.mainMode = 0x0,
  .defaultPhy.coding = 0x0,
  .loDivider = 0x00,
  .config.frontEndMode = 0x0, /* set by driver */
  .config.biasMode = 0x0, /* set by driver */
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = 0x7217, /* set by driver */
  .pRegOverrideCommon = rf_ble_overrides_common,
  .pRegOverride1Mbps = rf_ble_overrides_1mbps,
  .pRegOverride2Mbps = rf_ble_overrides_2mbps,
  .pRegOverrideCoded = rf_ble_overrides_coded,
};

#endif /* defined(DEVICE_CC1352R) */
/*---------------------------------------------------------------------------*/
#if defined(DEVICE_CC1352P)

/* Overrides for CMD_BLE5_RADIO_SETUP_PA */
uint32_t rf_ble_overrides_common[] CC_ALIGN(4) =
{
  // override_ble5_setup_override_common_hpa.xml
  // Bluetooth 5: Reconfigure to 35 us pilot tone length for high output power PA
  HW_REG_OVERRIDE(0x6024,0x5B20),
  // Bluetooth 5: Compensate for 35 us pilot tone length
  (uint32_t)0x01640263,
  // Bluetooth 5: Set IPEAK = 3 and DCDC dither off for TX
  (uint32_t)0x00F388D3,
  // Bluetooth 5: Default to no CTE.
  HW_REG_OVERRIDE(0x5328,0x0000),
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP_PA */
uint32_t rf_ble_overrides_1mbps[] CC_ALIGN(4) =
{
  // override_ble5_setup_override_1mbps_hpa.xml
  // Bluetooth 5: Reconfigure pilot tone length for high output power PA
  HW_REG_OVERRIDE(0x5320,0x0690),
  // Bluetooth 5: Compensate for modified pilot tone length
  (uint32_t)0x018F02A3,
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP_PA */
uint32_t rf_ble_overrides_2mbps[] CC_ALIGN(4) =
{
  // override_ble5_setup_override_2mbps_hpa.xml
  // Bluetooth 5: Reconfigure pilot tone length for high output power PA
  HW_REG_OVERRIDE(0x5320,0x0690),
  // Bluetooth 5: Compensate for modified pilot tone length
  (uint32_t)0x012D02A3,
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP_PA */
uint32_t rf_ble_overrides_coded[] CC_ALIGN(4) =
{
  // override_ble5_setup_override_coded_hpa.xml
  // Bluetooth 5: Reconfigure pilot tone length for high output power PA
  HW_REG_OVERRIDE(0x5320,0x0690),
  // Bluetooth 5: Compensate for modified pilot tone length
  (uint32_t)0x07E502A3,
  // Bluetooth 5: Set AGC mangnitude target to 0x1B.
  HW_REG_OVERRIDE(0x609C,0x001B),
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
// Overrides for CMD_BLE5_RADIO_SETUP_PA
uint32_t rf_ble_overrides_tx_std[] =
{
  // The TX Power element should always be the first in the list
  TX_STD_POWER_OVERRIDE(0x7217),
  // The ANADIV radio parameter based on the LO divider (0) and front-end (0) settings
  (uint32_t)0x05320703,
  // override_txstd_settings.xml
  // Bluetooth 5: Set RTIM offset to default for standard PA
  (uint32_t)0x00008783,
  // Bluetooth 5: Set synth mux to default value for standard PA
  (uint32_t)0x050206C3,
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
// Overrides for CMD_BLE5_RADIO_SETUP_PA
uint32_t rf_ble_overrides_tx_20[] =
{
  // The TX Power element should always be the first in the list
  TX20_POWER_OVERRIDE(0x003F75F5),
  // The ANADIV radio parameter based on the LO divider (0) and front-end (0) settings
  (uint32_t)0x01C20703,
  // override_tx20_settings.xml
  // Bluetooth 5: Set RTIM offset to 3 for high power PA
  (uint32_t)0x00038783,
  // Bluetooth 5: Set synth mux for high power PA
  (uint32_t)0x010206C3,
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* CMD_BLE5_RADIO_SETUP_PA: Bluetooth 5 Radio Setup Command for all PHYs */
rfc_CMD_BLE5_RADIO_SETUP_PA_t rf_ble_cmd_radio_setup =
{
  .commandNo = 0x1820,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .defaultPhy.mainMode = 0x0,
  .defaultPhy.coding = 0x0,
  .loDivider = 0x00,
  .config.frontEndMode = 0x0, /* set by driver */
  .config.biasMode = 0x0, /* set by driver */
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = 0x7217, /* set by driver */
  .pRegOverrideCommon = rf_ble_overrides_common,
  .pRegOverride1Mbps = rf_ble_overrides_1mbps,
  .pRegOverride2Mbps = rf_ble_overrides_2mbps,
  .pRegOverrideCoded = rf_ble_overrides_coded,
  .pRegOverrideTxStd = rf_ble_overrides_tx_std,
  .pRegOverrideTx20 = rf_ble_overrides_tx_20,
};

#endif /* defined(DEVICE_CC1352P) */
/*---------------------------------------------------------------------------*/
/* CMD_BLE5_ADV_NC: Bluetooth 5 Non-Connectable Advertiser Command */
rfc_CMD_BLE5_ADV_NC_t rf_ble_cmd_ble_adv_nc =
{
  .commandNo = CMD_BLE5_ADV_NC,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x1,
  .condition.rule = 0x0, /* set by driver */
  .condition.nSkip = 0x0,
  .channel = 0x00, /* set by driver */
  .whitening.init = 0x00, /* set by driver */
  .whitening.bOverride = 0x1,
  .phyMode.mainMode = 0x0,
  .phyMode.coding = 0x0,
  .rangeDelay = 0x00,
  .txPower = 0x0000,
  .pParams = 0x00000000, /* set by driver */
  .pOutput = 0x00000000, /* set by driver */
  .tx20Power = 0x00000000,
};
/*---------------------------------------------------------------------------*/
