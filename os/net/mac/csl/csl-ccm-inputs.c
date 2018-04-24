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

#include "net/mac/csl/csl-ccm-inputs.h"
#include "net/linkaddr.h"
#include "net/packetbuf.h"
#include "lib/aes-128.h"
#include "services/akes/akes-mac.h"

enum alpha {
  ALPHA_OTP = 0,
  ALPHA_HELLO = 1,
  ALPHA_UNICAST = 2,
  ALPHA_ACKNOWLEDGEMENT = 3,
};

/*---------------------------------------------------------------------------*/
#if CSL_COMPLIANT && LLSEC802154_USES_FRAME_COUNTER
void
csl_ccm_inputs_generate_nonce(uint8_t *nonce, int forward)
{
  const linkaddr_t *source_addr;

  source_addr = forward
      ? &linkaddr_node_addr
      : packetbuf_addr(PACKETBUF_ADDR_SENDER);
  memcpy(nonce, source_addr->u8, LINKADDR_SIZE);
  memset(nonce + LINKADDR_SIZE, 0, 8 - LINKADDR_SIZE);
  anti_replay_write_counter(nonce + 8);
  nonce[12] = packetbuf_attr(PACKETBUF_ATTR_SECURITY_LEVEL);
}
/*---------------------------------------------------------------------------*/
void
csl_ccm_inputs_generate_acknowledgement_nonce(uint8_t *nonce,
    const linkaddr_t *source_addr,
    frame802154_frame_counter_t *counter)
{
  memcpy(nonce, source_addr->u8, LINKADDR_SIZE);
  memset(nonce + LINKADDR_SIZE, 0, 8 - LINKADDR_SIZE);
  memcpy(nonce + 8, counter->u8, 4);
  nonce[12] = CSL_FRAMER_COMPLIANT_ACKNOWLEDGEMENT_SEC_LVL;
}
#elif !CSL_COMPLIANT
/*---------------------------------------------------------------------------*/
static void
generate_nonce(uint8_t *nonce,
    const linkaddr_t *source_addr,
    enum alpha alpha,
    uint8_t burst_index,
    wake_up_counter_t wuc)
{
  /* source address */
  memcpy(nonce, source_addr->u8, LINKADDR_SIZE);
  /* alpha */
  nonce[LINKADDR_SIZE] = alpha << 6;
  /* burst index */
  nonce[LINKADDR_SIZE] |= burst_index;
  /* wake-up counter */
  wake_up_counter_write(nonce + LINKADDR_SIZE + 1, wuc);
  /* fill with zeroes */
  memset(nonce + LINKADDR_SIZE + 1 + WAKE_UP_COUNTER_LEN,
      0,
      CCM_STAR_NONCE_LENGTH - LINKADDR_SIZE - 1 - WAKE_UP_COUNTER_LEN);
}
/*---------------------------------------------------------------------------*/
void
csl_ccm_inputs_generate_otp_nonce(uint8_t *nonce, int forward)
{
  generate_nonce(nonce,
      forward ? &linkaddr_node_addr : packetbuf_addr(PACKETBUF_ADDR_SENDER),
      ALPHA_OTP,
      0,
      forward ? csl_predict_wake_up_counter() : csl_wake_up_counter);
}
/*---------------------------------------------------------------------------*/
void
csl_ccm_inputs_generate_nonce(uint8_t *nonce, int forward)
{
  const linkaddr_t *source_addr;
  enum alpha alpha;
  uint8_t burst_index;
  wake_up_counter_t wuc;

  source_addr = forward
      ? &linkaddr_node_addr
      : packetbuf_addr(PACKETBUF_ADDR_SENDER);

  if(akes_mac_is_hello()) {
    alpha = ALPHA_HELLO;
    burst_index = 0;
    wuc = forward
        ? csl_get_wake_up_counter(csl_get_payload_frames_shr_end())
        : csl_restore_wake_up_counter();
  } else {
    alpha = ALPHA_UNICAST;
    burst_index = packetbuf_attr(PACKETBUF_ATTR_BURST_INDEX);
    wuc = forward
        ? csl_predict_wake_up_counter()
        : csl_wake_up_counter;
  }

  generate_nonce(nonce, source_addr, alpha, burst_index, wuc);
}
/*---------------------------------------------------------------------------*/
void
csl_ccm_inputs_generate_acknowledgement_nonce(uint8_t *nonce, int forward)
{
  generate_nonce(nonce,
      forward ? &linkaddr_node_addr : packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
      ALPHA_ACKNOWLEDGEMENT,
      packetbuf_attr(PACKETBUF_ATTR_BURST_INDEX),
      forward ? csl_wake_up_counter : csl_predict_wake_up_counter());
}
/*---------------------------------------------------------------------------*/
#endif /* CSL_COMPLIANT */

/** @} */
