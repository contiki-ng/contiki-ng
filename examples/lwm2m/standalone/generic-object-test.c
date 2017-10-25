/*
 * Copyright (c) 2017, RISE SICS AB
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
 * \addtogroup lwm2m-objects
 * @{
 */

/**
 * \file
 *         Implementation of OMA LWM2M / Generic Object Example
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include <stdint.h>
#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "coap-engine.h"
#include <string.h>
#include <stdio.h>

static lwm2m_object_t generic_object;

#define MAX_SIZE 512
#define NUMBER_OF_INSTANCES 50

#define DEBUG 0

#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)

#endif

static const lwm2m_resource_id_t resources[] =
  {
    RO(10000),
    RO(11000),
  };

/*---------------------------------------------------------------------------*/
static int read_data(uint8_t *buffer, int instance_id, int start, int len)
{
  int i;
  int start_index;
  start_index = instance_id;
  /* Write len bytes into the buffer from the start-offset */
  for(i = 0; i < len; i++) {
    buffer[i] =  '0' + ((start_index + i) & 0x3f);
    if(i + start >= MAX_SIZE) return i;
  }
  return i;
}

static lwm2m_status_t
opaque_callback(lwm2m_object_instance_t *object,
                lwm2m_context_t *ctx, int num_to_write)
{
  int len;
  PRINTF("opaque-stream callback num_to_write: %d off: %d outlen: %d\n",
         num_to_write, ctx->offset, ctx->outbuf->len);

  len = read_data(&ctx->outbuf->buffer[ctx->outbuf->len],
                  ctx->object_instance_id,
                  ctx->offset, num_to_write);

  ctx->outbuf->len += len;

  /* Do we need to write more */
  if(ctx->offset + len < MAX_SIZE) {
    ctx->writer_flags |= WRITER_HAS_MORE;
  }
  return LWM2M_STATUS_OK;
}

/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object,
               lwm2m_context_t *ctx)
{

  if(ctx->level <= 2) {
    return LWM2M_STATUS_ERROR;
  }

  /* Only support for read at the moment */
  if(ctx->operation == LWM2M_OP_READ) {
    switch(ctx->resource_id) {
    case 10000:
      {
        char str[30];
        snprintf(str, 30, "hello-%d", (int)ctx->object_instance_id);
        lwm2m_object_write_string(ctx, str, strlen(str));
      }
      break;
    case 11000:
      PRINTF("Preparing object write\n");
      lwm2m_object_write_opaque_stream(ctx, MAX_SIZE, opaque_callback);
      break;
    default:
      return LWM2M_STATUS_NOT_FOUND;
    }
  }
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static void
setup_instance(lwm2m_object_instance_t *instance, uint16_t instance_id)
{
  instance->object_id = generic_object.impl->object_id;
  instance->instance_id = instance_id;
  instance->callback = lwm2m_callback;
  instance->resource_ids = resources;
  instance->resource_count = sizeof(resources)  / sizeof(lwm2m_resource_id_t);
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
get_by_id(uint16_t instance_id, lwm2m_status_t *status)
{
  lwm2m_object_instance_t *instance = NULL;
  if(status != NULL) {
    *status = LWM2M_STATUS_OK;
  }
  if(instance_id < NUMBER_OF_INSTANCES) {
    instance = lwm2m_engine_get_instance_buffer();
    if(instance == NULL) {
      return NULL;
    }

    /* We are fine - update instance variable */
    setup_instance(instance, instance_id);
  }
  return instance;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
get_first(lwm2m_status_t *status)
{
  return get_by_id(0, status);
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t *
get_next(lwm2m_object_instance_t *instance, lwm2m_status_t *status)
{
  return get_by_id(instance->instance_id + 1, status);
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_impl_t generic_object_impl = {
  .object_id = 4712,
  .get_first = get_first,
  .get_next = get_next,
  .get_by_id = get_by_id
};

/*---------------------------------------------------------------------------*/
void
lwm2m_generic_object_test_init(void)
{
  generic_object.impl = &generic_object_impl;

  lwm2m_engine_add_generic_object(&generic_object);
}
/*---------------------------------------------------------------------------*/
