/*
 * Copyright (c) 2018, RISE SICS.
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
 *
 *
 */

#include "contiki.h"
#include "lib/sensors.h"
#include <em_device.h>
#include <em_gpio.h>

#define DEBOUNCE_DURATION (CLOCK_SECOND >> 5)

struct btn_timer {
  struct timer debounce;
  clock_time_t start;
  clock_time_t duration;
};

static struct btn_timer left_timer, right_timer;
/*---------------------------------------------------------------------------*/
static uint8_t
get_state(void)
{
  uint32_t portState;
  uint8_t  buttonState;

  portState   = GPIO_PortInGet(BOARD_BUTTON_PORT);
  buttonState = (uint8_t) ( (portState >> BOARD_BUTTON_SHIFT)
                            & BOARD_BUTTON_MASK);

  buttonState = ~buttonState & BOARD_BUTTON_MASK;

  return buttonState;
}
/*---------------------------------------------------------------------------*/
static void
enable_irq(int button, bool enable)
{
  if(button == 0) {
    GPIO_ExtIntConfig(BOARD_BUTTON_PORT, BOARD_BUTTON_LEFT_PIN,
                      EXTI_BUTTON0, true, true, enable);
  } else {
    GPIO_ExtIntConfig(BOARD_BUTTON_PORT, BOARD_BUTTON_RIGHT_PIN,
                      EXTI_BUTTON1, true, true, enable);
  }
}
/*---------------------------------------------------------------------------*/
void
button_sensor_button_irq(int button)
{
  struct btn_timer  *timer;
  int bval = 0;
  const struct sensors_sensor *button_sp;

  if(button == 0) {
    GPIO_IntClear(1 << EXTI_BUTTON0);
    timer = &left_timer;
    bval = BOARD_BUTTON_LEFT;
    button_sp = &button_left_sensor;
  } else {
    GPIO_IntClear(1 << EXTI_BUTTON1);
    timer = &right_timer;
    bval = BOARD_BUTTON_RIGHT;
    button_sp = &button_right_sensor;
  }

  if(!timer_expired(&timer->debounce)) {
    return;
  }

  if((get_state() & bval)) {
    timer->start = clock_time();
    timer->duration = 0;
    sensors_changed(button_sp);
  } else {
    timer->duration = clock_time() - timer->start;
    sensors_changed(button_sp);
  }
}
/*---------------------------------------------------------------------------*/
static void
config_buttons(int type, int c, int button)
{
  switch(type) {
  case SENSORS_HW_INIT:
    /* Configure at init */
    GPIO_PinModeSet(BOARD_BUTTON_LEFT_PORT, BOARD_BUTTON_LEFT_PIN,
                    gpioModeInput, 0);
    GPIO_PinModeSet(BOARD_BUTTON_RIGHT_PORT, BOARD_BUTTON_RIGHT_PIN,
                    gpioModeInput, 0);
    break;
  case SENSORS_ACTIVE:
    enable_irq(button, c > 0);
    break;
  default:
    break;
  }
}
/*---------------------------------------------------------------------------*/
static int
config_left(int type, int c)
{
  config_buttons(type, c, 0);
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
config_right(int type, int c)
{
  config_buttons(type, c, 1);
  return 1;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Status function for all buttons
 * \param type SENSORS_ACTIVE or SENSORS_READY
 * \param key_io_id BOARD_IOID_KEY_LEFT, BOARD_IOID_KEY_RIGHT etc
 * \return 1 if the button's port interrupt is enabled (edge detect)
 *
 * This function will only be called by status_left, status_right and the
 * called will pass the correct key_io_id
 */
static int
status(int type, int button)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    /* add something here... */
    return 1;
    break;
  default:
    break;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
value_left(int type)
{
  if(type == BUTTON_SENSOR_VALUE_STATE) {
    return (get_state() & BOARD_BUTTON_LEFT) ?
      BUTTON_SENSOR_VALUE_PRESSED : BUTTON_SENSOR_VALUE_RELEASED;
  } else if(type == BUTTON_SENSOR_VALUE_DURATION) {
    return (int)left_timer.duration;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
value_right(int type)
{
  if(type == BUTTON_SENSOR_VALUE_STATE) {
    return (get_state() & BOARD_BUTTON_RIGHT) ?
      BUTTON_SENSOR_VALUE_PRESSED : BUTTON_SENSOR_VALUE_RELEASED;
  } else if(type == BUTTON_SENSOR_VALUE_DURATION) {
    return (int)right_timer.duration;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
status_left(int type)
{
  return status(type, 0);
}
/*---------------------------------------------------------------------------*/
static int
status_right(int type)
{
  return status(type, 1);
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button_left_sensor, "Button Left", value_left, config_left,
               status_left);
SENSORS_SENSOR(button_right_sensor, "Button Right", value_right, config_right,
               status_right);
/*---------------------------------------------------------------------------*/
