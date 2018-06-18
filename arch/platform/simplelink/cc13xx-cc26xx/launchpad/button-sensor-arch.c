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
#include <ti/drivers/PIN.h>
#include <ti/drivers/Power.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#include "button-sensor.h"
#include "button-sensor-arch.h"
/*---------------------------------------------------------------------------*/
/* LaunchPad has 2 buttons: BTN1 and BTN2 */
/* Map the GPIO defines from the Board file */
#define BTN1_PIN  Board_PIN_BTN1
#define BTN2_PIN  Board_PIN_BTN2
/*---------------------------------------------------------------------------*/
#ifdef BUTTON_SENSOR_CONF_ENABLE_SHUTDOWN
#   define BUTTON_SENSOR_ENABLE_SHUTDOWN  BUTTON_SENSOR_CONF_ENABLE_SHUTDOWN
#else
#   define BUTTON_SENSOR_ENABLE_SHUTDOWN  0
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
static BtnTimer btn1_timer;
static BtnTimer btn2_timer;
/*---------------------------------------------------------------------------*/
static const PIN_Config btn_pin_table[] = {
  BTN1_PIN | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,             /* Button is active low */
  BTN2_PIN | PIN_INPUT_EN | PIN_PULLUP | PIN_IRQ_BOTHEDGES | PIN_HYSTERESIS,             /* Button is active low */
  PIN_TERMINATE
};

static PIN_State  pin_state;
static PIN_Handle pin_handle;
/*---------------------------------------------------------------------------*/
static void
button_press_cb(PIN_Handle handle, PIN_Id pin_id)
{
#if BUTTON_SENSOR_ENABLE_SHUTDOWN
  if (pin_id == BTN2_PIN) {
    Power_shutdown(Power_ENTERING_SHUTDOWN, 0);
    return;
  }
#endif

  BtnTimer *btn_timer = NULL;
  const struct sensors_sensor *btn_sensor = NULL;

  switch (pin_id) {
    case BTN1_PIN: btn_timer  = &btn1_timer;
                   btn_sensor = &btn1_sensor; break;
    case BTN2_PIN: btn_timer  = &btn2_timer;
                   btn_sensor = &btn2_sensor; break;
    default: return; /* No matching PIN */
  }

  if (!timer_expired(&btn_timer->debounce)) {
    return;
  }

  timer_set(&btn_timer->debounce, DEBOUNCE_DURATION);

  // Start press duration counter on press (falling), notify on release (rising)
  if (PIN_getInputValue(pin_id) == 0) {
    btn_timer->start = clock_time();
    btn_timer->duration = 0;
  } else {
    btn_timer->duration = clock_time() - btn_timer->start;
    sensors_changed(btn_sensor);
  }
}
/*---------------------------------------------------------------------------*/
static int
button_value(int type, uint8_t pin, BtnTimer *btn_timer)
{
  if (type == BUTTON_SENSOR_VALUE_STATE) {
    return (PIN_getInputValue(pin) == 0)
      ? BUTTON_SENSOR_VALUE_PRESSED
      : BUTTON_SENSOR_VALUE_RELEASED;
  } else if (type == BUTTON_SENSOR_VALUE_DURATION) {
    return (int)btn_timer->duration;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
button_config(int type, int value, uint8_t pin)
{
  switch (type) {
  case SENSORS_HW_INIT:
    // Open PIN handle
    if (pin_handle) {
      return 1;
    }
    pin_handle = PIN_open(&pin_state, btn_pin_table);
    if (!pin_handle) {
      return 0;
    }
    // Register button callback function
    PIN_registerIntCb(pin_handle, button_press_cb);
    break;

  case SENSORS_ACTIVE:
    if (value) {
      // Enable interrupts on both edges
      PIN_setInterrupt(pin_handle, pin | PIN_IRQ_BOTHEDGES);
    } else {
      // Disable pin interrupts
      PIN_setInterrupt(pin_handle, pin | PIN_IRQ_DIS);
    }
    break;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
static int
button_status(int type, uint8_t pin)
{
  switch(type) {
  case SENSORS_ACTIVE: /* fallthrough */
  case SENSORS_READY: {
    PIN_Config pin_cfg = PIN_getConfig(pin);
    return (pin_cfg & PIN_BM_IRQ) == PIN_IRQ_DIS;
  }
  default: break;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static int
btn1_value(int type)
{
  return button_value(type, BTN1_PIN, &btn1_timer);
}
/*---------------------------------------------------------------------------*/
static int
btn1_config(int type, int value)
{
  return button_config(type, value, BTN1_PIN);
}
/*---------------------------------------------------------------------------*/
static int
btn1_status(int type)
{
  return button_status(type, BTN1_PIN);
}
/*---------------------------------------------------------------------------*/
static int
btn2_value(int type)
{
  return button_value(type, BTN2_PIN, &btn2_timer);
}
/*---------------------------------------------------------------------------*/
static int
btn2_config(int type, int value)
{
  return button_config(type, value, BTN2_PIN);
}
/*---------------------------------------------------------------------------*/
static int
btn2_status(int type)
{
  return button_status(type, BTN2_PIN);
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(btn1_sensor, BUTTON_SENSOR, btn1_value, btn1_config, btn1_status);
SENSORS_SENSOR(btn2_sensor, BUTTON_SENSOR, btn2_value, btn2_config, btn2_status);
/*---------------------------------------------------------------------------*/
/** @} */
