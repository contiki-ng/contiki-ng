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
 */

/**
 * \file
 *         Implementation of OMA LWM2M / IPSO Generic Sensor
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "coap-engine.h"
#include "ipso-sensor-template.h"
#include <stdint.h>
#include <string.h>

static uint32_t temp = 19000;
static uint32_t hum = 30000;

static lwm2m_status_t get_temp_value(const ipso_sensor_t *sensor, int32_t *value);
static lwm2m_status_t get_hum_value(const ipso_sensor_t *sensor, int32_t *value);

IPSO_SENSOR(temp_sensor, 3303, get_temp_value,
            .max_range = 120000, /* milli celcius */
            .min_range = -30000, /* milli celcius */
            .unit = "Cel",
            .update_interval = 10
            );

IPSO_SENSOR(temp_sensor2, 3303, get_temp_value,
            .max_range = 120000, /* milli celcius */
            .min_range = -30000, /* milli celcius */
            .unit = "Cel",
            .update_interval = 10
            );

/*---------------------------------------------------------------------------*/

IPSO_SENSOR(hum_sensor, 3304, get_hum_value,
            .instance_id = 12,
            .max_range = 100000, /* milli  */
            .min_range = 0,      /* milli  */
            .unit = "%",
            .update_interval = 10
            );

/*---------------------------------------------------------------------------*/

static lwm2m_status_t
get_temp_value(const ipso_sensor_t *sensor, int32_t *value)
{
  *value = temp++;
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
get_hum_value(const ipso_sensor_t *sensor, int32_t *value)
{
  *value = hum++;
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
void
ipso_sensor_temp_init(void)
{
  ipso_sensor_add(&temp_sensor);
  ipso_sensor_add(&temp_sensor2);
  ipso_sensor_add(&hum_sensor);
}
/*---------------------------------------------------------------------------*/
/** @} */
