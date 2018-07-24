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
 * Adv. Data: 255
 * BLE Channel: 17
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
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_ble.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_rfe_ble.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "ble-settings.h"
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF Mode Object */
RF_Mode rf_ble_mode =
{
  .rfMode = RF_MODE_BLE,
  .cpePatchFxn = &rf_patch_cpe_ble,
  .mcePatchFxn = 0,
  .rfePatchFxn = &rf_patch_rfe_ble,
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_RADIO_SETUP */
uint32_t rf_ble_overrides[] CC_ALIGN(4) =
{
                                   /* override_use_patch_ble_1mbps.xml */
  MCE_RFE_OVERRIDE(0,0,0,1,0,0),   /* PHY: Use MCE ROM, RFE RAM patch */
                                   /* override_synth_ble_1mbps.xml */
  HW_REG_OVERRIDE(0x4038,0x0035),  /* Synth: Set recommended RTRIM to 5 */
  (uint32_t)0x000784A3,            /* Synth: Set Fref to 3.43 MHz */
  (uint32_t)0xA47E0583,            /* Synth: Set loop bandwidth after lock to 80 kHz */
  (uint32_t)0xEAE00603,            /* Synth: Set loop bandwidth after lock to 80 kHz */
  (uint32_t)0x00010623,            /* Synth: Set loop bandwidth after lock to 80 kHz */
  HW32_ARRAY_OVERRIDE(0x405C,1),   /* Synth: Configure PLL bias */
  (uint32_t)0x1801F800,            /* Synth: Configure PLL bias */
  HW32_ARRAY_OVERRIDE(0x402C,1),   /* Synth: Configure PLL latency */
  (uint32_t)0x00608402,            /* Synth: Configure PLL latency */
  (uint32_t)0x02010403,            /* Synth: Use 24 MHz XOSC as synth clock, enable extra PLL filtering */
  HW32_ARRAY_OVERRIDE(0x4034,1),   /* Synth: Configure extra PLL filtering */
  (uint32_t)0x177F0408,            /* Synth: Configure extra PLL filtering */
  (uint32_t)0x38000463,            /* Synth: Configure extra PLL filtering */
                                   /* override_phy_ble_1mbps.xml */
  (uint32_t)0x013800C3,            /* Tx: Configure symbol shape for BLE frequency deviation requirements */
  HW_REG_OVERRIDE(0x6088, 0x0045), /* Rx: Configure AGC reference level */
                                   /* Tx: Configure pilot tone length to ensure stable frequency */
  HW_REG_OVERRIDE(0x52AC, 0x0360), /* before start of packet */
  (uint32_t)0x01AD02A3,            /* Tx: Compensate timing offset to match new pilot tone setting */
  (uint32_t)0x01680263,            /* Tx: Compensate timing offset to match new pilot tone setting */
  (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
/* CMD_RADIO_SETUP: Radio Setup Command for Pre-Defined Schemes */
rfc_CMD_RADIO_SETUP_t rf_ble_cmd_radio_setup =
{
  .commandNo = CMD_RADIO_SETUP,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .mode = 0x00,
  .__dummy0 = 0x00,
  .config.frontEndMode = 0x0,
  .config.biasMode = 0x0,
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = 0x9330,
  .pRegOverride = rf_ble_overrides,
};
/*---------------------------------------------------------------------------*/
/* Structure for CMD_BLE_ADV_NC.pParams */
rfc_bleAdvPar_t rf_ble_adv_par =
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
  .advConfig.peerAddrType = 0x0,
  .advConfig.bStrictLenFilter = 0x0,
  .advConfig.rpaMode = 0x0,
  .advLen = 0x18,
  .scanRspLen = 0x00,
  .pAdvData = 0,
  .pScanRspData = 0,
  .pDeviceAddress = 0,
  .pWhiteList = 0,
  .__dummy0 = 0x0000,
  .__dummy1 = 0x00,
  .endTrigger.triggerType = TRIG_NEVER,
  .endTrigger.bEnaCmd = 0x0,
  .endTrigger.triggerNo = 0x0,
  .endTrigger.pastTrig = 0x0,
  .endTime = 0x00000000,
};
/*---------------------------------------------------------------------------*/
/* CMD_BLE_ADV_NC: BLE Non-Connectable Advertiser Command */
rfc_CMD_BLE_ADV_NC_t rf_ble_cmd_ble_adv_nc =
{
  .commandNo = CMD_BLE_ADV_NC,
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
  .pParams = &rf_ble_adv_par,
  .pOutput = 0,
};
/*---------------------------------------------------------------------------*/
