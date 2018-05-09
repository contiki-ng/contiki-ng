/*
 * Copyright (c) 2015, Yanzi Networks AB.
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
 *         Implementation of the Contiki OMA LWM2M device
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "lwm2m-object.h"
#include "lwm2m-device.h"
#include "lwm2m-engine.h"
#include <string.h>

/* Log configuration */
#include "coap-log.h"
#define LOG_MODULE "lwm2m-dev"
#define LOG_LEVEL  LOG_LEVEL_LWM2M

static const lwm2m_resource_id_t resources[] =
  { RO(LWM2M_DEVICE_MANUFACTURER_ID),
    RO(LWM2M_DEVICE_MODEL_NUMBER_ID),
    RO(LWM2M_DEVICE_SERIAL_NUMBER_ID),
    RO(LWM2M_DEVICE_FIRMWARE_VERSION_ID),
    RO(LWM2M_DEVICE_AVAILABLE_POWER_SOURCES), /* Multi-resource-instance */
    RO(LWM2M_DEVICE_POWER_SOURCE_VOLTAGE), /* Multi-resource-instance */
    RO(LWM2M_DEVICE_POWER_SOURCE_CURRENT), /* Multi-resource-instance */
    RO(LWM2M_DEVICE_TYPE_ID),
    EX(LWM2M_DEVICE_REBOOT_ID),
    RW(LWM2M_DEVICE_TIME_ID),
    EX(LWM2M_DEVICE_FACTORY_DEFAULT_ID),
  };

#ifndef LWM2M_DEVICE_MANUFACTURER
#define LWM2M_DEVICE_MANUFACTURER     "RISE SICS"
#endif
#ifndef LWM2M_DEVICE_MODEL_NUMBER
#define LWM2M_DEVICE_MODEL_NUMBER     "1"
#endif
#ifndef LWM2M_DEVICE_SERIAL_NUMBER
#define LWM2M_DEVICE_SERIAL_NUMBER    "1"
#endif
#ifndef LWM2M_DEVICE_FIRMWARE_VERSION
#define LWM2M_DEVICE_FIRMWARE_VERSION CONTIKI_VERSION
#endif
#ifndef LWM2M_DEVICE_TYPE
#define LWM2M_DEVICE_TYPE "Contiki-NG LWM2M"
#endif

/* All three must be defined */
#ifndef LWM2M_DEVICE_POWER_AVAILABLE
#define LWM2M_DEVICE_POWER_AVAILABLE {1,5}
#define LWM2M_DEVICE_POWER_VOLTAGE {2500,5000}
#define LWM2M_DEVICE_POWER_CURRENT {500,1000}
#endif

static int32_t time_offset = 0;

/* Internal battery and USB - just for test...*/
static uint16_t power_avail[] = LWM2M_DEVICE_POWER_AVAILABLE;
static uint16_t power_voltage[] = LWM2M_DEVICE_POWER_VOLTAGE;
static uint16_t power_current[] = LWM2M_DEVICE_POWER_CURRENT;
/*---------------------------------------------------------------------------*/
int32_t
lwm2m_device_get_time(void)
{
  return coap_timer_seconds() + time_offset;
}
/*---------------------------------------------------------------------------*/
void
lwm2m_device_set_time(int32_t time)
{
  time_offset = time - coap_timer_seconds();
}
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
write_string(lwm2m_context_t *ctx, const char *text)
{
  lwm2m_object_write_string(ctx, text, strlen(text));
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static int
output_multi_i16(lwm2m_context_t *ctx, const uint16_t *data, int count)
{
  int i;
  size_t len;
  len = lwm2m_object_write_enter_ri(ctx);
  for(i = 0; i < count; i++) {
    len += lwm2m_object_write_int_ri(ctx, i, data[i]);
  }
  len += lwm2m_object_write_exit_ri(ctx);
  return len;
}

/*---------------------------------------------------------------------------*/
static int
lwm2m_dim_callback(lwm2m_object_instance_t *object, uint16_t resource_id)
{
  switch(resource_id) {
  case LWM2M_DEVICE_AVAILABLE_POWER_SOURCES:
  case LWM2M_DEVICE_POWER_SOURCE_VOLTAGE:
  case LWM2M_DEVICE_POWER_SOURCE_CURRENT:
    return sizeof(power_avail) / sizeof(uint16_t);
    break;
  }
  /* zero means that it is no dim parameter to send?? */
  return 0;
}
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object, lwm2m_context_t *ctx)
{
  if(ctx->operation == LWM2M_OP_READ) {
    switch(ctx->resource_id) {
    case LWM2M_DEVICE_MANUFACTURER_ID:
      return write_string(ctx, LWM2M_DEVICE_MANUFACTURER);
    case LWM2M_DEVICE_MODEL_NUMBER_ID:
      return write_string(ctx, LWM2M_DEVICE_MODEL_NUMBER);
    case LWM2M_DEVICE_SERIAL_NUMBER_ID:
      return write_string(ctx, LWM2M_DEVICE_SERIAL_NUMBER);
    case LWM2M_DEVICE_FIRMWARE_VERSION_ID:
      return write_string(ctx, LWM2M_DEVICE_FIRMWARE_VERSION);
    case LWM2M_DEVICE_TYPE_ID:
      return write_string(ctx, LWM2M_DEVICE_TYPE);
    case LWM2M_DEVICE_TIME_ID:
      LOG_DBG("Reading time: %u\n", (unsigned int)lwm2m_device_get_time());
      lwm2m_object_write_int(ctx, lwm2m_device_get_time());
      return LWM2M_STATUS_OK;
    case LWM2M_DEVICE_AVAILABLE_POWER_SOURCES:
      /* Power Multi-resource case - just use array index as ID */
      output_multi_i16(ctx, power_avail,
                       sizeof(power_avail)/sizeof(uint16_t));
      return LWM2M_STATUS_OK;
    case LWM2M_DEVICE_POWER_SOURCE_VOLTAGE:
      output_multi_i16(ctx, power_voltage,
                       sizeof(power_voltage)/sizeof(uint16_t));
      return LWM2M_STATUS_OK;
    case LWM2M_DEVICE_POWER_SOURCE_CURRENT:
      output_multi_i16(ctx, power_current,
                       sizeof(power_current)/sizeof(uint16_t));
      return LWM2M_STATUS_OK;
    default:
      LOG_WARN("Not found: %u\n", ctx->resource_id);
      return LWM2M_STATUS_NOT_FOUND;
    }

  } else if(ctx->operation == LWM2M_OP_EXECUTE) {
    if(ctx->resource_id == LWM2M_DEVICE_REBOOT_ID) {
      /* Do THE REBOOT */
      LOG_INFO("REBOOT\n");
      return LWM2M_STATUS_OK;
    }

  } else if(ctx->operation == LWM2M_OP_WRITE) {
    if(ctx->resource_id == LWM2M_DEVICE_TIME_ID) {
      int32_t lw_time;
      size_t len;
      len = lwm2m_object_read_int(ctx, ctx->inbuf->buffer, ctx->inbuf->size,
                                  &lw_time);
      if(len == 0) {
        LOG_WARN("FAIL: could not write time\n");
        return LWM2M_STATUS_WRITE_ERROR;
      } else {
        lwm2m_device_set_time(lw_time);
        LOG_DBG("Write time %lu sec => offset = %ld\n",
                (unsigned long)lw_time, (long)time_offset);
        return LWM2M_STATUS_OK;
      }
    }
  }

  return LWM2M_STATUS_OPERATION_NOT_ALLOWED;
}
/*---------------------------------------------------------------------------*/
static lwm2m_object_instance_t device = {
  .object_id = LWM2M_OBJECT_DEVICE_ID,
  .instance_id = 0,
  .resource_ids = resources,
  .resource_count = sizeof(resources) / sizeof(lwm2m_resource_id_t),
  .resource_dim_callback = lwm2m_dim_callback,
  .callback = lwm2m_callback,
};
/*---------------------------------------------------------------------------*/
void
lwm2m_device_init(void)
{
  lwm2m_engine_add_object(&device);
}
/*---------------------------------------------------------------------------*/
/** @} */
