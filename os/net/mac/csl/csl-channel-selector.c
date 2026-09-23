/*
 * Copyright (c) 2023, Uppsala universitet.
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
 * \addtogroup csl
 * @{
 *
 * \file
 *
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/csl/csl-channel-selector.h"
#include "net/mac/csl/csl-nbr.h"
#include "net/mac/csl/csl.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL"
#define LOG_LEVEL LOG_LEVEL_MAC

/*---------------------------------------------------------------------------*/
void
csl_channel_selector_take_feedback(bool successful, uint_fast8_t burst_index)
{
#if !CSL_COMPLIANT
  switch(csl_state.transmit.result[burst_index]) {
  case MAC_TX_OK:
  case MAC_TX_COLLISION:
  case MAC_TX_NOACK:
    break;
  default:
    return;
  }

  csl_nbr_t *csl_nbr = csl_nbr_get_receiver();
  if(!csl_nbr) {
    LOG_ERR("receiver not found\n");
    return;
  }

  CSL_CHANNEL_SELECTOR.take_feedback(csl_nbr,
                                     successful,
                                     csl_get_channel_index());

#endif /* !CSL_COMPLIANT */
}
/*---------------------------------------------------------------------------*/
bool
csl_channel_selector_take_feedback_is_exploring(void)
{
#if !CSL_COMPLIANT
  if(csl_state.transmit.is_broadcast) {
    return false;
  }

  csl_nbr_t *csl_nbr = csl_nbr_get_receiver();
  if(!csl_nbr) {
    LOG_ERR("csl_nbr_get_receiver failed");
    LOG_ERR_LLADDR(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
    LOG_ERR_("\n");
    return false;
  }
  return CSL_CHANNEL_SELECTOR.is_exploring(csl_nbr);
#else /* !CSL_COMPLIANT */
  return false;
#endif /* !CSL_COMPLIANT */
}
/*---------------------------------------------------------------------------*/

/** @} */
