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

#ifndef IPSO_CONTROL_TEMPLATE_H_
#define IPSO_CONTROL_TEMPLATE_H_

#include "lwm2m-engine.h"

typedef struct ipso_control ipso_control_t;

#define IPSO_CONTROL_USE_DIMMER   0x01

typedef lwm2m_status_t (*ipso_control_set_value_t)(ipso_control_t *control,
                                                   uint8_t v);

/* Values of the IPSO control object */
struct ipso_control {
  lwm2m_object_instance_t reg_object;
  uint8_t flags;
  uint8_t value;  /* used to emulate on/off and dim-value */
  uint32_t on_time; /* on-time in seconds */
  uint64_t last_on_time;
  ipso_control_set_value_t set_value;
};

#define IPSO_CONTROL(name, oid, iid, setv)                              \
  static ipso_control_t name = {                                        \
    .reg_object.object_id = oid,                                        \
    .reg_object.instance_id = iid,                                      \
    .set_value = setv                                                   \
  }

int ipso_control_add(ipso_control_t *control);
int ipso_control_remove(ipso_control_t *control);

static inline uint16_t
ipso_control_get_object_id(const ipso_control_t *control)
{
  return control->reg_object.object_id;
}

static inline uint16_t
ipso_control_get_instance_id(const ipso_control_t *control)
{
  return control->reg_object.instance_id;
}

static inline uint8_t
ipso_control_is_on(const ipso_control_t *control)
{
  return (control->value & 0x80) != 0;
}

static inline uint8_t
ipso_control_get_value(const ipso_control_t *control)
{
  return (control->value & 0x80) != 0 ? (control->value & 0x7f) : 0;
}

lwm2m_status_t ipso_control_set_on(ipso_control_t *control, uint8_t onoroff);

lwm2m_status_t ipso_control_set_value(ipso_control_t *control, uint8_t dimm_value);

#endif /* IPSO_CONTROL_TEMPLATE_H_ */
/** @} */
