/*
 * Copyright (c) 2018, Hasso-Plattner-Institut.
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
 * \addtogroup csl
 * @{
 * \file
 *         Generates CCM inputs as required by our secure version of CSL
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CSL_CCM_INPUTS_H_
#define CSL_CCM_INPUTS_H_

#include "lib/ccm-star.h"
#include "net/mac/csl/csl.h"

/**
 * \brief Produces a 13-byte CCM nonce for securing wake-up frames
 */
void csl_ccm_inputs_generate_otp_nonce(uint8_t *nonce, int forward);

/**
 * \brief Produces a 13-byte CCM nonce for securing payload frames
 */
void csl_ccm_inputs_generate_nonce(uint8_t *nonce, int forward);

/**
 * \brief Produces a 13-byte CCM nonce for securing acknowledgement frames
 */
#if CSL_COMPLIANT && LLSEC802154_USES_FRAME_COUNTER
void csl_ccm_inputs_generate_acknowledgement_nonce(uint8_t *nonce,
    const linkaddr_t *source_addr,
    frame802154_frame_counter_t *counter);
#elif !CSL_COMPLIANT
void csl_ccm_inputs_generate_acknowledgement_nonce(uint8_t *nonce, int forward);
#endif /* !CSL_COMPLIANT */

#endif /* CSL_CCM_INPUTS_H_ */

/** @} */
