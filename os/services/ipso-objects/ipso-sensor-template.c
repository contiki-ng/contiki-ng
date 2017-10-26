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
 */

/**
 * \file
 *         Implementation of OMA LWM2M / IPSO sensor template.
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */
#include "ipso-sensor-template.h"
#include "lwm2m-engine.h"
#include <string.h>
#include <stdio.h>

#define IPSO_SENSOR_VALUE     5700
#define IPSO_SENSOR_UNIT      5701
#define IPSO_SENSOR_MIN_VALUE 5601
#define IPSO_SENSOR_MAX_VALUE 5602
#define IPSO_SENSOR_MIN_RANGE 5603
#define IPSO_SENSOR_MAX_RANGE 5604

#define IPSO_SENSOR_RESET_MINMAX 5605

static const lwm2m_resource_id_t resources[] =
  {
    RO(IPSO_SENSOR_VALUE), RO(IPSO_SENSOR_UNIT),
    RO(IPSO_SENSOR_MIN_VALUE), RO(IPSO_SENSOR_MAX_VALUE),
    RO(IPSO_SENSOR_MIN_RANGE), RO(IPSO_SENSOR_MAX_RANGE),
    EX(IPSO_SENSOR_RESET_MINMAX)
  };

/*---------------------------------------------------------------------------*/
static void update_last_value(ipso_sensor_value_t *sval, int32_t value,
                              uint8_t notify);
/*---------------------------------------------------------------------------*/
static int init = 0;
static coap_timer_t nt;

/* Currently support max 4 periodic sensors */
#define MAX_PERIODIC 4
struct periodic_sensor {
  ipso_sensor_value_t *value;
  uint16_t ticks_left;
} periodics[MAX_PERIODIC];

static void
timer_callback(coap_timer_t *timer)
{
  int i;
  coap_timer_reset(timer, 1000);

  for(i = 0; i < MAX_PERIODIC; i++) {
    if(periodics[i].value != NULL) {
      if(periodics[i].ticks_left > 0) {
        periodics[i].ticks_left--;
      } else {
        int32_t value;
        periodics[i].ticks_left = periodics[i].value->sensor->update_interval;
        if(periodics[i].value->sensor->get_value_in_millis(periodics[i].value->sensor, &value) == LWM2M_STATUS_OK) {
          update_last_value(periodics[i].value, value, 1);
        }
      }
    }
  }
}

static void
add_periodic(const ipso_sensor_t *sensor)
{
  int i;
  for(i = 0; i < MAX_PERIODIC; i++) {
    if(periodics[i].value == NULL) {
      periodics[i].value = sensor->sensor_value;
      periodics[i].ticks_left = sensor->update_interval;
      return;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
update_last_value(ipso_sensor_value_t *sval, int32_t value, uint8_t notify)
{
  /* No notification if this a regular read that cause the update */
  if(sval->last_value != value && notify) {
    lwm2m_notify_object_observers(&sval->reg_object, IPSO_SENSOR_VALUE);
  }
  sval->last_value = value;
  if(sval->min_value > value) {
    sval->min_value = value;
    lwm2m_notify_object_observers(&sval->reg_object, IPSO_SENSOR_MIN_VALUE);
  }
  if(sval->max_value < value) {
    sval->max_value = value;
    lwm2m_notify_object_observers(&sval->reg_object, IPSO_SENSOR_MAX_VALUE);
  }
}
/*---------------------------------------------------------------------------*/
static inline size_t
write_float32fix(lwm2m_context_t *ctx, int32_t value)
{
  int64_t tmp = value;
  tmp = (tmp * 1024) / 1000;
  return lwm2m_object_write_float32fix(ctx, (int32_t)tmp, 10);
}
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object,
               lwm2m_context_t *ctx)
{
  /* Here we cast to our sensor-template struct */
  const ipso_sensor_t *sensor;
  ipso_sensor_value_t *value;
  value = (ipso_sensor_value_t *) object;
  sensor = value->sensor;

  /* Do the stuff */
  if(ctx->level == 1) {
    /* Should not happen 3303 */
    return LWM2M_STATUS_ERROR;
  }
  if(ctx->level == 2) {
    /* This is a get whole object - or write whole object 3303/0 */
    return LWM2M_STATUS_ERROR;
  }
  if(ctx->level == 3) {
    /* This is a get request on 3303/0/3700 */
    /* NOW we assume a get.... which might be wrong... */
    if(ctx->operation == LWM2M_OP_READ) {
      switch(ctx->resource_id) {
      case IPSO_SENSOR_UNIT:
        if(sensor->unit != NULL) {
          lwm2m_object_write_string(ctx, sensor->unit, strlen(sensor->unit));
        }
        break;
      case IPSO_SENSOR_MAX_RANGE:
        write_float32fix(ctx, sensor->max_range);
        break;
      case IPSO_SENSOR_MIN_RANGE:
        write_float32fix(ctx, sensor->min_range);
        break;
      case IPSO_SENSOR_MAX_VALUE:
        write_float32fix(ctx, value->max_value);
        break;
      case IPSO_SENSOR_MIN_VALUE:
        write_float32fix(ctx, value->min_value);
        break;
      case IPSO_SENSOR_VALUE:
        if(sensor->get_value_in_millis != NULL) {
          int32_t v;
          if(sensor->get_value_in_millis(sensor, &v) == LWM2M_STATUS_OK) {
            update_last_value(value, v, 0);
            write_float32fix(ctx, value->last_value);
          }
        }
        break;
      default:
        return LWM2M_STATUS_ERROR;
      }
    } else if(ctx->operation == LWM2M_OP_EXECUTE) {
      if(ctx->resource_id == IPSO_SENSOR_RESET_MINMAX) {
        value->min_value = value->last_value;
        value->max_value = value->last_value;
      }
    }
  }
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
int
ipso_sensor_add(const ipso_sensor_t *sensor)
{
  if(sensor->update_interval > 0) {
    if(init == 0) {
      coap_timer_set_callback(&nt, timer_callback);
      coap_timer_set(&nt, 1000);
      init = 1;
    }
    add_periodic(sensor);
  }

  if(sensor->sensor_value == NULL) {
    return 0;
  }
  sensor->sensor_value->reg_object.object_id = sensor->object_id;
  sensor->sensor_value->sensor = sensor;
  if(sensor->instance_id == 0) {
    sensor->sensor_value->reg_object.instance_id = LWM2M_OBJECT_INSTANCE_NONE;
  } else {
    sensor->sensor_value->reg_object.instance_id = sensor->instance_id;
  }
  sensor->sensor_value->reg_object.callback = lwm2m_callback;
  sensor->sensor_value->reg_object.resource_ids = resources;
  sensor->sensor_value->reg_object.resource_count =
    sizeof(resources) / sizeof(lwm2m_resource_id_t);
  return lwm2m_engine_add_object(&sensor->sensor_value->reg_object);
}
/*---------------------------------------------------------------------------*/
int
ipso_sensor_remove(const ipso_sensor_t *sensor)
{
  lwm2m_engine_remove_object(&sensor->sensor_value->reg_object);
  return 1;
}
/*---------------------------------------------------------------------------*/
/** @} */
