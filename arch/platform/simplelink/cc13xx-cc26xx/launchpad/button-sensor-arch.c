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
/* LaunchPad has 2 buttons: BTN1 and BTN2 */
/* Map the GPIO defines from the Board file */
#define BTN1_GPIO  Board_GPIO_BTN1
#define BTN2_GPIO  Board_GPIO_BTN2
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
static BtnTimer g_btn1Timer;
static BtnTimer g_btn2Timer;
/*---------------------------------------------------------------------------*/
static void
button_press_cb(uint8_t index, BtnTimer *btnTimer, const struct sensors_sensor *btnSensor)
{
  if (!timer_expired(&btnTimer->debounce)) {
    return;
  }

  timer_set(&btnTimer->debounce, DEBOUNCE_DURATION);

  // Start press duration counter on press (falling), notify on release (rising)
  if (GPIO_read(index) == 0) {
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
static void
btn1_press_cb(unsigned char unusued)
{
  button_press_cb(BTN1_GPIO, &g_btn1Timer, &btn1_sensor);
}
/*---------------------------------------------------------------------------*/
static int
btn1_value(int type)
{
  return button_value(type, BTN1_GPIO, &g_btn1Timer);
}
/*---------------------------------------------------------------------------*/
static int
btn1_config(int type, int value)
{
  return button_config(type, value, BTN1_GPIO, btn1_press_cb);
}
/*---------------------------------------------------------------------------*/
static int
btn1_status(int type)
{
  return button_status(type, BTN1_GPIO);
}
/*---------------------------------------------------------------------------*/
static void
btn2_press_cb(unsigned char unusued)
{
  if (BUTTON_SENSOR_ENABLE_SHUTDOWN) {
    Power_shutdown(Power_ENTERING_SHUTDOWN, 0);
    return;
  }

  button_press_cb(BTN2_GPIO, &g_btn2Timer, &btn2_sensor);
}
/*---------------------------------------------------------------------------*/
static int
btn2_value(int type)
{
  return button_value(type, BTN2_GPIO, &g_btn2Timer);
}
/*---------------------------------------------------------------------------*/
static int
btn2_config(int type, int value)
{
  return button_config(type, value, BTN2_GPIO, btn2_press_cb);
}
/*---------------------------------------------------------------------------*/
static int
btn2_status(int type)
{
  return button_status(type, BTN1_GPIO);
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(btn1_sensor, BUTTON_SENSOR, btn1_value, btn1_config, btn1_status);
SENSORS_SENSOR(btn2_sensor, BUTTON_SENSOR, btn2_value, btn2_config, btn2_status);
/*---------------------------------------------------------------------------*/
/** @} */
