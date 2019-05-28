/*
 * Copyright (c) 2018-2019, Texas Instruments Incorporated - http://www.ti.com/
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
 * PDU Payload length: 30
 * TX Power: 9 dBm (requires define CCFG_FORCE_VDDR_HH = 1 in ccfg.c,
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
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_ble.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_rfe_ble.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "ble-settings.h"
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF Mode Object */
RF_Mode rf_ble_mode =
{
  .rfMode = RF_MODE_MULTIPLE,
  .cpePatchFxn = &rf_patch_cpe_ble,
  .mcePatchFxn = 0,
  .rfePatchFxn = &rf_patch_rfe_ble,
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_RADIO_SETUP */
uint32_t rf_ble_overrides[] CC_ALIGN(4) =
{
  // override_use_patch_ble_1mbps.xml
  // PHY: Use MCE ROM, RFE RAM patch
  MCE_RFE_OVERRIDE(0,0,0,1,0,0),
  // override_synth_ble_1mbps.xml
  // Synth: Set recommended RTRIM to 4
  HW_REG_OVERRIDE(0x4038,0x0034),
  // Synth: Set Fref to 3.43 MHz
  (uint32_t)0x000784A3,
  // Synth: Configure fine calibration setting
  HW_REG_OVERRIDE(0x4020,0x7F00),
  // Synth: Configure fine calibration setting
  HW_REG_OVERRIDE(0x4064,0x0040),
  // Synth: Configure fine calibration setting
  (uint32_t)0xB1070503,
  // Synth: Configure fine calibration setting
  (uint32_t)0x05330523,
  // Synth: Set loop bandwidth after lock to 80 kHz
  (uint32_t)0xA47E0583,
  // Synth: Set loop bandwidth after lock to 80 kHz
  (uint32_t)0xEAE00603,
  // Synth: Set loop bandwidth after lock to 80 kHz
  (uint32_t)0x00010623,
  // Synth: Configure PLL bias
  HW32_ARRAY_OVERRIDE(0x405C,1),
  // Synth: Configure PLL bias
  (uint32_t)0x18000000,
  // Synth: Configure VCO LDO (in ADI1, set VCOLDOCFG=0x9F to use voltage input reference)
  ADI_REG_OVERRIDE(1,4,0x9F),
  // Synth: Configure synth LDO (in ADI1, set SLDOCTL0.COMP_CAP=1)
  ADI_HALFREG_OVERRIDE(1,7,0x4,0x4),
  // override_phy_ble_1mbps.xml
  // Tx: Configure symbol shape for BLE frequency deviation requirements
  (uint32_t)0x013800C3,
  // Rx: Configure AGC reference level
  HW_REG_OVERRIDE(0x6088, 0x0045),
  // Rx: Configure AGC gain level
  HW_REG_OVERRIDE(0x6084, 0x05FD),
  // Rx: Configure LNA bias current trim offset
  (uint32_t)0x00038883,
  // override_frontend_xd.xml
  // Rx: Set RSSI offset to adjust reported RSSI by +13 dB
  (uint32_t)0x00F388A3,
#if RF_TXPOWER_BOOST_MODE
  // TX power override
  // Tx: Set PA trim to max (in ADI0, set PACTL0=0xF8)
  ADI_REG_OVERRIDE(0,12,0xF8),
#endif
  (uint32_t)0xFFFFFFFF
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
  .loDivider = 0x00,
  .config.frontEndMode = 0x0, /* set by driver */
  .config.biasMode = 0x0, /* set by driver */
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = 0x5F3C, /* set by driver */
  .pRegOverride = rf_ble_overrides,
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
  .startTrigger.pastTrig = 0x1,
  .condition.rule = 0x0, /* set by driver */
  .condition.nSkip = 0x0,
  .channel = 0x00, /* set by driver */
  .whitening.init = 0x00, /* set by driver */
  .whitening.bOverride = 0x1,
  .pParams = 0x00000000, /* set by driver */
  .pOutput = 0x00000000, /* set by driver */
};
/*---------------------------------------------------------------------------*/
