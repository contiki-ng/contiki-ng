/*
 * Copyright (c) 2017, RISE SICS AB
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

#include "em_device.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_timer.h"
#include "lib/sensors.h"
#include "dev/i2c.h"
#include "button-sensor.h"
#include "gpiointerrupt.h"
#include "bmp-280-sensor.h"
#include "rgbleds.h"

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "board"
#define LOG_LEVEL LOG_LEVEL_MAIN
/*---------------------------------------------------------------------------*/

#define VCOM_ENABLE_PORT gpioPortA
#define VCOM_ENABLE_PIN  5

i2c_bus_t i2c1_bus = {.lock_device = NULL,
                      .lock = 0,
                      .config = {.I2Cx = I2C1,
                                 .sda_loc = _I2C_ROUTELOC0_SDALOC_LOC17,
                                 .scl_loc = _I2C_ROUTELOC0_SCLLOC_LOC17},
                    };

/*---------------------------------------------------------------------------*/
void button_sensor_button_irq(int button);

static void gpioInterruptHandler(uint8_t pin)
{
  switch(pin) {
  case EXTI_BUTTON0:
    button_sensor_button_irq(0);
    break;
  case EXTI_BUTTON1:
    button_sensor_button_irq(1);
    break;
  }
}
/*---------------------------------------------------------------------------*/
void
board_init(void)
{
  /* Enable GPIO clock */
  CMU_ClockEnable(cmuClock_GPIO, true);
  /* enavble GPIO interrupts */
  GPIOINT_Init();

  /* Initialize LEDs (RED and Green) */
  GPIO_PinModeSet(BOARD_LED_RED_PORT, BOARD_LED_RED_PIN, gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_LED_GREEN_PORT, BOARD_LED_GREEN_PIN, gpioModePushPull, 0);

  /* Route UART output to USB */
  GPIO_PinModeSet(VCOM_ENABLE_PORT, VCOM_ENABLE_PIN, gpioModePushPull, 1);

  SENSORS_ACTIVATE(button_left_sensor);
  SENSORS_ACTIVATE(button_right_sensor);

  /* setup button interrupts  */
  GPIOINT_CallbackRegister(EXTI_BUTTON0, gpioInterruptHandler);
  GPIOINT_CallbackRegister(EXTI_BUTTON1, gpioInterruptHandler);

  SENSORS_ACTIVATE(bmp_280_sensor);

  rgbleds_init();
}
/*---------------------------------------------------------------------------*/
/** \brief Exports a global symbol to be used by the sensor API */
SENSORS(&button_left_sensor, &button_right_sensor, &bmp_280_sensor);
/*---------------------------------------------------------------------------*/
