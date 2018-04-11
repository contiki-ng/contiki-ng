/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
#ifndef CC2538_DEF_H_
#define CC2538_DEF_H_
/*---------------------------------------------------------------------------*/
#include "cm3/cm3-def.h"
/*---------------------------------------------------------------------------*/
#define RTIMER_ARCH_SECOND 32768
/*---------------------------------------------------------------------------*/
/* 352us from calling transmit() until the SFD byte has been sent */
#define RADIO_DELAY_BEFORE_TX     ((unsigned)US_TO_RTIMERTICKS(352))
/* 192us as in datasheet but ACKs are not always received, so adjusted to 250us */
#define RADIO_DELAY_BEFORE_RX     ((unsigned)US_TO_RTIMERTICKS(250))
#define RADIO_DELAY_BEFORE_DETECT 0
#ifndef TSCH_CONF_BASE_DRIFT_PPM
/* The drift compared to "true" 10ms slots.
 * Enable adaptive sync to enable compensation for this.
 * Slot length 10000 usec
 *             328 ticks
 * Tick duration 30.517578125 usec
 * Real slot duration 10009.765625 usec
 * Target - real duration = -9.765625 usec
 * TSCH_CONF_BASE_DRIFT_PPM -977
 */
#define TSCH_CONF_BASE_DRIFT_PPM -977
#endif

#if MAC_CONF_WITH_TSCH
#define TSCH_CONF_HW_FRAME_FILTERING  0
#endif /* MAC_CONF_WITH_TSCH */
/*---------------------------------------------------------------------------*/
#define SPI_CONF_CONTROLLER_COUNT 2
/*---------------------------------------------------------------------------*/
/* Path to CMSIS header */
#define CMSIS_CONF_HEADER_PATH               "cc2538_cm3.h"

/* Path to headers with implementation of mutexes and memory barriers */
#define MUTEX_CONF_ARCH_HEADER_PATH          "mutex-cortex.h"
#define MEMORY_BARRIER_CONF_ARCH_HEADER_PATH "memory-barrier-cortex.h"

#define GPIO_HAL_CONF_ARCH_HDR_PATH          "dev/gpio-hal-arch.h"
/*---------------------------------------------------------------------------*/
#endif /* CC2538_DEF_H_ */
/*---------------------------------------------------------------------------*/
