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
 * IEEE Channel: 11
 * Frequency: 2405 MHz
 * SFD: 0
 * Packet Data: 255
 * Preamble (32 bit): 01010101...
 * TX Power: 5 dBm
 */
/*---------------------------------------------------------------------------*/
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_ieee_cmd.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_ieee.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "ieee-settings.h"
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF Mode Object */
RF_Mode rf_ieee_mode =
{
  .rfMode = RF_MODE_MULTIPLE,
  .cpePatchFxn = &rf_patch_cpe_ieee,
  .mcePatchFxn = 0,
  .rfePatchFxn = 0,
};/*---------------------------------------------------------------------------*/
/* Overrides for CMD_RADIO_SETUP */
uint32_t rf_ieee_overrides[] CC_ALIGN(4) =
{
  // override_synth_ieee_15_4.xml
  // Synth: Set recommended RTRIM to 5
  HW_REG_OVERRIDE(0x4038,0x0035),
  // Synth: Set Fref to 3.43 MHz
  (uint32_t)0x000784A3,
  // Synth: Set loop bandwidth after lock to 80 kHz
  (uint32_t)0xA47E0583,
  // Synth: Set loop bandwidth after lock to 80 kHz
  (uint32_t)0xEAE00603,
  // Synth: Set loop bandwidth after lock to 80 kHz
  (uint32_t)0x00010623,
  // Synth: Configure PLL bias
  HW32_ARRAY_OVERRIDE(0x405C,1),
  // Synth: Configure PLL bias
  (uint32_t)0x1801F800,
  // Synth: Configure PLL latency
  HW32_ARRAY_OVERRIDE(0x402C,1),
  // Synth: Configure PLL latency
  (uint32_t)0x00608402,
  // Synth: Use 24 MHz XOSC as synth clock, enable extra PLL filtering
  (uint32_t)0x02010403,
  // Synth: Configure extra PLL filtering
  HW32_ARRAY_OVERRIDE(0x4034,1),
  // Synth: Configure extra PLL filtering
  (uint32_t)0x177F0408,
  // Synth: Configure extra PLL filtering
  (uint32_t)0x38000463,
  // override_phy_ieee_15_4.xml
  // Synth: Increase synth programming timeout
  (uint32_t)0x05000243,
  // Rx: Adjust Rx FIFO threshold to avoid overflow
  (uint32_t)0x002082C3,
  // override_frontend_id.xml
  // Rx: Set RSSI offset to adjust reported RSSI by -2 dB
  (uint32_t)0x000288A3,
  // Rx: Configure LNA bias current trim offset
  (uint32_t)0x000F8883,
  // Rx: Adjust AGC DC filter
  HW_REG_OVERRIDE(0x50DC,0x002B),
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* CMD_RADIO_SETUP: Radio Setup Command for Pre-Defined Schemes */
rfc_CMD_RADIO_SETUP_t rf_cmd_ieee_radio_setup =
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
  .mode = 0x01,
  .__dummy0 = 0x00,
  .config.frontEndMode = 0x0, /* set by driver */
  .config.biasMode = 0x0, /* set by driver */
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = 0x9330, /* set by driver */
  .pRegOverride = rf_ieee_overrides,
};
/*---------------------------------------------------------------------------*/
/* CMD_FS: Frequency Synthesizer Programming Command */
rfc_CMD_FS_t rf_cmd_ieee_fs =
{
  .commandNo = CMD_FS,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .frequency = 0x0965, /* set by driver */
  .fractFreq = 0x0000, /* set by driver */
  .synthConf.bTxMode = 0x1,
  .synthConf.refFreq = 0x0,
  .__dummy0 = 0x00,
  .__dummy1 = 0x00,
  .__dummy2 = 0x00,
  .__dummy3 = 0x0000,
};
/*---------------------------------------------------------------------------*/
/* CMD_IEEE_TX: IEEE 802.15.4 Transmit Command */
rfc_CMD_IEEE_TX_t rf_cmd_ieee_tx =
{
  .commandNo = CMD_IEEE_TX,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .txOpt.bIncludePhyHdr = 0x0,
  .txOpt.bIncludeCrc = 0x0,
  .txOpt.payloadLenMsb = 0x0,
  .payloadLen = 0x0, /* set by driver */
  .pPayload = 0, /* set by driver */
  .timeStamp = 0x00000000,
};
/*---------------------------------------------------------------------------*/
/* CMD_IEEE_RX: IEEE 802.15.4 Receive Command */
rfc_CMD_IEEE_RX_t rf_cmd_ieee_rx =
{
  .commandNo = CMD_IEEE_RX,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .channel = 0x00, /* set by driver */
  .rxConfig.bAutoFlushCrc = 0x1,
  .rxConfig.bAutoFlushIgn = 0x1,
  .rxConfig.bIncludePhyHdr = 0x0,
  .rxConfig.bIncludeCrc = 0x1,
  .rxConfig.bAppendRssi = 0x1,
  .rxConfig.bAppendCorrCrc = 0x1,
  .rxConfig.bAppendSrcInd = 0x0,
  .rxConfig.bAppendTimestamp = 0x1,
  .pRxQ = 0, /* set by driver */
  .pOutput = 0, /* set by driver */
  .frameFiltOpt.frameFiltEn = 0x0, /* set by driver */
  .frameFiltOpt.frameFiltStop = 0x1,
  .frameFiltOpt.autoAckEn = 0x0, /* set by driver */
  .frameFiltOpt.slottedAckEn = 0x0,
  .frameFiltOpt.autoPendEn = 0x0,
  .frameFiltOpt.defaultPend = 0x0,
  .frameFiltOpt.bPendDataReqOnly = 0x0,
  .frameFiltOpt.bPanCoord = 0x0,
  .frameFiltOpt.maxFrameVersion = 0x2,
  .frameFiltOpt.fcfReservedMask = 0x0,
  .frameFiltOpt.modifyFtFilter = 0x0,
  .frameFiltOpt.bStrictLenFilter = 0x0,
  .frameTypes.bAcceptFt0Beacon = 0x1,
  .frameTypes.bAcceptFt1Data = 0x1,
  .frameTypes.bAcceptFt2Ack = 0x1,
  .frameTypes.bAcceptFt3MacCmd = 0x1,
  .frameTypes.bAcceptFt4Reserved = 0x1,
  .frameTypes.bAcceptFt5Reserved = 0x1,
  .frameTypes.bAcceptFt6Reserved = 0x1,
  .frameTypes.bAcceptFt7Reserved = 0x1,
  .ccaOpt.ccaEnEnergy = 0x1,
  .ccaOpt.ccaEnCorr = 0x1,
  .ccaOpt.ccaEnSync = 0x1,
  .ccaOpt.ccaCorrOp = 0x1,
  .ccaOpt.ccaSyncOp = 0x0,
  .ccaOpt.ccaCorrThr = 0x3,
  .ccaRssiThr = 0x0, /* set by driver */
  .__dummy0 = 0x00,
  .numExtEntries = 0x00,
  .numShortEntries = 0x00,
  .pExtEntryList = 0,
  .pShortEntryList = 0,
  .localExtAddr = 0x0, /* set by driver */
  .localShortAddr = 0x0, /* set by driver */
  .localPanID = 0x0000,
  .__dummy1 = 0x000000,
  .endTrigger.triggerType = TRIG_NEVER,
  .endTrigger.bEnaCmd = 0x0,
  .endTrigger.triggerNo = 0x0,
  .endTrigger.pastTrig = 0x0,
  .endTime = 0x00000000,
};
/*---------------------------------------------------------------------------*/
/* CMD_IEEE_RX_ACK: IEEE 802.15.4 Receive ACK Command */
rfc_CMD_IEEE_RX_ACK_t rf_cmd_ieee_rx_ack =
{
  .commandNo = CMD_IEEE_RX_ACK,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .seqNo = 0x0,
  .endTrigger.triggerType = TRIG_NEVER,
  .endTrigger.bEnaCmd = 0x0,
  .endTrigger.triggerNo = 0x0,
  .endTrigger.pastTrig = 0x0,
  .endTime = 0x00000000,
};
/*---------------------------------------------------------------------------*/
