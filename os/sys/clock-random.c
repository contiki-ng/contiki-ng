/*
 * Copyright (c) 2022, Uppsala universitet.
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
 * \addtogroup clock
 * @{
 *
 * \file
 *         Generates uniformly distributed clock_time_t values.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "contiki.h"
#include "lib/random.h"
#include "sys/clock.h"
#include <stdint.h>
#include <stdbool.h>

#if CLOCK_SIZE == 8
#define HALF_CLOCK_MAX ((clock_time_t)UINT32_MAX)
typedef uint32_t half_clock_time_t;
#else /* CLOCK_SIZE == 8 */
#define HALF_CLOCK_MAX ((clock_time_t)UINT16_MAX)
typedef uint16_t half_clock_time_t;
#endif /* CLOCK_SIZE == 8 */

/*---------------------------------------------------------------------------*/
static half_clock_time_t
generate_random_half_clock_time(void)
{
#if CLOCK_SIZE == 8
  return ((half_clock_time_t)(random_rand() & RANDOM_RAND_MAX))
         | (((half_clock_time_t)(random_rand() & RANDOM_RAND_MAX)) << 16);
#else /* CLOCK_SIZE == 8 */
  return random_rand();
#endif /* CLOCK_SIZE == 8 */
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_random(clock_time_t max)
{
  clock_time_t result;

  /* sort out special cases */
  switch(max) {
  case 0:
    return 0;
  case CLOCK_MAX:
    /* generate a completely random clock_time_t */
    result = generate_random_half_clock_time();
    result <<= (CLOCK_SIZE / 2) * 8;
    result |= generate_random_half_clock_time();
    return result;
  case HALF_CLOCK_MAX:
    return generate_random_half_clock_time();
  default:
    break;
  }

  {
    bool has_overflowed;
    clock_time_t lower_half;
    clock_time_t upper_half;

    if(max > HALF_CLOCK_MAX) {
      has_overflowed = true;
      lower_half = max & HALF_CLOCK_MAX;
      upper_half = max & ~HALF_CLOCK_MAX;
      max >>= (CLOCK_SIZE / 2) * 8;
    } else {
      has_overflowed = false;
    }

    /* along the lines of https://jacquesheunis.com/post/bounded-random/ */
    max++; /* in order to get results <= max */
    result = generate_random_half_clock_time() * max;
    if((result & HALF_CLOCK_MAX) < max) {
      clock_time_t min_valid_value = (HALF_CLOCK_MAX + 1) % max;
      while((result & HALF_CLOCK_MAX) < min_valid_value) {
        result = generate_random_half_clock_time() * max;
      }
    }

    if(!has_overflowed) {
      return result >> ((CLOCK_SIZE / 2) * 8);
    }

    result &= ~HALF_CLOCK_MAX;
    result |= result < upper_half
              ? generate_random_half_clock_time()
              : clock_random(lower_half);
  }
  return result;
}
/*---------------------------------------------------------------------------*/

/** @} */
