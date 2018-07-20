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
 * For Default PA:
 *      Enable high output power PA: false
 *      TX Power: 5 dBm (requires define CCFG_FORCE_VDDR_HH = 0 in ccfg.c,
 *                       see CC13xx/CC26xx Technical Reference Manual)
 * For High PA:
 *      Enable high output power PA: true
 *      TX Power: 20 dBm (requires define CCFG_FORCE_VDDR_HH = 0 in ccfg.c,
 *                        see CC13xx/CC26xx Technical Reference Manual)
 */
/*---------------------------------------------------------------------------*/
#include "sys/cc.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_ieee_cmd.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_ieee_802_15_4.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_mce_ieee_802_15_4.h)

#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
#include "ieee-settings.h"
/*---------------------------------------------------------------------------*/
/* TI-RTOS RF Mode Object */
RF_Mode rf_ieee_mode =
{
  .rfMode = RF_MODE_AUTO,
  .cpePatchFxn = &rf_patch_cpe_ieee_802_15_4,
  .mcePatchFxn = &rf_patch_mce_ieee_802_15_4,
  .rfePatchFxn = 0,
};
/*---------------------------------------------------------------------------*/
#if defined(DEVICE_CC1352R)
/*
 * TX Power table
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
RF_TxPowerTable_Entry rf_ieee_tx_power_table[] =
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

/*
 * TX power table size, with one less entry excluding the
 * termination entry.
 */
const size_t rf_ieee_tx_power_table_size =
    (sizeof(rf_ieee_tx_power_table) / sizeof(rf_ieee_tx_power_table[0])) - 1;

/*
 * CMD_RADIO_SETUP must be configured with default TX power value
 * in the .txPower field.
 */
#define DEFAULT_TX_POWER    0x941E /* 5 dBm */
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_RADIO_SETUP */
uint32_t rf_ieee_overrides[] CC_ALIGN(4) =
{
                                  /* override_ieee_802_15_4.xml */
  MCE_RFE_OVERRIDE(1,0,0,0,1,0),  /* PHY: Use MCE RAM patch, RFE ROM bank 1 */
  (uint32_t)0x02400403,           /* Synth: Use 48 MHz crystal, enable extra PLL filtering */
  (uint32_t)0x001C8473,           /* Synth: Configure extra PLL filtering */
  (uint32_t)0x00088433,           /* Synth: Configure synth hardware */
  (uint32_t)0x00038793,           /* Synth: Set minimum RTRIM to 3 */
  HW32_ARRAY_OVERRIDE(0x4004,1),  /* Synth: Configure faster calibration */
  (uint32_t)0x1C0C0618,           /* Synth: Configure faster calibration */
  (uint32_t)0xC00401A1,           /* Synth: Configure faster calibration */
  (uint32_t)0x00010101,           /* Synth: Configure faster calibration */
  (uint32_t)0xC0040141,           /* Synth: Configure faster calibration */
  (uint32_t)0x00214AD3,           /* Synth: Configure faster calibration */
  (uint32_t)0x02980243,           /* Synth: Decrease synth programming time-out (0x0298 RAT ticks = 166 us) */
                                  /* DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). */
  (uint32_t)0xFCFC08C3,           /* In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). */
  (uint32_t)0x000F8883,           /* Rx: Set LNA bias current offset to +15 to saturate trim to max (default: 0) */
  (uint32_t)0xFFFFFFFF,
};

#endif /* defined(DEVICE_CC1352R) */
/*---------------------------------------------------------------------------*/
#if defined(DEVICE_CC1352P)

#if RF_CONF_TXPOWER_HIGH_PA
/*
 * TX Power table
 * The RF_TxPowerTable_HIGH_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_HIGH_PA_ENTRY(bias, ibboost, boost, coefficient, ldoTrim)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
RF_TxPowerTable_Entry rf_ieee_tx_power_table[] =
{
  {  0, RF_TxPowerTable_HIGH_PA_ENTRY(29, 0, 1, 17,  1) },
  {  3, RF_TxPowerTable_HIGH_PA_ENTRY(39, 0, 1, 20,  1) },
  {  6, RF_TxPowerTable_HIGH_PA_ENTRY(46, 0, 1, 26,  7) },
  {  9, RF_TxPowerTable_HIGH_PA_ENTRY(40, 0, 1, 39, 41) },
  { 10, RF_TxPowerTable_HIGH_PA_ENTRY(23, 2, 1, 65,  5) },
  { 11, RF_TxPowerTable_HIGH_PA_ENTRY(24, 2, 1, 29,  7) },
  { 12, RF_TxPowerTable_HIGH_PA_ENTRY(19, 2, 1, 16, 25) },
  { 13, RF_TxPowerTable_HIGH_PA_ENTRY(27, 2, 1, 19, 13) },
  { 14, RF_TxPowerTable_HIGH_PA_ENTRY(24, 2, 1, 19, 27) },
  { 15, RF_TxPowerTable_HIGH_PA_ENTRY(23, 2, 1, 20, 39) },
  { 16, RF_TxPowerTable_HIGH_PA_ENTRY(34, 2, 1, 26, 23) },
  { 17, RF_TxPowerTable_HIGH_PA_ENTRY(38, 2, 1, 33, 25) },
  { 18, RF_TxPowerTable_HIGH_PA_ENTRY(30, 2, 1, 37, 53) },
  { 19, RF_TxPowerTable_HIGH_PA_ENTRY(36, 2, 1, 57, 59) },
  { 20, RF_TxPowerTable_HIGH_PA_ENTRY(56, 2, 1, 45, 63) },
  RF_TxPowerTable_TERMINATION_ENTRY
};

/*
 * TX power table size, with one less entry excluding the
 * termination entry.
 */
const size_t rf_ieee_tx_power_table_size =
    (sizeof(rf_ieee_tx_power_table) / sizeof(rf_ieee_tx_power_table[0])) - 1;

/*
 * CMD_PROP_RADIO_DIV_SETUP must be configured with default TX power value
 * in the .txPower field. For High PA, this must be 0xFFFF.
 */
#define DEFAULT_TX_POWER    0xFFFF /* High PA */
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_RADIO_SETUP with high PA */
uint32_t rf_ieee_overrides[] CC_ALIGN(4) =
{
                                  /* override_ieee_802_15_4.xml */
  MCE_RFE_OVERRIDE(1,0,0,0,1,0),  /* PHY: Use MCE RAM patch, RFE ROM bank 1 */
  (uint32_t)0x02400403,           /* Synth: Use 48 MHz crystal, enable extra PLL filtering */
  (uint32_t)0x001C8473,           /* Synth: Configure extra PLL filtering */
  (uint32_t)0x00088433,           /* Synth: Configure synth hardware */
  (uint32_t)0x00038793,           /* Synth: Set minimum RTRIM to 3 */
  HW32_ARRAY_OVERRIDE(0x4004,1),  /* Synth: Configure faster calibration */
  (uint32_t)0x1C0C0618,           /* Synth: Configure faster calibration */
  (uint32_t)0xC00401A1,           /* Synth: Configure faster calibration */
  (uint32_t)0x00010101,           /* Synth: Configure faster calibration */
  (uint32_t)0xC0040141,           /* Synth: Configure faster calibration */
  (uint32_t)0x00214AD3,           /* Synth: Configure faster calibration */
  (uint32_t)0x02980243,           /* Synth: Decrease synth programming time-out (0x0298 RAT ticks = 166 us) */
                                  /* DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). */
  (uint32_t)0xFCFC08C3,           /* In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). */
  (uint32_t)0x000F8883,           /* Rx: Set LNA bias current offset to +15 to saturate trim to max (default: 0) */
                                  /* override_frontend_xd.xml */
                                  /* TX power override */
  (uint32_t)0xFD6EE02B,           /* txHighPA=0x3F5BB8 */
  (uint32_t)0xFFFFFFFF,
};
/*---------------------------------------------------------------------------*/
#else
/*
 * TX Power table
 * The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
 * RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
 * See the Technical Reference Manual for further details about the "txPower" Command field.
 * The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
 */
RF_TxPowerTable_Entry rf_ieee_tx_power_table[] =
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

/*
 * TX power table size, with one less entry excluding the
 * termination entry.
 */
const size_t rf_ieee_tx_power_table_size =
    (sizeof(rf_ieee_tx_power_table) / sizeof(rf_ieee_tx_power_table[0])) - 1;

/*
 * CMD_RADIO_SETUP must be configured with default TX power value
 * in the .txPower field.
 */
#define DEFAULT_TX_POWER    0x941E /* 5 dBm */
/*---------------------------------------------------------------------------*/
/* Overrides for CMD_RADIO_SETUP with default PA */
uint32_t rf_ieee_overrides[] CC_ALIGN(4) =
{
                                  /* override_ieee_802_15_4.xml */
  MCE_RFE_OVERRIDE(1,0,0,0,1,0),  /* PHY: Use MCE RAM patch, RFE ROM bank 1 */
  (uint32_t)0x02400403,           /* Synth: Use 48 MHz crystal, enable extra PLL filtering */
  (uint32_t)0x001C8473,           /* Synth: Configure extra PLL filtering */
  (uint32_t)0x00088433,           /* Synth: Configure synth hardware */
  (uint32_t)0x00038793,           /* Synth: Set minimum RTRIM to 3 */
  HW32_ARRAY_OVERRIDE(0x4004,1),  /* Synth: Configure faster calibration */
  (uint32_t)0x1C0C0618,           /* Synth: Configure faster calibration */
  (uint32_t)0xC00401A1,           /* Synth: Configure faster calibration */
  (uint32_t)0x00010101,           /* Synth: Configure faster calibration */
  (uint32_t)0xC0040141,           /* Synth: Configure faster calibration */
  (uint32_t)0x00214AD3,           /* Synth: Configure faster calibration */
  (uint32_t)0x02980243,           /* Synth: Decrease synth programming time-out (0x0298 RAT ticks = 166 us) */
                                  /* DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). */
  (uint32_t)0xFCFC08C3,           /* In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). */
  (uint32_t)0x000F8883,           /* Rx: Set LNA bias current offset to +15 to saturate trim to max (default: 0) */
  (uint32_t)0xFFFFFFFF,
};

#endif /* RF_CONF_TXPOWER_HIGH_PA */

#endif /* defined(DEVICE_CC1352P) */
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
  .loDivider = 0x00,
  .config.frontEndMode = 0x0,
  .config.biasMode = 0x0,
  .config.analogCfgMode = 0x0,
  .config.bNoFsPowerUp = 0x0,
  .txPower = DEFAULT_TX_POWER, /* 5 dBm default */
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
