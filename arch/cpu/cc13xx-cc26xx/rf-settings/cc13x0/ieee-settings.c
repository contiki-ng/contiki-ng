//*********************************************************************************
// Parameter summary
// IEEE Channel: 11
// Frequency: 2405 MHz
// SFD: 0
// Packet Data: 255
// Preamble (32 bit): 01010101...
// TX Power: 5 dBm

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/rf_mailbox.h)
#include DeviceFamily_constructPath(driverlib/rf_common_cmd.h)
// This must be included "locally" frm the cpu directory,
// as it isn't defined in CC13x0 driverlib
#include "driverlib/rf_ieee_cmd.h"
#include "rf_patches/rf_patch_cpe_ieee.h"

#include <ti/drivers/rf/RF.h>

#include "ieee-settings.h"


// TI-RTOS RF Mode Object
RF_Mode RF_ieeeMode =
{
    .rfMode = RF_MODE_IEEE_15_4,
    .cpePatchFxn = &rf_patch_cpe_ieee,
    .mcePatchFxn = 0,
    .rfePatchFxn = 0,
};


// TX Power table
// The RF_TxPowerTable_DEFAULT_PA_ENTRY macro is defined in RF.h and requires the following arguments:
// RF_TxPowerTable_DEFAULT_PA_ENTRY(bias, gain, boost coefficient)
// See the Technical Reference Manual for further details about the "txPower" Command field.
// The PA settings require the CCFG_FORCE_VDDR_HH = 0 unless stated otherwise.
RF_TxPowerTable_Entry ieeeTxPowerTable[14] =
{
    { -21, RF_TxPowerTable_DEFAULT_PA_ENTRY( 7, 3, 0,  6) },
    { -18, RF_TxPowerTable_DEFAULT_PA_ENTRY( 9, 3, 0,  6) },
    { -15, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 3, 0,  6) },
    { -12, RF_TxPowerTable_DEFAULT_PA_ENTRY(11, 1, 0, 10) },
    {  -9, RF_TxPowerTable_DEFAULT_PA_ENTRY(14, 1, 1, 12) },
    {  -6, RF_TxPowerTable_DEFAULT_PA_ENTRY(18, 1, 1, 14) },
    {  -3, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 1, 1, 18) },
    {   0, RF_TxPowerTable_DEFAULT_PA_ENTRY(33, 1, 1, 24) },
    {   1, RF_TxPowerTable_DEFAULT_PA_ENTRY(20, 0, 0, 33) },
    {   2, RF_TxPowerTable_DEFAULT_PA_ENTRY(24, 0, 0, 39) },
    {   3, RF_TxPowerTable_DEFAULT_PA_ENTRY(28, 0, 0, 45) },
    {   4, RF_TxPowerTable_DEFAULT_PA_ENTRY(36, 0, 1, 73) },
    {   5, RF_TxPowerTable_DEFAULT_PA_ENTRY(48, 0, 1, 73) },
    RF_TxPowerTable_TERMINATION_ENTRY
};


// Overrides for CMD_RADIO_SETUP
uint32_t pIeeeOverrides[] =
{
                                    // override_synth_ieee_15_4.xml
    HW_REG_OVERRIDE(0x4038,0x0035), // Synth: Set recommended RTRIM to 5
    (uint32_t)0x000784A3,           // Synth: Set Fref to 3.43 MHz
    (uint32_t)0xA47E0583,           // Synth: Set loop bandwidth after lock to 80 kHz
    (uint32_t)0xEAE00603,           // Synth: Set loop bandwidth after lock to 80 kHz
    (uint32_t)0x00010623,           // Synth: Set loop bandwidth after lock to 80 kHz
    HW32_ARRAY_OVERRIDE(0x405C,1),  // Synth: Configure PLL bias
    (uint32_t)0x1801F800,           // Synth: Configure PLL bias
    HW32_ARRAY_OVERRIDE(0x402C,1),  // Synth: Configure PLL latency
    (uint32_t)0x00608402,           // Synth: Configure PLL latency
    (uint32_t)0x02010403,           // Synth: Use 24 MHz XOSC as synth clock, enable extra PLL filtering
    HW32_ARRAY_OVERRIDE(0x4034,1),  // Synth: Configure extra PLL filtering
    (uint32_t)0x177F0408,           // Synth: Configure extra PLL filtering
    (uint32_t)0x38000463,           // Synth: Configure extra PLL filtering
                                    // override_phy_ieee_15_4.xml
    (uint32_t)0x05000243,           // Synth: Increase synth programming timeout
    (uint32_t)0x002082C3,           // Rx: Adjust Rx FIFO threshold to avoid overflow
                                    // override_frontend_id.xml
    (uint32_t)0x000288A3,           // Rx: Set RSSI offset to adjust reported RSSI by -2 dB
    (uint32_t)0x000F8883,           // Rx: Configure LNA bias current trim offset
    HW_REG_OVERRIDE(0x50DC,0x002B), // Rx: Adjust AGC DC filter
    (uint32_t)0xFFFFFFFF,
};


// Old override list
uint32_t ieee_overrides[] = {
  (uint32_t)0x00354038,     // Synth: Set RTRIM (POTAILRESTRIM) to 5
  (uint32_t)0x000784A3,     // Synth: Set FREF = 3.43 MHz (24 MHz / 7)
  (uint32_t)0xA47E0583,     // Synth: Set loop bandwidth after lock to 80 kHz (K2)
  (uint32_t)0xEAE00603,     // Synth: Set loop bandwidth after lock to 80 kHz (K3, LSB)
  (uint32_t)0x00010623,     // Synth: Set loop bandwidth after lock to 80 kHz (K3, MSB)
//  (uint32_t)0x1801F800,    // Synth: Set ANADIV DIV_BIAS_MODE to PG1 (value)
  (uint32_t)0x4001402D,     // Synth: Correct CKVD latency setting (address)
  (uint32_t)0x00608402,     // Synth: Correct CKVD latency setting (value)
//  (uint32_t)0x4001405D,    // Synth: Set ANADIV DIV_BIAS_MODE to PG1 (address)
  (uint32_t)0x002B50DC,     // Adjust AGC DC filter
  (uint32_t)0x05000243,     // Increase synth programming timeout
  (uint32_t)0x002082C3,     // Increase synth programming timeout
  (uint32_t)0xFFFFFFFF,
};


// CMD_RADIO_SETUP
// Radio Setup Command for Pre-Defined Schemes
rfc_CMD_RADIO_SETUP_t RF_cmdRadioSetup =
{
    .commandNo = 0x0802,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .mode = 0x01,
    .config.frontEndMode = 0x0,
    .config.biasMode = 0x0,
    .config.analogCfgMode = 0x0,
    .config.bNoFsPowerUp = 0x0,
    .txPower = 0x9330,
    .pRegOverride = pIeeeOverrides,
};

// CMD_FS
// Frequency Synthesizer Programming Command
rfc_CMD_FS_t RF_cmdIeeeFs =
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
    .frequency = 0x0965,
    .fractFreq = 0x0000,
    .synthConf.bTxMode = 0x1,
    .synthConf.refFreq = 0x0,
    .__dummy0 = 0x00,
    .__dummy1 = 0x00,
    .__dummy2 = 0x00,
    .__dummy3 = 0x0000,
};

// CMD_IEEE_TX
// The command ID number 0x2C01
rfc_CMD_IEEE_TX_t RF_cmdIeeeTx =
{
    .commandNo = 0x2C01,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .txOpt.bIncludePhyHdr = 0x0,
    .txOpt.bIncludeCrc = 0x0,
    .txOpt.payloadLenMsb = 0x0,
    .payloadLen = 0x1E,
    .pPayload = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .timeStamp = 0x00000000,
};

// CMD_IEEE_RX
// The command ID number 0x2801
rfc_CMD_IEEE_RX_t RF_cmdIeeeRx =
{
    .commandNo = 0x2801,
    .status = 0x0000,
    .pNextOp = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .startTime = 0x00000000,
    .startTrigger.triggerType = 0x0,
    .startTrigger.bEnaCmd = 0x0,
    .startTrigger.triggerNo = 0x0,
    .startTrigger.pastTrig = 0x0,
    .condition.rule = 0x1,
    .condition.nSkip = 0x0,
    .channel = 0x00,
    .rxConfig.bAutoFlushCrc = 0x0,
    .rxConfig.bAutoFlushIgn = 0x0,
    .rxConfig.bIncludePhyHdr = 0x0,
    .rxConfig.bIncludeCrc = 0x0,
    .rxConfig.bAppendRssi = 0x1,
    .rxConfig.bAppendCorrCrc = 0x1,
    .rxConfig.bAppendSrcInd = 0x0,
    .rxConfig.bAppendTimestamp = 0x0,
    .pRxQ = 0, // INSERT APPLICABLE POINTER: (dataQueue_t*)&xxx
    .pOutput = 0, // INSERT APPLICABLE POINTER: (uint8_t*)&xxx
    .frameFiltOpt.frameFiltEn = 0x0,
    .frameFiltOpt.frameFiltStop = 0x0,
    .frameFiltOpt.autoAckEn = 0x0,
    .frameFiltOpt.slottedAckEn = 0x0,
    .frameFiltOpt.autoPendEn = 0x0,
    .frameFiltOpt.defaultPend = 0x0,
    .frameFiltOpt.bPendDataReqOnly = 0x0,
    .frameFiltOpt.bPanCoord = 0x0,
    .frameFiltOpt.maxFrameVersion = 0x3,
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
    .ccaOpt.ccaEnEnergy = 0x0,
    .ccaOpt.ccaEnCorr = 0x0,
    .ccaOpt.ccaEnSync = 0x0,
    .ccaOpt.ccaCorrOp = 0x1,
    .ccaOpt.ccaSyncOp = 0x1,
    .ccaOpt.ccaCorrThr = 0x0,
    .ccaRssiThr = 0x64,
    .__dummy0 = 0x00,
    .numExtEntries = 0x00,
    .numShortEntries = 0x00,
    .pExtEntryList = 0, // INSERT APPLICABLE POINTER: (uint32_t*)&xxx
    .pShortEntryList = 0, // INSERT APPLICABLE POINTER: (uint32_t*)&xxx
    .localExtAddr = 0x0000000012345678,
    .localShortAddr = 0xABBA,
    .localPanID = 0x0000,
    .__dummy1 = 0x000000,
    .endTrigger.triggerType = 0x1,
    .endTrigger.bEnaCmd = 0x0,
    .endTrigger.triggerNo = 0x0,
    .endTrigger.pastTrig = 0x0,
    .endTime = 0x00000000,
};
