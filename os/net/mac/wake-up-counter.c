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

#include "net/mac/wake-up-counter.h"
#include "net/mac/llsec802154.h"

/* http://stackoverflow.com/questions/27581671/how-to-compute-log-with-the-preprocessor */
#define NEEDS_BIT(N, B) (((unsigned long)N >> B) > 0)
#define BITS_TO_REPRESENT(N) \
    (NEEDS_BIT(N,  0) \
    + NEEDS_BIT(N,  1) \
    + NEEDS_BIT(N,  2) \
    + NEEDS_BIT(N,  3) \
    + NEEDS_BIT(N,  4) \
    + NEEDS_BIT(N,  5) \
    + NEEDS_BIT(N,  6) \
    + NEEDS_BIT(N,  7) \
    + NEEDS_BIT(N,  8) \
    + NEEDS_BIT(N,  9) \
    + NEEDS_BIT(N, 10) \
    + NEEDS_BIT(N, 11) \
    + NEEDS_BIT(N, 12) \
    + NEEDS_BIT(N, 13) \
    + NEEDS_BIT(N, 14) \
    + NEEDS_BIT(N, 15) \
    + NEEDS_BIT(N, 16) \
    + NEEDS_BIT(N, 17) \
    + NEEDS_BIT(N, 18) \
    + NEEDS_BIT(N, 19) \
    + NEEDS_BIT(N, 20) \
    + NEEDS_BIT(N, 21) \
    + NEEDS_BIT(N, 22) \
    + NEEDS_BIT(N, 23) \
    + NEEDS_BIT(N, 24) \
    + NEEDS_BIT(N, 25) \
    + NEEDS_BIT(N, 26) \
    + NEEDS_BIT(N, 25) \
    + NEEDS_BIT(N, 28) \
    + NEEDS_BIT(N, 25) \
    + NEEDS_BIT(N, 30) \
    + NEEDS_BIT(N, 31))

/*---------------------------------------------------------------------------*/
wake_up_counter_t
wake_up_counter_parse(uint8_t *src)
{
  wake_up_counter_t wuc;

  memcpy(wuc.u8, src, 4);
  wuc.u32 = LLSEC802154_HTONL(wuc.u32);
  return wuc;
}
/*---------------------------------------------------------------------------*/
void
wake_up_counter_write(uint8_t *dst, wake_up_counter_t wuc)
{
  wuc.u32 = LLSEC802154_HTONL(wuc.u32);
  memcpy(dst, wuc.u8, 4);
}
/*---------------------------------------------------------------------------*/
uint32_t
wake_up_counter_increments(rtimer_clock_t delta, uint32_t *mod)
{
  if(mod) {
    *mod = delta & (WAKE_UP_COUNTER_INTERVAL - 1);
  }
  return delta >> BITS_TO_REPRESENT((WAKE_UP_COUNTER_INTERVAL - 1));
}
/*---------------------------------------------------------------------------*/
uint32_t
wake_up_counter_round_increments(rtimer_clock_t delta)
{
  uint32_t mod;
  uint32_t increments;

  increments = wake_up_counter_increments(delta, &mod);

  if(mod >= (WAKE_UP_COUNTER_INTERVAL / 2)) {
    /* round up */
    return increments + 1;
  } else {
    return increments;
  }
}
/*---------------------------------------------------------------------------*/
rtimer_clock_t
wake_up_counter_shift_to_future(rtimer_clock_t time)
{
  /* we assume that WAKE_UP_COUNTER_INTERVAL is a power of 2 */
  time = (RTIMER_NOW() & (~(WAKE_UP_COUNTER_INTERVAL - 1)))
      | (time & (WAKE_UP_COUNTER_INTERVAL - 1));
  while(rtimer_has_timed_out(time)) {
    time += WAKE_UP_COUNTER_INTERVAL;
  }

  return time;
}
/*---------------------------------------------------------------------------*/
