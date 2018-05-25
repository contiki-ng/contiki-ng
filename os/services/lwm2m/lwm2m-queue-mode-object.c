/*
 * Copyright (c) 2017, RISE SICS AB.
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
 */

/**
 * \file
 *         Implementation of the Contiki OMA LWM2M Queue Mode object
 		   to manage the parameters from the server side
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#include "lwm2m-object.h"
#include "lwm2m-queue-mode.h"

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-qmode-object"
#define LOG_LEVEL  LOG_LEVEL_LWM2M

#if LWM2M_QUEUE_MODE_ENABLED && LWM2M_QUEUE_MODE_OBJECT_ENABLED

#define LWM2M_QUEUE_MODE_OBJECT_ID 30000
#define LWM2M_AWAKE_TIME_ID 30000
#define LWM2M_SLEEP_TIME_ID 30001

#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
#define LWM2M_DYNAMIC_ADAPTATION_FLAG_ID 30002
#define UPDATE_WITH_MEAN 0 /* 1-mean time 0-maximum time */
#endif

static const lwm2m_resource_id_t resources[] =
{ RW(LWM2M_AWAKE_TIME_ID),
  RW(LWM2M_SLEEP_TIME_ID),
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
  RW(LWM2M_DYNAMIC_ADAPTATION_FLAG_ID),
#endif
};

/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object, lwm2m_context_t *ctx)
{
  if(ctx->operation == LWM2M_OP_READ) {
    switch(ctx->resource_id) {
    case LWM2M_AWAKE_TIME_ID:
      lwm2m_object_write_int(ctx, (int32_t)lwm2m_queue_mode_get_awake_time());
      return LWM2M_STATUS_OK;
    case LWM2M_SLEEP_TIME_ID:
      lwm2m_object_write_int(ctx, (int32_t)lwm2m_queue_mode_get_sleep_time());
      return LWM2M_STATUS_OK;
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
    case LWM2M_DYNAMIC_ADAPTATION_FLAG_ID:
      lwm2m_object_write_int(ctx, (int32_t)lwm2m_queue_mode_get_dynamic_adaptation_flag());
      return LWM2M_STATUS_OK;
#endif
    }
  } else if(ctx->operation == LWM2M_OP_WRITE) {
    switch(ctx->resource_id) {
      int32_t value_read;
      size_t len;

    case LWM2M_AWAKE_TIME_ID:

      len = lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size,
                                  &value_read);
      LOG_DBG("Client Awake Time write request value: %d\n", (int)value_read);
      if(len == 0) {
        LOG_WARN("FAIL: could not write awake time\n");
        return LWM2M_STATUS_WRITE_ERROR;
      } else {
        lwm2m_queue_mode_set_awake_time(value_read);
        return LWM2M_STATUS_OK;
      }

    case LWM2M_SLEEP_TIME_ID:
      len = lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size,
                                  &value_read);
      LOG_DBG("Client Sleep Time write request value: %d\n", (int)value_read);
      if(len == 0) {
        LOG_WARN("FAIL: could not write sleep time\n");
        return LWM2M_STATUS_WRITE_ERROR;
      } else {
        lwm2m_queue_mode_set_sleep_time(value_read);
        return LWM2M_STATUS_OK;
      }
#if LWM2M_QUEUE_MODE_INCLUDE_DYNAMIC_ADAPTATION
    case LWM2M_DYNAMIC_ADAPTATION_FLAG_ID:
      len = lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size,
                                  &value_read);
      LOG_DBG("Dynamic Adaptation Flag request value: %d\n", (int)value_read);
      if(len == 0) {
        LOG_WARN("FAIL: could not write dynamic flag\n");
        return LWM2M_STATUS_WRITE_ERROR;
      } else {
        lwm2m_queue_mode_set_dynamic_adaptation_flag(value_read);
        return LWM2M_STATUS_OK;
      }
#endif
    }
  }

  return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t queue_object = {
  .object_id = LWM2M_QUEUE_MODE_OBJECT_ID,
  .instance_id = 0,
  .resource_ids = resources,
  .resource_count = sizeof(resources) / sizeof(lwm2m_resource_id_t),
  .resource_dim_callback = NULL,
  .callback = lwm2m_callback,
};
/*---------------------------------------------------------------------------*/
void
lwm2m_queue_mode_object_init(void)
{
  lwm2m_engine_add_object(&queue_object);
}
#endif /* LWM2M_QUEUE_MODE_ENABLED && LWM2M_QUEUE_MODE_OBJECT_ENABLED */
/** @} */