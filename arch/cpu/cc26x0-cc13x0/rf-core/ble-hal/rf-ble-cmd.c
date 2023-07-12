/*
 * Copyright (c) 2017, Graz University of Technology
 * Copyright (c) 2018, University of Bristol - http://www.bristol.ac.uk/
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

/**
 * \file
 *    BLE commands for the TI CC26xx BLE radio.
 *    These functions are specific to the TI CC26xx and cannot be
 *    reused by other BLE radios.
 *
 * \author
 *    Michael Spoerk <michael.spoerk@tugraz.at>
 *    Jinyan BAI <onefreebjy@outlook.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "rf_ble_cmd.h"
#include "rf-core/rf-core.h"
#include "rf-core/ble-hal/rf-ble-cmd.h"

/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "BLE-RADIO"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/
#define CMD_GET_STATUS(X)         (((rfc_radioOp_t *)X)->status)
/*---------------------------------------------------------------------------*/
/* values for a selection of available TX powers (values from SmartRF Studio) */
/*static uint16_t tx_power = 0x9330;						/ * +5 dBm * / */
static uint16_t tx_power = 0x3161;                /*  0 dBm */
/*static uint16_t tx_power = 0x0CCB;                / *  -15 dBm * / */
/*---------------------------------------------------------------------------*/
/* BLE overrides */
#if RADIO_CONF_BLE5
uint32_t ble_overrides_common[] =
{
  /* Rx: Set LNA IB trim value based on the selected defaultPhy.mainMode setting. (NOTE: The value 0x8 is a placeholder. The value to use should be set during run-time by radio driver function.) */
  ADI_HALFREG_OVERRIDE(0,4,0xF,0x8),
  /* Rx: Set LNA IB offset used for automatic software compensation to 0 */
  (uint32_t)0x00008883,
  /* Synth: Use 24 MHz crystal, enable extra PLL filtering */
  (uint32_t)0x02010403,
  /* Synth: Set fine top and bottom code to 127 and 0 */
  HW_REG_OVERRIDE(0x4020, 0x7F00),
  /* Synth: Configure faster calibration */
  HW32_ARRAY_OVERRIDE(0x4004, 1),
  /* Synth: Configure faster calibration */
  (uint32_t)0x1C0C0618,
  /* Synth: Configure faster calibration */
  (uint32_t)0xC00401A1,
  /* Synth: Configure faster calibration */
  (uint32_t)0x21010101,
  /* Synth: Configure faster calibration */
  (uint32_t)0xC0040141,
  /* Synth: Configure faster calibration */
  (uint32_t)0x00214AD3,
  /* Synth: Decrease synth programming time-out by 90 us (0x0298 RAT ticks = 166 us) */
  (uint32_t)0x02980243,
  /* Bluetooth 5: Set correct total clock accuracy for received AuxPtr assuming local sleep clock of 50 ppm */
  (uint32_t)0x0E490823,
  /* override_frontend_id.xml */
  (uint32_t)0xFFFFFFFF,
};

uint32_t ble_overrides_1Mbps[] =
{
  /* Rx: Set LNA IB trim to normal trim value. (NOTE: The value 0x8 is a placeholder. The value to use should be set during run-time by radio driver function.) */
  ADI_HALFREG_OVERRIDE(0,4,0xF,0x8),
  /* Rx: Configure AGC to use gain table for improved performance */
  HW_REG_OVERRIDE(0x6084, 0x05F8),
  (uint32_t)0xFFFFFFFF,
};

uint32_t ble_overrides_2Mbps[] =
{
  /* Rx: Set LNA IB trim to normal trim value. (NOTE: The value 0x8 is a placeholder. The value to use should be set during run-time by radio driver function.) */
  ADI_HALFREG_OVERRIDE(0,4,0xF,0x8),
  (uint32_t)0xFFFFFFFF,
};

uint32_t ble_overrides_coded[] =
{
  /* Rx: Set LNA IB trim to 0xF (maximum) */
  ADI_HALFREG_OVERRIDE(0,4,0xF,0xF),
  /* Rx: Override AGC target gain to improve performance */
  HW_REG_OVERRIDE(0x6088, 0x0018),
  (uint32_t)0xFFFFFFFF,
};
#else
static uint32_t ble_overrides[] = {
  0x00364038, /* Synth: Set RTRIM (POTAILRESTRIM) to 6 */
  0x000784A3, /* Synth: Set FREF = 3.43 MHz (24 MHz / 7) */
  0xA47E0583, /* Synth: Set loop bandwidth after lock to 80 kHz (K2) */
  0xEAE00603, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, LSB) */
  0x00010623, /* Synth: Set loop bandwidth after lock to 80 kHz (K3, MSB) */
  0x00456088, /* Adjust AGC reference level */
  0x008F88B3, /* GPIO mode: https://e2e.ti.com/support/wireless_connectivity/proprietary_sub_1_ghz_simpliciti/f/156/t/488244?*/
  0xFFFFFFFF, /* End of override list */
};
#endif
/*---------------------------------------------------------------------------*/
unsigned short
rf_ble_cmd_send(uint8_t *command)
{
  uint32_t cmdsta;
  rfc_radioOp_t *cmd = (rfc_radioOp_t *)command;

  if(rf_core_send_cmd((uint32_t)cmd, &cmdsta) != RF_CORE_CMD_OK) {
    LOG_ERR("rf_ble_cmd_send() could not send cmd. status: 0x%04X\n",
            CMD_GET_STATUS(cmd));
    return RF_BLE_CMD_ERROR;
  }
  return RF_BLE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
unsigned short
rf_ble_cmd_wait(uint8_t *command)
{
  rfc_radioOp_t *cmd = (rfc_radioOp_t *)command;
  if(rf_core_wait_cmd_done((void *)cmd) != RF_CORE_CMD_OK) {
    LOG_ERR("rf_ble_cmd_wait() could not wait. status: 0x%04X\n",
            CMD_GET_STATUS(cmd));
    return RF_BLE_CMD_ERROR;
  }
  return RF_BLE_CMD_OK;
}
/*---------------------------------------------------------------------------*/
unsigned short
rf_ble_cmd_setup_ble_mode(void)
{
#if RADIO_CONF_BLE5
  rfc_CMD_BLE5_RADIO_SETUP_t cmd;
  
  /* Create radio setup command */
  rf_core_init_radio_op((rfc_radioOp_t *)&cmd, sizeof(cmd), CMD_BLE5_RADIO_SETUP);
  
  cmd.startTrigger.bEnaCmd = 0;
  cmd.defaultPhy.mainMode = 1;
  cmd.defaultPhy.coding = 1;
  cmd.pRegOverrideCommon = ble_overrides_common;
  cmd.pRegOverride1Mbps = ble_overrides_1Mbps;
  cmd.pRegOverride2Mbps = ble_overrides_2Mbps;
  cmd.pRegOverrideCoded = ble_overrides_coded;
#else
  rfc_CMD_RADIO_SETUP_t cmd;

  /* Create radio setup command */
  rf_core_init_radio_op((rfc_radioOp_t *)&cmd, sizeof(cmd), CMD_RADIO_SETUP);

  cmd.mode = 0;
  cmd.pRegOverride = ble_overrides;
#endif
  cmd.txPower = tx_power;

  /* Send Radio setup to RF Core */
  if(rf_ble_cmd_send((uint8_t *)&cmd) != RF_BLE_CMD_OK) {
    return RF_BLE_CMD_ERROR;
  }

  /* Wait until radio setup is done */
  return rf_ble_cmd_wait((uint8_t *)&cmd);
}
/*---------------------------------------------------------------------------*/
/* ADVERTISING functions                                                     */
/*---------------------------------------------------------------------------*/
void
rf_ble_cmd_create_adv_cmd(uint8_t *command, uint8_t channel,
                          uint8_t *param, uint8_t *output)
{
#if RADIO_CONF_BLE5
  rfc_CMD_BLE5_ADV_t *c = (rfc_CMD_BLE5_ADV_t *)command;
  
  memset(c, 0x00, sizeof(rfc_CMD_BLE5_ADV_t));
  
  c->commandNo = CMD_BLE5_ADV;
  c->rangeDelay = 0;
  
  c->txPower = tx_power;
#else
  rfc_CMD_BLE_ADV_t *c = (rfc_CMD_BLE_ADV_t *)command;

  memset(c, 0x00, sizeof(rfc_CMD_BLE_ADV_t));
  c->commandNo = CMD_BLE_ADV;
#endif
  c->condition.rule = COND_NEVER;
  c->whitening.bOverride = 0;
  c->channel = channel;
  c->pParams = (rfc_bleAdvPar_t *)param;
  c->startTrigger.triggerType = TRIG_NOW;
  c->pOutput = (rfc_bleAdvOutput_t *)output;
}
/*---------------------------------------------------------------------------*/
void
rf_ble_cmd_create_adv_params(uint8_t *param, dataQueue_t *rx_queue,
                             uint8_t adv_data_len, uint8_t *adv_data,
                             uint8_t scan_resp_data_len, uint8_t *scan_resp_data,
                             ble_addr_type_t own_addr_type, uint8_t *own_addr)
{
  rfc_bleAdvPar_t *p = (rfc_bleAdvPar_t *)param;

  memset(p, 0x00, sizeof(rfc_bleAdvPar_t));

  p->pRxQ = rx_queue;
  p->rxConfig.bAutoFlushIgnored = 1;
  p->rxConfig.bAutoFlushCrcErr = 0;
  p->rxConfig.bAutoFlushEmpty = 1;
  p->rxConfig.bIncludeLenByte = 1;
  p->rxConfig.bIncludeCrc = 0;
  p->rxConfig.bAppendRssi = 1;
  p->rxConfig.bAppendStatus = 1;
  p->rxConfig.bAppendTimestamp = 1;
  p->advConfig.advFilterPolicy = 0;
  p->advConfig.bStrictLenFilter = 0;
  p->advConfig.deviceAddrType = own_addr_type;
  p->pDeviceAddress = (uint16_t *)own_addr;
  p->advLen = adv_data_len;
  p->scanRspLen = scan_resp_data_len;
  p->pAdvData = adv_data;
  p->pScanRspData = scan_resp_data;
  p->endTrigger.triggerType = TRIG_NEVER;
}
/*---------------------------------------------------------------------------*/
/* INITIATOR functions                                                       */
/*---------------------------------------------------------------------------*/
void
rf_ble_cmd_create_initiator_cmd(uint8_t *cmd, uint8_t channel, uint8_t *params,
                                uint8_t *output, uint32_t start_time)
{
#if RADIO_CONF_BLE5
  rfc_CMD_BLE5_INITIATOR_t *c = (rfc_CMD_BLE5_INITIATOR_t *)cmd;
  
  memset(c, 0x00, sizeof(rfc_CMD_BLE5_INITIATOR_t));
  
  c->commandNo = CMD_BLE5_INITIATOR;
  c->condition.rule = COND_NEVER;
  c->whitening.bOverride = 0;
  c->channel = channel;
  c->pParams = (rfc_ble5InitiatorPar_t *)params;
  c->startTrigger.triggerType = TRIG_ABSTIME;
  c->startTime = start_time;
  c->pOutput = (rfc_ble5ScanInitOutput_t *)output;
  
  c->txPower = tx_power;
  c->rangeDelay = 0;
#else
  rfc_CMD_BLE_INITIATOR_t *c = (rfc_CMD_BLE_INITIATOR_t *)cmd;
  
  memset(c, 0x00, sizeof(rfc_CMD_BLE_INITIATOR_t));
  
  c->commandNo = CMD_BLE_INITIATOR;
  c->condition.rule = COND_NEVER;
  c->whitening.bOverride = 0;
  c->channel = channel;
  c->pParams = (rfc_bleInitiatorPar_t *)params;
  c->startTrigger.triggerType = TRIG_ABSTIME;
  c->startTime = start_time;
  c->pOutput = (rfc_bleInitiatorOutput_t *)output;
#endif
}
/*---------------------------------------------------------------------------*/
void
rf_ble_cmd_create_initiator_params(uint8_t *param, dataQueue_t *rx_queue,
                                   uint32_t initiator_time,
                                   ble_addr_type_t own_addr_type, uint8_t *own_addr,
                                   ble_addr_type_t peer_addr_type, uint8_t *peer_addr,
                                   uint32_t connect_time, uint8_t *conn_req_data)
{
#if RADIO_CONF_BLE5
  rfc_ble5InitiatorPar_t *p = (rfc_ble5InitiatorPar_t *)param;
  p->backoffPar.bLastSucceeded = 0;
  p->backoffPar.bLastFailed = 0;
  p->maxWaitTimeForAuxCh = 0;
  p->rxStartTime = 0;
  p->rxListenTime = 0;
#else
  rfc_bleInitiatorPar_t *p = (rfc_bleInitiatorPar_t *)param;
#endif
  p->pRxQ = rx_queue;
  p->rxConfig.bAutoFlushIgnored = 1;
  p->rxConfig.bAutoFlushCrcErr = 0;
  p->rxConfig.bAutoFlushEmpty = 1;
  p->rxConfig.bIncludeLenByte = 1;
  p->rxConfig.bIncludeCrc = 0;
  p->rxConfig.bAppendRssi = 1;
  p->rxConfig.bAppendStatus = 1;
  p->rxConfig.bAppendTimestamp = 1;
  
  /*    p->initConfig.bUseWhiteList = 0; */
  p->initConfig.bUseWhiteList = 1;
  p->initConfig.bDynamicWinOffset = 0;
  p->initConfig.deviceAddrType = own_addr_type;
  p->initConfig.peerAddrType = peer_addr_type;
  p->initConfig.bStrictLenFilter = 1;
  
  p->connectReqLen = 22;
  p->pConnectReqData = conn_req_data;
  p->pDeviceAddress = (uint16_t *)own_addr;
  p->pWhiteList = (rfc_bleWhiteListEntry_t *)peer_addr;
  p->connectTime = connect_time;
  p->timeoutTrigger.triggerType = TRIG_REL_START;
  p->timeoutTime = initiator_time;
  p->endTrigger.triggerType = TRIG_NEVER;
}
/*---------------------------------------------------------------------------*/
/* CONNECTION slave functions                                                */
/*---------------------------------------------------------------------------*/
void
rf_ble_cmd_create_slave_cmd(uint8_t *cmd, uint8_t channel, uint8_t *params,
                            uint8_t *output, uint32_t start_time)
{
#if RADIO_CONF_BLE5
  rfc_CMD_BLE5_SLAVE_t *c = (rfc_CMD_BLE5_SLAVE_t *)cmd;
  
  memset(c, 0x00, sizeof(rfc_CMD_BLE5_SLAVE_t));
  
  c->commandNo = CMD_BLE5_SLAVE;
  c->condition.rule = COND_NEVER;
  c->whitening.bOverride = 0;
  c->channel = channel;
  c->pParams = (rfc_ble5SlavePar_t *)params;
  c->startTrigger.triggerType = TRIG_ABSTIME;
  c->startTime = start_time;
  c->pOutput = (rfc_bleMasterSlaveOutput_t *)output;
  
  c->phyMode.mainMode = 1;
  c->phyMode.coding = 1;
  c->txPower = tx_power;
  c->rangeDelay = 0;
#else
  rfc_CMD_BLE_SLAVE_t *c = (rfc_CMD_BLE_SLAVE_t *)cmd;

  memset(c, 0x00, sizeof(rfc_CMD_BLE_SLAVE_t));

  c->commandNo = CMD_BLE_SLAVE;
  c->condition.rule = COND_NEVER;
  c->whitening.bOverride = 0;
  c->channel = channel;
  c->pParams = (rfc_bleSlavePar_t *)params;
  c->startTrigger.triggerType = TRIG_ABSTIME;
  c->startTrigger.pastTrig = 0;
  c->startTime = start_time;
  c->pOutput = (rfc_bleMasterSlaveOutput_t *)output;
#endif
}
/*---------------------------------------------------------------------------*/
void
rf_ble_cmd_create_slave_params(uint8_t *params, dataQueue_t *rx_queue,
                               dataQueue_t *tx_queue, uint32_t access_address,
                               uint8_t crc_init_0, uint8_t crc_init_1,
                               uint8_t crc_init_2, uint32_t win_size,
                               uint32_t window_widening, uint8_t first_packet)
{
#if RADIO_CONF_BLE5
  rfc_ble5SlavePar_t *p = (rfc_ble5SlavePar_t *)params;
  p->maxRxPktLen = 255;
  p->maxLenLowRate = 0;
#else
  rfc_bleSlavePar_t *p = (rfc_bleSlavePar_t *)params;
#endif
  p->pRxQ = rx_queue;
  p->pTxQ = tx_queue;
  p->rxConfig.bAutoFlushIgnored = 1;
  p->rxConfig.bAutoFlushCrcErr = 1;
  p->rxConfig.bAutoFlushEmpty = 1;
  p->rxConfig.bIncludeLenByte = 1;
  p->rxConfig.bIncludeCrc = 0;
  p->rxConfig.bAppendRssi = 1;
  p->rxConfig.bAppendStatus = 1;
  p->rxConfig.bAppendTimestamp = 1;

  if(first_packet) {
    /* set parameters for first packet according to TI Technical Reference Manual */
    p->seqStat.lastRxSn = 1;
    p->seqStat.lastTxSn = 1;
    p->seqStat.nextTxSn = 0;
    p->seqStat.bFirstPkt = 1;
    p->seqStat.bAutoEmpty = 0;
    p->seqStat.bLlCtrlTx = 0;
    p->seqStat.bLlCtrlAckRx = 0;
    p->seqStat.bLlCtrlAckPending = 0;
  }

  p->maxNack = 0;
  p->maxPkt = 0;
  p->accessAddress = access_address;
  p->crcInit0 = crc_init_0;
  p->crcInit1 = crc_init_1;
  p->crcInit2 = crc_init_2;
  p->timeoutTrigger.triggerType = TRIG_REL_START;
  if(first_packet) {
    p->timeoutTime = (uint32_t)(10 * win_size);
  } else {
    p->timeoutTime = (uint32_t)(win_size + 2 * window_widening);
  }
  p->endTrigger.triggerType = TRIG_NEVER;
}
/*---------------------------------------------------------------------------*/
/* CONNECTION master functions                                               */
/*---------------------------------------------------------------------------*/
void
rf_ble_cmd_create_master_cmd(uint8_t *cmd, uint8_t channel, uint8_t *params,
                             uint8_t *output, uint32_t start_time)
{
#if RADIO_CONF_BLE5
  rfc_CMD_BLE5_MASTER_t *c = (rfc_CMD_BLE5_MASTER_t *)cmd;
  
  memset(c, 0x00, sizeof(rfc_CMD_BLE5_MASTER_t));
  
  c->commandNo = CMD_BLE5_MASTER;
  c->condition.rule = COND_NEVER;
  c->whitening.bOverride = 0;
  c->channel = channel;
  c->pParams = (rfc_ble5MasterPar_t *)params;
  c->startTrigger.triggerType = TRIG_ABSTIME;
  c->startTime = start_time;
  c->pOutput = (rfc_bleMasterSlaveOutput_t *)output;
  
  c->phyMode.mainMode = 1;
  c->phyMode.coding = 1;
  c->txPower = tx_power;
  c->rangeDelay = 0;
#else
  rfc_CMD_BLE_MASTER_t *c = (rfc_CMD_BLE_MASTER_t *)cmd;
  
  memset(c, 0x00, sizeof(rfc_CMD_BLE_MASTER_t));
  
  c->commandNo = CMD_BLE_MASTER;
  c->condition.rule = COND_NEVER;
  c->whitening.bOverride = 0;
  c->channel = channel;
  c->pParams = (rfc_bleMasterPar_t *)params;
  c->startTrigger.triggerType = TRIG_ABSTIME;
  c->startTime = start_time;
  c->pOutput = (rfc_bleMasterSlaveOutput_t *)output;
#endif
}
/*---------------------------------------------------------------------------*/
void
rf_ble_cmd_create_master_params(uint8_t *params, dataQueue_t *rx_queue,
                                dataQueue_t *tx_queue, uint32_t access_address,
                                uint8_t crc_init_0, uint8_t crc_init_1,
                                uint8_t crc_init_2, uint8_t first_packet)
{
#if RADIO_CONF_BLE5
  rfc_ble5MasterPar_t *p = (rfc_ble5MasterPar_t *)params;
  p->maxRxPktLen = 255;
  p->maxLenLowRate = 0;
#else
  rfc_bleMasterPar_t *p = (rfc_bleMasterPar_t *)params;
#endif
  p->pRxQ = rx_queue;
  p->pTxQ = tx_queue;
  p->rxConfig.bAutoFlushIgnored = 1;
  p->rxConfig.bAutoFlushCrcErr = 1;
  p->rxConfig.bAutoFlushEmpty = 1;
  p->rxConfig.bIncludeLenByte = 1;
  p->rxConfig.bIncludeCrc = 0;
  p->rxConfig.bAppendRssi = 1;
  p->rxConfig.bAppendStatus = 1;
  p->rxConfig.bAppendTimestamp = 1;
  
  if(first_packet) {
    /* set parameters for first packet according to TI Technical Reference Manual */
    p->seqStat.lastRxSn = 1;
    p->seqStat.lastTxSn = 1;
    p->seqStat.nextTxSn = 0;
    p->seqStat.bFirstPkt = 1;
    p->seqStat.bAutoEmpty = 0;
    p->seqStat.bLlCtrlTx = 0;
    p->seqStat.bLlCtrlAckRx = 0;
    p->seqStat.bLlCtrlAckPending = 0;
  }
  
  p->maxPkt = 12;
  p->accessAddress = access_address;
  p->crcInit0 = crc_init_0;
  p->crcInit1 = crc_init_1;
  p->crcInit2 = crc_init_2;
  p->endTrigger.triggerType = TRIG_REL_START;
  p->endTime = (uint32_t)15 * 4000;   /* a connection event must end after 10 ms */
}
/*---------------------------------------------------------------------------*/
/* DATA queue functions                                                      */
/*---------------------------------------------------------------------------*/
unsigned short
rf_ble_cmd_add_data_queue_entry(dataQueue_t *q, uint8_t *e)
{
  uint32_t cmdsta;

  rfc_CMD_ADD_DATA_ENTRY_t cmd;
  cmd.commandNo = CMD_ADD_DATA_ENTRY;
  cmd.pQueue = q;
  cmd.pEntry = e;

  if(rf_core_send_cmd((uint32_t)&cmd, &cmdsta) != RF_CORE_CMD_OK) {
    LOG_ERR("could not add entry to data queue. status: 0x%04" PRIX32 "\n", cmdsta);
    return RF_BLE_CMD_ERROR;
  }
  return RF_BLE_CMD_OK;
}
