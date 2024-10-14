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
 *        AT86RF215 radio constants and defines. Required by the Contiki-NG.
 *        The file is included by contiki-conf.h
 * \author
 *        Grega Morano <grega.morano@ijs.si>
 */
/*---------------------------------------------------------------------------*/

#ifndef AT86RF215_DEF_H_
#define AT86RF215_DEF_H_


/**
 * The delay between radio Tx request and SFD sent
 * 1) transition to TXPREP --> done in prepare(), needed only if CCA is enabled
 * 2) tx_bb_delay    --> <3 us for OQPSK
 * 3) tx_proc_delay  --> depends on register settings (~10 us)
 * 4) tx_start_delay --> 4 us
 * 
 * However, the current drivers require appx. 240 us delay before TX.
 *
 * If we put state transition to TXPREP into at86rf215_prepare(), then this
 * time can be reduced for 80us. However, that would disrupt the CSMA ...
 */
#define AT86RF215_DELAY_BEFORE_TX		((unsigned)US_TO_RTIMERTICKS(315))

/**
 * The delay between radio Rx request and start listening
 * 1) transition to RX --> 90 us
 * 
 * The radio drivers are built in a way, that radio must be on during the
 * timeslot (TSCH_CONF_RADIO_ON_DURING_TIMESLOT), so this time could be 
 * smaller. The delay is added to compensate the "large" TX_DELAY.
 */
#define AT86RF215_DELAY_BEFORE_RX       ((unsigned)US_TO_RTIMERTICKS(100)) 

/**
 * The delay between the end of SFD reception and the radio returning 1 
 * to receiving_packet(). Preamble length is 
 * 8 symbols (16 us) + SFD (2 us) = 18 us
 */
#define AT86RF215_DELAY_BEFORE_DETECT   ((unsigned)US_TO_RTIMERTICKS(20)) 

/**
 * The number of header and footer bytes of overhead at the PHY 
 * layer after SFD (1 length + 2 CRC)
 */
#define AT86RF215_PHY_OVERHEAD			(3)

/**
 * The air time for one byte in microsecond: 
 * 1 / (250kbps/8) == 32 us/byte
 */
#define AT86RF215_BYTE_AIR_TIME			(32)

#define AT86RF215_MAX_PAYLOAD_SIZE      (127)
#define AT86RF215_OUTPUT_POWER_MIN      (-15)
#define AT86RF215_OUTPUT_POWER_MAX      (15)
#define AT86RF215_RF_CHANNEL_MIN        (11)
#define AT86RF215_RF_CHANNEL_MAX        (26)


/*---------------------------------------------------------------------------*/
/* TSCH additional configuration.
 * 
 * Prior to now, the OpenMoteB used only CC2538 radio so the setup/config of 
 * the node is adopted only to that radio.
 * Some values are already defined in cc2538-def.h and the file must be / is
 * included before this one (by contiki-conf.h). Since I don't want to change
 * the original file, I just comment-out the values here.
 *---------------------------------------------------------------------------*/

/* Drivers optimized so radio can be turned off within the timeslot */
#define TSCH_CONF_RADIO_ON_DURING_TIMESLOT      (0)

/* Frame filtering is done in software (promiscuous mode) */
//#define TSCH_CONF_HW_FRAME_FILTERING            (0)

/* The drift compared to "true" 10ms slots.*/
//#define TSCH_CONF_BASE_DRIFT_PPM -977

/* Rtimer arch second */
//#define RTIMER_ARCH_SECOND 32768

#ifndef TSCH_CONF_RX_WAIT
#define TSCH_CONF_RX_WAIT 2200
#endif

#endif /* AT86RF215_DEF_H_ */
