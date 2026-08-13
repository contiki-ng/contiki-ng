/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
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
 * \addtogroup lib
 * @{
 *
 * \defgroup random Generation of non-cryptographic random numbers
 * @{
 *
 * \file
 *         Header file for generating non-cryptographic random numbers.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef RANDOM_H_
#define RANDOM_H_

#include "contiki.h"
#include <stdint.h>

#ifdef RANDOM_CONF_PRNG
#define RANDOM_PRNG RANDOM_CONF_PRNG
#else /* RANDOM_CONF_PRNG */
#define RANDOM_PRNG sfc32_prng
#endif /* RANDOM_CONF_PRNG */

/**
 *  Structure of PRNG drivers.
 */
struct random_prng {

  /**
   * \brief      Seeds the PRNG with a seed.
   * \param seed The seed.
   */
  void (* seed)(uint64_t seed);

  /**
   * \brief  Generates a 16-bit pseudo-random number.
   * \return The 16-bit pseudo-random number.
   */
  uint_fast16_t (* rand)(void);
};

extern const struct random_prng RANDOM_PRNG;

/*
 * Seeds RANDOM_PRNG using the CSPRNG if enabled and else with the MAC address.
 */
void random_init(void);

/*
 * Calculate a pseudo random number between 0 and 65535.
 *
 * \return A pseudo-random number between 0 and 65535.
 */
static inline unsigned short
random_rand(void)
{
  return RANDOM_PRNG.rand();
}

/* In gcc int rand() uses RAND_MAX and long random() uses RANDOM_MAX */
/* Since random_rand casts to unsigned short, we'll use this maxmimum */
#define RANDOM_RAND_MAX 65535U

#endif /* RANDOM_H_ */

/** @} */
/** @} */
