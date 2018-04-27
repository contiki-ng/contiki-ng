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
 *         Implementation of the Contiki OMA LWM2M Queue Object for managing the queue mode
 * \author
 *         Carlos Gonzalo Peces <carlosgp143@gmail.com>
 */

#include "lwm2m-qmode-object.h"

#if LWM2M_Q_MODE_ENABLED

#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "lwm2m-rd-client.h"
#include "lib/memb.h"
#include "lib/list.h"
#include <string.h>

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-qmode-object"
#define LOG_LEVEL  LOG_LEVEL_LWM2M

#define LWM2M_Q_OBJECT_ID 6000
#define LWM2M_AWAKE_TIME_ID 3000
#define LWM2M_SLEEP_TIME_ID 3001

#if LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION
#define LWM2M_DYNAMIC_ADAPTATION_FLAG_ID 3002
#define UPDATE_WITH_MEAN 0 /* 1-mean time 0-maximum time */
#endif

static const lwm2m_resource_id_t resources[] =
{ RW(LWM2M_AWAKE_TIME_ID),
  RW(LWM2M_SLEEP_TIME_ID),
#if LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION
  RW(LWM2M_DYNAMIC_ADAPTATION_FLAG_ID),
#endif
};

static uint16_t q_mode_awake_time = LWM2M_Q_MODE_DEFAULT_CLIENT_AWAKE_TIME;
static uint32_t q_mode_sleep_time = LWM2M_Q_MODE_DEFAULT_CLIENT_SLEEP_TIME;
#if LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION
static uint8_t q_mode_dynamic_adaptation_flag = LWM2M_Q_MODE_DEFAULT_DYNAMIC_ADAPTATION_FLAG;

/* Window to save the times and do the dynamic adaptation of the awake time*/
uint16_t times_window[LWM2M_Q_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH] = { 0 };
uint8_t times_window_index = 0;

#endif
/*---------------------------------------------------------------------------*/
uint16_t
lwm2m_q_object_get_awake_time()
{
  LOG_DBG("Client Awake Time: %d ms\n", (int)q_mode_awake_time);
  return q_mode_awake_time;
}
/*---------------------------------------------------------------------------*/
static void
lwm2m_q_object_set_awake_time(uint16_t time)
{
  q_mode_awake_time = time;
}
/*---------------------------------------------------------------------------*/
uint32_t
lwm2m_q_object_get_sleep_time()
{
  LOG_DBG("Client Sleep Time: %d ms\n", (int)q_mode_sleep_time);
  return q_mode_sleep_time;
}
/*---------------------------------------------------------------------------*/
static void
lwm2m_q_object_set_sleep_time(uint32_t time)
{
  q_mode_sleep_time = time;
}
/*---------------------------------------------------------------------------*/
#if LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION
uint8_t
lwm2m_q_object_get_dynamic_adaptation_flag()
{
  LOG_DBG("Dynamic Adaptation Flag: %d ms\n", (int)q_mode_dynamic_adaptation_flag);
  return q_mode_dynamic_adaptation_flag;
}
/*---------------------------------------------------------------------------*/
static void
lwm2m_q_object_set_dynamic_adaptation_flag(uint8_t flag)
{
  q_mode_dynamic_adaptation_flag = flag;
}
/*---------------------------------------------------------------------------*/
#if !UPDATE_WITH_MEAN
static uint16_t
get_maximum_time()
{
  uint16_t max_time = 0;
  uint8_t i;
  for(i = 0; i < LWM2M_Q_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH; i++) {
    if(times_window[i] > max_time) {
      max_time = times_window[i];
    }
  }
  return max_time;
}
#endif
/*---------------------------------------------------------------------------*/
#if UPDATE_WITH_MEAN
static uint16_t
get_mean_time()
{
  uint16_t mean_time = 0;
  uint8_t i;
  for(i = 0; i < LWM2M_Q_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH; i++) {
    if(mean_time == 0) {
      mean_time = times_window[i];
    } else {
      if(times_window[i] != 0) {
        mean_time = (mean_time + times_window[i]) / 2;
      }
    }
  }
  return mean_time;
}
#endif
/*---------------------------------------------------------------------------*/
static void
update_awake_time()
{
#if UPDATE_WITH_MEAN
  uint16_t mean_time = get_mean_time();
  LOG_DBG("Dynamic Adaptation: updated awake time: %d ms\n", (int)mean_time);
  lwm2m_q_object_set_awake_time(mean_time + (mean_time >> 1)); /* 50% margin */
  return;
#else
  uint16_t max_time = get_maximum_time();
  LOG_DBG("Dynamic Adaptation: updated awake time: %d ms\n", (int)max_time);
  lwm2m_q_object_set_awake_time(max_time + (max_time >> 1)); /* 50% margin */
  return;
#endif
}
/*---------------------------------------------------------------------------*/
void
lwm2m_q_object_add_time_to_window(uint16_t time)
{
  if(times_window_index == LWM2M_Q_MODE_DYNAMIC_ADAPTATION_WINDOW_LENGTH) {
    times_window_index = 0;
  }
  times_window[times_window_index] = time;
  times_window_index++;
  update_awake_time();
}
#endif /* LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION */
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object, lwm2m_context_t *ctx)
{
  if(ctx->operation == LWM2M_OP_READ) {
    switch(ctx->resource_id) {
    case LWM2M_AWAKE_TIME_ID:
      lwm2m_object_write_int(ctx, (int32_t)q_mode_awake_time);
      return LWM2M_STATUS_OK;
    case LWM2M_SLEEP_TIME_ID:
      lwm2m_object_write_int(ctx, (int32_t)q_mode_sleep_time);
      return LWM2M_STATUS_OK;
#if LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION
    case LWM2M_DYNAMIC_ADAPTATION_FLAG_ID:
      lwm2m_object_write_int(ctx, (int32_t)q_mode_dynamic_adaptation_flag);
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
        lwm2m_q_object_set_awake_time(value_read);
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
        lwm2m_q_object_set_sleep_time(value_read);
        return LWM2M_STATUS_OK;
      }
#if LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION
    case LWM2M_DYNAMIC_ADAPTATION_FLAG_ID:
      len = lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size,
                                  &value_read);
      LOG_DBG("Dynamic Adaptation Flag request value: %d\n", (int)value_read);
      if(len == 0) {
        LOG_WARN("FAIL: could not write dynamic flag\n");
        return LWM2M_STATUS_WRITE_ERROR;
      } else {
        lwm2m_q_object_set_dynamic_adaptation_flag(value_read);
        return LWM2M_STATUS_OK;
      }
#endif
    }
  }

  return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t queue_object = {
  .object_id = LWM2M_Q_OBJECT_ID,
  .instance_id = 0,
  .resource_ids = resources,
  .resource_count = sizeof(resources) / sizeof(lwm2m_resource_id_t),
  .resource_dim_callback = NULL,
  .callback = lwm2m_callback,
};
/*---------------------------------------------------------------------------*/
void
lwm2m_q_object_init(void)
{
  lwm2m_engine_add_object(&queue_object);
}
/*---------------------------------------------------------------------------*/
void
lwm2m_q_object_send_notifications()
{
  lwm2m_notify_object_observers(&queue_object, LWM2M_AWAKE_TIME_ID);
  lwm2m_notify_object_observers(&queue_object, LWM2M_SLEEP_TIME_ID);
#if LWM2M_Q_MODE_INCLUDE_DYNAMIC_ADAPTATION
  lwm2m_notify_object_observers(&queue_object, LWM2M_DYNAMIC_ADAPTATION_FLAG_ID);
#endif
}
/*---------------------------------------------------------------------------*/
#endif /* LWM2M_Q_MODE_ENABLED */
/** @} */

