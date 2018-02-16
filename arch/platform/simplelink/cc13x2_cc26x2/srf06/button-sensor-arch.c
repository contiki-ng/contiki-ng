/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
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
/**
 * \addtogroup launchpad-button-sensor
 * @{
 *
 * \file
 * Driver for LaunchPad buttons
 */
/*---------------------------------------------------------------------------*/
#include <contiki.h>
#include <sys/timer.h>
#include <lib/sensors.h>
/*---------------------------------------------------------------------------*/
#include <Board.h>
#include <ti/drivers/GPIO.h>
#include <ti/drivers/Power.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
#include "button-sensor.h"
#include "button-sensor-arch.h"
/*---------------------------------------------------------------------------*/
/* SmartRF06 EB has 5 buttons: Select, Up, Down, Left, and Right */
/* Map the GPIO defines from the Board file */
#define BUTTON_SELECT_GPIO  Board_KEY_SELECT
#define BUTTON_UP_GPIO      Board_KEY_UP
#define BUTTON_DOWN_GPIO    Board_KEY_DOWN
#define BUTTON_LEFT_GPIO    Board_KEY_LEFT
#define BUTTON_RIGHT_GPIO   Board_KEY_RIGHT
/*---------------------------------------------------------------------------*/
#ifdef BUTTON_SENSOR_CONF_ENABLE_SHUTDOWN
#   define BUTTON_SENSOR_ENABLE_SHUTDOWN  BUTTON_SENSOR_CONF_ENABLE_SHUTDOWN
#else
#   define BUTTON_SENSOR_ENABLE_SHUTDOWN  1
#endif
/*---------------------------------------------------------------------------*/
#define DEBOUNCE_DURATION (CLOCK_SECOND >> 5)
/*---------------------------------------------------------------------------*/
typedef struct {
  struct timer debounce;
  clock_time_t start;
  clock_time_t duration;
} BtnTimer;
/*---------------------------------------------------------------------------*/
static BtnTimer g_buttonSelectTimer;
static BtnTimer g_buttonUpTimer;
static BtnTimer g_buttonDownTimer;
static BtnTimer g_buttonLeftTimer;
static BtnTimer g_buttonRightTimer;
/*---------------------------------------------------------------------------*/
static void
button_press_cb(uint8_t index, BtnTimer *btnTimer, const struct sensors_sensor *btnSensor)
{
  if (!timer_expired(&btnTimer->debounce)) {
    return;
  }

  timer_set(&btnTimer->debounce, DEBOUNCE_DURATION);

  // Start press duration counter on press (falling), notify on release (rising)
  if(GPIO_read(index) == 0) {
    btnTimer->start = clock_time();
    btnTimer->duration = 0;
  } else {
    btnTimer->duration = clock_time() - btnTimer->start;
    sensors_changed(btnSensor);
  }
}
/*---------------------------------------------------------------------------*/
static int
button_value(int type, uint8_t index, BtnTimer *btnTimer)
{
  if (type == BUTTON_SENSOR_VALUE_STATE) {
    return (GPIO_read(index) == 0)
      ? BUTTON_SENSOR_VALUE_PRESSED
      : BUTTON_SENSOR_VALUE_RELEASED;
  } else if (type == BUTTON_SENSOR_VALUE_DURATION) {
    return (int)btnTimer->duration;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
button_config(int type, int value, uint8_t index, GPIO_CallbackFxn callback)
{
  switch (type) {
  case SENSORS_HW_INIT:
    GPIO_clearInt(index);
    GPIO_setCallback(index, callback);
    break;

  case SENSORS_ACTIVE:
    if (value) {
      GPIO_clearInt(index);
      GPIO_enableInt(index);
    } else {
      GPIO_disableInt(index);
    }
    break;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
button_status(int type, uint8_t index)
{
  switch(type) {
  case SENSORS_ACTIVE: /* fallthrough */
  case SENSORS_READY: {
    GPIO_PinConfig pinCfg = 0;
    GPIO_getConfig(index, &pinCfg);
    return (pinCfg & GPIO_CFG_IN_INT_NONE) == 0;
  }
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Select Button */
/*---------------------------------------------------------------------------*/
static void
button_select_press_cb(unsigned char unusued)
{
  button_press_cb(BUTTON_SELECT_GPIO, &g_buttonSelectTimer, &button_select_sensor);
}
/*---------------------------------------------------------------------------*/
static int
button_select_value(int type)
{
  return button_value(type, BUTTON_SELECT_GPIO, &g_buttonSelectTimer);
}
/*---------------------------------------------------------------------------*/
static int
button_select_config(int type, int value)
{
  return button_config(type, value, BUTTON_SELECT_GPIO, button_select_press_cb);
}
/*---------------------------------------------------------------------------*/
static int
button_select_status(int type)
{
  return button_status(type, BUTTON_SELECT_GPIO);
}
/*---------------------------------------------------------------------------*/
/* Up Button */
/*---------------------------------------------------------------------------*/
static void
button_up_press_cb(unsigned char unusued)
{
  button_press_cb(BUTTON_UP_GPIO, &g_buttonUpTimer, &button_up_sensor);
}
/*---------------------------------------------------------------------------*/
static int
button_up_value(int type)
{
  return button_value(type, BUTTON_UP_GPIO, &g_buttonUpTimer);
}
/*---------------------------------------------------------------------------*/
static int
button_up_config(int type, int value)
{
  return button_config(type, value, BUTTON_UP_GPIO, button_up_press_cb);
}
/*---------------------------------------------------------------------------*/
static int
button_up_status(int type)
{
  return button_status(type, BUTTON_UP_GPIO);
}
/*---------------------------------------------------------------------------*/
/* Down Button */
/*---------------------------------------------------------------------------*/
static void
button_down_press_cb(unsigned char unusued)
{
  button_press_cb(BUTTON_DOWN_GPIO, &g_buttonDownTimer, &button_down_sensor);
}
/*---------------------------------------------------------------------------*/
static int
button_down_value(int type)
{
  return button_value(type, BUTTON_DOWN_GPIO, &g_buttonDownTimer);
}
/*---------------------------------------------------------------------------*/
static int
button_down_config(int type, int value)
{
  return button_config(type, value, BUTTON_DOWN_GPIO, button_down_press_cb);
}
/*---------------------------------------------------------------------------*/
static int
button_down_status(int type)
{
  return button_status(type, BUTTON_DOWN_GPIO);
}
/*---------------------------------------------------------------------------*/
/* Left Button */
/*---------------------------------------------------------------------------*/
static void
button_left_press_cb(unsigned char unusued)
{
  button_press_cb(BUTTON_LEFT_GPIO, &g_buttonLeftTimer, &button_left_sensor);
}
/*---------------------------------------------------------------------------*/
static int
button_left_value(int type)
{
  return button_value(type, BUTTON_LEFT_GPIO, &g_buttonLeftTimer);
}
/*---------------------------------------------------------------------------*/
static int
button_left_config(int type, int value)
{
  return button_config(type, value, BUTTON_LEFT_GPIO, button_left_press_cb);
}
/*---------------------------------------------------------------------------*/
static int
button_left_status(int type)
{
  return button_status(type, BUTTON_LEFT_GPIO);
}
/*---------------------------------------------------------------------------*/
/* Right Button */
/*---------------------------------------------------------------------------*/
static void
button_right_press_cb(unsigned char unusued)
{
  if (BUTTON_SENSOR_ENABLE_SHUTDOWN) {
    Power_shutdown(Power_ENTERING_SHUTDOWN, 0);
    return;
  }

  button_press_cb(BUTTON_RIGHT_GPIO, &g_buttonRightTimer, &button_right_sensor);
}
/*---------------------------------------------------------------------------*/
static int
button_right_value(int type)
{
  return button_value(type, BUTTON_RIGHT_GPIO, &g_buttoRightimer);
}
/*---------------------------------------------------------------------------*/
static int
button_right_config(int type, int value)
{
  return button_config(type, value, BUTTON_RIGHT_GPIO, button_right_press_cb);
}
/*---------------------------------------------------------------------------*/
static int
button_right_status(int type)
{
  return button_status(type, BUTTON_RIGHT_GPIO);
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(button_select_sensor, BUTTON_SENSOR,
               button_select_value, button_select_config, button_select_status);
SENSORS_SENSOR(button_up_sensor, BUTTON_SENSOR,
               button_up_value, button_up_config, button_up_status);
SENSORS_SENSOR(button_down_sensor, BUTTON_SENSOR,
               button_down_value, button_down_config, button_down_status);
SENSORS_SENSOR(button_left_sensor, BUTTON_SENSOR,
               button_left_value, button_left_config, button_left_status);
SENSORS_SENSOR(button_right_sensor, BUTTON_SENSOR,
               button_right_value, button_right_config, button_right_status);
/*---------------------------------------------------------------------------*/
/** @} */
