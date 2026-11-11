/*
 * Copyright (c) 2018, RISE SICS AB.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
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
 * \addtogroup coap
 * @{
 */

/**
 * \file
 *         Common request state for all the APIs
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */
#ifndef COAP_REQUEST_STATE_H_
#define COAP_REQUEST_STATE_H_

#include "coap.h"
#include "coap-transactions.h"
#include <stdbool.h>

typedef enum {
  COAP_REQUEST_STATUS_RESPONSE, /* Response received and no more blocks */
  COAP_REQUEST_STATUS_MORE, /* Response received and there are more blocks */
  COAP_REQUEST_STATUS_FINISHED, /* Request finished */
  COAP_REQUEST_STATUS_TIMEOUT, /* Request Timeout after all retransmissions */
  COAP_REQUEST_STATUS_BLOCK_ERROR /* Blocks in wrong order */
} coap_request_status_t;


typedef struct coap_request_state {
  coap_transaction_t *transaction;
  coap_message_t *response;
  coap_message_t *request;
  coap_endpoint_t *remote_endpoint;
  uint32_t block_num;
  uint32_t res_block;
  uint16_t block_size;
  uint8_t more;
  uint8_t block_error;
  void *user_data;
  coap_request_status_t status;
} coap_request_state_t;

/**
 * \brief Prepare the request state for a new blockwise transfer.
 * \param state The request state.
 */
void coap_request_state_init_block2(coap_request_state_t *state);

/**
 * \brief Check whether a response carries the next block of a transfer.
 * \param state The request state.
 * \param response The received response.
 * \return true if the response carries the next block, false otherwise.
 *
 * Sets res_block and more from the Block2 option of the response, if it
 * has one. When the response carries the next block, block_size and
 * block_num are updated to request the block after it, in the block
 * size that the server chose if it is smaller than the one requested
 * (RFC 7959, Section 2.4). A block larger than block_size has its
 * payload trimmed to block_size, and the rest of it is requested next.
 */
bool coap_request_state_next_block2(coap_request_state_t *state,
                                    coap_message_t *response);


#endif /* COAP_REQUEST_STATE_H_ */
/** @} */
