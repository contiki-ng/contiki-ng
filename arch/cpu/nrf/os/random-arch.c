/*
 * Copyright (c) 2020, Toshiba BRIL
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-os OS drivers
 * @{
 *
 * \addtogroup nrf-random Random driver
 * @{
 * 
 * \file
 *         Random driver for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include <stdlib.h>

#include "nrfx.h"

/*---------------------------------------------------------------------------*/
/**
 * @brief Generates a new random number
 * @return The random number.
 */
unsigned short
random_rand(void)
{
  return (unsigned short)rand();
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Initialize the random number generator
 * @param seed Ignored if NRF_RNG is present.
 */
void
random_init(unsigned short seed)
{
/* If board supports RNG uses it to seed srand */
#ifdef NRF_RNG
  (void)seed;

  unsigned short hwrng = 0;
  NRF_RNG->TASKS_START = 1;

  NRF_RNG->EVENTS_VALRDY = 0;
  while(!NRF_RNG->EVENTS_VALRDY);
  hwrng = (NRF_RNG->VALUE & 0xFF);

  NRF_RNG->EVENTS_VALRDY = 0;
  while(!NRF_RNG->EVENTS_VALRDY);
  hwrng |= ((NRF_RNG->VALUE & 0xFF) << 8);

  NRF_RNG->TASKS_STOP = 1;

  srand(hwrng);
#else /* NRF_RNG */
  srand(seed);
#endif /* NRF_RNG */
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
