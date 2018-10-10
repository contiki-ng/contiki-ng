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

#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_

/* Set to enable TSCH security */
#ifndef WITH_SECURITY
#define WITH_SECURITY 0
#endif /* WITH_SECURITY */

/*******************************************************/
/********************* Enable TSCH *********************/
/*******************************************************/

/* Needed for CC2538 platforms only */
/* For TSCH we have to use the more accurate crystal oscillator
 * by default the RC oscillator is activated */
#define SYS_CTRL_CONF_OSC32K_USE_XTAL 1

/* Needed for cc2420 platforms only */
/* Disable DCO calibration (uses timerB) */
#define DCOSYNCH_CONF_ENABLED 0
/* Enable SFD timestamps (uses timerB) */
#define CC2420_CONF_SFD_TIMESTAMPS 1

/* Enable Sixtop Implementation */
#define TSCH_CONF_WITH_SIXTOP 1

/*******************************************************/
/******************* Configure TSCH ********************/
/*******************************************************/

/* IEEE802.15.4 PANID */
#define IEEE802154_CONF_PANID 0xabcd

/* Do not start TSCH at init, wait for NETSTACK_MAC.on() */
#define TSCH_CONF_AUTOSTART 0

/* 6TiSCH schedule length */
#define TSCH_SCHEDULE_CONF_DEFAULT_LENGTH 11

#if WITH_SECURITY

/* Enable security */
#define LLSEC802154_CONF_ENABLED 1

#endif /* WITH_SECURITY */

#endif /* PROJECT_CONF_H_ */
