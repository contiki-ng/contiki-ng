/*
 * Copyright (c) 2015, Nordic Semiconductor
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \addtogroup nrf52840generic All nRF52840 platforms
 * @{
 *
 * \addtogroup nrf52840-devices Device drivers
 * @{
 *
 * \addtogroup nrf52840-devices-temp Temperature sensor driver
 * This is a driver for nRF52840 hardware sensor.
 *
 * @{
 *
 * \file
 *         Temperature sensor implementation.
 * \author
 *         Wojciech Bober <wojciech.bober@nordicsemi.no>
 *
 */
#include "nrf_temp.h"
#include "contiki.h"
#include "temperature-sensor.h"

const struct sensors_sensor temperature_sensor;

/*---------------------------------------------------------------------------*/
/**
 * \brief Returns device temperature
 * \param type ignored
 * \return Device temperature in degrees Celsius
 */
static int
value(int type)
{
  int32_t volatile temp;

  NRF_TEMP->TASKS_START = 1;
  /* nRF52832 datasheet: one temperature measurement takes typically 36 us */
  RTIMER_BUSYWAIT_UNTIL(NRF_TEMP->EVENTS_DATARDY, RTIMER_SECOND * 72 / 1000000);
  NRF_TEMP->EVENTS_DATARDY = 0;
  temp = nrf_temp_read();
  NRF_TEMP->TASKS_STOP = 1;

  return temp;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Configures temperature sensor
 * \param type initializes the hardware sensor when \a type is set to
 *             \a SENSORS_HW_INIT
 * \param c ignored
 * \return 1
 */
static int
configure(int type, int c)
{
  if(type == SENSORS_HW_INIT) {
    nrf_temp_init();
  }
  return 1;
}
/**
 * \brief Return temperature sensor status
 * \param type ignored
 * \return 1
 */
/*---------------------------------------------------------------------------*/
static int
status(int type)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(temperature_sensor, TEMPERATURE_SENSOR, value, configure, status);
/**
 * @}
 * @}
 * @}
 */
