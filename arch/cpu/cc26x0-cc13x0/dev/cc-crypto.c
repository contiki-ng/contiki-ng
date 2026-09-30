/*
 * Copyright (c) 2016, University of Bristol - http://www.bristol.ac.uk
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
 * \addtogroup cc-crypto
 * @{
 *
 * \file
 *         Implementation of general functions of the AES/SHA cryptoprocessor.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "dev/crypto/cc/cc-crypto.h"
#include "ti-lib.h"

struct cc_crypto *const cc_crypto = (struct cc_crypto *)CRYPTO_BASE;

/*---------------------------------------------------------------------------*/
void
cc_crypto_init(void)
{
  IntDisable(INT_CRYPTO_RESULT_AVAIL_IRQ);
  cc_crypto_enable();
  cc_crypto->ctrl.sw_reset = CC_CRYPTO_CTRL_SW_RESET_SW_RESET;
}
/*---------------------------------------------------------------------------*/
void
cc_crypto_enable(void)
{
  ti_lib_prcm_peripheral_run_enable(PRCM_PERIPH_CRYPTO);
  ti_lib_prcm_load_set();
}
/*---------------------------------------------------------------------------*/
void
cc_crypto_disable(void)
{
  ti_lib_prcm_peripheral_run_disable(PRCM_PERIPH_CRYPTO);
  ti_lib_prcm_load_set();
}
/*---------------------------------------------------------------------------*/
bool
cc_crypto_is_enabled(void)
{
  return HWREG(PRCM_BASE + PRCM_O_SECDMACLKGR)
         & PRCM_SECDMACLKGR_CRYPTO_CLK_EN;
}
/*---------------------------------------------------------------------------*/

/** @} */
