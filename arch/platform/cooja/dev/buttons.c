/*
 * Copyright (c) 2019, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/button-hal.h"
#include "lib/simEnvChange.h"

#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "Cooja Button"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
/* Variables required by the Cooja button interface */
char simButtonChanged;
char simButtonIsDown;
char simButtonIsActive = 1;
const struct simInterface button_interface;
/*---------------------------------------------------------------------------*/
/* The cooja virtual button on pin COOJA_BTN_PIN, active low */
BUTTON_HAL_BUTTON(button_user, "User button", COOJA_BTN_PIN,
                  GPIO_HAL_PIN_CFG_PULL_UP, BUTTON_HAL_ID_BUTTON_ZERO, true);
/*---------------------------------------------------------------------------*/
BUTTON_HAL_BUTTONS(&button_user);
/*---------------------------------------------------------------------------*/
static void
doInterfaceActionsBeforeTick(void)
{
  if(simButtonChanged) {
    LOG_DBG("Cooja button changed. simButtonIsDown=%u, ", simButtonIsDown);
    /*
     * First check what the simulator wants us to do: press or release. Based
     * on that, set the correct state for COOJA_BTN_PIN.
     */
    if(simButtonIsDown) {
      /* The button is active low, so clear the pin when pressed */
      LOG_DBG_("clearing pin");
      gpio_hal_arch_no_port_clear_pin(COOJA_BTN_PIN);
    } else {
      LOG_DBG_("setting pin");
      gpio_hal_arch_no_port_set_pin(COOJA_BTN_PIN);
    }

    /*
     * Subsequently, simply raise a virtual edge event on the pin, but only if
     * the interrupt is "enabled"
     */
    if(gpio_hal_arch_no_port_pin_cfg_get(COOJA_BTN_PIN) & GPIO_HAL_PIN_CFG_INT_ENABLE) {
      LOG_DBG_(", triggering edge event");
      gpio_hal_event_handler(gpio_hal_pin_to_mask(COOJA_BTN_PIN));
    }
    LOG_DBG_("\n");
  }

  simButtonChanged = 0;
}
/*---------------------------------------------------------------------------*/
static void
doInterfaceActionsAfterTick(void)
{
}
/*---------------------------------------------------------------------------*/
SIM_INTERFACE(button_interface,
              doInterfaceActionsBeforeTick,
              doInterfaceActionsAfterTick);
/*---------------------------------------------------------------------------*/
/*
 * Everything below needed to satisfy Cooja build dependency, which can be
 * removed when we change Cooja to no longer expect a button_sensor symbol
 */
#include "lib/sensors.h"
#include "dev/button-sensor.h"
/*---------------------------------------------------------------------------*/
static int
value(int type)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
configure(int type, int c)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button_sensor, BUTTON_SENSOR, value, configure, status);
