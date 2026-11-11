/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
 * This file is part of the Contiki-NG operating system.
 */

/**
 * \addtogroup coap
 * @{
 */

/**
 * \file
 *         Block2 tracking for the request state of the client APIs
 */

#include "coap-request-state.h"

/*---------------------------------------------------------------------------*/
void
coap_request_state_init_block2(coap_request_state_t *state)
{
  state->block_num = 0;
  state->res_block = 0;
  state->block_size = COAP_MAX_BLOCK_SIZE;
  state->more = 0;
  state->block_error = 0;
}
/*---------------------------------------------------------------------------*/
bool
coap_request_state_next_block2(coap_request_state_t *state,
                               coap_message_t *response)
{
  uint16_t size;
  uint32_t offset;

  if(!coap_get_header_block2(response, &state->res_block, &state->more,
                             &size, &offset)) {
    /*
     * A response without a Block2 option carries the whole
     * representation, so it can only be the first block.
     */
    if(state->block_num != 0) {
      return false;
    }
    state->block_num = 1;
    return true;
  }

  /*
   * Block numbers depend on the block size, so compare offsets. The
   * parser limits the Block2 option to 3 bytes, so the offset fits in
   * 30 bits.
   */
  if(offset != state->block_num * state->block_size) {
    return false;
  }

  if(size > state->block_size) {
    /*
     * The parser truncates a payload to COAP_MAX_CHUNK_SIZE, so a block
     * larger than requested may not have arrived whole. Keep only the
     * requested size, and request the rest of the block in that size.
     * Both sizes are powers of two, so the offset is a multiple of the
     * requested size.
     */
    if(response->payload_len > state->block_size ||
       response->payload_truncated) {
      response->payload_len = state->block_size;
      state->more = 1;
    }
    size = state->block_size;
  } else {
    /* Request the following blocks in the size that the server chose. */
    state->block_size = size;
  }

  /* Every block but the last must fill its size (RFC 7959, Section 2.2). */
  if(state->more && response->payload_len != size) {
    return false;
  }

  state->block_num = offset / size + 1;
  return true;
}
/*---------------------------------------------------------------------------*/
/** @} */
