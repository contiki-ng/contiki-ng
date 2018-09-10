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
 *         Implementation of OMA LWM2M / IPSO Temperature
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include <stdint.h>
#include "ipso-sensor-template.h"
#include "ipso-objects.h"
#include "lwm2m-object.h"
#include "lwm2m-engine.h"

#ifdef IPSO_TEMPERATURE
extern const struct ipso_objects_sensor IPSO_TEMPERATURE;
#endif /* IPSO_TEMPERATURE */

#ifndef IPSO_TEMPERATURE_MIN
#define IPSO_TEMPERATURE_MIN -50000
#endif

#ifndef IPSO_TEMPERATURE_MAX
#define IPSO_TEMPERATURE_MAX 80000
#endif

static lwm2m_status_t get_temp_value(const ipso_sensor_t *sensor,
                                     int32_t *value);

IPSO_SENSOR(temp_sensor, 3303, get_temp_value,
            .max_range = IPSO_TEMPERATURE_MAX, /* milli celcius */
            .min_range = IPSO_TEMPERATURE_MIN, /* milli celcius */
            .unit = "Cel",
            .update_interval = 10
            );

/*---------------------------------------------------------------------------*/
static lwm2m_status_t
get_temp_value(const ipso_sensor_t *s, int32_t *value)
{
#ifdef IPSO_TEMPERATURE
  if(IPSO_TEMPERATURE.read_value != NULL &&
     IPSO_TEMPERATURE.read_value(value) == 0) {
    return LWM2M_STATUS_OK;
  }
#endif /* IPSO_TEMPERATURE */
  return LWM2M_STATUS_ERROR;
}
/*---------------------------------------------------------------------------*/
void
ipso_temperature_init(void)
{
#ifdef IPSO_TEMPERATURE
  if(IPSO_TEMPERATURE.init) {
    IPSO_TEMPERATURE.init();
  }
#endif /* IPSO_TEMPERATURE */

  ipso_sensor_add(&temp_sensor);
}
/*---------------------------------------------------------------------------*/
/** @} */
