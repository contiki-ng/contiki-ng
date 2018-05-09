/*
 * Copyright (c) 2016, SICS Swedish ICT AB
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
 * \addtogroup ipso-objects
 * @{
 *
 * Code to test blockwise transfer together with the LWM2M-engine.
 * This object tests get with BLOCK2 and put with BLOCK1.
 *
 */

#include "lwm2m-engine.h"
#include "coap.h"
#include <string.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

static lwm2m_object_instance_t reg_object;
static char junk[64];

static const lwm2m_resource_id_t resources[] =
  {
    RO(10000),
    RO(11000),
    RW(11001)
  };

#define LEN 900

static lwm2m_status_t
opaque_callback(lwm2m_object_instance_t *object,
                lwm2m_context_t *ctx, int num_to_write)
{
  int i;
  PRINTF("opaque-stream callback num_to_write: %d off: %d outlen: %d\n",
         num_to_write, ctx->offset, ctx->outbuf->len);
  for(i = 0; i < num_to_write; i++) {
    ctx->outbuf->buffer[i + ctx->outbuf->len] = '0' + (i & 31);
    if(i + ctx->offset == LEN) break;
  }
  ctx->outbuf->len += i;
  if(ctx->offset + i < LEN) {
    ctx->writer_flags |= WRITER_HAS_MORE;
  }
  return LWM2M_STATUS_OK;
}

/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object,
               lwm2m_context_t *ctx)
{
  uint32_t num;
  uint8_t more;
  uint16_t size;
  uint32_t offset;

  char *str = "just a string";

  PRINTF("Got request at: %d/%d/%d lv:%d\n", ctx->object_id, ctx->object_instance_id, ctx->resource_id, ctx->level);

  if(ctx->level == 1) {
    /* Should not happen */
    return LWM2M_STATUS_ERROR;
  }
  if(ctx->level == 2) {
    /* This is a get whole object - or write whole object */
    return LWM2M_STATUS_ERROR;
  }

  if(ctx->operation == LWM2M_OP_READ) {
#if DEBUG
    if(coap_get_header_block2(ctx->request, &num, &more, &size, &offset)) {
      PRINTF("CoAP BLOCK2: %d/%d/%d offset:%d\n", num, more, size, offset);
    }
#endif

    switch(ctx->resource_id) {
    case 10000:
      lwm2m_object_write_string(ctx, str, strlen(str));
      break;
    case 11000:
    case 11001:
      PRINTF("Preparing object write\n");
      lwm2m_object_write_opaque_stream(ctx, LEN, opaque_callback);
      break;
    default:
      return LWM2M_STATUS_NOT_FOUND;
    }
  } else if(ctx->operation == LWM2M_OP_WRITE) {
    if(coap_get_header_block1(ctx->request, &num, &more, &size, &offset)) {
      PRINTF("CoAP BLOCK1: %d/%d/%d offset:%d\n", num, more, size, offset);
      coap_set_header_block1(ctx->response, num, 0, size);
    }
  }
  return LWM2M_STATUS_OK;
}

void
ipso_blockwise_test_init(void)
{
  int i;
  PRINTF("Starting blockwise\n");
  reg_object.object_id = 4711;
  reg_object.instance_id = 0;
  reg_object.resource_ids = resources;
  reg_object.resource_count =
    sizeof(resources) / sizeof(lwm2m_resource_id_t);
  reg_object.callback = lwm2m_callback;

  for(i = 0; i < sizeof(junk); i++) {
    junk[i] = '0' + i;
  }
  junk[i - 1] = 0;

  lwm2m_engine_add_object(&reg_object);
}
/** @} */
