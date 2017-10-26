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
 *
 */

/**
 * \file
 *         Implementation of OMA LWM2M / IPSO Light Control
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "ipso-objects.h"
#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "ipso-control-template.h"

#ifdef IPSO_LIGHT_CONTROL
extern const struct ipso_objects_actuator IPSO_LIGHT_CONTROL;
#endif /* IPSO_LIGHT_CONTROL */

static lwm2m_status_t set_value(ipso_control_t *control, uint8_t value);

IPSO_CONTROL(light_control, 3311, 0, set_value);
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
set_value(ipso_control_t *control, uint8_t value)
{
#ifdef IPSO_LIGHT_CONTROL
  if(IPSO_LIGHT_CONTROL.set_dim_level) {
    IPSO_LIGHT_CONTROL.set_dim_level(value);
  } else if(IPSO_LIGHT_CONTROL.set_on) {
    IPSO_LIGHT_CONTROL.set_on(value);
  }
#endif /* IPSO_LIGHT_CONTROL */
  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
void
ipso_light_control_init(void)
{
#ifdef IPSO_LIGHT_CONTROL
  if(IPSO_LIGHT_CONTROL.init) {
    IPSO_LIGHT_CONTROL.init();
  }
  if(IPSO_LIGHT_CONTROL.get_dim_level) {
    ipso_control_set_value(&light_control,
                           IPSO_LIGHT_CONTROL.get_dim_level());
  } else if(IPSO_LIGHT_CONTROL.is_on) {
    ipso_control_set_on(&light_control, IPSO_LIGHT_CONTROL.is_on());
  }
#endif /* IPSO_LIGHT_CONTROL */

  ipso_control_add(&light_control);
}
/*---------------------------------------------------------------------------*/
/** @} */
