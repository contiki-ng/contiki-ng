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
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_cmd.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_multi_protocol.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "prop-settings.h"
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF Mode Object */
RF_Mode rf_prop_mode =
{
  .rfMode = RF_MODE_AUTO,
  .cpePatchFxn = &rf_patch_cpe_multi_protocol,
  .mcePatchFxn = 0,
  .rfePatchFxn = 0,
};
/*---------------------------------------------------------------------------*/
#if defined(DEVICE_CC1312R) || defined(DEVICE_CC1352R)

/* Overrides for CMD_PROP_RADIO_DIV_SETUP */
uint32_t rf_prop_overrides[] CC_ALIGN(4) =
{
  // override_prop_common.xml
  // DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0x7 (DITHER_EN=0 and IPEAK=7).
  (uint32_t)0x00F788D3,
  // override_tc106.xml
  // Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[4:3]=0x3)
  ADI_2HALFREG_OVERRIDE(0,16,0x8,0x8,17,0x1,0x1),
  // Rx: Set AGC reference level to 0x1A (default: 0x2E)
  HW_REG_OVERRIDE(0x609C,0x001A),
  // Rx: Set RSSI offset to adjust reported RSSI by -1 dB (default: -2), trimmed for external bias and differential configuration
  (uint32_t)0x000188A3,
  // Rx: Set anti-aliasing filter bandwidth to 0xD (in ADI0, set IFAMPCTL3[7:4]=0xD)
  ADI_HALFREG_OVERRIDE(0,61,0xF,0xD),
  // Tx: Set wait time before turning off ramp to 0x1A (default: 0x1F)
  HW_REG_OVERRIDE(0x6028,0x001A),
#if RF_TXPOWER_BOOST_MODE
  // TX power override
  // Tx: Set PA trim to max to maximize its output power (in ADI0, set PACTL0=0xF8)
  ADI_REG_OVERRIDE(0,12,0xF8),
#endif
  (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
/* CMD_PROP_RADIO_DIV_SETUP: Proprietary Mode Radio Setup Command for All Frequency Bands */
rfc_CMD_PROP_RADIO_DIV_SETUP_t rf_cmd_prop_radio_div_setup =
{
  .commandNo = CMD_PROP_RADIO_DIV_SETUP,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .modulation.modType = 0x1,
  .modulation.deviation = 0x64,
  .modulation.deviationStepSz = 0x0,
  .symbolRate.preScale = 0xF,
  .symbolRate.rateWord = 0x8000,
  .symbolRate.decimMode = 0x0,
  .rxBw = 0x52,
  .preamConf.nPreamBytes = 0x3,
  .preamConf.preamMode = 0x0,
  .formatConf.nSwBits = 0x18,
  .formatConf.bBitReversal = 0x0,
  .formatConf.bMsbFirst = 0x1,
  .formatConf.fecMode = 0x0,
  .formatConf.whitenMode = 0x7,
  .config.frontEndMode = 0x0, /* set by driver */
  .config.biasMode = 0x0, /* set by driver */
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = 0xCC14, /* set by driver */
  .pRegOverride = rf_prop_overrides,
  .centerFreq = 0x0393, /* set by driver */
  .intFreq = 0x8000, /* set by driver */
  .loDivider = 0x05, /* set by driver */
};

#endif /* defined(DEVICE_CC1312R) || defined(DEVICE_CC1352R) */
/*---------------------------------------------------------------------------*/
#if defined(DEVICE_CC1352P)

/* Overrides for CMD_PROP_RADIO_DIV_SETUP with high PA */
uint32_t rf_prop_overrides[] CC_ALIGN(4) =
{
  // override_tc706.xml
  // Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[4:3]=0x3)
  ADI_2HALFREG_OVERRIDE(0,16,0x8,0x8,17,0x1,0x1),
  // Rx: Set AGC reference level to 0x1A (default: 0x2E)
  HW_REG_OVERRIDE(0x609C,0x001A),
  // Rx: Set RSSI offset to adjust reported RSSI by -1 dB (default: -2), trimmed for external bias and differential configuration
  (uint32_t)0x000188A3,
  // Rx: Set anti-aliasing filter bandwidth to 0xD (in ADI0, set IFAMPCTL3[7:4]=0xD)
  ADI_HALFREG_OVERRIDE(0,61,0xF,0xD),
  // override_prop_common.xml
  // DC/DC regulator: In Tx with 14 dBm PA setting, use DCDCCTL5[3:0]=0xF (DITHER_EN=1 and IPEAK=7). In Rx, use default settings.
  (uint32_t)0x00F788D3,
#if RF_TXPOWER_BOOST_MODE
  // TX power override
  // Tx: Set PA trim to max to maximize its output power (in ADI0, set PACTL0=0xF8)
  ADI_REG_OVERRIDE(0,12,0xF8),
#endif
  (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_PROP_RADIO_DIV_SETUP with defualt PA */
uint32_t rf_prop_overrides_tx_std[] CC_ALIGN(4) =
{
  // The TX Power element should always be the first in the list
  TX_STD_POWER_OVERRIDE(0xB224),
  // The ANADIV radio parameter based on the LO divider (0) and front-end (0) settings
  (uint32_t)0x11310703,
  // override_phy_tx_pa_ramp_genfsk_std.xml
  // Tx: Configure PA ramping, set wait time before turning off (0x1A ticks of 16/24 us = 17.3 us).
  HW_REG_OVERRIDE(0x6028,0x001A),
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_PROP_RADIO_DIV_SETUP with defualt PA */
uint32_t rf_prop_overrides_tx_20[] CC_ALIGN(4) =
{
  // The TX Power element should always be the first in the list
  TX20_POWER_OVERRIDE(0x001B8ED2),
  // The ANADIV radio parameter based on the LO divider (0) and front-end (0) settings
  (uint32_t)0x11C10703,
  // override_phy_tx_pa_ramp_genfsk_hpa.xml
  // Tx: Configure PA ramping, set wait time before turning off (0x1F ticks of 16/24 us = 20.3 us).
  HW_REG_OVERRIDE(0x6028,0x001F),
  (uint32_t)0xFFFFFFFF
};
/*---------------------------------------------------------------------------*/
/* CMD_PROP_RADIO_DIV_SETUP: Proprietary Mode Radio Setup Command for All Frequency Bands */
rfc_CMD_PROP_RADIO_DIV_SETUP_PA_t rf_cmd_prop_radio_div_setup =
{
  .commandNo = 0x3807,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .modulation.modType = 0x1,
  .modulation.deviation = 0x64,
  .modulation.deviationStepSz = 0x0,
  .symbolRate.preScale = 0xF,
  .symbolRate.rateWord = 0x8000,
  .symbolRate.decimMode = 0x0,
  .rxBw = 0x52,
  .preamConf.nPreamBytes = 0x7,
  .preamConf.preamMode = 0x0,
  .formatConf.nSwBits = 0x18,
  .formatConf.bBitReversal = 0x0,
  .formatConf.bMsbFirst = 0x1,
  .formatConf.fecMode = 0x0,
  .formatConf.whitenMode = 0x7,
  .config.frontEndMode = 0x0, /* set by driver */
  .config.biasMode = 0x0, /* set by driver */
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = 0xB224, /* set by driver */
  .pRegOverride = rf_prop_overrides,
  .centerFreq = 0x0393, /* set by driver */
  .intFreq = 0x8000, /* set by driver */
  .loDivider = 0x05, /* set by driver */
  .pRegOverrideTxStd = rf_prop_overrides_tx_std,
  .pRegOverrideTx20 = rf_prop_overrides_tx_20,
};

#endif /* defined(DEVICE_CC1352P) */
/*---------------------------------------------------------------------------*/
/* CMD_FS: Frequency Synthesizer Programming Command */
rfc_CMD_FS_t rf_cmd_prop_fs =
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
  .frequency = 0x0393, /* set by driver */
  .fractFreq = 0x0000, /* set by driver */
  .synthConf.bTxMode = 0x0,
  .synthConf.refFreq = 0x0,
  .__dummy0 = 0x00,
  .__dummy1 = 0x00,
  .__dummy2 = 0x00,
  .__dummy3 = 0x0000,
};
/*---------------------------------------------------------------------------*/
/* CMD_PROP_TX_ADV: Proprietary Mode Advanced Transmit Command */
rfc_CMD_PROP_TX_ADV_t rf_cmd_prop_tx_adv =
{
  .commandNo = CMD_PROP_TX_ADV,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .pktConf.bFsOff = 0x0,
  .pktConf.bUseCrc = 0x1,
  .pktConf.bCrcIncSw = 0x0,
  .pktConf.bCrcIncHdr = 0x0,
  .numHdrBits = 0x10,
  .pktLen = 0x0, /* set by driver */
  .startConf.bExtTxTrig = 0x0,
  .startConf.inputMode = 0x0,
  .startConf.source = 0x0,
  .preTrigger.triggerType = TRIG_REL_START,
  .preTrigger.bEnaCmd = 0x0,
  .preTrigger.triggerNo = 0x0,
  .preTrigger.pastTrig = 0x1,
  .preTime = 0x00000000,
  .syncWord = 0x0055904E,
  .pPkt = 0, /* set by driver */
};
/*---------------------------------------------------------------------------*/
/* CMD_PROP_RX_ADV: Proprietary Mode Advanced Receive Command */
rfc_CMD_PROP_RX_ADV_t rf_cmd_prop_rx_adv =
{
  .commandNo = CMD_PROP_RX_ADV,
  .status = IDLE,
  .pNextOp = 0,
  .startTime = 0x00000000,
  .startTrigger.triggerType = TRIG_NOW,
  .startTrigger.bEnaCmd = 0x0,
  .startTrigger.triggerNo = 0x0,
  .startTrigger.pastTrig = 0x0,
  .condition.rule = COND_NEVER,
  .condition.nSkip = 0x0,
  .pktConf.bFsOff = 0x0,
  .pktConf.bRepeatOk = 0x1,
  .pktConf.bRepeatNok = 0x1,
  .pktConf.bUseCrc = 0x1,
  .pktConf.bCrcIncSw = 0x0,
  .pktConf.bCrcIncHdr = 0x0,
  .pktConf.endType = 0x0,
  .pktConf.filterOp = 0x1,
  .rxConf.bAutoFlushIgnored = 0x1,
  .rxConf.bAutoFlushCrcErr = 0x1,
  .rxConf.bIncludeHdr = 0x0,
  .rxConf.bIncludeCrc = 0x0,
  .rxConf.bAppendRssi = 0x1,
  .rxConf.bAppendTimestamp = 0x1,
  .rxConf.bAppendStatus = 0x1,
  .syncWord0 = 0x0055904E,
  .syncWord1 = 0x00000000,
  .maxPktLen = 0x0, /* set by driver */
  .hdrConf.numHdrBits = 0x10,
  .hdrConf.lenPos = 0x0,
  .hdrConf.numLenBits = 0x0B,
  .addrConf.addrType = 0x0,
  .addrConf.addrSize = 0x0,
  .addrConf.addrPos = 0x0,
  .addrConf.numAddr = 0x0,
  .lenOffset = 0xFC,
  .endTrigger.triggerType = TRIG_NEVER,
  .endTrigger.bEnaCmd = 0x0,
  .endTrigger.triggerNo = 0x0,
  .endTrigger.pastTrig = 0x0,
  .endTime = 0x00000000,
  .pAddr = 0, /* set by driver */
  .pQueue = 0, /* set by driver */
  .pOutput = 0, /* set by driver */
};
/*---------------------------------------------------------------------------*/
