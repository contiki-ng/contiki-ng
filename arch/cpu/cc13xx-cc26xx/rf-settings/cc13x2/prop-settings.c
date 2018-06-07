//*********************************************************************************
// Parameter summary
// Address: 0
// Address0: 0xAA
// Address1: 0xBB
// Frequency: 915.00000 MHz
// Data Format: Serial mode disable
// Deviation: 25.000 kHz
// pktLen: 30
// 802.15.4g Mode: 0
// Select bit order to transmit PSDU octets:: 1
// Packet Length Config: Variable
// Max Packet Length: 255
// Packet Length: 20
// Packet Data: 255
// RX Filter BW: 98.0 kHz
// Symbol Rate: 50.00000 kBaud
// Sync Word Length: 24 Bits
// For Default PA:
//      TX Power: 13.5 dBm (requires define CCFG_FORCE_VDDR_HH = 1 in ccfg.c, see CC13xx/CC26xx Technical Reference Manual)
//      Enable high output power PA: false
// For High PA:
//      TX Power: 20 dBm (requires define CCFG_FORCE_VDDR_HH = 0 in ccfg.c, see CC13xx/CC26xx Technical Reference Manual)
//      Enable high output power PA: true
// Whitening: Dynamically IEEE 802.15.4g compatible whitener and 16/32-bit CRC

#include "sys/cc.h"

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
#include DeviceFamily_constructPath(driverlib/rf_prop_cmd.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_cpe_prop.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_rfe_genfsk.h)
#include DeviceFamily_constructPath(rf_patches/rf_patch_mce_genfsk.h)

#include <ti/drivers/rf/RF.h>

#include "prop-settings.h"


// TI-RTOS RF Mode Object
RF_Mode RF_propMode =
{
    .rfMode = RF_MODE_AUTO,
    .cpePatchFxn = &rf_patch_cpe_prop,
    .mcePatchFxn = &rf_patch_mce_genfsk,
    .rfePatchFxn = &rf_patch_rfe_genfsk,
};


// TX Power table
// The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
// RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
// See the Technical Reference Manual for further details about the "txPower" Command field.
// The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
RF_TxPowerTable_Entry propDefaultPaTxPowerTable[19] =
{
    { -20, RF_TxPowerTable_DEFAULT_PA_ENTRY( 0, 3, 0,  2) },
    { -15, RF_TxPowerTable_DEFAULT_PA_ENTRY( 1, 3, 0,  3) },
    { -10, RF_TxPowerTable_DEFAULT_PA_ENTRY( 2, 3, 0,  3) },
    {  -5, RF_TxPowerTable_DEFAULT_PA_ENTRY( 4, 3, 0,  6) },
    {   0, RF_TxPowerTable_DEFAULT_PA_ENTRY( 7, 3, 0,  8) },
    {   1, RF_TxPowerTable_DEFAULT_PA_ENTRY( 8, 3, 0,  9) },
    {   2, RF_TxPowerTable_DEFAULT_PA_ENTRY( 9, 3, 0,  9) },
    {   3, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0, 11) },
    {   4, RF_TxPowerTable_DEFAULT_PA_ENTRY(12, 3, 0, 12) },
    {   5, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 3, 0, 14) },
    {   6, RF_TxPowerTable_DEFAULT_PA_ENTRY( 6, 2, 0, 14) },
    {   7, RF_TxPowerTable_DEFAULT_PA_ENTRY( 4, 1, 0, 16) },
    {   8, RF_TxPowerTable_DEFAULT_PA_ENTRY( 6, 1, 0, 19) },
    {   9, RF_TxPowerTable_DEFAULT_PA_ENTRY( 8, 1, 0, 25) },
    {  10, RF_TxPowerTable_DEFAULT_PA_ENTRY(13, 1, 0, 40) },
    {  11, RF_TxPowerTable_DEFAULT_PA_ENTRY(10, 0, 0, 71) },
    {  12, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 0, 64) },
    // This setting requires CCFG_FORCE_VDDR_HH = 1.
    // The original PA value (13.5 dBm) have been rounded to an integer value.
    {  14, RF_TxPowerTable_DEFAULT_PA_ENTRY(63, 0, 1,  0) },
    RF_TxPowerTable_TERMINATION_ENTRY
};


// TX Power table
// The RF_TxPowerTable_HIGH_PA_ENTRY macro is defined in RF.h and requires the following arguments:
// RF_TxPowerTable_HIGH_PA_ENTRY(bias, ibboost, boost, coefficient, ldoTrim)
// See the Technical Reference Manual for further details about the "txPower" Command field.
// The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
RF_TxPowerTable_Entry propHighPaTxPowerTable[8] =
{
    { 14, RF_TxPowerTable_HIGH_PA_ENTRY( 7, 0, 0, 23,  4) },
    { 15, RF_TxPowerTable_HIGH_PA_ENTRY(10, 0, 0, 26,  4) },
    { 16, RF_TxPowerTable_HIGH_PA_ENTRY(14, 0, 0, 33,  4) },
    { 17, RF_TxPowerTable_HIGH_PA_ENTRY(18, 0, 0, 40,  6) },
    { 18, RF_TxPowerTable_HIGH_PA_ENTRY(24, 0, 0, 51,  8) },
    { 19, RF_TxPowerTable_HIGH_PA_ENTRY(32, 0, 0, 73, 12) },
    { 20, RF_TxPowerTable_HIGH_PA_ENTRY(27, 0, 0, 85, 32) },
    RF_TxPowerTable_TERMINATION_ENTRY
};


// Overrides for CMD_PROP_RADIO_DIV_SETUP
uint32_t pPropDefaultPaOverrides[] CC_ALIGN(4) =
{
                                        // override_use_patch_prop_genfsk.xml
    MCE_RFE_OVERRIDE(1,0,0,1,0,0),      // PHY: Use MCE RAM patch, RFE RAM patch
                                        // override_synth_prop_863_930_div5.xml
    (uint32_t)0x02400403,               // Synth: Use 48 MHz crystal as synth clock, enable extra PLL filtering
    (uint32_t)0x00068793,               // Synth: Set minimum RTRIM to 6
    (uint32_t)0x001C8473,               // Synth: Configure extra PLL filtering
    (uint32_t)0x00088433,               // Synth: Configure extra PLL filtering
    (uint32_t)0x000684A3,               // Synth: Set Fref to 4 MHz
    HW32_ARRAY_OVERRIDE(0x4004,1),      // Synth: Configure faster calibration
    (uint32_t)0x180C0618,               // Synth: Configure faster calibration
    (uint32_t)0xC00401A1,               // Synth: Configure faster calibration
    (uint32_t)0x00010101,               // Synth: Configure faster calibration
    (uint32_t)0xC0040141,               // Synth: Configure faster calibration
    (uint32_t)0x00214AD3,               // Synth: Configure faster calibration
    (uint32_t)0x02980243,               // Synth: Decrease synth programming time-out by 90 us from default (0x0298 RAT ticks = 166 us)
    (uint32_t)0x0A480583,               // Synth: Set loop bandwidth after lock to 20 kHz
    (uint32_t)0x7AB80603,               // Synth: Set loop bandwidth after lock to 20 kHz
    (uint32_t)0x00000623,               // Synth: Set loop bandwidth after lock to 20 kHz
                                        // override_phy_tx_pa_ramp_genfsk_hpa.xml
    HW_REG_OVERRIDE(0x6028,0x002F),     // Tx: Configure PA ramping, set wait time before turning off (0x2F ticks of 16/24 us = 31.3 us).
    ADI_HALFREG_OVERRIDE(0,16,0x8,0x8), // Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[3]=1)
    ADI_HALFREG_OVERRIDE(0,17,0x1,0x1), // Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[4]=1)
    // override_phy_rx_frontend_genfsk.xml
    HW_REG_OVERRIDE(0x609C,0x001A),     // Rx: Set AGC reference level to 0x1A (default: 0x2E)
    (uint32_t)0x00018883,               // Rx: Set LNA bias current offset to adjust +1 (default: 0)
    (uint32_t)0x000288A3,               // Rx: Set RSSI offset to adjust reported RSSI by -2 dB (default: 0)
                                        // override_phy_rx_aaf_bw_0xd.xml
    ADI_HALFREG_OVERRIDE(0,61,0xF,0xD), // Rx: Set anti-aliasing filter bandwidth to 0xD (in ADI0, set IFAMPCTL3[7:4]=0xD)
                                        // TX power override
    (uint32_t)0xFFFC08C3,               // DC/DC regulator: In Tx with 14 dBm PA setting, use DCDCCTL5[3:0]=0xF (DITHER_EN=1 and IPEAK=7). In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4).
    ADI_REG_OVERRIDE(0,12,0xF8),        // Tx: Set PA trim to max to maximize its output power (in ADI0, set PACTL0=0xF8)
    (uint32_t)0xFFFFFFFF,
};


// Overrides for CMD_PROP_RADIO_DIV_SETUP
uint32_t pPropHighPaOverrides[] CC_ALIGN(4) =
{
                                        // override_use_patch_prop_genfsk.xml
    MCE_RFE_OVERRIDE(1,0,0,1,0,0),      // PHY: Use MCE RAM patch, RFE RAM patch
                                        // override_synth_prop_863_930_div5.xml
    (uint32_t)0x02400403,               // Synth: Use 48 MHz crystal as synth clock, enable extra PLL filtering
    (uint32_t)0x00068793,               // Synth: Set minimum RTRIM to 6
    (uint32_t)0x001C8473,               // Synth: Configure extra PLL filtering
    (uint32_t)0x00088433,               // Synth: Configure extra PLL filtering
    (uint32_t)0x000684A3,               // Synth: Set Fref to 4 MHz
    HW32_ARRAY_OVERRIDE(0x4004,1),      // Synth: Configure faster calibration
    (uint32_t)0x180C0618,               // Synth: Configure faster calibration
    (uint32_t)0xC00401A1,               // Synth: Configure faster calibration
    (uint32_t)0x00010101,               // Synth: Configure faster calibration
    (uint32_t)0xC0040141,               // Synth: Configure faster calibration
    (uint32_t)0x00214AD3,               // Synth: Configure faster calibration
    (uint32_t)0x02980243,               // Synth: Decrease synth programming time-out by 90 us from default (0x0298 RAT ticks = 166 us)
    (uint32_t)0x0A480583,               // Synth: Set loop bandwidth after lock to 20 kHz
    (uint32_t)0x7AB80603,               // Synth: Set loop bandwidth after lock to 20 kHz
    (uint32_t)0x00000623,               // Synth: Set loop bandwidth after lock to 20 kHz
                                        // override_phy_tx_pa_ramp_genfsk_hpa.xml
    HW_REG_OVERRIDE(0x6028,0x002F),     // Tx: Configure PA ramping, set wait time before turning off (0x2F ticks of 16/24 us = 31.3 us).
    ADI_HALFREG_OVERRIDE(0,16,0x8,0x8), // Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[3]=1)
    ADI_HALFREG_OVERRIDE(0,17,0x1,0x1), // Tx: Configure PA ramp time, PACTL2.RC=0x3 (in ADI0, set PACTL2[4]=1)
                                        // override_phy_rx_frontend_genfsk.xml
    HW_REG_OVERRIDE(0x609C,0x001A),     // Rx: Set AGC reference level to 0x1A (default: 0x2E)
    (uint32_t)0x00018883,               // Rx: Set LNA bias current offset to adjust +1 (default: 0)
    (uint32_t)0x000288A3,               // Rx: Set RSSI offset to adjust reported RSSI by -2 dB (default: 0)
                                        // override_phy_rx_aaf_bw_0xd.xml
    ADI_HALFREG_OVERRIDE(0,61,0xF,0xD), // Rx: Set anti-aliasing filter bandwidth to 0xD (in ADI0, set IFAMPCTL3[7:4]=0xD)
                                        // TX power override
    (uint32_t)0xFCFC08C3,               // DC/DC regulator: In Tx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4). In Rx, use DCDCCTL5[3:0]=0xC (DITHER_EN=1 and IPEAK=4).
    (uint32_t)0x82A86C2B,               // txHighPA=0x20AA1B
    (uint32_t)0xFFFFFFFF,
};


// CMD_PROP_RADIO_DIV_SETUP
// Proprietary Mode Radio Setup Command for All Frequency Bands
rfc_CMD_PROP_RADIO_DIV_SETUP_t RF_cmdPropRadioDivSetup =
{
    .commandNo = 0x3807,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
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
    .config.frontEndMode = 0x0,
    .config.biasMode = 0x1,
    .config.analogCfgMode = 0x0,
    .config.bNoFsPowerUp = 0x0,
    .txPower = 0x013F,
    .pRegOverride = pPropDefaultPaOverrides,
    .centerFreq = 0x0393,
    .intFreq = 0x8000,
    .loDivider = 0x05,
};

// CMD_FS
// Frequency Synthesizer Programming Command
rfc_CMD_FS_t RF_cmdPropFs =
{
    .commandNo = 0x0803,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .frequency = 0x0393,
    .fractFreq = 0x0000,
    .synthConf.bTxMode = 0x0,
    .synthConf.refFreq = 0x0,
    .__dummy0 = 0x00,
    .__dummy1 = 0x00,
    .__dummy2 = 0x00,
    .__dummy3 = 0x0000,
};

// CMD_PROP_TX_ADV
// Proprietary Mode Advanced Transmit Command
rfc_CMD_PROP_TX_ADV_t RF_cmdPropTxAdv =
{
    .commandNo = 0x3803,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x2,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x1,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .pktConf.bFsOff = 0x0,
    .pktConf.bUseCrc = 0x1,
    .pktConf.bCrcIncSw = 0x0,
    .pktConf.bCrcIncHdr = 0x0,
    .numHdrBits = 0x10,
    .pktLen = 0x0014,
    .startConf.bExtTxTrig = 0x0,
    .startConf.inputMode = 0x0,
    .startConf.source = 0x0,
    .preTrigger.triggerType = 0x4,
    .preTrigger.bEnaCmd = 0x0,
    .preTrigger.triggerNo = 0x0,
    .preTrigger.pastTrig = 0x1,
    .preTime = 0x00000000,
    .syncWord = 0x0055904E,
    .pPkt = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
};

// CMD_PROP_RX_ADV
// Proprietary Mode Advanced Receive Command
rfc_CMD_PROP_RX_ADV_t RF_cmdPropRxAdv =
{
    .commandNo = 0x3804,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .pktConf.bFsOff = 0x0,
    .pktConf.bRepeatOk = 0x0,
    .pktConf.bRepeatNok = 0x0,
    .pktConf.bUseCrc = 0x0,
    .pktConf.bCrcIncSw = 0x0,
    .pktConf.bCrcIncHdr = 0x0,
    .pktConf.endType = 0x0,
    .pktConf.filterOp = 0x0,
    .rxConf.bAutoFlushIgnored = 0x0,
    .rxConf.bAutoFlushCrcErr = 0x0,
    .rxConf.bIncludeHdr = 0x0,
    .rxConf.bIncludeCrc = 0x0,
    .rxConf.bAppendRssi = 0x0,
    .rxConf.bAppendTimestamp = 0x0,
    .rxConf.bAppendStatus = 0x0,
    .syncWord0 = 0x930B51DE,
    .syncWord1 = 0x00000000,
    .maxPktLen = 0x00FF,
    .hdrConf.numHdrBits = 0x0,
    .hdrConf.lenPos = 0x0,
    .hdrConf.numLenBits = 0x0,
    .addrConf.addrType = 0x0,
    .addrConf.addrSize = 0x0,
    .addrConf.addrPos = 0x0,
    .addrConf.numAddr = 0x0,
    .lenOffset = 0x00,
    .endTrigger.triggerType = 0x0,
    .endTrigger.bEnaCmd = 0x0,
    .endTrigger.triggerNo = 0x0,
    .endTrigger.pastTrig = 0x0,
    .endTime = 0x00000000,
    .pAddr = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .pQueue = 0, // INSERT APPLICABLE POINTER: (dataQueue_t*)&xxx
    .pOutput = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
};
