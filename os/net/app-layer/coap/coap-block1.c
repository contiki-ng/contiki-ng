/*
 * Copyright (c) 2014, Lars Schmertmann <SmallLars@t-online.de>.
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
 */

/**
 * \file
 *      CoAP module for block 1 handling
 * \author
 *      Lars Schmertmann <SmallLars@t-online.de>
 */

/**
 * \addtogroup coap
 * @{
 */

#include <string.h>
#include <inttypes.h>

#include "coap.h"
#include "coap-block1.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "coap"
#define LOG_LEVEL  LOG_LEVEL_COAP

/*---------------------------------------------------------------------------*/

/**
 * \brief Block 1 support within a coap-ressource
 *
 *        This function will help you to use block 1. If target is null
 *        error handling and response configuration is active. On return
 *        value 0, the last block was recived, while on return value 1
 *        more blocks will follow. With target, len and maxlen this
 *        function will assemble the blocks.
 *
 *        You can find an example in:
 *        examples/er-rest-example/resources/res-b1-sep-b2.c
 *
 * \param request   Request pointer from the handler
 * \param response  Response pointer from the handler
 * \param target    Pointer to the buffer where the request payload can be assembled
 * \param len       Pointer to the variable, where the function stores the actual length
 * \param max_len   Length of the "target"-Buffer
 *
 * \return 0 if initialisation was successful
 *         -1 if initialisation failed
 */
int
coap_block1_handler(coap_message_t *request, coap_message_t *response,
                    uint8_t *target, size_t *len, size_t max_len)
{
  const uint8_t *payload = 0;
  int pay_len = coap_get_payload(request, &payload);

  if(!pay_len || !payload) {
    coap_status_code = BAD_REQUEST_4_00;
    coap_error_message = "NoPayload";
    return -1;
  }

  if(request->block1_offset + pay_len > max_len) {
    coap_status_code = REQUEST_ENTITY_TOO_LARGE_4_13;
    coap_error_message = "Message to big";
    return -1;
  }

  if(target && len) {
    memcpy(target + request->block1_offset, payload, pay_len);
    *len = request->block1_offset + pay_len;
  }

  if(coap_is_option(request, COAP_OPTION_BLOCK1)) {
    LOG_DBG("Blockwise: block 1 request: Num: %"PRIu32
            ", More: %u, Size: %u, Offset: %"PRIu32"\n",
            request->block1_num,
            request->block1_more,
            request->block1_size,
            request->block1_offset);

    coap_set_header_block1(response, request->block1_num, request->block1_more, request->block1_size);
    if(request->block1_more) {
      coap_set_status_code(response, CONTINUE_2_31);
      return 1;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
/** @} */
