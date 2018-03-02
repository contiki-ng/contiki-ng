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
 *         Implementation of OMA LWM2M / IPSO Light Control for LEDs
 * \author
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 */

#include "lwm2m-object.h"
#include "lwm2m-engine.h"
#include "ipso-control-template.h"
#include "dev/leds.h"
#include <stdint.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

#if LEDS_LEGACY_API
#if LEDS_ALL & LEDS_BLUE || LEDS_ALL & LEDS_RED || LEDS_ALL & LEDS_BLUE
#define LEDS_CONTROL_NUMBER (((LEDS_ALL & LEDS_BLUE) ? 1 : 0) + ((LEDS_ALL & LEDS_RED) ? 1 : 0) + ((LEDS_ALL & LEDS_GREEN) ? 1 : 0))
#else
#define LEDS_CONTROL_NUMBER 1
#endif
#else /* LEDS_LEGACY_API */
#define LEDS_CONTROL_NUMBER LEDS_COUNT
#endif /* LEDS_LEGACY_API */

typedef struct led_state {
  ipso_control_t control;
  uint8_t led_value;
} led_state_t;

static led_state_t leds_controls[LEDS_CONTROL_NUMBER];
/*---------------------------------------------------------------------------*/
static lwm2m_status_t
set_value(ipso_control_t *control, uint8_t value)
{
#if PLATFORM_HAS_LEDS || LEDS_COUNT
  led_state_t *state;

  state = (led_state_t *)control;

  if(value) {
    leds_on(state->led_value);
  } else {
    leds_off(state->led_value);
  }
#endif /* PLATFORM_HAS_LEDS */

  return LWM2M_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static int
bit_no(int bit)
{
  int i;
  for(i = 0; i < 8; i++) {
    if(LEDS_ALL & (1 << i)) {
      if(bit == 0) {
        /* matching bit */
        return 1 << i;
      } else {
        /* matching but used */
        bit--;
      }
    }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
void
ipso_leds_control_init(void)
{
  ipso_control_t *c;
  int i;

  /* Initialize the instances */
  for(i = 0; i < LEDS_CONTROL_NUMBER; i++) {
    c = &leds_controls[i].control;
    c->reg_object.object_id = 3311;
    c->reg_object.instance_id = i;
    c->set_value = set_value;
    leds_controls[i].led_value = bit_no(i);
    ipso_control_add(c);
  }

  PRINTF("IPSO leds control initialized with %u instances\n",
         LEDS_CONTROL_NUMBER);
}
/*---------------------------------------------------------------------------*/
/** @} */
