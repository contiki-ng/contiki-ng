/*
 * Copyright (c) 2018, Hasso-Plattner-Institut.
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
 *         Common functionality for dealing with wake-up counters.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef WAKE_UP_COUNTER_H_
#define WAKE_UP_COUNTER_H_

#include "contiki.h"
#include "sys/rtimer.h"

#ifdef WAKE_UP_COUNTER_CONF_RATE
#define WAKE_UP_COUNTER_RATE WAKE_UP_COUNTER_CONF_RATE
#else /* WAKE_UP_COUNTER_CONF_RATE */
#define WAKE_UP_COUNTER_RATE 8
#endif /* WAKE_UP_COUNTER_CONF_RATE */

#if (WAKE_UP_COUNTER_RATE & (WAKE_UP_COUNTER_RATE - 1)) != 0
#error WAKE_UP_COUNTER_RATE must be a power of two (i.e., 1, 2, 4, 8, 16, 32, 64, ...).
#endif

#define WAKE_UP_COUNTER_INTERVAL (RTIMER_ARCH_SECOND / WAKE_UP_COUNTER_RATE)
#define WAKE_UP_COUNTER_LEN (4)

typedef union {
  uint32_t u32;
  uint8_t u8[4];
} wake_up_counter_t;

wake_up_counter_t wake_up_counter_parse(uint8_t *src);
void wake_up_counter_write(uint8_t *dst, wake_up_counter_t wuc);
uint32_t wake_up_counter_increments(rtimer_clock_t delta, uint32_t *mod);
uint32_t wake_up_counter_round_increments(rtimer_clock_t delta);
rtimer_clock_t wake_up_counter_shift_to_future(rtimer_clock_t time);

#endif /* WAKE_UP_COUNTER_H_ */
