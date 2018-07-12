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
 * TX Power: 5 dBm
 * Whitening: true
 */
/*---------------------------------------------------------------------------*/
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_ble_cmd.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_bt5.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_rfe_bt5.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_mce_bt5.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "ble-settings.h"
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF Mode Object */
RF_Mode rf_ble_mode =
{
  .rfMode = RF_MODE_AUTO,
  .cpePatchFxn = &rf_patch_cpe_bt5,
  .mcePatchFxn = &rf_patch_mce_bt5,
  .rfePatchFxn = &rf_patch_rfe_bt5,
};
/*---------------------------------------------------------------------------*/
/*
 * TX Power table
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
RF_TxPowerTable_Entry rf_ble_tx_power_table[RF_BLE_TX_POWER_TABLE_SIZE+1] =
{
  { -21, RF_TxPowerTable_DEFAULT_PA_ENTRY( 7, 3, 0,  3) },
  { -18, RF_TxPowerTable_DEFAULT_PA_ENTRY( 9, 3, 0,  3) },
  { -15, RF_TxPowerTable_DEFAULT_PA_ENTRY( 8, 2, 0,  6) },
  { -12, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 2, 0,  8) },
  { -10, RF_TxPowerTable_DEFAULT_PA_ENTRY(12, 2, 0, 11) },
  {  -9, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 2, 0,  5) },
  {  -6, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 1, 0, 16) },
  {  -5, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 1, 0, 17) },
  {  -3, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 1, 0, 20) },
  {   0, RF_TxPowerTable_DEFAULT_PA_ENTRY(25, 1, 0, 26) },
  {   1, RF_TxPowerTable_DEFAULT_PA_ENTRY(28, 1, 0, 28) },
  {   2, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 0, 0, 34) },
  {   3, RF_TxPowerTable_DEFAULT_PA_ENTRY(17, 0, 0, 42) },
  {   4, RF_TxPowerTable_DEFAULT_PA_ENTRY(22, 0, 0, 54) },
  {   5, RF_TxPowerTable_DEFAULT_PA_ENTRY(30, 0, 0, 74) },
  RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP */
uint32_t rf_ble_overrides_common[] CC_ALIGN(4) =
{
                                 /* override_ble5_setup_override_common.xml */
  (uint32_t)0x02400403,          /* Synth: Use 48 MHz crystal, enable extra PLL filtering */
  (uint32_t)0x001C8473,          /* Synth: Configure extra PLL filtering */
  (uint32_t)0x00088433,          /* Synth: Configure synth hardware */
  (uint32_t)0x00038793,          /* Synth: Set minimum RTRIM to 3 */
  HW32_ARRAY_OVERRIDE(0x4004,1), /* Synth: Configure faster calibration */
  (uint32_t)0x1C0C0618,          /* Synth: Configure faster calibration */
  (uint32_t)0xC00401A1,          /* Synth: Configure faster calibration */
  (uint32_t)0x00010101,          /* Synth: Configure faster calibration */
  (uint32_t)0xC0040141,          /* Synth: Configure faster calibration */
  (uint32_t)0x00214AD3,          /* Synth: Configure faster calibration */
  (uint32_t)0x02980243,          /* Synth: Decrease synth programming time-out (0x0298 RAT ticks = 166 us) */
                                 /* DC/DC regulator: */
                                 /* In Tx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). */
  (uint32_t)0xFCFC08C3,          /* In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). */
  (uint32_t)0x00038883,          /* Rx: Set LNA bias current offset to adjust +3 (default: 0) */
  (uint32_t)0x000288A3,          /* Rx: Set RSSI offset to adjust reported RSSI by -2 dB (default: 0) */
  (uint32_t)0x01080263,          /* Bluetooth 5: Compensate for reduced pilot tone length */
  (uint32_t)0x08E90AA3,          /* Bluetooth 5: Compensate for reduced pilot tone length */
  (uint32_t)0x00068BA3,          /* Bluetooth 5: Compensate for reduced pilot tone length */
                                 /* Bluetooth 5: Set correct total clock accuracy for received AuxPtr */
  (uint32_t)0x0E490C83,          /* assuming local sleep clock of 50 ppm */
  (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP */
uint32_t rf_ble_overrides_1mbps[] CC_ALIGN(4) =
{
                                  /* override_ble5_setup_override_1mbps.xml */
  MCE_RFE_OVERRIDE(1,0,0,1,0,0),  /* PHY: Use MCE RAM patch (mode 0), RFE RAM patch (mode 0) */
  HW_REG_OVERRIDE(0x5320,0x0240), /* Bluetooth 5: Reduce pilot tone length */
  (uint32_t)0x013302A3,           /* Bluetooth 5: Compensate for reduced pilot tone length */
  (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP */
uint32_t rf_ble_overrides_2mbps[] CC_ALIGN(4) =
{
                                  /* override_ble5_setup_override_2mbps.xml */
  MCE_RFE_OVERRIDE(1,0,2,1,0,2),  /* PHY: Use MCE RAM patch (mode 2), RFE RAM patch (mode 2) */
  HW_REG_OVERRIDE(0x5320,0x0240), /* Bluetooth 5: Reduce pilot tone length */
  (uint32_t)0x00D102A3,           /* Bluetooth 5: Compensate for reduced pilot tone length */
  (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_BLE5_RADIO_SETUP */
uint32_t rf_ble_overrides_coded[] CC_ALIGN(4) =
{
                                  /* override_ble5_setup_override_coded.xml */
  MCE_RFE_OVERRIDE(1,0,1,1,0,1),  /* PHY: Use MCE RAM patch (mode 1), RFE RAM patch (mode 1) */
  HW_REG_OVERRIDE(0x5320,0x0240), /* Bluetooth 5: Reduce pilot tone length */
  (uint32_t)0x078902A3,           /* Bluetooth 5: Compensate for reduced pilot tone length */
  (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
/* CMD_BLE5_RADIO_SETUP: Bluetooth 5 Radio Setup Command for all PHYs */
rfc_CMD_BLE5_RADIO_SETUP_t RF_cmdBle5RadioSetup =
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
  .config.frontEndMode = 0x0,
  .config.biasMode = 0x0,
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = 0x941E,
  .pRegOverrideCommon = rf_ble_overrides_common,
  .pRegOverride1Mbps = rf_ble_overrides_1mbps,
  .pRegOverride2Mbps = rf_ble_overrides_2mbps,
  .pRegOverrideCoded = rf_ble_overrides_coded,
};
/*---------------------------------------------------------------------------*/
/* Structure for CMD_BLE5_ADV_AUX.pParams */
rfc_ble5AdvAuxPar_t rf_ble5_adv_aux_par =
{
  .pRxQ = 0,
  .rxConfig.bAutoFlushIgnored = 0x0,
  .rxConfig.bAutoFlushCrcErr = 0x0,
  .rxConfig.bAutoFlushEmpty = 0x0,
  .rxConfig.bIncludeLenByte = 0x0,
  .rxConfig.bIncludeCrc = 0x0,
  .rxConfig.bAppendRssi = 0x0,
  .rxConfig.bAppendStatus = 0x0,
  .rxConfig.bAppendTimestamp = 0x0,
  .advConfig.advFilterPolicy = 0x0,
  .advConfig.deviceAddrType = 0x0,
  .advConfig.targetAddrType = 0x0,
  .advConfig.bStrictLenFilter = 0x0,
  .advConfig.bDirected = 0x0,
  .advConfig.privIgnMode = 0x0,
  .advConfig.rpaMode = 0x0,
  .__dummy0 = 0x00,
  .auxPtrTargetType = 0x00,
  .auxPtrTargetTime = 0x00000000,
  .pAdvPkt = 0,
  .pRspPkt = 0,
  .pDeviceAddress = 0,
  .pWhiteList = 0,
};
/*---------------------------------------------------------------------------*/
/* CMD_BLE5_ADV_AUX: Bluetooth 5 Secondary Channel Advertiser Command */
rfc_CMD_BLE5_ADV_AUX_t rf_cmd_ble5_adv_aux =
{
  .commandNo = CMD_BLE5_ADV_AUX,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .channel = 0x8C,
  .whitening.init = 0x51,
  .whitening.bOverride = 0x1,
  .phyMode.mainMode = 0x0,
  .phyMode.coding = 0x0,
  .rangeDelay = 0x00,
  .txPower = 0x0000,
  .pParams = &rf_ble5_adv_aux_par,
  .pOutput = 0,
  .tx20Power = 0x00000000,
};
/*---------------------------------------------------------------------------*/
