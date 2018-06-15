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
// Parameter summary
// Address: 0
// Address0: 0xAA
// Address1: 0xBB
// Frequency: 868.00000 MHz
// Data Format: Serial mode disable
// Deviation: 25.000 kHz
// pktLen: 30
// 802.15.4g Mode: 0
// Select bit order to transmit PSDU octets:: 1
// Packet Length Config: Variable
// Max Packet Length: 255
// Packet Length: 0
// Packet Data: 255
// RX Filter BW: 98 kHz
// Symbol Rate: 50.00000 kBaud
// Sync Word Length: 24 Bits
// TX Power: 14 dBm (requires define CCFG_FORCE_VDDR_HH = 1 in ccfg.c, see CC13xx/CC26xx Technical Reference Manual)
// Whitening: Dynamically IEEE 802.15.4g compatible whitener and 16/32-bit CRC
/*---------------------------------------------------------------------------*/
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_cmd.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_genfsk.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_rfe_genfsk.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "prop-settings.h"
/*---------------------------------------------------------------------------*/
// TI-RTOS RF Mode Object
RF_Mode rf_prop_mode =
{
    .rfMode = RF_MODE_PROPRIETARY_SUB_1,
    .cpePatchFxn = &rf_patch_cpe_genfsk,
    .mcePatchFxn = 0,
    .rfePatchFxn = &rf_patch_rfe_genfsk,
};
/*---------------------------------------------------------------------------*/
// TX Power table
// The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
// RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
// See the Technical Reference Manual for further details about the "txPower" Command field.
// The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
RF_TxPowerTable_Entry rf_prop_tx_power_table[RF_PROP_TX_POWER_TABLE_SIZE+1] =
{
    { -10, RF_TxPowerTable_DEFAULT_PA_ENTRY( 0, 3, 0,  2) },
    {   0, RF_TxPowerTable_DEFAULT_PA_ENTRY( 3, 3, 0,  9) },
    {   1, RF_TxPowerTable_DEFAULT_PA_ENTRY( 4, 3, 0, 11) },
    {   2, RF_TxPowerTable_DEFAULT_PA_ENTRY( 5, 3, 0, 12) },
    {   3, RF_TxPowerTable_DEFAULT_PA_ENTRY( 6, 3, 0, 14) },
    {   4, RF_TxPowerTable_DEFAULT_PA_ENTRY( 4, 1, 0, 12) },
    {   5, RF_TxPowerTable_DEFAULT_PA_ENTRY( 8, 3, 0, 16) },
    {   6, RF_TxPowerTable_DEFAULT_PA_ENTRY( 9, 3, 0, 18) },
    {   7, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 21) },
    {   8, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 0, 25) },
    {   9, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 3, 0, 32) },
    {  10, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 3, 0, 44) },
    {  11, RF_TxPowerTable_DEFAULT_PA_ENTRY(37, 3, 0, 72) },
    {  12, RF_TxPowerTable_DEFAULT_PA_ENTRY(43, 0, 0, 94) },
    // This setting requires CCFG_FORCE_VDDR_HH = 1.
    {  14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1, 85) },
    RF_TxPowerTable_TERMINATION_ENTRY
};
/*---------------------------------------------------------------------------*/
// Overrides for CMD_PROP_RADIO_DIV_SETUP
uint32_t rf_prop_overrides[] CC_ALIGN(4) =
{
                                        // override_use_patch_prop_genfsk.xml
    MCE_RFE_OVERRIDE(0,4,0,1,0,0),      // PHY: Use MCE ROM bank 4, RFE RAM patch
                                        // override_synth_prop_863_930_div5.xml
    HW_REG_OVERRIDE(0x4038,0x0037),     // Synth: Set recommended RTRIM to 7
    (uint32_t)0x000684A3,               // Synth: Set Fref to 4 MHz
    HW_REG_OVERRIDE(0x4020,0x7F00),     // Synth: Configure fine calibration setting
    HW_REG_OVERRIDE(0x4064,0x0040),     // Synth: Configure fine calibration setting
    (uint32_t)0xB1070503,               // Synth: Configure fine calibration setting
    (uint32_t)0x05330523,               // Synth: Configure fine calibration setting
    (uint32_t)0x0A480583,               // Synth: Set loop bandwidth after lock to 20 kHz
    (uint32_t)0x7AB80603,               // Synth: Set loop bandwidth after lock to 20 kHz
    ADI_REG_OVERRIDE(1,4,0x9F),         // Synth: Configure VCO LDO (in ADI1, set VCOLDOCFG=0x9F to use voltage input reference)
    ADI_HALFREG_OVERRIDE(1,7,0x4,0x4),  // Synth: Configure synth LDO (in ADI1, set SLDOCTL0.COMP_CAP=1)
    (uint32_t)0x02010403,               // Synth: Use 24 MHz XOSC as synth clock, enable extra PLL filtering
    (uint32_t)0x00108463,               // Synth: Configure extra PLL filtering
    (uint32_t)0x04B00243,               // Synth: Increase synth programming timeout (0x04B0 RAT ticks = 300 us)
                                        // override_phy_rx_aaf_bw_0xd.xml
    ADI_HALFREG_OVERRIDE(0,61,0xF,0xD), // Rx: Set anti-aliasing filter bandwidth to 0xD (in ADI0, set IFAMPCTL3[7:4]=0xD)
                                        // override_phy_gfsk_rx.xml
    (uint32_t)0x00038883,               // Rx: Set LNA bias current trim offset to 3
    HW_REG_OVERRIDE(0x6084,0x35F1),     // Rx: Freeze RSSI on sync found event
                                        // override_phy_gfsk_pa_ramp_agc_reflevel_0x1a.xml
    HW_REG_OVERRIDE(0x6088,0x411A),     // Tx: Configure PA ramping setting (0x41). Rx: Set AGC reference level to 0x1A.
    HW_REG_OVERRIDE(0x608C,0x8213),     // Tx: Configure PA ramping setting
                                        // override_crc_ieee_802_15_4.xml
    (uint32_t)0x00000943,               // IEEE 802.15.4g: Fix incorrect initialization value for CRC-16 calculation (see TRM section 23.7.5.2.1)
    (uint32_t)0x00000963,               // IEEE 802.15.4g: Fix incorrect initialization value for CRC-16 calculation (see TRM section 23.7.5.2.1)
                                        // override_phy_rx_rssi_offset_5db.xml
    (uint32_t)0x00FB88A3,               // Rx: Set RSSI offset to adjust reported RSSI by +5 dB
                                        // TX power override
    ADI_REG_OVERRIDE(0,12,0xF8),        // Tx: Set PA trim to max (in ADI0, set PACTL0=0xF8)
    (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
// CMD_PROP_RADIO_DIV_SETUP
// Proprietary Mode Radio Setup Command for All Frequency Bands
rfc_CMD_PROP_RADIO_DIV_SETUP_t rf_cmd_prop_radio_div_setup =
{
    .commandNo = CMD_PROP_RADIO_DIV_SETUP,
    .status = IDLE,
    .pNextOp = 0,
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = COND_NEVER,
    .condition.nSkip = 0x0,
    .modulation.modType = 0x1,
    .modulation.deviation = 0x64,
    .symbolRate.preScale = 0xF,
    .symbolRate.rateWord = 0x8000,
    .rxBw = 0x52,
    .preamConf.nPreamBytes = 0x3,
    .preamConf.preamMode = 0x0,
    .formatConf.nSwBits = 0x18,
    .formatConf.bBitReversal = 0x0,
    .formatConf.bMsbFirst = 0x1,
    .formatConf.fecMode = 0x0,
    .formatConf.whitenMode = 0x7,
    .config.frontEndMode = 0x0,
    .config.biasMode = 0x1,
    .config.analogCfgMode = 0x0,
    .config.bNoFsPowerUp = 0x0,
    .txPower = 0xAB3F,
    .pRegOverride = rf_prop_overrides,
    .centerFreq = 0x0364,
    .intFreq = 0x8000,
    .loDivider = 0x05,
};
/*---------------------------------------------------------------------------*/
// CMD_FS
// Frequency Synthesizer Programming Command
rfc_CMD_FS_t rf_cmd_prop_fs =
{
    .commandNo = CMD_FS,
    .status = IDLE,
    .pNextOp = 0,
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = COND_NEVER,
    .condition.nSkip = 0x0,
    .frequency = 0x0364,
    .fractFreq = 0x0000,
    .synthConf.bTxMode = 0x0,
    .synthConf.refFreq = 0x0,
    .__dummy0 = 0x00,
    .__dummy1 = 0x00,
    .__dummy2 = 0x00,
    .__dummy3 = 0x0000,
};
/*---------------------------------------------------------------------------*/
// CMD_PROP_TX_ADV
// Proprietary Mode Advanced Transmit Command
rfc_CMD_PROP_TX_ADV_t rf_cmd_prop_tx_adv =
{
    .commandNo = CMD_PROP_TX_ADV,
    .status = IDLE,
    .pNextOp = 0,
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x2,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x1,
    .condition.rule = COND_NEVER,
    .condition.nSkip = 0x0,
    .pktConf.bFsOff = 0x0,
    .pktConf.bUseCrc = 0x1,
    .pktConf.bCrcIncSw = 0x0,
    .pktConf.bCrcIncHdr = 0x0,
    .numHdrBits = 0x10,
    .pktLen = 0x0000,
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
// CMD_PROP_RX_ADV
// Proprietary Mode Advanced Receive Command
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
    .rxConf.bAppendTimestamp = 0x0,
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
