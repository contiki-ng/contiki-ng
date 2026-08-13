/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden
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
 *         Project configuration for Serial Radio
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/*---------------------------------------------------------------------------*/
/* Logging configuration */
/*---------------------------------------------------------------------------*/

/* Keep the serial line quiet by default: it carries the binary SLIP/CBOR
   protocol, so verbose debug text would interleave with the frames. The
   serialradio module logs at INFO (startup banner and key events); the
   netstack layers only warn/err. Raise any of these for debugging. */
#define LOG_CONF_LEVEL_SERIAL_RADIO LOG_LEVEL_INFO
#define LOG_CONF_LEVEL_NULLNET LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_MAC LOG_LEVEL_WARN
#define LOG_CONF_LEVEL_RADIO LOG_LEVEL_WARN

/*---------------------------------------------------------------------------*/
/* Network stack configuration */
/*---------------------------------------------------------------------------*/

/* Use nullnet - we don't need IP networking */
#define NETSTACK_CONF_NETWORK nullnet_driver

/* Use sniffer MAC - forwards all packets to network layer */
extern const struct mac_driver sniffer_mac_driver;
#define NETSTACK_CONF_MAC sniffer_mac_driver

/* No framer - we handle raw frames */
#define NETSTACK_CONF_FRAMER no_framer

/*---------------------------------------------------------------------------*/
/* Radio configuration */
/*---------------------------------------------------------------------------*/

/* Disable address filtering for sniffing */
#define RADIO_CONF_RX_MODE_ADDRESS_FILTER 0

/* Disable auto-ACK for raw frame handling */
#define RADIO_CONF_RX_MODE_AUTOACK 0

/*---------------------------------------------------------------------------*/
/* Serial Radio configuration */
/*---------------------------------------------------------------------------*/

/* Buffer sizes */
#define SERIAL_RADIO_CONF_BUF_SIZE 256

/* Maximum frame size */
#define SERIAL_RADIO_CONF_MAX_FRAME_SIZE 127

/*---------------------------------------------------------------------------*/
/* Platform-specific configuration */
/*---------------------------------------------------------------------------*/

#ifdef CONTIKI_TARGET_SIMPLELINK

/* CC13xx/CC26xx specific */

/* Use Sub-GHz 868 MHz mode */
#define RF_CONF_MODE RF_MODE_SUB_1_GHZ

#endif /* CONTIKI_TARGET_SIMPLELINK */

/* Zolertia Zoul/Firefly specific */
/* Use the built-in CC2538 2.4 GHz radio (set to 1 for CC1200 Sub-GHz).
   The 2.4 GHz radio does hardware address filtering + auto-ACK, which the
   border-router router mode relies on. */
#define ZOUL_CONF_USE_CC1200_RADIO 0

/*---------------------------------------------------------------------------*/
/* UART/Serial configuration */
/*---------------------------------------------------------------------------*/

/* UART baud rate - 115200 is standard */
#ifndef UART_CONF_BAUD_RATE
#define UART_CONF_BAUD_RATE 115200
#endif

/*---------------------------------------------------------------------------*/

#endif /* PROJECT_CONF_H_ */
