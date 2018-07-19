/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup cc13xx-cc26xx-prng
 * @{
 *
 * Implementation based on Bob Jenkins' small noncryptographic PRNG.
 *  http://burtleburtle.net/bob/rand/smallprng.html
 *
 * \file
 *        This file overrides os/lib/random.c.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include <contiki.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
typedef struct {
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
} ranctx_t;

static ranctx_t ranctx;
/*---------------------------------------------------------------------------*/
#define rot32(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
/*---------------------------------------------------------------------------*/
/**
 * \brief   Generates a new random number using the PRNG.
 * \return  The random number.
 */
unsigned short
random_rand(void)
{
  uint32_t e;

  e        = ranctx.a - rot32(ranctx.b, 27);
  ranctx.a = ranctx.b ^ rot32(ranctx.c, 17);
  ranctx.b = ranctx.c + ranctx.d;
  ranctx.c = ranctx.d + e;
  ranctx.d = e        + ranctx.a;

  return (unsigned short)ranctx.d;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Function required by the API
 * \param  seed Ignored.
 */
void
random_init(unsigned short seed)
{
  uint32_t i;

  ranctx.a = 0xf1ea5eed;
  ranctx.b = ranctx.c = ranctx.d = (uint32_t)seed;
  for(i = 0; i < 20; ++i) {
    (void)random_rand();
  }
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
