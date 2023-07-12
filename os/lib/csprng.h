/*
 * Copyright (c) 2013, Hasso-Plattner-Institut.
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
 *         An OFB-AES-128-based CSPRNG.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/**
 * \addtogroup lib
 * @{
 */

/**
 * \defgroup csprng Cryptographically-secure PRNG
 *
 * \brief Expands a truly random seed into a stream of pseudo-random numbers.
 *
 * In contrast to a normal PRNG, a CSPRNG generates a stream of pseudo-random
 * numbers that is indistinguishable from the uniform distribution to a
 * computationally-bounded adversary who does not know the seed.
 *
 * @{
 */

#ifndef CSPRNG_H_
#define CSPRNG_H_

#include "contiki.h"
#include "lib/aes-128.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef CSPRNG_CONF_ENABLED
#define CSPRNG_ENABLED CSPRNG_CONF_ENABLED
#else /* CSPRNG_CONF_ENABLED */
#define CSPRNG_ENABLED 0
#endif /* CSPRNG_CONF_ENABLED */

#define CSPRNG_KEY_LEN AES_128_KEY_LENGTH
#define CSPRNG_STATE_LEN AES_128_BLOCK_SIZE
#define CSPRNG_SEED_LEN (CSPRNG_KEY_LEN + CSPRNG_STATE_LEN)

/** This is the structure of a seed. */
struct csprng_seed {
  union {
    struct {
      uint8_t key[CSPRNG_KEY_LEN]; /**< AES-128 key of the CSPRNG */
      uint8_t state[CSPRNG_STATE_LEN]; /**< internal state of the CSPRNG */
    };

    uint8_t u8[CSPRNG_SEED_LEN]; /**< for convenience */
  };
};

/**
 * \brief          Mixes a new seed with the current one.
 * \param new_seed Pointer to the new seed.
 *
 *                 This function is called at start up and/or at runtime by
 *                 what we call a "seeder". Seeders generate seeds in arbi-
 *                 trary ways and feed this CSPRNG with their generated seeds.
 */
void csprng_feed(struct csprng_seed *new_seed);

/**
 * \brief        Generates a cryptographic random number.
 * \param result The place to store the generated cryptographic random number.
 * \param len    The length of the cryptographic random number to be generated.
 *
 *               We use output feedback mode (OFB) for generating cryptographic
 *               pseudo-random numbers [RFC 4086]. A potential problem with OFB
 *               is that OFB at some point enters a cycle. However, the
 *               expected cycle length given a random key and a random state
 *               is about 2^127 in our case [Davies and Parkin, The Average
 *               Cycle Size of The Key Stream in Output Feedback Encipherment].
 * \return       Returns true on success and false otherwise.
 */
bool csprng_rand(uint8_t *result, unsigned len);

#endif /* CSPRNG_H_ */

/** @} */
/** @} */
