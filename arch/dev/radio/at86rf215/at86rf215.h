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
 *        Header file for the at86rf215.c
 * \author
 *        Grega Morano <grega.morano@ijs.si>
 */
/*---------------------------------------------------------------------------*/
#ifndef AT86RF215_H_
#define AT86RF215_H_


/* Radio states and flags */
typedef union {
	struct {
		// Corresponds to radio states
		uint8_t RECEIVE_ON:1;	// Indicates that radio is (was) in RX state - it is used manly for CSMA, so that the radio is always on except when it is set to be off with _off();
		uint8_t PLL_LOCK:1; 	// Indicates when TXPREP state was reached
		uint8_t PLL_ERR:1;	
		uint8_t PACKET_PEN:1;	// Packet is pending in the radios buffer

		// Corresponds to radio interrupts
		uint8_t RX_START:1; 	// Indicates when frame was detected
		uint8_t AMI:1;			// Indicates when frame address matched
		uint8_t TX_END:1;		// Indicates TX operation completed
		uint8_t CCA:1;			// Indicates when CCA has ended (single Energy Detection has Completed)
	};
	uint8_t value;
} at86rf215_flags_t;

/* RF IRQ bits */
typedef union {
	struct {
		uint8_t IRQ1_WAKEUP:1;	// If radio came from SLEEP state
		uint8_t IRQ2_TRXRDY:1;	// Indicates lock of the PLL (or state change from TRCOFF to TXPREP)
		uint8_t IRQ3_EDC:1;		// End of manual measurement of Energy Detection
		uint8_t IRQ4_BATLOW:1;

		uint8_t IRQ5_TRXERR:1;	// If error during TRX (PLL-unlocks)
		uint8_t IRQ6_IQIFSF:1;	// Applicable in I/Q radio mode only.
	};
	uint8_t value;
} at86rf215_rf_irq_t;

/* BBC IRQ bits */
typedef union {
	struct {
		uint8_t IRQ1_RXFS:1;	// If a valid PHY header is received
		uint8_t IRQ2_RXFE:1;	// Issued at the end of a successful frame reception (first level filtering must pass)
		uint8_t IRQ3_RXAM:1;	// If address filter is enabled and frame is extended
		uint8_t IRQ4_RXEM:1;	// If address recognised as matching
		
		uint8_t IRQ5_TXFE:1;	// When the frame is transmitted (at the end)
		uint8_t IRQ6_AGCH:1;	// used by AGC (detected SHR)
		uint8_t IRQ7_AGCR:1;	// used by AGC (end of RX)
		uint8_t IRQ8_FBLI:1;	// If the preprogramed number of octects is received in the FB
	};
	uint8_t value;
} at86rf215_bbc_irq_t;

/* RX frame struct */
typedef struct {
    //uint8_t content[AT86RF215_MAX_PAYLOAD_SIZE];
    //uint8_t len;
    //uint16_t crc;
	rtimer_clock_t timestamp;
    uint8_t lqi;
    int8_t rssi;
} at86rf215_rx_frame_t;

/* Basic radio configuration */
typedef struct {
    uint16_t address;
    uint8_t value;
} at86rf215_radio_config_t;

/*---------------------------------------------------------------------------*/
/* NETSTACK API radio driver functions */
/*---------------------------------------------------------------------------*/
int at86rf215_init(void);
int at86rf215_prepare(const void *payload, unsigned short payload_len);
int at86rf215_transmit(unsigned short payload_len);
int at86rf215_send(const void *payload, unsigned short payload_len);
int at86rf215_read(void *buf, unsigned short buf_len);
int at86rf215_receiving_packet(void);
int at86rf215_pending_packet(void);
int at86rf215_on(void);
int at86rf215_off(void);
int at86rf215_channel_clear(void);
static radio_result_t at86rf215_get_value(radio_param_t param, radio_value_t *value);
static radio_result_t at86rf215_set_value(radio_param_t param, radio_value_t value);
static radio_result_t at86rf215_get_object(radio_param_t param, void *dest, size_t size);
static radio_result_t at86rf215_set_object(radio_param_t param, const void *src, size_t size);

/*---------------------------------------------------------------------------*/
/** The NETSTACK data structure for the AT86RF215 driver */
extern const struct radio_driver at86rf215_driver;

#endif /* AT86RF215_H_ */
