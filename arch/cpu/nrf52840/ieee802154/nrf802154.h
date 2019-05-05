/*
 * Copyright (C) 2019 University of Pisa
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
 */

/**
 * @ingroup     drivers_nrf52_802154
 * @{
 *
 * @file
 * @brief       Header for the 802154 radio driver for nRF52 radios
 *
 * @author      Carlo Vallati <carlo.vallati@unipi.it>
 * @}
 */

#ifndef NRF802154_H_
#define NRF802154_H_

#include "dev/radio.h"

extern const struct radio_driver nrf52840_driver;

#define RADIO_PHY_HEADER_LEN   5
/* 250kbps data rate. One byte = 32us */
#define RADIO_BYTE_AIR_TIME       32
/* Delay between GO signal and SFD */
#define NRF52_DELAY_BEFORE_TX ((unsigned)US_TO_RTIMERTICKS(RADIO_PHY_HEADER_LEN * RADIO_BYTE_AIR_TIME + 40))
/* Delay between GO signal and start listening.
 * This value is so small because the radio is constantly on within each timeslot. */
#define NRF52_DELAY_BEFORE_RX ((unsigned)US_TO_RTIMERTICKS(40))
/* Delay between the SFD finishes arriving and it is detected in software. */
#define NRF52_DELAY_BEFORE_DETECT ((unsigned)US_TO_RTIMERTICKS(5))

#endif /* NRF802154_H_ */
