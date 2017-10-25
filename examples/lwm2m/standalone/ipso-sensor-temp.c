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

#include <stdint.h>
#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "coap-engine.h"
#include "ipso-sensor-template.h"
#include <string.h>

uint32_t temp = 19000;
uint32_t hum = 30000;

ipso_sensor_value_t temp_value;
ipso_sensor_value_t temp_value2;
ipso_sensor_value_t hum_value;

lwm2m_status_t get_temp_value(const ipso_sensor_t *sensor, int32_t *value);
lwm2m_status_t get_hum_value(const ipso_sensor_t *sensor, int32_t *value);

static const ipso_sensor_t temp_sensor = {
  .object_id = 3303,
  .sensor_value = &temp_value,
  .max_range = 120000, /* milli celcius */
  .min_range = -30000, /* milli celcius */
  .get_value_in_millis = get_temp_value,
  .unit = "Cel",
  .update_interval = 10
};

static const ipso_sensor_t temp_sensor2 = {
  .object_id = 3303,
  .sensor_value = &temp_value2,
  .max_range = 120000, /* milli celcius */
  .min_range = -30000, /* milli celcius */
  .get_value_in_millis = get_temp_value,
  .unit = "Cel",
  .update_interval = 10
};

/*---------------------------------------------------------------------------*/

static const ipso_sensor_t hum_sensor = {
  .object_id = 3304,
  .instance_id = 12,
  .sensor_value = &hum_value,
  .max_range = 100000, /* milli  */
  .min_range = 0, /* milli  */
  .get_value_in_millis = get_hum_value,
  .unit = "%",
  .update_interval = 10
};

/*---------------------------------------------------------------------------*/

lwm2m_status_t
get_temp_value(const ipso_sensor_t *sensor, int32_t *value)
{
  *value = temp++;
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
lwm2m_status_t
get_hum_value(const ipso_sensor_t *sensor, int32_t *value)
{
  *value = temp++;
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
