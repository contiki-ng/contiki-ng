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
 * \addtogroup random
 * @{
 *
 * \file
 *         Utility functions for non-cryptographic random numbers.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "lib/random.h"
#include "lib/csprng.h"
#include "net/linkaddr.h"
#include "net/netstack.h"
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "random"
#define LOG_LEVEL LOG_LEVEL_NONE

/*---------------------------------------------------------------------------*/
void
random_init(void)
{
  uint64_t seed;
#if CSPRNG_ENABLED
  if(csprng_rand((uint8_t *)&seed, sizeof(seed))) {
    LOG_DBG("Generated seed with CSPRNG: 0x%" PRIx64 "\n", seed);
  } else
#endif /* CSPRNG_ENABLED */
  {
#if LINKADDR_SIZE < 8
    if(NETSTACK_RADIO.get_object(RADIO_PARAM_64BIT_ADDR, &seed, sizeof(seed))
       == RADIO_RESULT_OK) {
      LOG_DBG("Using 64-bit address as seed: 0x%" PRIx64 "\n", seed);
    } else
#endif /* LINKADDR_SIZE < 8 */
    {
      memcpy(&seed, linkaddr_node_addr.u8, sizeof(linkaddr_node_addr.u8));
      LOG_DBG("Using %zu-byte linkaddr as seed: 0x%" PRIx64 "\n",
              (size_t)LINKADDR_SIZE,
              seed);
    }
  }
  RANDOM_PRNG.seed(seed);
}
/*---------------------------------------------------------------------------*/

/** @} */
