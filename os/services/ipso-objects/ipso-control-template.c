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
 *         Implementation of OMA LWM2M / IPSO control template.
 *         Useful for implementing controllable objects
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */
#include "ipso-control-template.h"
#include "lwm2m-engine.h"
#include "coap-timer.h"
#include <inttypes.h>
#include <string.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define IPSO_ONOFF        5850
#define IPSO_DIMMER       5851
#define IPSO_ON_TIME      5852

static const lwm2m_resource_id_t resources[] =
  {
    RW(IPSO_ONOFF),
    RW(IPSO_DIMMER),
    RW(IPSO_ON_TIME)
  };
/*---------------------------------------------------------------------------*/
lwm2m_status_t
ipso_control_set_on(ipso_control_t *control, uint8_t onoroff)
{
  uint8_t v;

  if(onoroff) {
    v = control->value & 0x7f;
    if(v == 0) {
      v = 100;
    }
  } else {
    v = 0;
  }

  return ipso_control_set_value(control, v);
}
/*---------------------------------------------------------------------------*/
lwm2m_status_t
ipso_control_set_value(ipso_control_t *control, uint8_t value)
{
  lwm2m_status_t status = LWM2M_STATUS_OK;
  int was_on;

  was_on = ipso_control_is_on(control);

  if(value == 0) {
    if(was_on) {
      /* Turn off */
      status = control->set_value(control, 0);
      if(status == LWM2M_STATUS_OK) {
        control->value &= 0x7f;
        control->on_time +=
          (coap_timer_uptime() - control->last_on_time) / 1000;
      }
    }
  } else {
    /* Restrict value between 0 - 100 */
    if(value > 100) {
      value = 100;
    }
    value |= 0x80;

    if(value != control->value) {
      status = control->set_value(control, value & 0x7f);
      if(status == LWM2M_STATUS_OK) {
        control->value = value;
        if(! was_on) {
          control->last_on_time = coap_timer_uptime();
        }
      }
    }
  }
  return status;
}
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object, lwm2m_context_t *ctx)
{
  ipso_control_t *control;
  int32_t v;

  /* Here we cast to our sensor-template struct */
  control = (ipso_control_t *)object;

  if(ctx->operation == LWM2M_OP_READ) {
    switch(ctx->resource_id) {
    case IPSO_ONOFF:
      v = ipso_control_is_on(control) ? 1 : 0;
      break;
    case IPSO_DIMMER:
      v = ipso_control_get_value(control);
      break;
    case IPSO_ON_TIME:
      v = control->on_time;
      if(ipso_control_is_on(control)) {
        v += (coap_timer_uptime() - control->last_on_time) / 1000;
      }
      PRINTF("ON-TIME: %"PRId32"   (last on: %"PRIu32"\n", v, control->on_time);
      break;
    default:
      return LWM2M_STATUS_ERROR;
    }
    lwm2m_object_write_int(ctx, v);
    return LWM2M_STATUS_OK;

  } else if(ctx->operation == LWM2M_OP_WRITE) {
    switch(ctx->resource_id) {
    case IPSO_ONOFF:
      if(lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size, &v) == 0) {
        return LWM2M_STATUS_ERROR;
      }
      return ipso_control_set_on(control, v > 0);
    case IPSO_DIMMER:
      if(lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size, &v) == 0) {
        return LWM2M_STATUS_ERROR;
      }
      if(v < 0) {
        v = 0;
      } else if(v > 100) {
        v = 100;
      }
      return ipso_control_set_value(control, v & 0xff);
    case IPSO_ON_TIME:
      if(lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size, &v) == 0) {
        return LWM2M_STATUS_ERROR;
      }

      if(v == 0) {
        control->on_time = 0;
        control->last_on_time = coap_timer_uptime();
        return LWM2M_STATUS_OK;
      } else {
        /* Only allowed to write 0 to reset ontime */
        return LWM2M_STATUS_FORBIDDEN;
      }
      break;
    default:
      return LWM2M_STATUS_ERROR;
    }
  } else {
    return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
  }
}
/*---------------------------------------------------------------------------*/
int
ipso_control_add(ipso_control_t *control)
{
  control->reg_object.resource_ids = resources;
  control->reg_object.resource_count =
    sizeof(resources) / sizeof(lwm2m_resource_id_t);

  control->reg_object.callback = lwm2m_callback;
  return lwm2m_engine_add_object(&control->reg_object);
}
/*---------------------------------------------------------------------------*/
int
ipso_control_remove(ipso_control_t *control)
{
  lwm2m_engine_remove_object(&control->reg_object);
  return 1;
}
/*---------------------------------------------------------------------------*/
/** @} */
