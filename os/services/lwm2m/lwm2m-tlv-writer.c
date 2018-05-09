/*
 * Copyright (c) 2015-2018, Yanzi Networks AB.
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
 * \addtogroup lwm2m
 * @{
 *
 */

/**
 * \file
 *         Implementation of the Contiki OMA LWM2M TLV writer
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "lwm2m-object.h"
#include "lwm2m-tlv.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-tlv"
#define LOG_LEVEL  LOG_LEVEL_NONE

/*---------------------------------------------------------------------------*/
static size_t
init_write(lwm2m_context_t *ctx)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static size_t
end_write(lwm2m_context_t *ctx)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static size_t
write_int_tlv(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen,
              int32_t value)
{
  uint8_t type = ctx->writer_flags & WRITER_RESOURCE_INSTANCE ?
    LWM2M_TLV_TYPE_RESOURCE_INSTANCE : LWM2M_TLV_TYPE_RESOURCE;
  uint16_t id = ctx->writer_flags & WRITER_RESOURCE_INSTANCE ?
    ctx->resource_instance_id : ctx->resource_id;
  return lwm2m_tlv_write_int32(type, id, value, outbuf, outlen);
}
/*---------------------------------------------------------------------------*/
static size_t
write_boolean_tlv(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen,
                  int value)
{
  return write_int_tlv(ctx, outbuf, outlen, value != 0 ? 1 : 0);
}
/*---------------------------------------------------------------------------*/
static size_t
write_float32fix_tlv(lwm2m_context_t *ctx, uint8_t *outbuf,
                     size_t outlen, int32_t value, int bits)
{
  uint8_t type = ctx->writer_flags & WRITER_RESOURCE_INSTANCE ?
    LWM2M_TLV_TYPE_RESOURCE_INSTANCE : LWM2M_TLV_TYPE_RESOURCE;
  uint16_t id = ctx->writer_flags & WRITER_RESOURCE_INSTANCE ?
    ctx->resource_instance_id : ctx->resource_id;
  return lwm2m_tlv_write_float32(type, id, value, bits, outbuf, outlen);
}
/*---------------------------------------------------------------------------*/
static size_t
write_string_tlv(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen,
                 const char *value, size_t stringlen)
{
  lwm2m_tlv_t tlv;
  tlv.type = ctx->writer_flags & WRITER_RESOURCE_INSTANCE ?
    LWM2M_TLV_TYPE_RESOURCE_INSTANCE : LWM2M_TLV_TYPE_RESOURCE;
  tlv.value = (uint8_t *) value;
  tlv.length = (uint32_t) stringlen;
  tlv.id = ctx->resource_id;
  return lwm2m_tlv_write(&tlv, outbuf, outlen);
}
/*---------------------------------------------------------------------------*/
static size_t
write_opaque_header(lwm2m_context_t *ctx, size_t payloadsize)
{
  lwm2m_tlv_t tlv;
  tlv.type = LWM2M_TLV_TYPE_RESOURCE;
  tlv.value = (uint8_t *) NULL;
  tlv.length = (uint32_t) payloadsize;
  tlv.id = ctx->resource_id;
  return lwm2m_tlv_write(&tlv, &ctx->outbuf->buffer[ctx->outbuf->len],
                         ctx->outbuf->size - ctx->outbuf->len);
}
/*---------------------------------------------------------------------------*/
static size_t
enter_sub(lwm2m_context_t *ctx)
{
  /* set some flags in state */
  lwm2m_tlv_t tlv;
  int len = 0;
  LOG_DBG("Enter sub-resource rsc=%d mark:%d\n", ctx->resource_id, ctx->outbuf->len);
  ctx->writer_flags |= WRITER_RESOURCE_INSTANCE;
  tlv.type = LWM2M_TLV_TYPE_MULTI_RESOURCE;
  tlv.length = 8; /* create an 8-bit TLV */
  tlv.value = NULL;
  tlv.id = ctx->resource_id;
  len = lwm2m_tlv_write(&tlv, &ctx->outbuf->buffer[ctx->outbuf->len],
                      ctx->outbuf->size - ctx->outbuf->len);
  /* store position for deciding where to re-write the TLV when we
     know the length - NOTE: either this or memmov of buffer later... */
  ctx->out_mark_pos_ri = ctx->outbuf->len;
  return len;
}
/*---------------------------------------------------------------------------*/
static size_t
exit_sub(lwm2m_context_t *ctx)
{
  /* clear out state info */
  int pos = 2; /* this is the lenght pos */
  int len;
  ctx->writer_flags &= ~WRITER_RESOURCE_INSTANCE;

  if(ctx->resource_id > 0xff) {
    pos++;
  }
  len = ctx->outbuf->len - ctx->out_mark_pos_ri;

  LOG_DBG("Exit sub-resource rsc=%d mark:%d len=%d\n", ctx->resource_id,
          ctx->out_mark_pos_ri, len);

  /* update the lenght byte... Assume TLV header is pos + 1 bytes. */
  ctx->outbuf->buffer[pos + ctx->out_mark_pos_ri] = len - (pos + 1);
  return 0;
}
/*---------------------------------------------------------------------------*/
const lwm2m_writer_t lwm2m_tlv_writer = {
  init_write,
  end_write,
  enter_sub,
  exit_sub,
  write_int_tlv,
  write_string_tlv,
  write_float32fix_tlv,
  write_boolean_tlv,
  write_opaque_header
};
/*---------------------------------------------------------------------------*/
/** @} */
