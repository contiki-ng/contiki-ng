/*
 * Copyright (c) 2025, Konrad-Felix Krentz
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
 */

/**
 * \addtogroup random
 * @{
 *
 * \file
 *         Implements sfc32 of PractRand.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "lib/random.h"
#include <stdbool.h>

enum {
  BARREL_SHIFT = 21,
  RSHIFT = 9,
  LSHIFT = 3
};

static bool cached;
static uint32_t a;
static uint32_t b;
static uint32_t c;
static uint32_t counter;

/*---------------------------------------------------------------------------*/
static uint_fast16_t
rand(void)
{
  static uint32_t tmp;
  if(cached) {
    cached = false;
    return tmp >> 16;
  }
  tmp = a + b + counter++;
  a = b ^ (b >> RSHIFT);
  b = c + (c << LSHIFT);
  c = ((c << BARREL_SHIFT) | (c >> (32 - BARREL_SHIFT))) + tmp;
  cached = true;
  return tmp & 0xFFFF;
}
/*---------------------------------------------------------------------------*/
static void
seed(uint64_t seed)
{
  cached = false;
  a = 0;
  b = seed;
  c = seed >> 32;
  counter = 1;
  for(uint_fast8_t i = 0; i < 24; i++) {
    rand();
  }
}
/*---------------------------------------------------------------------------*/
const struct random_prng sfc32_prng = {
  seed,
  rand
};
/*---------------------------------------------------------------------------*/

/** @} */
