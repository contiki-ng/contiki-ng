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
 * \addtogroup cc13xx-cc26xx-trng
 * @{
 *
 * \file
 *        Implementation of True Random Number Generator for CC13xx/CC26xx.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "trng-arch.h"
/*---------------------------------------------------------------------------*/
#include <ti/drivers/TRNG.h>
#include <ti/drivers/cryptoutils/cryptokey/CryptoKeyPlaintext.h>
/*---------------------------------------------------------------------------*/
/*
 * Very dirty workaround because the pre-compiled TI drivers library for
 * CC13x0/CC26x0 is missing the CryptoKey object file. This can be removed
 * when the pre-compiled library includes the missing object file.
 */
#include <ti/devices/DeviceFamily.h>
#if (DeviceFamily_PARENT == DeviceFamily_PARENT_CC13X0_CC26X0)
#include <ti/drivers/cryptoutils/cryptokey/CryptoKeyPlaintextCC26XX.c>
#endif
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
bool
trng_rand(uint8_t *entropy_buf, size_t entropy_len, uint32_t timeout_us)
{
  TRNG_Params trng_params;
  TRNG_Handle trng_handle;
  CryptoKey entropy_key;
  int_fast16_t result;

  TRNG_Params_init(&trng_params);
  trng_params.returnBehavior = TRNG_RETURN_BEHAVIOR_BLOCKING;
  if(timeout_us != TRNG_WAIT_FOREVER) {
    trng_params.timeout = timeout_us;
  }

  trng_handle = TRNG_open(0, &trng_params);
  if(!trng_handle) {
    return false;
  }

  CryptoKeyPlaintext_initBlankKey(&entropy_key, entropy_buf, entropy_len);

  result = TRNG_generateEntropy(trng_handle, &entropy_key);

  TRNG_close(trng_handle);

  return result == TRNG_STATUS_SUCCESS;
}
/*---------------------------------------------------------------------------*/
/** @} */
