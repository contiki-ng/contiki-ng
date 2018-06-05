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
 * \addtogroup apps
 * @{
 */

/**
 * \defgroup lwm2m An implementation of LWM2M
 * @{
 *
 * This is an implementation of OMA Lightweight M2M (LWM2M).
 */

/**
 * \file
 *         Header file for the LWM2M object API
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#ifndef LWM2M_OBJECT_H_
#define LWM2M_OBJECT_H_

#include "coap.h"
#include "coap-observe.h"

/* Operation permissions on the resources - read/write/execute */
#define LWM2M_RESOURCE_READ    0x10000
#define LWM2M_RESOURCE_WRITE   0x20000
#define LWM2M_RESOURCE_EXECUTE 0x40000
#define LWM2M_RESOURCE_OP_MASK 0x70000

/* The resource id type of lwm2m objects - 16 bits for the ID - the rest
   is flags */
typedef uint32_t lwm2m_resource_id_t;

/* Defines for the resource definition array */
#define RO(x) (x | LWM2M_RESOURCE_READ)
#define WO(x) (x | LWM2M_RESOURCE_WRITE)
#define RW(x) (x | LWM2M_RESOURCE_READ | LWM2M_RESOURCE_WRITE)
#define EX(x) (x | LWM2M_RESOURCE_EXECUTE)

#define LWM2M_OBJECT_SECURITY_ID                0
#define LWM2M_OBJECT_SERVER_ID                  1
#define LWM2M_OBJECT_ACCESS_CONTROL_ID          2
#define LWM2M_OBJECT_DEVICE_ID                  3
#define LWM2M_OBJECT_CONNECTIVITY_MONITORING_ID 4
#define LWM2M_OBJECT_FIRMWARE_ID                5
#define LWM2M_OBJECT_LOCATION_ID                6
#define LWM2M_OBJECT_CONNECTIVITY_STATISTICS_ID 7

typedef enum {
  LWM2M_OP_NONE,
  LWM2M_OP_READ,
  LWM2M_OP_DISCOVER,
  LWM2M_OP_WRITE,
  LWM2M_OP_WRITE_ATTR,
  LWM2M_OP_EXECUTE,
  LWM2M_OP_CREATE,
  LWM2M_OP_DELETE
} lwm2m_operation_t;

typedef enum {
  LWM2M_STATUS_OK,

  /* Internal server error */
  LWM2M_STATUS_ERROR,
  /* Error from writer */
  LWM2M_STATUS_WRITE_ERROR,
  /* Error from reader */
  LWM2M_STATUS_READ_ERROR,

  LWM2M_STATUS_BAD_REQUEST,
  LWM2M_STATUS_UNAUTHORIZED,
  LWM2M_STATUS_FORBIDDEN,
  LWM2M_STATUS_NOT_FOUND,
  LWM2M_STATUS_OPERATION_NOT_ALLOWED,
  LWM2M_STATUS_NOT_ACCEPTABLE,
  LWM2M_STATUS_UNSUPPORTED_CONTENT_FORMAT,

  LWM2M_STATUS_NOT_IMPLEMENTED,
  LWM2M_STATUS_SERVICE_UNAVAILABLE,
} lwm2m_status_t;

/* remember that we have already output a value - can be between two block's */
#define WRITER_OUTPUT_VALUE      1
#define WRITER_RESOURCE_INSTANCE 2
#define WRITER_HAS_MORE          4

typedef struct lwm2m_reader lwm2m_reader_t;
typedef struct lwm2m_writer lwm2m_writer_t;

typedef struct lwm2m_object_instance lwm2m_object_instance_t;

typedef struct {
  uint16_t len; /* used for current length of the data in the buffer */
  uint16_t pos; /* position in the buffer - typically write position or similar */
  uint16_t size;
  uint8_t *buffer;
} lwm2m_buffer_t;

/* Data model for OMA LWM2M objects */
typedef struct lwm2m_context {
  uint16_t object_id;
  uint16_t object_instance_id;
  uint16_t resource_id;
  uint16_t resource_instance_id;

  uint8_t resource_index;
  uint8_t resource_instance_index; /* for use when stepping to next sub-resource if having multiple */
  uint8_t level;  /* 0/1/2/3 = 3 = resource */
  lwm2m_operation_t operation;

  coap_message_t *request;
  coap_message_t *response;

  unsigned int content_type;
  lwm2m_buffer_t *outbuf;
  lwm2m_buffer_t *inbuf;

  uint8_t  out_mark_pos_oi; /* mark pos for last object instance   */
  uint8_t  out_mark_pos_ri; /* mark pos for last resource instance */

  uint32_t offset; /* If we do blockwise - this needs to change */

  /* Info on last_instance read/write */
  uint16_t last_instance;
  uint16_t last_value_len;

  uint8_t writer_flags; /* flags for reader/writer */
  const lwm2m_reader_t *reader;
  const lwm2m_writer_t *writer;
} lwm2m_context_t;

/* LWM2M format writer for the various formats supported */
struct lwm2m_writer {
  size_t (* init_write)(lwm2m_context_t *ctx);
  size_t (* end_write)(lwm2m_context_t *ctx);
  /* For sub-resources */
  size_t (* enter_resource_instance)(lwm2m_context_t *ctx);
  size_t (* exit_resource_instance)(lwm2m_context_t *ctx);
  size_t (* write_int)(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int32_t value);
  size_t (* write_string)(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen,  const char *value, size_t strlen);
  size_t (* write_float32fix)(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int32_t value, int bits);
  size_t (* write_boolean)(lwm2m_context_t *ctx, uint8_t *outbuf, size_t outlen, int value);
  size_t (* write_opaque_header)(lwm2m_context_t *ctx, size_t total_size);
};

struct lwm2m_reader {
  size_t (* read_int)(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value);
  size_t (* read_string)(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, uint8_t *value, size_t strlen);
  size_t (* read_float32fix)(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value, int bits);
  size_t (* read_boolean)(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int *value);
};

typedef lwm2m_status_t
(* lwm2m_write_opaque_callback)(lwm2m_object_instance_t *object,
                                lwm2m_context_t *ctx, int num_to_write);

void lwm2m_engine_set_opaque_callback(lwm2m_context_t *ctx, lwm2m_write_opaque_callback cb);

static inline void
lwm2m_notify_observers(char *path)
{
  coap_notify_observers_sub(NULL, path);
}

static inline size_t
lwm2m_object_read_int(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value)
{
  return ctx->reader->read_int(ctx, inbuf, len, value);
}

static inline size_t
lwm2m_object_read_string(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, uint8_t *value, size_t strlen)
{
  return ctx->reader->read_string(ctx, inbuf, len, value, strlen);
}

static inline size_t
lwm2m_object_read_float32fix(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int32_t *value, int bits)
{
  return ctx->reader->read_float32fix(ctx, inbuf, len, value, bits);
}

static inline size_t
lwm2m_object_read_boolean(lwm2m_context_t *ctx, const uint8_t *inbuf, size_t len, int *value)
{
  return ctx->reader->read_boolean(ctx, inbuf, len, value);
}

static inline size_t
lwm2m_object_write_int(lwm2m_context_t *ctx, int32_t value)
{
  size_t s;
  s = ctx->writer->write_int(ctx, &ctx->outbuf->buffer[ctx->outbuf->len],
                             ctx->outbuf->size - ctx->outbuf->len, value);
  ctx->outbuf->len += s;
  return s;
}

static inline size_t
lwm2m_object_write_string(lwm2m_context_t *ctx, const char *value, size_t strlen)
{
  size_t s;
  s = ctx->writer->write_string(ctx, &ctx->outbuf->buffer[ctx->outbuf->len],
                                ctx->outbuf->size - ctx->outbuf->len, value, strlen);
  ctx->outbuf->len += s;
  return s;
}

static inline size_t
lwm2m_object_write_float32fix(lwm2m_context_t *ctx, int32_t value, int bits)
{
  size_t s;
  s = ctx->writer->write_float32fix(ctx, &ctx->outbuf->buffer[ctx->outbuf->len],
                                    ctx->outbuf->size - ctx->outbuf->len, value, bits);
  ctx->outbuf->len += s;
  return s;
}

static inline size_t
lwm2m_object_write_boolean(lwm2m_context_t *ctx, int value)
{
  size_t s;
  s = ctx->writer->write_boolean(ctx, &ctx->outbuf->buffer[ctx->outbuf->len],
                                 ctx->outbuf->size - ctx->outbuf->len, value);
  ctx->outbuf->len += s;
  return s;
}

static inline int
lwm2m_object_write_opaque_stream(lwm2m_context_t *ctx, int size, lwm2m_write_opaque_callback cb)
{
  /* 1. - create a header of either OPAQUE (nothing) or TLV if the format is TLV */
  size_t s;
  if(ctx->writer->write_opaque_header != NULL) {
    s = ctx->writer->write_opaque_header(ctx, size);
    ctx->outbuf->len += s;
  } else {
    return 0;
  }
  /* 2. - set the callback so that future data will be grabbed from the callback */
  lwm2m_engine_set_opaque_callback(ctx, cb);
  return 1;
}

/* Resource instance functions (_ri)*/

static inline size_t
lwm2m_object_write_enter_ri(lwm2m_context_t *ctx)
{
  if(ctx->writer->enter_resource_instance != NULL) {
    size_t s;
    s = ctx->writer->enter_resource_instance(ctx);
    ctx->outbuf->len += s;
    return s;
  }
  return 0;
}

static inline size_t
lwm2m_object_write_exit_ri(lwm2m_context_t *ctx)
{
  if(ctx->writer->exit_resource_instance != NULL) {
    size_t s;
    s = ctx->writer->exit_resource_instance(ctx);
    ctx->outbuf->len += s;
    return s;
  }
  return 0;
}

static inline size_t
lwm2m_object_write_int_ri(lwm2m_context_t *ctx, uint16_t id, int32_t value)
{
  size_t s;
  ctx->resource_instance_id = id;
  s = ctx->writer->write_int(ctx, &ctx->outbuf->buffer[ctx->outbuf->len],
                             ctx->outbuf->size - ctx->outbuf->len, value);
  ctx->outbuf->len += s;
  return s;
}

static inline size_t
lwm2m_object_write_string_ri(lwm2m_context_t *ctx, uint16_t id, const char *value, size_t strlen)
{
  size_t s;
  ctx->resource_instance_id = id;
  s = ctx->writer->write_string(ctx, &ctx->outbuf->buffer[ctx->outbuf->len],
                                ctx->outbuf->size - ctx->outbuf->len, value, strlen);
  ctx->outbuf->len += s;
  return s;
}

static inline size_t
lwm2m_object_write_float32fix_ri(lwm2m_context_t *ctx, uint16_t id, int32_t value, int bits)
{
  size_t s;
  ctx->resource_instance_id = id;
  s = ctx->writer->write_float32fix(ctx, &ctx->outbuf->buffer[ctx->outbuf->len],
                                    ctx->outbuf->size - ctx->outbuf->len, value, bits);
  ctx->outbuf->len += s;
  return s;
}

static inline size_t
lwm2m_object_write_boolean_ri(lwm2m_context_t *ctx, uint16_t id, int value)
{
  size_t s;
  ctx->resource_instance_id = id;
  s = ctx->writer->write_boolean(ctx, &ctx->outbuf->buffer[ctx->outbuf->len],
                                 ctx->outbuf->size - ctx->outbuf->len, value);
  ctx->outbuf->len += s;
  return s;
}

static inline int
lwm2m_object_is_final_incoming(lwm2m_context_t *ctx)
{
  uint8_t more;
  if(coap_get_header_block1(ctx->request, NULL, &more, NULL, NULL)) {
    return !more;
  }
  /* If we do not know this is final... it might not be... */
  return 0;
}

#include "lwm2m-engine.h"

#endif /* LWM2M_OBJECT_H_ */
/**
 * @}
 * @}
 */
