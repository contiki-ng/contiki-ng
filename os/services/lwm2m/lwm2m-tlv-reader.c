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
 *         Implementation of the Contiki OMA LWM2M TLV reader
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "lwm2m-object.h"
#include "lwm2m-tlv-reader.h"
#include "lwm2m-tlv.h"
#include <string.h>

/*---------------------------------------------------------------------------*/
static size_t
read_int(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len,
         int32_t *value)
{
  lwm2m_tlv_t tlv;
  size_t size;
  size = lwm2m_tlv_read(&tlv, inbuf, len);
  if(size > 0) {
    *value = lwm2m_tlv_get_int32(&tlv);
    ctx->last_value_len = tlv.length;
  }
  return size;
}
/*---------------------------------------------------------------------------*/
static size_t
read_string(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len,
            uint8_t *value, size_t stringlen)
{
  lwm2m_tlv_t tlv;
  size_t size;
  size = lwm2m_tlv_read(&tlv, inbuf, len);
  if(size > 0) {
    if(stringlen <= tlv.length) {
      /* The outbuffer can not contain the full string including ending zero */
      return 0;
    }
    memcpy(value, tlv.value, tlv.length);
    value[tlv.length] = '\0';
    ctx->last_value_len = tlv.length;
  }
  return size;
}
/*---------------------------------------------------------------------------*/
static size_t
read_float32fix(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len,
                int32_t *value, int bits)
{
  lwm2m_tlv_t tlv;
  size_t size;
  size = lwm2m_tlv_read(&tlv, inbuf, len);
  if(size > 0) {
    lwm2m_tlv_float32_to_fix(&tlv, value, bits);
    ctx->last_value_len = tlv.length;
  }
  return size;
}
/*---------------------------------------------------------------------------*/
static size_t
read_boolean(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len,
             int *value)
{
  lwm2m_tlv_t tlv;
  size_t size;
  size = lwm2m_tlv_read(&tlv, inbuf, len);
  if(size > 0) {
    *value = lwm2m_tlv_get_int32(&tlv) != 0;
    ctx->last_value_len = tlv.length;
  }
  return size;
}
/*---------------------------------------------------------------------------*/
const lwm2m_reader_t lwm2m_tlv_reader = {
  read_int,
  read_string,
  read_float32fix,
  read_boolean
};
/*---------------------------------------------------------------------------*/
/** @} */
