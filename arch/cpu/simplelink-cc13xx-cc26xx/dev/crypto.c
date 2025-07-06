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
 * \addtogroup cc13xx-cc26xx-crypto
 * @{
 *
 * \file
 *         General functions of the AES and Hash Cryptoprocessor.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "dev/crypto.h"
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_crypto.h)
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(inc/hw_memmap.h)
#include DeviceFamily_constructPath(inc/hw_prcm.h)
#include DeviceFamily_constructPath(driverlib/interrupt.h)
#include DeviceFamily_constructPath(driverlib/prcm.h)

#ifndef CRYPTO_SWRESET_SW_RESET
/* for backwards compatibility with CC13x0/CC26x0 */
#define CRYPTO_SWRESET_SW_RESET CRYPTO_SWRESET_RESET
#endif /* !CRYPTO_SWRESET_SW_RESET */

/*---------------------------------------------------------------------------*/
void
crypto_init(void)
{
  crypto_enable();
  HWREG(CRYPTO_BASE + CRYPTO_O_SWRESET) = 1;
  crypto_disable();
  IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);
}
/*---------------------------------------------------------------------------*/
void
crypto_enable(void)
{
  HWREG(PRCM_BASE + PRCM_O_SECDMACLKGR) |= PRCM_SECDMACLKGR_CRYPTO_CLK_EN;
  PRCMLoadSet();
}
/*---------------------------------------------------------------------------*/
void
crypto_disable(void)
{
  HWREG(PRCM_BASE + PRCM_O_SECDMACLKGR) &= ~PRCM_SECDMACLKGR_CRYPTO_CLK_EN;
  PRCMLoadSet();
}
/*---------------------------------------------------------------------------*/
bool
crypto_is_enabled(void)
{
  return HWREG(PRCM_BASE + PRCM_O_SECDMACLKGR)
         & PRCM_SECDMACLKGR_CRYPTO_CLK_EN;
}
/*---------------------------------------------------------------------------*/

/** @} */
