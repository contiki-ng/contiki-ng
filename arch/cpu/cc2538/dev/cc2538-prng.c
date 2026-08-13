/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
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
 *
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
 * \addtogroup random
 * @{
 *
 * \file
 * Driver for the cc2538 Hardware Random Number Generator
 */

#include "lib/random.h"
#include "dev/soc-adc.h"
#include "reg.h"

/*---------------------------------------------------------------------------*/
static uint_fast16_t
rand(void)
{
  /* Clock the RNG LSFR once */
  REG(SOC_ADC_ADCCON1) |= SOC_ADC_ADCCON1_RCTRL0;
  return REG(SOC_ADC_RNDL) | (REG(SOC_ADC_RNDH) << 8);
}
/*---------------------------------------------------------------------------*/
static void
seed(uint64_t seed)
{
  /* Make sure the RNG is on */
  REG(SOC_ADC_ADCCON1) &= ~(SOC_ADC_ADCCON1_RCTRL1 | SOC_ADC_ADCCON1_RCTRL0);

  /* Invalid seeds are 0x0000 and 0x8003 and should not be used. */
  while(((seed & 0xFFFF) == 0x8003) || !(seed & 0xFFFF)) {
    seed >>= 1;
    /* Prepend a one in order to eventually find a seed */
    seed |= 1ULL << 63;
  }

  /* High byte first */
  REG(SOC_ADC_RNDL) = (seed >> 8) & 0xFF;
  REG(SOC_ADC_RNDL) = seed & 0xFF;
}
/*---------------------------------------------------------------------------*/
const struct random_prng cc2538_prng = {
  seed,
  rand
};
/*---------------------------------------------------------------------------*/

/** @} */
