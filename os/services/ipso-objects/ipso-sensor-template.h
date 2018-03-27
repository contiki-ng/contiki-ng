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

#ifndef IPSO_SENSOR_TEMPLATE_H_
#define IPSO_SENSOR_TEMPLATE_H_

#include "lwm2m-engine.h"

typedef struct ipso_sensor ipso_sensor_t;

typedef lwm2m_status_t (*ipso_sensor_get_value_millis_t)(const ipso_sensor_t *sensor, int32_t *v);

/* Values of the IPSO object */
typedef struct ipso_sensor_value {
  lwm2m_object_instance_t reg_object;
  const ipso_sensor_t *sensor;
  uint8_t flags;
  int32_t last_value;
  int32_t min_value;
  int32_t max_value;
} ipso_sensor_value_t;

/* Meta data about an IPSO sensor object */
struct ipso_sensor {
  /* LWM2M object type */
  uint16_t object_id;
  uint16_t instance_id;
  /* When we read out the value we send in a context to write to */
  ipso_sensor_get_value_millis_t get_value_in_millis;
  int32_t min_range;
  int32_t max_range;
  char *unit;
  /* update interval in seconds */
  uint16_t update_interval;
  ipso_sensor_value_t *sensor_value;
};

#define IPSO_SENSOR(name, oid, get_value, ...)  \
  static ipso_sensor_value_t name##_value;      \
  static const ipso_sensor_t name = {           \
    .object_id = oid,                           \
    .sensor_value = &name##_value,              \
    .get_value_in_millis = get_value,           \
    __VA_ARGS__                                 \
  }

int ipso_sensor_add(const ipso_sensor_t *sensor);
int ipso_sensor_remove(const ipso_sensor_t *sensor);

#endif /* IPSO_SENSOR_TEMPLATE_H_ */
/** @} */
