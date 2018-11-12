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
  uint8_t more;
  uint8_t block_error;
  void *user_data;
  coap_request_status_t status;
} coap_request_state_t;


#endif /* COAP_REQUEST_STATE_H_ */
/** @} */
