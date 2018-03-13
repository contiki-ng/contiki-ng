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
 * \addtogroup ipso-objects
 * @{
 */

/**
 * \file
 *         Implementation of OMA LWM2M / IPSO button as a digital input
 *         NOTE: only works with a Contiki Button Sensor (not for Standalone)
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "contiki.h"
#include "lwm2m-object.h"
#include "lwm2m-engine.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#define IPSO_INPUT_STATE     5500
#define IPSO_INPUT_COUNTER   5501
#define IPSO_INPUT_DEBOUNCE  5503
#define IPSO_INPUT_EDGE_SEL  5504
#define IPSO_INPUT_CTR_RESET 5505
#define IPSO_INPUT_SENSOR_TYPE 5751

#if PLATFORM_HAS_BUTTON
#if PLATFORM_SUPPORTS_BUTTON_HAL
#include "dev/button-hal.h"
#else
#include "dev/button-sensor.h"
#define IPSO_BUTTON_SENSOR button_sensor
static struct etimer timer;
#endif

PROCESS(ipso_button_process, "ipso-button");
#endif /* PLATFORM_HAS_BUTTON */


static lwm2m_status_t lwm2m_callback(lwm2m_object_instance_t *object,
                                     lwm2m_context_t *ctx);

static int input_state = 0;
static int32_t counter = 0;
static int32_t edge_selection = 3; /* both */
static int32_t debounce_time = 10;

static const lwm2m_resource_id_t resources[] = {
  RO(IPSO_INPUT_STATE), RO(IPSO_INPUT_COUNTER),
  RW(IPSO_INPUT_DEBOUNCE), RW(IPSO_INPUT_EDGE_SEL), EX(IPSO_INPUT_CTR_RESET),
  RO(IPSO_INPUT_SENSOR_TYPE)
};

/* Only support for one button for now */
static lwm2m_object_instance_t reg_object = {
  .object_id = 3200,
  .instance_id = 0,
  .resource_ids = resources,
  .resource_count = sizeof(resources) / sizeof(lwm2m_resource_id_t),
  .callback = lwm2m_callback,
};

/*---------------------------------------------------------------------------*/
static int
read_state(void)
{
  PRINTF("Read button state: %d\n", input_state);
  return input_state;
}
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
lwm2m_callback(lwm2m_object_instance_t *object,
               lwm2m_context_t *ctx)
{
  if(ctx->operation == LWM2M_OP_READ) {
    switch(ctx->resource_id) {
    case IPSO_INPUT_STATE:
      lwm2m_object_write_int(ctx, read_state());
      break;
    case IPSO_INPUT_COUNTER:
      lwm2m_object_write_int(ctx, counter);
      break;
    case IPSO_INPUT_DEBOUNCE:
      lwm2m_object_write_int(ctx, debounce_time);
      break;
    case IPSO_INPUT_EDGE_SEL:
      lwm2m_object_write_int(ctx, edge_selection);
      break;
    case IPSO_INPUT_SENSOR_TYPE:
      lwm2m_object_write_string(ctx, "button", strlen("button"));
      break;
    default:
      return LWM2M_STATUS_ERROR;
    }
  } else if(ctx->operation == LWM2M_OP_EXECUTE) {
    if(ctx->resource_id == IPSO_INPUT_CTR_RESET) {
      counter = 0;
    } else {
      return LWM2M_STATUS_ERROR;
    }
  }
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
void
ipso_button_init(void)
{
  /* register this device and its handlers - the handlers automatically
     sends in the object to handle */
  lwm2m_engine_add_object(&reg_object);

#if PLATFORM_HAS_BUTTON
  process_start(&ipso_button_process, NULL);
#endif /* PLATFORM_HAS_BUTTON */
}
/*---------------------------------------------------------------------------*/
#if PLATFORM_HAS_BUTTON
PROCESS_THREAD(ipso_button_process, ev, data)
{
  PROCESS_BEGIN();

#if !PLATFORM_SUPPORTS_BUTTON_HAL
  SENSORS_ACTIVATE(IPSO_BUTTON_SENSOR);
#endif

  while(1) {
    PROCESS_WAIT_EVENT();

#if PLATFORM_SUPPORTS_BUTTON_HAL
    if(ev == button_hal_press_event) {
      input_state = 1;
      counter++;
      if((edge_selection & 2) != 0) {
        lwm2m_notify_object_observers(&reg_object, IPSO_INPUT_STATE);
      }
      lwm2m_notify_object_observers(&reg_object, IPSO_INPUT_COUNTER);
    } else if(ev == button_hal_release_event) {
      input_state = 0;
      if((edge_selection & 1) != 0) {
        lwm2m_notify_object_observers(&reg_object, IPSO_INPUT_STATE);
      }
    }
#else /* PLATFORM_SUPPORTS_BUTTON_HAL */
    if(ev == sensors_event && data == &IPSO_BUTTON_SENSOR) {
      if(!input_state) {
        int32_t time;
        input_state = 1;
        counter++;
        if((edge_selection & 2) != 0) {
          lwm2m_notify_object_observers(&reg_object, IPSO_INPUT_STATE);
        }
        lwm2m_notify_object_observers(&reg_object, IPSO_INPUT_COUNTER);

        time = (debounce_time * CLOCK_SECOND / 1000);
        if(time < 1) {
          time = 1;
        }
        etimer_set(&timer, (clock_time_t)time);
      }
    } else if(ev == PROCESS_EVENT_TIMER && data == &timer) {
      if(!input_state) {
        /* Button is not in pressed state */
      } else if(IPSO_BUTTON_SENSOR.value(0) != 0) {
        /* Button is still pressed */
        etimer_reset(&timer);
      } else {
        input_state = 0;
        if((edge_selection & 1) != 0) {
          lwm2m_notify_object_observers(&reg_object, IPSO_INPUT_STATE);
        }
      }
    }
#endif /* PLATFORM_SUPPORTS_BUTTON_HAL */
  }

  PROCESS_END();
}
#endif /* PLATFORM_HAS_BUTTON */
/*---------------------------------------------------------------------------*/
/** @} */
