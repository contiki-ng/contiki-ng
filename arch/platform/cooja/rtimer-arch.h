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

#ifndef RTIMER_ARCH_H_
#define RTIMER_ARCH_H_

#include "contiki.h"
#include "sys/clock.h"
#include "lib/simEnvChange.h"
#include "sys/cooja_mt.h"

#define RTIMER_ARCH_SECOND UINT64_C(1000000)

#define US_TO_RTIMERTICKS(US)   (US)
#define RTIMERTICKS_TO_US(T)    (T)
#define RTIMERTICKS_TO_US_64(T) (T)

rtimer_clock_t rtimer_arch_now(void);
int rtimer_arch_check(void);
int rtimer_arch_pending(void);
rtimer_clock_t rtimer_arch_next(void);

/** \brief A platform-specific implementation that calls cooja_mt_yield()
 * periodically. Without this, Cooja will get stuck in the busy-loop
 * without ever updating the current rtimer time. */
#define RTIMER_BUSYWAIT_UNTIL_ABS(cond, t0, max_time) \
  ({                                                                \
    bool c;                                                         \
    while(!(c = cond) && RTIMER_CLOCK_LT(RTIMER_NOW(), (t0) + (max_time))) { \
      simProcessRunValue = 1;                                       \
      cooja_mt_yield();                                             \
    }                                                               \
    c;                                                              \
  })

#endif /* RTIMER_ARCH_H_ */
