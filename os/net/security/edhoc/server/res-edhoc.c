/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
 * Copyright (c) 2020, Industrial Systems Institute (ISI), Patras, Greece
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \file
 *      EDHOC plugtest resource [RFC9528] with CoAP Block-Wise Transfer [RFC7959]
 * \author
 *      Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund
 */

#include <stdio.h>
#include <string.h>
#include "coap-engine.h"
#include "coap.h"
#include "edhoc-server.h"

#include "sys/log.h"
#define LOG_MODULE "res-edhoc"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*---------------------------------------------------------------------------*/
edhoc_server_t servidor;

static uint8_t msg_rx[EDHOC_MAX_PAYLOAD_LEN];
static size_t msg_rx_len;

static void res_edhoc_post_handler(coap_message_t *request,
                                   coap_message_t *response,
                                   uint8_t *buffer,
                                   uint16_t preferred_size,
                                   int32_t *offset);

RESOURCE(res_edhoc, "title=\"EDHOC resource\"", NULL,
         res_edhoc_post_handler, NULL, NULL);

/*---------------------------------------------------------------------------*/
/* Example allows only one request on time. There are no checks for multiple access !!! */
static void
res_edhoc_post_handler(coap_message_t *request,
                       coap_message_t *response,
                       uint8_t *buffer,
                       uint16_t preferred_size,
                       int32_t *offset)
{
  int block_size = 300; /* FIXME: Make configurable */

  if(*offset == 0) {
    if(coap_block1_handler(request, response, msg_rx, &msg_rx_len,
                           EDHOC_MAX_PAYLOAD_LEN)) {
      LOG_DBG("handler (%d)\n", (int)msg_rx_len);
      LOG_DBG_BYTES(msg_rx, msg_rx_len);
      LOG_DBG_("\n");
      return;
    } else {
      LOG_DBG("RX msg (%d)\n", (int)msg_rx_len);
      LOG_DBG_BYTES(msg_rx, msg_rx_len);
      LOG_DBG_("\n");
      edhoc_server_process(request, response, &servidor, msg_rx, msg_rx_len);
    }
    response->payload = (uint8_t *)edhoc_ctx->buffers.msg_tx;
    response->payload_len = edhoc_ctx->buffers.tx_sz;
    coap_set_header_block1(response, request->block1_num, 0,
                           request->block1_size);

    if(response->payload_len > block_size) {
      coap_set_option(response, COAP_OPTION_BLOCK2);
      coap_set_header_block2(response, 0, 1, block_size);
    }
  } else {
    coap_set_status_code(response, CHANGED_2_04);
    memcpy(buffer, edhoc_ctx->buffers.msg_tx + *offset, block_size);
    if(edhoc_ctx->buffers.tx_sz - *offset < preferred_size) {
      preferred_size = edhoc_ctx->buffers.tx_sz - *offset;
      *offset = -1;
    } else {
      *offset += preferred_size;
    }
    coap_set_payload(response, buffer, preferred_size);
  }
}
/*---------------------------------------------------------------------------*/