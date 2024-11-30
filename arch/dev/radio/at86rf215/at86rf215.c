/*
 * Copyright (c) 2023, ComLab, Jozef Stefan Institute - https://e6.ijs.si/
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
/**
 * \file
 *      AT86RF215 driver for Contiki-NG, version 0.1
 * 
 * \author
 *      Grega Morano <grega.morano@ijs.si>
 *
 * \short
 *      Drivers for AT86RF215 radio, supporting only one BBC (RF core) at a 
 *      time. Currently only 2.4 GHz O-QPSK modulation tested.
 *      Drivers designed to support TSCH, but CSMA works as well. Radio
 *      config can be done in at86rf215-conf.h
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio.h"
#include "net/netstack.h"
#include "net/mac/tsch/tsch.h"

#include "at86rf215.h"
#include "at86rf215-arch.h"
#include "at86rf215-conf.h"
#include "at86rf215-registermap.h"
#include "at86rf215-def.h"

#include "sys/log.h"
/*---------------------------------------------------------------------------*/
#define LOG_MODULE          "AT86RF215"
#define LOG_LEVEL           AT86RF215_LOG_LEVEL
/*---------------------------------------------------------------------------*/
/**
 * Current drivers support only one BBC (RF band) at a time. Regarding the 
 * defined frequency band, the following macros are used to select the 
 * corresponding registers.
 * 
 * Note: The driver is not tested for the Sub-GHz band yet!
 */
#if AT86RF215_FREQUENCY_BAND == FREQ_BAND_24GHZ

    // Radio's state register 
    #define RG_RFn_STATE                 RG_RF24_STATE
    #define RG_RFn_CMD                   RG_RF24_CMD

    // IRQ mask and state registers 
    #define RG_RFn_IRQS                  RG_RF24_IRQS
    #define RG_BBCn_IRQS                 RG_BBC1_IRQS

    // TX and RX frame buffer start and stop registers 
    #define RG_BBCn_FBRXS                RG_BBC1_FBRXS
    #define RG_BBCn_FBRXE                RG_BBC1_FBRXE
    #define RG_BBCn_FBTXS                RG_BBC1_FBTXS
    #define RG_BBCn_FBTXE                RG_BBC1_FBTXE

    // TX and RX frame length High and Low Bytes 
    #define RG_BBCn_RXFLL                RG_BBC1_RXFLL
    #define RG_BBCn_RXFLH                RG_BBC1_RXFLH
    #define RG_BBCn_TXFLL                RG_BBC1_TXFLL
    #define RG_BBCn_TXFLH                RG_BBC1_TXFLH

    // PHY control register (used for FCS valid check) 
    #define SR_BBCn_FCSOK                SR_BBC1_FCSOK
    #define SR_BBCn_FCSFE                SR_BBC1_FCSFE
    #define SR_BBCn_TXAFCS               SR_BBC1_TXAFCS
    #define SR_BBCn_FCST                 SR_BBC1_FCST
    #define SR_BBCn_BBEN                 SR_BBC1_BBEN

    // RSSI and ED registers 
    #define RG_RFn_EDV                   RG_RF24_EDV
    #define RG_RFn_RSSI                  RG_RF24_RSSI

    // TX Power 
    #define SR_RFn_TXPWR                 SR_RF24_TXPWR

    // PHY status - UR detection
    #define RG_BBCn_PS                   RG_BBC1_PS

    // Frame filtering
    #define RG_BBCn_AFC0                 RG_BBC1_AFC0
    #define RG_BBCn_MACPID0F0            RG_BBC1_MACPID0F0
    #define RG_BBCn_MACPID1F0            RG_BBC1_MACPID1F0
    #define RG_BBCn_MACSHA0F0            RG_BBC1_MACSHA0F0
    #define RG_BBCn_MACSHA1F0            RG_BBC1_MACSHA1F0
    #define RG_BBCn_MACEA0               RG_BBC1_MACEA0

    // CCA
    #define RG_RFn_EDC                   RG_RF24_EDC

    // AACK
    #define RG_BBCn_AMAACKTL             RG_BBC1_AMAACKTL
    #define RG_BBCn_AMAACKTH             RG_BBC1_AMAACKTH
    #define SR_BBCn_AACK                 SR_BBC1_AACK

#else   
#error "SUB GHZ band not tested yet!"
#endif

/*---------------------------------------------------------------------------*/
/**
 * Contiki-NG requires that the driver stores the values of last RSSI, LQI 
 * and timestamp -> we are using at86rf215_rx_frame_t for that 
 */
static at86rf215_rx_frame_t rx_frame;

/**
 * It is easier to store the current channel in a global variable than
 * to read it from the radio register every time it is needed.
*/
static uint8_t current_channel = IEEE802154_DEFAULT_CHANNEL;

/**
 * The radio driver uses the following flags to keep track of the current
 * state of the radio and IRQ events.
*/
volatile static at86rf215_flags_t flags;

/**
 * Radio configuration parameters according to the Contiki-NG radio API.
*/
static uint8_t radio_send_on_cca = 0;
static uint8_t radio_mode_frame_filter = 0;
static uint8_t radio_mode_aack = 0;
static uint8_t radio_mode_poll_mode = 0;

/*---------------------------------------------------------------------------*/
/* Private functions */
static void set_auto_ack(uint8_t enable);
static void set_frame_filtering(uint8_t enable);
static void set_poll_mode(uint8_t enable);
static void set_send_on_cca(uint8_t enable);
static void set_cca_threshold(uint8_t threshold);
static uint8_t get_cca_threshold(void);
static void set_pan_ID(uint16_t pan);
static uint16_t get_pan_ID(void);
static void set_short_addr(uint16_t addr);
static uint16_t get_short_addr(void);
static void set_long_addr(const uint8_t *addr, uint8_t len);
static void get_long_addr(uint8_t *addr, uint8_t len);
static void set_tx_power(int8_t power);
static int8_t get_tx_power(void);
static void set_channel(uint8_t channel);
static uint8_t get_channel(void);
static void set_frequency(uint16_t freq, uint8_t decimal);

/*---------------------------------------------------------------------------*/
/* HAL */
static uint8_t regRead(uint16_t address);
static void regWrite(uint16_t address, uint8_t data);
static uint8_t bitRead(uint16_t address, uint8_t mask, uint8_t offset);
static void bitWrite(uint16_t address, uint8_t mask, uint8_t offset, uint8_t value);
static void burstRead(uint16_t address, uint8_t *data, uint16_t len);
static void burstWrite(uint16_t address, uint8_t *data, uint16_t len);
static void frameRead(uint8_t *frame, uint16_t *frame_len);
static void frameWrite(uint8_t *frame, uint16_t frame_len);

/*---------------------------------------------------------------------------*/
PROCESS(at86rf215_process, "AT86RF215 radio driver");



/*---------------------------------------------------------------------------*/
/* NETSTACK API - radio driver functions */
/*---------------------------------------------------------------------------*/
int 
at86rf215_init(void)
{
    LOG_DBG("Initializing AT86RF215 radio ...\n");

    at86rf215_arch_init();

    /* Reset the radio (again) */
    at86rf215_arch_set_RSTN();
    clock_delay_usec(5);
    at86rf215_arch_clear_RSTN();

    do {
        LOG_DBG("Detecting radio ...\n");
        if(regRead(RG_RF_PN) == 0x34){
            break;
        }
    }while(1);

    /* Radio config must be done in TRXOFF state*/
    regWrite(RG_RFn_CMD, RF_CMD_TRXOFF);
    while(regRead(RG_RFn_STATE) != RF_STATE_TRXOFF);

    /* Enable and configure only one Base Band Core */
#if AT86RF215_FREQUENCY_BAND == FREQ_BAND_24GHZ
    regWrite(RG_BBC0_PC, 0x00);

    /* Configure the radio based on given modulation scheme */
    for (uint8_t i = 0; i < (sizeof(AT86RF215_RADIO_CONFIGURATION)/sizeof(at86rf215_radio_config_t)); i++) {
        regWrite(AT86RF215_RADIO_CONFIGURATION[i].address, AT86RF215_RADIO_CONFIGURATION[i].value);
    }
#else
    regWrite(RG_BBC1_PC, 0x00);
    LOG_ERR("SUB GHZ band not tested yet!\n");
    while(1);
#endif

#if AT86RF215_AUTO_CRC
    /* Enable auto generation of FCS (CRC) for TX, 16-bit */
    bitWrite(SR_BBCn_TXAFCS, 1);
    bitWrite(SR_BBCn_FCST, AT86RF215_CRC_16BIT);
#endif

    /* Disable first level frame filter (CRC check at receive) - we do it 
    manually in at85rf215_read(). If enabled, there is no IRQ at RXFE. */
    bitWrite(SR_BBCn_FCSFE, 0);

    /* Default radio configuration (Contiki will do it again) */
    set_frame_filtering(AT86RF215_FRAME_FILTER);
    set_auto_ack(AT86RF215_AUTO_ACK);
    set_poll_mode(AT86RF215_POLL_MODE);
    set_send_on_cca(AT86RF215_SEND_ON_CCA);

    /* Configure the interrupts - active low (falling edge), masked IRQs are not shown in IRQS */
    regWrite(RG_RF_CFG, 0x05);

    /* IRQ source only from 2.4 GHz */
#if AT86RF215_FREQUENCY_BAND == FREQ_BAND_24GHZ
    regWrite(RG_BBC0_IRQM, 0x00);
    regWrite(RG_RF09_IRQM, 0x00);
    regWrite(RG_BBC1_IRQM, 0x13);
    regWrite(RG_RF24_IRQM, 0x04);
#endif

    /* Clear all 4 IRQs */
    uint8_t dummy __attribute__((unused));
    dummy = regRead(RG_RF09_IRQS);
    dummy = regRead(RG_RF24_IRQS);
    dummy = regRead(RG_BBC0_IRQS);
    dummy = regRead(RG_BBC1_IRQS);

    flags.value = 0;

    at86rf215_arch_enable_EXTI();

    /* Start Contiki process which will take care of received packets (for CSMA) */
	process_start(&at86rf215_process, NULL);

    /* Go to TXPREP state */
    regWrite(RG_RFn_CMD, RF_CMD_TXPREP);
    while(regRead(RG_RFn_STATE) != RF_STATE_TXPREP);

    return 1;
}

/*---------------------------------------------------------------------------*/
int 
at86rf215_on(void)
{
    LOG_DBG("%s\n", __func__);
    regWrite(RG_RFn_CMD, RF_CMD_RX);
    while(regRead(RG_RFn_STATE) != RF_STATE_RX);
    flags.RECEIVE_ON = 1;

    /* Do not clear the flags in TSCH - they are cleared when they should be.
     * We could also check the (radio_mode_poll_mode) */
#if MAC_CONF_WITH_CSMA
    flags.RX_START = 0;
    flags.AMI = 0;
    flags.PACKET_PEN = 0;
#endif
    return 1;
}

/*---------------------------------------------------------------------------*/
int 
at86rf215_off(void)
{
    LOG_DBG("%s\n", __func__);
    regWrite(RG_RFn_CMD, RF_CMD_TRXOFF);
    while(regRead(RG_RFn_STATE) != RF_STATE_TRXOFF);
    flags.RECEIVE_ON = 0;
    return 1;
}

/*---------------------------------------------------------------------------*/
int 
at86rf215_prepare(const void *payload, unsigned short payload_len)
{
    LOG_DBG("%s\n", __func__);

    uint8_t buffer[127];

    if(payload_len > AT86RF215_MAX_PAYLOAD_SIZE) {
        LOG_ERR("Payload larger than radio buffer: %u > %u\n", payload_len, AT86RF215_MAX_PAYLOAD_SIZE);
        return RADIO_TX_ERR;
    }

    /* Tranistion to TXPREP might interupt ongoing reception in CSMA MAC mode */
    //regWrite(RG_RFn_CMD, RF_CMD_TXPREP);
    //while(regRead(RG_RFn_STATE) != RF_STATE_TXPREP);

    memcpy(buffer, payload, payload_len);

    /* Add CRC if not calculated by the radio */
#if !AT86RF215_AUTO_CRC
    uint16_t crc = crc16_data(payload, payload_len, 0);
    buffer[payload_len] = (uint8_t)(crc & 0x00FF);
    buffer[payload_len + 1] = (uint8_t)((crc & 0xFF00) >> 8);
    payload_len += 2;
#else
    buffer[payload_len] = 0x00;
    buffer[payload_len + 1] = 0x00;
    payload_len += 2;
#endif

    /* Store the buffer to the radio */
    frameWrite(buffer, (uint16_t)(payload_len));

    return RADIO_TX_OK;
}

/*---------------------------------------------------------------------------*/
int 
at86rf215_transmit(unsigned short payload_len)
{
    LOG_DBG("%s\n", __func__);

    regWrite(RG_RFn_CMD, RF_CMD_TXPREP);
    while(regRead(RG_RFn_STATE) != RF_STATE_TXPREP);

    flags.TX_END = 0;

    /* Initiate transmission */
    regWrite(RG_RFn_CMD, RF_CMD_TX);

    /* Wait until frame is sent */
    while(!flags.TX_END){
        if(regRead(RG_RFn_STATE) == RF_STATE_TXPREP){
            /* If this happens, maybe the frame was not transmitted correctly.
             * Or it happens because of the delay before actual frame end 
             * and the delay before the TXFE IRQ is issued - the frame is 
             * still sent correctly */
            LOG_DBG("No IRQ \n");
            break;
        }
    }
    flags.TX_END = 0;

    /* Turn the radio back to RX state if needed */
    if(flags.RECEIVE_ON){
        at86rf215_on();
    }

    if(regRead(RG_BBCn_PS)){
        LOG_WARN("TX - Underrun \n");
        return RADIO_TX_ERR;
    }

    return RADIO_TX_OK;
}

/*---------------------------------------------------------------------------*/
int 
at86rf215_send(const void *payload, unsigned short payload_len)
{
    LOG_DBG("%s\n", __func__);

    at86rf215_prepare(payload, payload_len);
    return at86rf215_transmit(payload_len);
}

/*---------------------------------------------------------------------------*/
int 
at86rf215_read(void *buf, unsigned short buf_len)
{
    //LOG_DBG("%s\n", __func__);

    uint16_t len = 0;
    uint8_t buffer[127];

    flags.PACKET_PEN = 0;
    flags.RX_START = 0;
    flags.AMI = 0;

    frameRead(buffer, &len);

    if(len < 3 || len > 127){
        LOG_WARN("Read - invalid length \n");
        return 0;
    }

    /* Check whether the CRC is valid and remove it from buff. 
     * If a new frame is detected in the mean time, FCSOK is set to 0 */
    /* TODO: We can move this up - do a check before reading the frame */
#if AT86RF215_AUTO_CRC
    if(bitRead(SR_BBCn_FCSOK) != 1){
        //LOG_WARN("Read - invalid CRC \n");
        return 0;
    }
#else
    uint16_t frame_crc = (uint16_t)(buffer[len-1] << 8) | (buffer[len]);
    uint16_t crc = crc16_data(buffer, len, 0x00);
    if(crc != frame_crc){
        LOG_WARN("Read - wrong CRC \n");
        return 0;
    }
#endif
    len -= 2;

    memcpy(buf, buffer, len);

    rx_frame.rssi = (int8_t)regRead(RG_RFn_EDV);

    /* TODO: AT86RF215 does not support LQI measurement */
    rx_frame.lqi = 255;

    LOG_DBG("Read %d bytes \n", len);

    return len;
}

/*---------------------------------------------------------------------------*/
int 
at86rf215_receiving_packet(void)
{
    /* Pooling radio IRQ state register doesn't work ok - too slow to detect 
     * that's why we never disale IRQs, not even in the pooling mode */
    //at86rf215_bbc_irq_t bbc_irq;
    //if (radio_mode_poll_mode) {
    //    bbc_irq.value = regRead(RG_BBC_IRQS);
    //    
    //    if(bbc_irq.IRQ1_RXFS){
    //        rx_frame.timestamp = RTIMER_NOW();
    //        LOG_DBG("Receiving packet\n");
    //    }
    //    return (bbc_irq.IRQ1_RXFS && !bbc_irq.IRQ2_RXFE);
    //}
    //else {
    //    return flags.RX_START;
    //}

    /* In CSMA it might happen that RXFS was triggered (which sets the 
     * RX_START flag), but the frame did not pass the frame filter. In that case, 
     * the RXFE is not trigerred and the radio is not actually receiving any
     * packet. 
     * In TSCH the frame filter is disabled, allowing all detected
     * frames to trigger RXFE interrupt, which clears the RX_START flag */

    if(radio_mode_frame_filter){
        if((regRead(RG_RFn_STATE) != RF_STATE_RX)){ // || (flags.AMI != 1)){
            flags.RX_START = 0;
        }
    }

    LOG_DBG("%s = %d \n", __func__, flags.RX_START);
    return flags.RX_START;
}
/*---------------------------------------------------------------------------*/
int 
at86rf215_pending_packet(void)
{
    LOG_DBG("%s = %d \n", __func__, flags.PACKET_PEN);

    //at86rf215_bbc_irq_t bbc_irq;
    //if (radio_mode_poll_mode) {
    //    bbc_irq.value = regRead(RG_BBC_IRQS);
    //
    //    if(bbc_irq.IRQ2_RXFE){  
    //        LOG_DBG("Packet pending\n");
    //    }
    //    return bbc_irq.IRQ2_RXFE;
    //}
    //else {
    //    return flags.PACKET_PEN;
    //}

    /* Pooling radios IRQ state register doesn't work - too slow to detect */
    return flags.PACKET_PEN;
}

/*---------------------------------------------------------------------------*/
int 
at86rf215_channel_clear(void)
{
    /**
     * Contiki-NG CSMA mode calls the channel clear function to sense if there
     * is an ACK in the air (besides calling receiving_packet and pending_packet)
     * So instead of receiving an ACK, with this function (implemented from the
     * reference manual) what we do is we put the radio to TXPREP and 
     * disable BBC to avoid packet detection to be able to measure Energy 
     * Detection value. TODO: ...
     * So at this point, we just return 1 --> chanel not busy
     *
     * For TSCH, this implementation is usefull, but the CCA in TSCH is 
     * disabled by default (TSCH_CCA_ENABLED) ...
     */
    
    LOG_DBG("%s = ", __func__);
    // int8_t cca;
    
    // /* Contiki (TSCH) will put the radio to RX state - 
    //  * do we need to change it before disabeling BBC? */
    // regWrite(RG_RFn_CMD, RF_CMD_TXPREP);

    // /* Disable BBC to avoid packet detection */
    // bitWrite(SR_BBCn_BBEN, 0);

    // /* Trigger the energy measurement and wait for IRQ */
    // regWrite(RG_RFn_CMD, RF_CMD_RX);
    // regWrite(RG_RFn_EDC, 0x01);
    // while(!flags.CCA);
    // flags.CCA = 0;

    // /* Value from -127 to +4 */
    // cca = regRead(RG_RFn_EDV);

    // regWrite(RG_RFn_CMD, RF_CMD_TXPREP);
    // regWrite(RG_RFn_EDC, 0x00);
    // bitWrite(SR_BBCn_BBEN, 1);

    // /* If it was in RX state (CSMA) put it back */
    // if(flags.RECEIVE_ON){
    //     at86rf215_on();
    // }

    // LOG_DBG("%d\n", cca <= AT86RF215_CCA_THRESHOLD);

    // /* Return 0 if channel is busy */
    // return cca <= AT86RF215_CCA_THRESHOLD;
    return 1;
}

/*---------------------------------------------------------------------------*/
static radio_result_t 
at86rf215_get_value(radio_param_t param, radio_value_t *value)
{
    if (!value) return RADIO_RESULT_INVALID_VALUE;

    switch (param){
        case RADIO_PARAM_POWER_MODE:
        {
            uint8_t state = regRead(RG_RFn_STATE);
            switch (state){
                case RF_STATE_RX:
                    *value = RADIO_POWER_MODE_ON;
                    return RADIO_RESULT_OK;

                case RF_STATE_TX:
                case RF_STATE_TXPREP:
                    *value = RADIO_POWER_MODE_CARRIER_ON;
                    return RADIO_RESULT_OK;

                default:
                    *value = RADIO_POWER_MODE_OFF;
                    return RADIO_RESULT_OK;
            }
        }

        case RADIO_PARAM_CHANNEL:
            *value = (radio_value_t)get_channel();
            return RADIO_RESULT_OK;

        case RADIO_PARAM_TXPOWER:
			*value = (radio_value_t)get_tx_power();
			return RADIO_RESULT_OK;

        case RADIO_PARAM_PAN_ID:
			*value = (radio_value_t)get_pan_ID();
			return RADIO_RESULT_OK;

		case RADIO_PARAM_16BIT_ADDR:
			*value = (radio_value_t)get_short_addr();
			return RADIO_RESULT_OK;

        case RADIO_PARAM_RX_MODE:
            *value = 0;
			if (radio_mode_frame_filter) *value |= RADIO_RX_MODE_ADDRESS_FILTER;
			if (radio_mode_aack) *value |= RADIO_RX_MODE_AUTOACK;
			if (radio_mode_poll_mode) *value |= RADIO_RX_MODE_POLL_MODE;
            return RADIO_RESULT_OK;

		case RADIO_PARAM_TX_MODE:
            *value = 0;
			if (radio_send_on_cca) *value |= RADIO_TX_MODE_SEND_ON_CCA;
			return RADIO_RESULT_OK;

        case RADIO_PARAM_CCA_THRESHOLD:
			*value = (radio_value_t)get_cca_threshold();
			return RADIO_RESULT_OK;

		case RADIO_PARAM_RSSI:
            *value = regRead(RG_RFn_RSSI);
			return RADIO_RESULT_OK;

        case RADIO_PARAM_LAST_RSSI:
            *value = (radio_value_t)rx_frame.rssi;
            return RADIO_RESULT_OK;

		case RADIO_PARAM_LAST_LINK_QUALITY:
            /* Radio does not support LQI measurement */
			*value = (radio_value_t)rx_frame.lqi;
			return RADIO_RESULT_NOT_SUPPORTED;

		case RADIO_CONST_CHANNEL_MIN:
			*value = AT86RF215_RF_CHANNEL_MIN;
			return RADIO_RESULT_OK;

		case RADIO_CONST_CHANNEL_MAX:
			*value = AT86RF215_RF_CHANNEL_MAX;
			return RADIO_RESULT_OK;

		case RADIO_CONST_TXPOWER_MIN:
			*value = AT86RF215_OUTPUT_POWER_MIN;
			return RADIO_RESULT_OK;

		case RADIO_CONST_TXPOWER_MAX:
			*value = AT86RF215_OUTPUT_POWER_MAX;
			return RADIO_RESULT_OK;

		case RADIO_CONST_PHY_OVERHEAD:
			*value = (radio_value_t)AT86RF215_PHY_OVERHEAD;
			return RADIO_RESULT_OK;

		case RADIO_CONST_BYTE_AIR_TIME:
			*value = (radio_value_t)AT86RF215_BYTE_AIR_TIME;
			return RADIO_RESULT_OK;

		case RADIO_CONST_DELAY_BEFORE_TX:
			*value = (radio_value_t)AT86RF215_DELAY_BEFORE_TX;
			return RADIO_RESULT_OK;

		case RADIO_CONST_DELAY_BEFORE_RX:
			*value = (radio_value_t)AT86RF215_DELAY_BEFORE_RX;
			return RADIO_RESULT_OK;

		case RADIO_CONST_DELAY_BEFORE_DETECT:
			*value = (radio_value_t)AT86RF215_DELAY_BEFORE_DETECT;
			return RADIO_RESULT_OK;

        case RADIO_CONST_MAX_PAYLOAD_LEN:
            *value = (radio_value_t)AT86RF215_MAX_PAYLOAD_SIZE;
            return RADIO_RESULT_OK;

		default:
			return RADIO_RESULT_NOT_SUPPORTED;
	}
}

/*---------------------------------------------------------------------------*/
static radio_result_t 
at86rf215_set_value(radio_param_t param, radio_value_t value)
{
    switch (param) {
        case RADIO_PARAM_POWER_MODE:
            switch (value) 
            {
                case RADIO_POWER_MODE_ON:
                    at86rf215_on();
                    return RADIO_RESULT_OK;

                case RADIO_POWER_MODE_OFF:
                    at86rf215_off();
                    return RADIO_RESULT_OK;

                case RADIO_POWER_MODE_CARRIER_ON:
                    // TODO

                case RADIO_POWER_MODE_CARRIER_OFF:
                    // TODO
                    return RADIO_RESULT_NOT_SUPPORTED;

                default:
                    return RADIO_RESULT_INVALID_VALUE;
            }

        case RADIO_PARAM_CHANNEL:
            set_channel(value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_PAN_ID:
            set_pan_ID(value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_16BIT_ADDR:
            set_short_addr(value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_RX_MODE:
            set_frame_filtering((value & RADIO_RX_MODE_ADDRESS_FILTER) != 0);
            set_auto_ack((value & RADIO_RX_MODE_AUTOACK) != 0);
            set_poll_mode((value & RADIO_RX_MODE_POLL_MODE) != 0);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_TX_MODE:
            set_send_on_cca((value & RADIO_TX_MODE_SEND_ON_CCA) != 0);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_TXPOWER:
            set_tx_power(value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_CCA_THRESHOLD:
            set_cca_threshold(value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_SHR_SEARCH:
            //TODO

        default:
            return RADIO_RESULT_NOT_SUPPORTED;
    }

}

/*---------------------------------------------------------------------------*/
static radio_result_t 
at86rf215_get_object(radio_param_t param, void *dest, size_t size)
{
    if (dest == NULL) return RADIO_RESULT_ERROR;

	switch (param) {
		case RADIO_PARAM_64BIT_ADDR:
            if (size != 8) return RADIO_RESULT_INVALID_VALUE;
			get_long_addr((uint8_t *)dest, (uint8_t)size);
			return RADIO_RESULT_NOT_SUPPORTED;

		case RADIO_PARAM_LAST_PACKET_TIMESTAMP:
		    *(rtimer_clock_t *)dest = rx_frame.timestamp;
		    return RADIO_RESULT_OK;

    #if MAC_CONF_WITH_TSCH
		case RADIO_CONST_TSCH_TIMING:
			*(const uint16_t **)dest = tsch_timeslot_timing_us_10000;
    #endif /* MAC_CONF_WITH_TSCH */

		default:
			return RADIO_RESULT_NOT_SUPPORTED;
	}
}

/*---------------------------------------------------------------------------*/
static radio_result_t 
at86rf215_set_object(radio_param_t param, const void *src, size_t size)
{
    if (src == NULL) return RADIO_RESULT_ERROR;

	switch (param) {
		case RADIO_PARAM_64BIT_ADDR:
            if (size != 8) return RADIO_RESULT_INVALID_VALUE;
			set_long_addr((const uint8_t *)src, (uint8_t)size);
			return RADIO_RESULT_NOT_SUPPORTED;

		default:
			return RADIO_RESULT_NOT_SUPPORTED;
	}
}


/*---------------------------------------------------------------------------*/
const struct radio_driver at86rf215_driver = {
    .init              = at86rf215_init,
    .prepare           = at86rf215_prepare,
    .transmit          = at86rf215_transmit,
    .send              = at86rf215_send,
    .read              = at86rf215_read,
    .channel_clear     = at86rf215_channel_clear,
    .receiving_packet  = at86rf215_receiving_packet,
    .pending_packet    = at86rf215_pending_packet,
    .on                = at86rf215_on,
    .off               = at86rf215_off,
    .get_value         = at86rf215_get_value,
    .set_value         = at86rf215_set_value,
    .get_object        = at86rf215_get_object,
    .set_object        = at86rf215_set_object,
};

/*---------------------------------------------------------------------------*/
/** 
 * Interrupt service routine for the AT86RF215
 * 
 * This function is called by the GPIO interrupt handler. The interrupt can be
 * triggered by the radio (RF) or the baseband part (BBC). Depending on the 
 * predefined frequency band, correct register addresses are used to read the
 * interrupt states. The states are then used to set the corresponding flags.
 * To clear the IRQ line, all 4 interrupts registers must be read.
 */
void 
at86rf215_isr(void) 
{
    at86rf215_rf_irq_t rf_irq;
    at86rf215_bbc_irq_t bbc_irq;
    
    uint8_t dummy __attribute__((unused));
    dummy = regRead(RG_RF09_IRQS);
    dummy = regRead(RG_BBC0_IRQS);
    rf_irq.value = regRead(RG_RF24_IRQS);
    bbc_irq.value = regRead(RG_BBC1_IRQS);

    /* Do not use the UART in ISR!!! Only for dev... */
    //LOG_DBG("RF IRQ: %X\n", rf_irq.value);
    //LOG_DBG("BBC IRQ: %X\n", bbc_irq.value);

    if(rf_irq.IRQ2_TRXRDY){
        flags.PLL_LOCK = 1;
    }
    if(rf_irq.IRQ5_TRXERR){
        flags.PLL_ERR = 0;
        LOG_WARN("PLL ERR\n");  // Not used yet
    }
    if(rf_irq.IRQ3_EDC){
        flags.CCA = 1;
    }

    if(bbc_irq.IRQ1_RXFS){
        flags.RX_START = 1;
        flags.AMI = 0;
        flags.TX_END = 0;

        rx_frame.timestamp = RTIMER_NOW();
    }
    if(bbc_irq.IRQ3_RXAM){
        flags.AMI = 1;
    }
    if(bbc_irq.IRQ2_RXFE){
        flags.RX_START = 0;
        flags.PACKET_PEN = 1;

        if(!radio_mode_poll_mode){  
            process_poll(&at86rf215_process);
        }
    }
    if(bbc_irq.IRQ5_TXFE){
        flags.TX_END = 1;
    }
}


/*---------------------------------------------------------------------------*/
/**
 * Contiki-NG process for the AT86RF215 driver.
 * 
 * The process is started by the at86rf215_init() function and waits idle for
 * an event. The event is triggered at the at86rf215_isr() function when the 
 * radio has received a packet. In pool mode, the upper layer has to poll the
 * driver for new packets and therefore this process is not used.
*/
PROCESS_THREAD(at86rf215_process, ev, data)
{
	PROCESS_BEGIN();

	LOG_INFO("AT86RF215 driver process started!\n");

	while(1) {
        
		PROCESS_YIELD_UNTIL(!radio_mode_poll_mode && ev == PROCESS_EVENT_POLL);

        LOG_DBG("Packet detected ... address match = %d \n", flags.AMI);

        if(radio_mode_frame_filter && (flags.AMI != 1)){
            /* Frame was not for us & radio went to TXPREP --> back to RX */
            at86rf215_on();
        }
        else{

            packetbuf_clear();
            uint8_t len = at86rf215_read(packetbuf_dataptr(), PACKETBUF_SIZE);
            packetbuf_set_attr(PACKETBUF_ATTR_RSSI, rx_frame.rssi);
            packetbuf_set_attr(PACKETBUF_ATTR_LINK_QUALITY, rx_frame.lqi);

            /* Radio will go automatically to TXPREP after reception --> back to
             * RX. But if we trun it back on right away, we might distrup the 
             * Auto ACK transmission. TODO
             * Only RXFE irq indicates the end of Auto ACK, but we can not wait 
             * for it if the frame was broadcast and therefore the ACK is not sent */
            at86rf215_on();

            if(len) {
                packetbuf_set_datalen(len);
                NETSTACK_MAC.input();
            }
        }
        
	}
	PROCESS_END();
}





/*---------------------------------------------------------------------------*/
/* Private functions */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
set_send_on_cca(uint8_t enable)
{
    LOG_DBG("%s(%d)\n", __func__, enable);
    radio_send_on_cca = enable;
}
/*---------------------------------------------------------------------------*/
static void
set_auto_ack(uint8_t enable)
{
    /**
     * Auto ACK is sent if the frame passes all third level filtering rules,
     * therefore frame filtering must be enabled beforehand. 
     */
    LOG_INFO("%s(%d)\n", __func__, enable);

    radio_mode_aack = enable;
    if(enable) {
        if(!radio_mode_frame_filter){
            LOG_WARN("Enabeling frame filter \n");
            set_frame_filtering(1);
        }
        /* TODO: Change the configuration of the ACK time delay ...
         * CSMA_ACK_WAIT_TIME is defined in csma.h and it is equal to 400us */
        regWrite(RG_BBCn_AMAACKTL, 0x90);
        regWrite(RG_BBCn_AMAACKTH, 0x01);

        bitWrite(SR_BBCn_AACK, 1);
    }
    else {
        bitWrite(SR_BBCn_AACK, 0);
    }
}
/*---------------------------------------------------------------------------*/
static void
set_frame_filtering(uint8_t enable)
{
    LOG_INFO("%s(%d)\n", __func__, enable);

    radio_mode_frame_filter = enable;
    if(enable) {
        /* Enable frame filtering with unit #0 while keeping the promiscuous 
        mode on - we need it, so that RXFE is issued even if the frame does 
        not contain our address */
        regWrite(RG_BBCn_AFC0, 0x11);
    }
    else {
        /* Disable address frame filtering and enable promiscuous mode */
        regWrite(RG_BBCn_AFC0, 0x10);
    }
    /* NOTE: that 1st level filtering (CRC check on reception) is always 
     * disabled so RXFE interupt can be generated (datasheet p. 145)*/
}
/*---------------------------------------------------------------------------*/
/*
 * In poll mode, radio interrupts should be disabled, and the radio driver
 * never calls upper layers. TSCH will poll the driver for incoming packets.
 * 
 * However, in this implementation, the drivers requires interrupts to be
 * enabled to detect incoming packets and correctly obtain timestamps.
 */
static void
set_poll_mode(uint8_t enable)
{
    LOG_INFO("%s(%d)\n", __func__, enable);

    radio_mode_poll_mode = enable;

    if(enable) {
        //at86rf215_arch_disable_EXTI();
    }
    else {
        //at86rf215_arch_enable_EXTI();
    }
}
/*---------------------------------------------------------------------------*/
static void
set_cca_threshold(uint8_t threshold)
{
    /* CCA threshold is not stored in the radio - we could store it here as
     * a global variable? But currently this function is never used ... */
}
/*---------------------------------------------------------------------------*/
static uint8_t
get_cca_threshold(void)
{
    return AT86RF215_CCA_THRESHOLD;
}
/*---------------------------------------------------------------------------*/
/*
 * The radio will only accept packets with matching PAN ID and address.
 * The address are stored in the unit #0!
 */
static uint16_t
get_pan_ID(void)
{
    uint16_t panid = ((uint16_t)regRead(RG_BBCn_MACPID1F0) << 8) & 0xFF00;
    panid |= regRead(RG_BBCn_MACPID0F0) & 0xFF;
    return panid;
}
/*---------------------------------------------------------------------------*/
static void 
set_pan_ID(uint16_t pan)
{
    /* Store the address to unit #0 */
    regWrite(RG_BBCn_MACPID0F0, (uint8_t) pan & 0xFF);
    regWrite(RG_BBCn_MACPID1F0, (uint8_t) (pan >> 8) & 0xFF);
}
/*---------------------------------------------------------------------------*/
static uint16_t
get_short_addr(void)
{
    uint16_t addr = ((uint16_t)regRead(RG_BBCn_MACSHA1F0) << 8) & 0xFF00;
    addr |= regRead(RG_BBCn_MACSHA0F0) & 0xFF;
    return addr;
}
/*---------------------------------------------------------------------------*/
static void
set_short_addr(uint16_t addr)
{
    /* Store the address to unit #0 */
    regWrite(RG_BBCn_MACSHA0F0, (uint8_t) addr & 0xFF);
    regWrite(RG_BBCn_MACSHA1F0, (uint8_t) (addr >> 8) & 0xFF);
}
/*---------------------------------------------------------------------------*/
static void
get_long_addr(uint8_t *addr, uint8_t len)
{
    if (len > 8) len = 8;
    for (uint8_t i=0; i<len; i++) {
        addr[7 - i] = regRead(RG_BBCn_MACEA0 + i);
    }
}
/*---------------------------------------------------------------------------*/
static void
set_long_addr(const uint8_t *addr, uint8_t len)
{
    /* Store the address to unit #0 */
    if (len > 8) len = 8;
    for(uint8_t i=0; i<len; i++) {
        regWrite(RG_BBCn_MACEA0 + i, addr[7 - i]);
    }
}
/*---------------------------------------------------------------------------*/
/*
 * Radio accepts values from 0 to 31 (0x1F) which translates into range
 * -15 dBm to 15 dBm (depending on the modulation used).
 */
static void
set_tx_power(int8_t power)
{
    uint8_t p;
    if (power < AT86RF215_OUTPUT_POWER_MIN || power > AT86RF215_OUTPUT_POWER_MAX) {
        p = AT86RF215_OUTPUT_POWER_MAX;
        LOG_WARN("Invalid power setting (%d)\n", power);
    }
    else {
        p = power + 16;
    }
    bitWrite(SR_RFn_TXPWR, p);
}
/*---------------------------------------------------------------------------*/
static int8_t
get_tx_power(void)
{
    return (bitRead(SR_RFn_TXPWR) - 16);
}
/*---------------------------------------------------------------------------*/
/*
 * Radio supports various setups, but this function is used to set up the
 * channels according to IEEE 802.15.4 - TSCH. Valid channels are 11 to 26.
 *      f = 2405 + 5 * (ch - 11) MHz
 */
static void
set_channel(uint8_t channel)
{
    LOG_DBG("%s(%d)\n", __func__, channel);
    /* Frequency should supposedly be updated in TRXOFF state (datasheet: 6.3.2).
     * However, if we are within the same range (the same band) it can also be
     * updated in TXPREP, which is faster than going to TRXOFF and than back. */
    
    if(flags.RECEIVE_ON){
        regWrite(RG_RFn_CMD, RF_CMD_TXPREP);
    }

#if AT86RF215_FREQUENCY_BAND == FREQ_BAND_24GHZ

        if(channel < AT86RF215_RF_CHANNEL_MIN || channel > AT86RF215_RF_CHANNEL_MAX) {
            LOG_WARN("Invalid channel %u", channel);
            channel = AT86RF215_RF_CHANNEL_MIN;
        }

        /* Channel spacing and center ferq are setted at the config .. select
         * only the channel (CNM must be written last)*/
        regWrite(RG_RF24_CNL, (uint8_t)(channel - 11));
        regWrite(RG_RF24_CNM, 0x00);

        /* TODO: Maybe wait for PLL to settle (10 - 100 us)*/
        //while(!flags.PLL_LOCK);
        //flags.value = 0;
        /* Or use register to check wether the PLL is locked */
        //while(!bitRead(SR_RFn_PLL_LS));

        current_channel = channel;

        /* Turn the radio back on */
        if(flags.RECEIVE_ON){
            at86rf215_on();
        }
#else
        LOG_WARN("Frequency band %u not tested!", AT86RF215_FREQUENCY_BAND);
#endif
}
/*---------------------------------------------------------------------------*/
/*
 * Easier to store it than to read it from the radio...
 */
static uint8_t
get_channel(void)
{
    return current_channel;
}
/*---------------------------------------------------------------------------*/
/**
 * Set frequency via fine resolution obtion.
 * param freq is valid in range of 2400 to 2482 MHz. 
 * param decimal is valid in range of 0 to 9.
 * 
 * NOTE: This implementation is valid only for 2.4 GHz band!
 * 
 * To set the requency to 2440.5 MHz, set: freq = 2440 and decimal = 5.
 * f = 2366 MHz + (26 MHz * N) / 2^16
 */
inline void
set_frequency(uint16_t freq, uint8_t decimal)
{
    uint32_t N = ((freq - 2366) * 65536) / 26;

    /* Offset of 0.5 MHz is equal to 1260 */
    /* Decimal value of 0.1 MHz is equal to 252 */
    N += decimal * 252;

    if(N > 296172){
        N = 296172;
        LOG_WARN("Freq to high\n");
    }
    else if(N < 85700){
        N = 85700;
        LOG_WARN("Freq to low\n");
    }

    printf("N: %lu\n", N);

    regWrite(RG_RF24_CNL,   (uint8_t)( N & 0x00FF));
    regWrite(RG_RF24_CCF0L, (uint8_t)((N & 0xFF00) >> 8));
    regWrite(RG_RF24_CCF0H, (uint8_t)((N & 0xFF0000) >> 16));
    regWrite(RG_RF24_CNM, 0xC0);
}

/*---------------------------------------------------------------------------*/
/* HAL */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/**
 * Read a single register (16-bit) from the AT86RF215
 * Implementation of the so called single access mode
 */
static uint8_t 
regRead(uint16_t address)
{
    uint8_t data;
    at86rf215_arch_spi_select();
    at86rf215_arch_spi_txrx(SPI_CMD_READ | (uint8_t)((address & 0xFF00) >> 8));
    at86rf215_arch_spi_txrx((uint8_t)(address & 0x00FF));
    data = at86rf215_arch_spi_txrx(0);
    at86rf215_arch_spi_deselect();
    return data;
}
/*---------------------------------------------------------------------------*/
/**
 * Write a single register (16-bit) to the AT86RF215
 */
static void 
regWrite(uint16_t address, uint8_t data)
{
    at86rf215_arch_spi_select();
    at86rf215_arch_spi_txrx(SPI_CMD_WRITE |(uint8_t)((address & 0xFF00) >> 8));   
    at86rf215_arch_spi_txrx((uint8_t)(address & 0x00FF));     
    at86rf215_arch_spi_txrx(data);
    at86rf215_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
/**
 * Read one ore multiple bits from the radio's register
 * 
 * For params (addr, mask, offset) --> u can use SR_ defines in registermap.h
 */
static uint8_t
bitRead(uint16_t address, uint8_t mask, uint8_t offset){
    uint8_t tmp = regRead(address);
    return ((tmp & mask) >> offset);
}
/*---------------------------------------------------------------------------*/
/**
 * Write one ore multiple bits to the radio's register. The previous content 
 * of the registers is not overwritten.
 * 
 * For (addr, mask, offset) --> u can use SR_ defines in registermap.h
*/
static void
bitWrite(uint16_t address, uint8_t mask, uint8_t offset, uint8_t value){
    uint8_t tmp = regRead(address);
    tmp = (tmp & ~mask) | ((value << offset) & mask);
    regWrite(address, tmp);
}
/*---------------------------------------------------------------------------*/
/**
 * Read a burst of data from the AT86RF215 registers.
 * Implementation of so called block access mode.
 */
static void burstRead(uint16_t address, uint8_t *data, uint16_t len)
{
    at86rf215_arch_spi_select();
    at86rf215_arch_spi_txrx(SPI_CMD_READ | (uint8_t)((address & 0xFF00) >> 8));
    at86rf215_arch_spi_txrx((uint8_t)(address & 0x00FF));

    for (uint8_t i = 0; i < len; i++)
    {
        data[i] = at86rf215_arch_spi_txrx(0);
    }

    at86rf215_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
/**
 * Write a burst of data to the AT86RF215 registers.
 * Implementation of so called block access mode.
 */
static void
burstWrite(uint16_t address, uint8_t *data, uint16_t len)
{
    uint8_t dummy __attribute__((unused));
    at86rf215_arch_spi_select();
    at86rf215_arch_spi_txrx(SPI_CMD_WRITE |(uint8_t)((address & 0xFF00) >> 8));   
    at86rf215_arch_spi_txrx((uint8_t)(address & 0x00FF)); 

    for(uint8_t i = 0; i < len; i++) {
        dummy = at86rf215_arch_spi_txrx(data[i]);
    }

    at86rf215_arch_spi_deselect();
}
/*---------------------------------------------------------------------------*/
/**
 * Custom fuction to read a packet frame from the radio's memory
*/
static void
frameRead(uint8_t *frame, uint16_t *frame_len)
{
    uint8_t len_h = regRead(RG_BBCn_RXFLH);
    uint8_t len_l = regRead(RG_BBCn_RXFLL);

    uint16_t len = (uint16_t)len_l | ((uint16_t) (len_h & 0x07) << 8);

    burstRead(RG_BBCn_FBRXS, frame, len);

    *frame_len = len;
}
/*---------------------------------------------------------------------------*/
/**
 * Custom fuction to write a packet frame to the radio's memory
*/
static void
frameWrite(uint8_t *frame, uint16_t frame_len)
{
    regWrite(RG_BBCn_TXFLL, (uint8_t)(frame_len & 0xFF));
    regWrite(RG_BBCn_TXFLH, (uint8_t)((frame_len >> 8) & 0x07));

    burstWrite(RG_BBCn_FBTXS, frame, frame_len);
}
