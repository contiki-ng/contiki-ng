#include <stdio.h>
#include <string.h>

#include <lib/crc16.h>

#include "contiki.h"
#include "sys/log.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "sys/energest.h"
#include "sys/rtimer.h"

#include "sys/critical.h"

#include "rf2xx_registermap.h"
#include "rf2xx_hal.h"
#include "rf2xx.h"
#include "rf2xx_arch.h"

#define LOG_MODULE  "rf2xx"
#define LOG_LEVEL   LOG_LEVEL_RF2XX

PROCESS(rf2xx_process, "AT86RF2xx driver");


#if RF2XX_STATS
volatile uint32_t rf2xxStats[RF2XX_STATS_COUNT] = { 0 };
#endif


// SRC: https://barrgroup.com/Embedded-Systems/How-To/Define-Assert-Macro
#define ASSERT(expr) ({ if (!(expr)) LOG_ERR("Err: " #expr "\n"); })
#define ASSERT_EQUAL(x, y)  ({ if (x != y) LOG_ERR("Err: " #x " != " #y ", %i != %i\n", x, y); })


#define BUSYWAIT_UNTIL(expr)    ({ while(!(expr)); })

static rxFrame_t rxFrame;
static txFrame_t txFrame;

uint8_t rf2xxChip = RF2XX_UNDEFINED;

volatile static rf2xx_flags_t flags;


void
setPanID(uint16_t pan)
{
	regWrite(RG_PAN_ID_0, pan & 0xFF);
	regWrite(RG_PAN_ID_1, pan >> 8);
	LOG_DBG("PAN == 0x%02x\n", pan);
}


uint16_t
getPanID(void)
{
	uint16_t pan = ((uint16_t)regRead(RG_PAN_ID_1) << 8) & 0xFF00;
    pan |= (uint16_t)regRead(RG_PAN_ID_0) & 0xFF;
    return pan;
}

uint16_t
getShortAddr(void)
{
	uint16_t addr = ((uint16_t)regRead(RG_SHORT_ADDR_1) << 8) & 0xFF00;
    addr |= (uint16_t)regRead(RG_SHORT_ADDR_0) & 0xFF;
    return addr;
}

void
setShortAddr(uint16_t addr)
{
	regWrite(RG_SHORT_ADDR_0, addr & 0xFF);
	regWrite(RG_SHORT_ADDR_1, addr >> 8);
	LOG_DBG("Short addr == 0x%02x\n", addr);
}

void
setLongAddr(const uint8_t * addr, uint8_t len)
{
    if (len > 8) len = 8;

    // The usual representation of MAC address has big-endianess. However,
    // This radio uses little-endian, so the order of bytes has to be reversed.
	// When we define IEEE addr, the most important byte is ext_addr[0], while
	// on radio RG_IEEE_ADDR_0 must contain the lowest/least important byte.
    for (uint8_t i = 0; i < len; i++) {
        regWrite(RG_IEEE_ADDR_7 - i, addr[i]);
    }
}


void
getLongAddr(uint8_t *addr, uint8_t len)
{
	if (len > 8) len = 8;
    for (uint8_t i = 0; i < len; i++) {
        addr[i] = regRead(RG_IEEE_ADDR_7 - i);
    }
}


int
rf2xx_init(void)
{
    LOG_DBG("%s\n", __func__);

    // Initialize I/O
    rf2xx_initHW();

    // Reset internal and hardware states
	rf2xx_reset();

	// Start Contiki process which will take care of received packets
	process_start(&rf2xx_process, NULL);
	// process_start(&rf2xx_calibration_process, NULL);

    rf2xx_on();
	return 1;
}


int
rf2xx_prepare(const void *payload, unsigned short payload_len)
{
    LOG_DBG("%s\n", __func__);

    if (payload_len > RF2XX_MAX_PAYLOAD_SIZE) {
        LOG_ERR("Payload larger than radio buffer: %u > %u\n", payload_len, RF2XX_MAX_PAYLOAD_SIZE);
        return RADIO_TX_ERR;
    }

    memcpy(txFrame.content, payload, payload_len);
    txFrame.len = payload_len;

    LOG_DBG("Prepared %u bytes\n", payload_len);

#if !RF2XX_CHECKSUM
    txFrame.crc = (uint16_t *)(txFrame.content + txFrame.len);
    *txBuffer.crc = crc16_data(txFrame.content, txFrame.len, 0x00);
#endif

    return RADIO_TX_OK;
}


int
rf2xx_transmit(unsigned short transmit_len)
{
    LOG_DBG("%s\n", __func__);

    uint8_t trxState, dummy __attribute__((unused));
    vsnSPI_ErrorStatus status;

again:
    trxState = bitRead(SR_TRX_STATUS);
    switch (trxState) {
        case TRX_STATUS_STATE_TRANSITION:
            goto again;

        case TRX_STATUS_TRX_OFF:
        case TRX_STATUS_RX_AACK_ON:
        case TRX_STATUS_BUSY_RX_AACK:
        case TRX_STATUS_RX_ON:
        case TRX_STATUS_BUSY_RX:
            // 1-hop migration
            ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
            flags.value = 0;

            regWrite(RG_TRX_STATE, (RF2XX_ARET) ? TRX_CMD_TX_ARET_ON : TRX_CMD_TX_ON);
            while (bitRead(SR_TRX_STATUS) == TRX_STATUS_STATE_TRANSITION);
            //BUSYWAIT_UNTIL(bitRead(SR_TRX_STATUS) == TRX_STATUS_STATE_TRANSITION);
            //BUSYWAIT_UNTIL(flags.PLL_LOCK); // Is not triggered when Tx -> Rx

        case TRX_STATUS_TX_ON:
        case TRX_STATUS_BUSY_TX:
        case TRX_STATUS_TX_ARET_ON:
        case TRX_STATUS_BUSY_TX_ARET:
            // Already in proper state;
            ENERGEST_ON(ENERGEST_TYPE_TRANSMIT);
            break;

        default: // Unknown state
            LOG_ERR("Radio in state: 0x%02x\n", trxState);
            return RADIO_TX_ERR;
    }


    setSLPTR();
    clearSLPTR();

    status = frameWrite(&txFrame);
    if (status != VSN_SPI_SUCCESS) return RADIO_TX_ERR;

    // Wait to complete BUSY STATE
    BUSYWAIT_UNTIL(flags.TRX_END);

    txFrame.trac = (RF2XX_ARET) ? bitRead(SR_TRAC_STATUS) : TRAC_SUCCESS;
    
    ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);

    flags.value = 0;
    regWrite(RG_TRX_STATE, (RF2XX_AACK) ? TRX_CMD_RX_AACK_ON: TRX_CMD_RX_ON);

    ENERGEST_ON(ENERGEST_TYPE_LISTEN);

	switch (txFrame.trac) {
        case TRAC_SUCCESS:
            RF2XX_STATS_ADD(txSuccess);
			LOG_DBG("TRAC=OK\n");
            return RADIO_TX_OK;

        case TRAC_NO_ACK:
            RF2XX_STATS_ADD(txNoAck);
            LOG_DBG("TRAC=NO-ACK\n");
            return RADIO_TX_NOACK;

        case TRAC_CHANNEL_ACCESS_FAILURE:
            RF2XX_STATS_ADD(txCollision);
            LOG_DBG("TRAC=collision\n");
            return RADIO_TX_COLLISION;

        default:
            LOG_DBG("TRAC=invalid (%u)\n", txFrame.trac);
            return RADIO_TX_ERR;
	}
}

int
rf2xx_send(const void *payload, unsigned short payload_len)
{
	rf2xx_prepare(payload, payload_len);
	return rf2xx_transmit(payload_len);
}


int rf2xx_read(void *buf, unsigned short buf_len)
{
    int_master_status_t status;
    uint8_t frame_len = rxFrame.len;

    status = critical_enter();

    memcpy(buf, rxFrame.content, rxFrame.len);
    rxFrame.len = 0;

    critical_exit(status);

    LOG_DBG("Got %u bytes\n", frame_len);
    return frame_len;
}


int
rf2xx_channel_clear(void)
{
    if (RF2XX_CCA) {
        return 1;
    }

    uint8_t cca;

    rf2xx_on();

    bitWrite(SR_RX_PDT_DIS, 1); // disable reception

    bitWrite(SR_CCA_REQUEST, 1); // trigger CCA sensing
    BUSYWAIT_UNTIL(flags.CCA);
    flags.CCA = 0;

    cca = bitRead(SR_CCA_STATUS); // 1 = IDLE, 0 = BUSY

    bitWrite(SR_RX_PDT_DIS, 0); // Enable reception

    return cca;
}


int // Check if the radio driver is currently receiving a packet 
rf2xx_receiving_packet(void)
{
    if (flags.RX_START) {
        uint8_t trxState = bitRead(SR_TRX_STATUS);
        switch (trxState) {
            case TRX_STATUS_BUSY_RX:
            case TRX_STATUS_BUSY_RX_AACK:
            case TRX_STATUS_BUSY_RX_AACK_NOCLK:
                // in any busy Rx state
                return 1;

            default:
                // false alarm
                flags.RX_START = 0; 
                return 0;
        }
    }

    return 0;
}

int
rf2xx_pending_packet(void)
{
    return rxFrame.len > 0;
}


int
rf2xx_on(void)
{
    uint8_t trxState;

again:
    trxState = bitRead(SR_TRX_STATUS);
    switch (trxState) {
        case TRX_STATUS_STATE_TRANSITION:
            goto again;

        case TRX_STATUS_TRX_OFF:
        case TRX_STATUS_TX_ARET_ON:
        case TRX_STATUS_BUSY_TX_ARET:
        case TRX_STATUS_TX_ON:
        case TRX_STATUS_BUSY_TX:
            // 1-hop migration
            flags.value = 0;
            regWrite(RG_TRX_STATE, (RF2XX_AACK) ? TRX_CMD_RX_AACK_ON : TRX_CMD_RX_ON);
            while (bitRead(SR_TRX_STATUS) == TRX_STATUS_STATE_TRANSITION);
            //BUSYWAIT_UNTIL(flags.PLL_LOCK);
            ENERGEST_OFF(ENERGEST_TYPE_TRANSMIT);
            // fall-thru

        //case TRX_STATUS_RX_AACK_ON_NOCLK:
        //case TRX_STATUS_BUSY_RX_AACK_NOCLK:
        case TRX_STATUS_RX_AACK_ON:
        case TRX_STATUS_BUSY_RX_AACK:
        case TRX_STATUS_RX_ON:
        case TRX_STATUS_BUSY_RX:
            // Proper state
            ENERGEST_ON(ENERGEST_TYPE_LISTEN);
            return 1;

        default:
            LOG_DBG("Unknown state: 0x%02x\n", trxState);
            return 0;
    }
}


int
rf2xx_off(void)
{
    uint8_t trxState;

again:
    trxState = bitRead(SR_TRX_STATUS);
    switch (trxState) {
        case TRX_STATUS_STATE_TRANSITION:
            goto again;

        case TRX_STATUS_TRX_OFF:
            // already in OFF state
            return 1;

        case TRX_STATUS_RX_ON:
        case TRX_STATUS_RX_AACK_ON:
        case TRX_STATUS_RX_AACK_ON_NOCLK:
        case TRX_STATUS_TX_ON:
        case TRX_STATUS_TX_ARET_ON:
            // Idle Tx/Rx state
            regWrite(RG_TRX_STATE, TRX_CMD_FORCE_TRX_OFF);
            flags.value = 0;
            ENERGEST_OFF(ENERGEST_TYPE_LISTEN);
            return 1;

        case TRX_STATUS_BUSY_RX:
        case TRX_STATUS_BUSY_RX_AACK:
        case TRX_STATUS_BUSY_RX_AACK_NOCLK:
        case TRX_STATUS_BUSY_TX:
        case TRX_STATUS_BUSY_TX_ARET:
            // Busy states
            LOG_DBG("Busy state\n");
            return 0;

        default:
            LOG_ERR("Unknown state: 0x%02x\n", trxState);
            return 0;
    }
}

void
rf2xx_isr(void)
{
    ENERGEST_ON(ENERGEST_TYPE_IRQ);

    rf2xx_irq_t irq;
    irq.value = regRead(RG_IRQ_STATUS);

    if (irq.IRQ1_PLL_UNLOCK) {
        flags.PLL_LOCK = 0;
    }

    if (irq.IRQ0_PLL_LOCK) {
        flags.PLL_LOCK = 1;
    }

    if (irq.IRQ2_RX_START) {
        flags.RX_START = 1;
        flags.AMI = 0;
        flags.TRX_END = 0;

        rxFrame.timestamp = RTIMER_NOW();
        RF2XX_STATS_ADD(rxDetected);
    }

    if (irq.IRQ5_AMI) {
        flags.AMI = 1;
        RF2XX_STATS_ADD(rxAddrMatch);
    }

    if (irq.IRQ6_TRX_UR) {
        flags.TRX_UR = 1;
    }

    if (irq.IRQ3_TRX_END) {
        flags.TRX_END = 1;

        if (flags.RX_START) {
            frameRead(&rxFrame);
            process_poll(&rf2xx_process);
 
            RF2XX_STATS_ADD(rxSuccess);
        } else {
            RF2XX_STATS_ADD(txCount);
        }
    }

	if (irq.IRQ4_AWAKE_END) { // CCA_***_IRQ
		flags.SLEEP = 0;
		flags.CCA = 0;
	}

	if (irq.IRQ7_BAT_LOW) {}

    ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}


PROCESS_THREAD(rf2xx_process, ev, data)
{
	int len;
	PROCESS_BEGIN();

	LOG_INFO("AT86RF2xx driver process started!\n");

	while(1) {
		PROCESS_YIELD_UNTIL(!RF2XX_POLLING_MODE && ev == PROCESS_EVENT_POLL);
        RF2XX_STATS_ADD(rxToStack);

        packetbuf_clear();
        packetbuf_set_attr(PACKETBUF_ATTR_TIMESTAMP, rxFrame.timestamp);
        len = rf2xx_read(packetbuf_dataptr(), PACKETBUF_SIZE);

        packetbuf_set_datalen(len);

        NETSTACK_MAC.input();
        
	}
	PROCESS_END();
}




static radio_result_t
get_value(radio_param_t param, radio_value_t *value)
{
	if (!value) return RADIO_RESULT_INVALID_VALUE;

	switch (param) {
		case RADIO_PARAM_POWER_MODE:
        {
            uint8_t trxState = bitRead(SR_TRX_STATUS);
            switch (trxState) {
                case TRX_STATUS_RX_ON:
                case TRX_STATUS_RX_ON_NOCLK:
                case TRX_STATUS_RX_AACK_ON:
                case TRX_STATUS_RX_AACK_ON_NOCLK:
                case TRX_STATUS_BUSY_RX:
                case TRX_STATUS_BUSY_RX_AACK:
                case TRX_STATUS_BUSY_RX_AACK_NOCLK:
                    *value = RADIO_POWER_MODE_ON;
                    return RADIO_RESULT_OK;

                case TRX_STATUS_TX_ON:
                case TRX_STATUS_TX_ARET_ON:
                case TRX_STATUS_BUSY_TX:
                case TRX_STATUS_BUSY_TX_ARET:
                    *value = RADIO_POWER_MODE_CARRIER_ON;
                    return RADIO_RESULT_OK;

                case TRX_STATUS_TRX_OFF:
                default:
                    *value = RADIO_POWER_MODE_OFF;
                    return RADIO_RESULT_OK;
            }
        }

		case RADIO_PARAM_CHANNEL:
			*value = (radio_value_t)bitRead(SR_CHANNEL);
			return RADIO_RESULT_OK;

		case RADIO_PARAM_PAN_ID:
			*value = (radio_value_t)getPanID();
			return RADIO_RESULT_OK;

		case RADIO_PARAM_16BIT_ADDR:
			*value = (radio_value_t)getShortAddr();
			return RADIO_RESULT_OK;

		case RADIO_PARAM_RX_MODE:
            *value = 0;
			if (RF2XX_AACK && !RF2XX_PROMISCOUS_MODE) *value |= RADIO_RX_MODE_ADDRESS_FILTER;
			if (RF2XX_AACK) *value |= RADIO_RX_MODE_AUTOACK;
			if (RF2XX_POLLING_MODE) *value |= RADIO_RX_MODE_POLL_MODE;
            return RADIO_RESULT_OK;

		case RADIO_PARAM_TX_MODE:
            *value = 0;
			if (RF2XX_CCA) *value |= RADIO_TX_MODE_SEND_ON_CCA;
			return RADIO_RESULT_OK;

		case RADIO_PARAM_TXPOWER:
			*value = (radio_value_t)bitRead(SR_TX_PWR);
			return RADIO_RESULT_OK;

		case RADIO_PARAM_CCA_THRESHOLD:
			return RSSI_BASE_VAL + 2 * (radio_value_t)bitRead(SR_CCA_ED_THRES);

		case RADIO_PARAM_RSSI:
            LOG_DBG("Request current RSSI\n");
			*value = (3 * ((radio_value_t)bitRead(SR_RSSI) - 1) + RSSI_BASE_VAL);
			return RADIO_RESULT_OK;

        case RADIO_PARAM_LAST_RSSI:
            LOG_DBG("Request last RSSI\n");
            *value = (radio_value_t)rxFrame.rssi;
            return RADIO_RESULT_OK;

		case RADIO_PARAM_LAST_LINK_QUALITY:
			*value = (radio_value_t)rxFrame.lqi;
			return RADIO_RESULT_OK;

		case RADIO_CONST_CHANNEL_MIN:
			*value = 11;
			return RADIO_RESULT_OK;

		case RADIO_CONST_CHANNEL_MAX:
			*value = 26;
			return RADIO_RESULT_OK;

		case RADIO_CONST_TXPOWER_MIN:
			*value = 0xF;
			return RADIO_RESULT_OK;

		case RADIO_CONST_TXPOWER_MAX:
			*value = 0x0;
			return RADIO_RESULT_OK;

		case RADIO_CONST_PHY_OVERHEAD:
			*value = (radio_value_t)RF2XX_PHY_OVERHEAD;
			return RADIO_RESULT_OK;

		case RADIO_CONST_BYTE_AIR_TIME:
			*value = (radio_value_t)RF2XX_BYTE_AIR_TIME;
			return RADIO_RESULT_OK;

		case RADIO_CONST_DELAY_BEFORE_TX:
			*value = (radio_value_t)RF2XX_DELAY_BEFORE_TX;
			return RADIO_RESULT_OK;

		case RADIO_CONST_DELAY_BEFORE_RX:
			*value = (radio_value_t)RF2XX_DELAY_BEFORE_RX;
			return RADIO_RESULT_OK;

		case RADIO_CONST_DELAY_BEFORE_DETECT:
			*value = (radio_value_t)RF2XX_DELAY_BEFORE_DETECT;
			return RADIO_RESULT_OK;

		default:
			return RADIO_RESULT_NOT_SUPPORTED;
	}

}



static radio_result_t
set_value(radio_param_t param, radio_value_t value)
{
	switch (param) {
        case RADIO_PARAM_POWER_MODE:
            switch (value) {
                case RADIO_POWER_MODE_ON:
                    rf2xx_on();
                    return RADIO_RESULT_OK;

                case RADIO_POWER_MODE_OFF:
                    rf2xx_off();
                    return RADIO_RESULT_OK;

                default:
                    return RADIO_RESULT_INVALID_VALUE;
            }

        case RADIO_PARAM_CHANNEL:
            if (value < 11 || value > 26) {
                return RADIO_RESULT_INVALID_VALUE;
            }
            bitWrite(SR_CHANNEL, value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_PAN_ID:
            setPanID(value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_16BIT_ADDR:
            setShortAddr(value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_RX_MODE:
            //RF2XX_PROMISCOUS_MODE = (value & RADIO_RX_MODE_ADDRESS_FILTER) > 0;

            // Configure Promiscuous mode (AACK-mode only)
            //bitWrite(SR_AACK_PROM_MODE, RF2XX_PROMISCOUS_MODE);
            //bitWrite(SR_AACK_UPLD_RES_FT, RF2XX_PROMISCOUS_MODE);
            //bitWrite(SR_AACK_FLTR_RES_FT, RF2XX_PROMISCOUS_MODE);

            //RF2XX_AACK = (value & RADIO_RX_MODE_AUTOACK) > 0;
            //RF2XX_POLLING_MODE = (value & RADIO_RX_MODE_POLL_MODE) > 0;


        {
            bool addrFilter = (value & RADIO_RX_MODE_ADDRESS_FILTER) > 0;
            bool aackMode = (value & RADIO_RX_MODE_AUTOACK) > 0;
            bool pollMode = (value & RADIO_RX_MODE_POLL_MODE) > 0;

            if (addrFilter != (RF2XX_AACK && !RF2XX_PROMISCOUS_MODE)) {
                //LOG_ERR("Invalid ADDR_FILTER settings\n");
            }

            if (aackMode != RF2XX_AACK) {
                LOG_ERR("Invalid RF2XX_AACK settings\n");
            }

            if (pollMode != RF2XX_POLLING_MODE) {
                LOG_ERR("Invalid RF2XX_POLLING_MODE settings\n");
            }

        }
            return RADIO_RESULT_OK;
            //return RADIO_RESULT_NOT_SUPPORTED;
    
        case RADIO_PARAM_TX_MODE:
        {
            bool sendOnCCA = (value & RADIO_TX_MODE_SEND_ON_CCA) > 0;
            if (sendOnCCA != RF2XX_CCA) {
                LOG_ERR("Invalid RF2XX_CCA settings\n");
            }
        }
            //RF2XX_CCA = (value & RADIO_TX_MODE_SEND_ON_CCA) > 0;
            //bitWrite(SR_MAX_CSMA_RETRIES, RF2XX_CCA ? RF2XX_CSMA_RETRIES : 7);

            return RADIO_RESULT_OK;
            //return RADIO_RESULT_NOT_SUPPORTED;

        case RADIO_PARAM_TXPOWER:
            if (value < 0 || value > 0xF) {
                return RADIO_RESULT_INVALID_VALUE;
            }
            bitWrite(SR_TX_PWR, value);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_CCA_THRESHOLD:
            bitWrite(SR_CCA_ED_THRES, value / 2 + 91);
            return RADIO_RESULT_OK;

        case RADIO_PARAM_SHR_SEARCH:
        default:
            return RADIO_RESULT_NOT_SUPPORTED;
	}
}


static radio_result_t
get_object(radio_param_t param, void *dest, size_t size)
{
	if (dest == NULL) return RADIO_RESULT_ERROR;

	switch (param) {
		case RADIO_PARAM_64BIT_ADDR:
			getLongAddr((uint8_t *)dest, size);
			return RADIO_RESULT_OK;

		case RADIO_PARAM_LAST_PACKET_TIMESTAMP:
		    *(rtimer_clock_t *)dest = rxFrame.timestamp;
		    return RADIO_RESULT_OK;

        #if MAC_CONF_WITH_TSCH
		case RADIO_CONST_TSCH_TIMING:
            LOG_INFO("Reading radio's RADIO_CONST_TSCH_TIMING matrix\n");
			*(const uint16_t **)dest = RF2XX_CONF_DEFAULT_TIMESLOT_TIMING;
        #endif

		default:
			return RADIO_RESULT_NOT_SUPPORTED;
	}
}


static radio_result_t
set_object(radio_param_t param, const void *src, size_t size)
{
	if (src == NULL) return RADIO_RESULT_ERROR;

	switch (param) {
		case RADIO_PARAM_64BIT_ADDR:
			setLongAddr((const uint8_t *)src, size);
			return RADIO_RESULT_OK;

		default:
			return RADIO_RESULT_NOT_SUPPORTED;
	}
}


const struct radio_driver rf2xx_driver = {
	.init = rf2xx_init,
	.prepare = rf2xx_prepare,
	.transmit = rf2xx_transmit,
	.send = rf2xx_send,
	.read = rf2xx_read,
	.channel_clear = rf2xx_channel_clear,
	.receiving_packet = rf2xx_receiving_packet,
	.pending_packet = rf2xx_pending_packet,
	.on = rf2xx_on,
	.off = rf2xx_off,
	.get_value = get_value,
	.set_value = set_value,
	.get_object = get_object,
	.set_object = set_object,
};
