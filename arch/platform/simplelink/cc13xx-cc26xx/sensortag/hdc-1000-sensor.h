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
/**
 * \addtogroup sensortag-peripherals
 * @{
 *
 * \defgroup sensortag-hdc-sensor SensorTag HDC1000 - Temperature and
 *                                Humidity Sensor
 *
 * Due to the time required for the sensor to startup, this driver is meant to
 * be used in an asynchronous fashion. The caller must first activate the
 * sensor by calling SENSORS_ACTIVATE(). This will trigger the sensor's startup
 * sequence, but the call will not wait for it to complete so that the CPU can
 * perform other tasks or drop to a low power mode. Once the sensor has taken
 * readings, it will automatically go back to low power mode.
 *
 * Once the sensor is stable, the driver will retrieve readings from the sensor
 * and latch them. It will then generate a sensors_changed event.
 *
 * The user can then retrieve readings by calling .value() and by passing
 * either HDC_1000_SENSOR_TYPE_TEMP or HDC_1000_SENSOR_TYPE_HUMID as the
 * argument. Multiple calls to value() will not trigger new readings, they will
 * simply return the most recent latched values.
 *
 * The user can query the sensor's status by calling status().
 *
 * To get a fresh reading, the user must trigger a new reading cycle by calling
 * SENSORS_ACTIVATE().
 * @{
 *
 * \file
 *        Header file for the Sensortag HDC1000 sensor.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef HDC_1000_SENSOR_H
#define HDC_1000_SENSOR_H
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/sensors.h"
/*---------------------------------------------------------------------------*/
#include "board-conf.h"
/*---------------------------------------------------------------------------*/
#if BOARD_SENSORS_ENABLE
#if (TI_I2C_CONF_ENABLE == 0) || (TI_I2C_CONF_I2C0_ENABLE == 0)
#error "The HDC-1000 requires the I2C driver (TI_I2C_CONF_ENABLE = 1)"
#endif
#endif
/*---------------------------------------------------------------------------*/
typedef enum {
  HDC_1000_SENSOR_TYPE_TEMP,
  HDC_1000_SENSOR_TYPE_HUMID
} HDC_1000_SENSOR_TYPE;
/*---------------------------------------------------------------------------*/
/**
 * \name HDC1000 driver states
 * @{
 */
typedef enum {
  HDC_1000_SENSOR_STATUS_DISABLED,          /**< Not initialised */
  HDC_1000_SENSOR_STATUS_INITIALISED,       /**< Initialised but idle */
  HDC_1000_SENSOR_STATUS_TAKING_READINGS,   /**< Readings in progress */
  HDC_1000_SENSOR_STATUS_I2C_ERROR,         /**< I2C transaction failed */
  HDC_1000_SENSOR_STATUS_READINGS_READY     /**< Both readings ready */
} HDC_1000_SENSOR_STATUS;
/** @} */
/*---------------------------------------------------------------------------*/
#define HDC_1000_READING_ERROR    -1
/*---------------------------------------------------------------------------*/
extern const struct sensors_sensor hdc_1000_sensor;
/*---------------------------------------------------------------------------*/
#endif /* HDC_1000_SENSOR_H */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
