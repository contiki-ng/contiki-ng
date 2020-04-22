/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         CC2420 driver header file
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CC2420_H_
#define CC2420_H_

#include "contiki.h"
#include "dev/spi-legacy.h"
#include "dev/radio.h"
#include "dev/radio/cc2420/cc2420_const.h"
#include "lib/aes-128.h"

#define WITH_SEND_CCA 1

#ifndef CC2420_CONF_CCA_THRESH
#define CC2420_CONF_CCA_THRESH -45
#endif /* CC2420_CONF_CCA_THRESH */

#ifndef CC2420_CONF_AUTOACK
#define CC2420_CONF_AUTOACK 1
#endif /* CC2420_CONF_AUTOACK */

#define CHECKSUM_LEN        2
#define FOOTER_LEN          2
#define FOOTER1_CRC_OK      0x80
#define FOOTER1_CORRELATION 0x7f

#ifdef CC2420_CONF_RSSI_OFFSET
#define RSSI_OFFSET CC2420_CONF_RSSI_OFFSET
#else /* CC2420_CONF_RSSI_OFFSET */
/* The RSSI_OFFSET is approximate -45 (see CC2420 specification) */
#define RSSI_OFFSET -45
#endif /* CC2420_CONF_RSSI_OFFSET */

int cc2420_init(void);

#define CC2420_MAX_PACKET_LEN      127

int cc2420_set_channel(int channel);
int cc2420_get_channel(void);

void cc2420_set_pan_addr(unsigned pan,
                                unsigned addr,
                                const uint8_t *ieee_addr);

extern signed char cc2420_last_rssi;
extern uint8_t cc2420_last_correlation;

int cc2420_rssi(void);

extern const struct radio_driver cc2420_driver;

/**
 * \param power Between 1 and 31.
 */
void cc2420_set_txpower(uint8_t power);
int cc2420_get_txpower(void);
#define CC2420_TXPOWER_MAX  31
#define CC2420_TXPOWER_MIN   0

/**
 * Interrupt function, called from the simple-cc2420-arch driver.
 *
 */
int cc2420_interrupt(void);

extern int cc2420_authority_level_of_sender;

int cc2420_on(void);
int cc2420_off(void);

void cc2420_set_cca_threshold(int value);

extern const struct aes_128_driver cc2420_aes_128_driver;

#endif /* CC2420_H_ */
