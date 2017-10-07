/*
 * Copyright (c) 2015, SICS Swedish ICT.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \author Simon Duquennoy <simonduq@sics.se>
 */

#ifndef __PROJECT_CONF_H__
#define __PROJECT_CONF_H__

/* Set to use the Contiki shell */
#define WITH_SHELL 1

/* Set to enable TSCH security */
#ifndef WITH_SECURITY
#define WITH_SECURITY 0
#endif /* WITH_SECURITY */

/*******************************************************/
/********************* Enable TSCH *********************/
/*******************************************************/

/* TSCH and RPL callbacks */
#define RPL_CALLBACK_PARENT_SWITCH tsch_rpl_callback_parent_switch
#define RPL_CALLBACK_NEW_DIO_INTERVAL tsch_rpl_callback_new_dio_interval
#define TSCH_CALLBACK_KA_SENT tsch_rpl_callback_ka_sent
#define TSCH_CALLBACK_JOINING_NETWORK tsch_rpl_callback_joining_network
#define TSCH_CALLBACK_LEAVING_NETWORK tsch_rpl_callback_leaving_network

/*******************************************************/
/******************* Configure TSCH ********************/
/*******************************************************/

/* IEEE802.15.4 PANID */
#define IEEE802154_CONF_PANID 0x81a5

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0

/* 6TiSCH minimal schedule length.
 * Larger values result in less frequent active slots: reduces capacity and saves energy. */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 11

#if WITH_SECURITY
/* Enable security */
#define LLSEC802154_CONF_ENABLED 1
#define LLSEC802154_CONF_USES_EXPLICIT_KEYS 0
#define LLSEC802154_CONF_USES_FRAME_COUNTER 0
#define TSCH_SECURITY_CONF_K1 { 0x11, 0x11, 0x11, 0x11,         \
                                0x11, 0x11, 0x11, 0x11,         \
                                0x11, 0x11, 0x11, 0x11,         \
                                0x11, 0x11, 0x11, 0x11 }
#define TSCH_SECURITY_CONF_K2 { 0x22, 0x22, 0x22, 0x22,         \
                                0x22, 0x22, 0x22, 0x22,         \
                                0x22, 0x22, 0x22, 0x22,         \
                                0x22, 0x22, 0x22, 0x22 }
#endif /* WITH_SECURITY */

#define TSCH_CONF_MAC_MAX_FRAME_RETRIES       3

#define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE (uint8_t[]){ 20 }
//#define TSCH_CONF_DEFAULT_HOPPING_SEQUENCE TSCH_HOPPING_SEQUENCE_16_16

#define TSCH_PACKET_CONF_EACK_WITH_SRC_ADDR 1

#define TSCH_PACKET_CONF_EB_WITH_SLOTFRAME_AND_LINK 1

#define TSCH_CONF_EB_PERIOD (1 * CLOCK_SECOND)
#define TSCH_CONF_MAX_EB_PERIOD (1 * CLOCK_SECOND)

/*******************************************************/
/******************* Configure 6top ********************/
/*******************************************************/

#define TSCH_CONF_WITH_SIXTOP                 1
#define SF_PLUGTEST_SFID                      0x00
#define SF_PLUGTEST_TIMEOUT                   CLOCK_SECOND
#define SIXP_CONF_WITH_PAYLOAD_TERMINATION_IE 0

/*******************************************************/
/************* Platform dependent configuration ********/
/*******************************************************/

#if CONTIKI_TARGET_CC2538DK || CONTIKI_TARGET_ZOUL || \
  CONTIKI_TARGET_OPENMOTE_CC2538
#define TSCH_CONF_HW_FRAME_FILTERING    0
#endif /* CONTIKI_TARGET_CC2538DK || CONTIKI_TARGET_ZOUL \
       || CONTIKI_TARGET_OPENMOTE_CC2538 */

/* Needed for CC2538 platforms only */
/* For TSCH we have to use the more accurate crystal oscillator
 * by default the RC oscillator is activated */
#define SYS_CTRL_CONF_OSC32K_USE_XTAL 1

#define USB_SERIAL_CONF_ENABLE 1
/* USB serial takes space, free more space elsewhere */
#define SICSLOWPAN_CONF_FRAG 0
#define UIP_CONF_BUFFER_SIZE 160

#if CONTIKI_TARGET_SRF06_CC26XX
#define CC2650_FAST_RADIO_STARTUP               1
#endif /* CONTIK_TARGET_SRF06_CC26XX */

#if CONTIKI_TARGET_COOJA
#define COOJA_CONF_SIMULATE_TURNAROUND 0
#endif /* CONTIKI_TARGET_COOJA */

/* Needed for cc2420 platforms only */
/* Disable DCO calibration (uses timerB) */
#define DCOSYNCH_CONF_ENABLED 0
/* Enable SFD timestamps (uses timerB) */
#define CC2420_CONF_SFD_TIMESTAMPS 1

/*******************************************************/
/******************* Configure 6LoWPAN/IPv6 ************/
/*******************************************************/

#define UIP_CONF_IPV6_CHECKS 1

#define SICSLOWPAN_CONF_COMPRESSION SICSLOWPAN_COMPRESSION_6LORH

/*******************************************************/
/********* Enable RPL non-storing mode *****************/
/*******************************************************/

#define RPL_CONF_MOP RPL_MOP_NON_STORING /* Mode of operation*/

/*******************************************************/
/************* Other system configuration **************/
/*******************************************************/

/* Logging */
#define LOG_CONF_LEVEL_RPL                         LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_TCPIP                       LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_IPV6                        LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_6LOWPAN                     LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC                         LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_FRAMER                      LOG_LEVEL_DBG
#define LOG_CONF_LEVEL_6TOP                        LOG_LEVEL_DBG
#define TSCH_LOG_CONF_PER_SLOT                     1

#define TCPIP_CONF_ANNOTATE_TRANSMISSIONS          0

#endif /* __PROJECT_CONF_H__ */
