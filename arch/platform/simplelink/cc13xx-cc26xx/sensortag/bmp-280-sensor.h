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
 * \defgroup sensortag-bmp-sensor SensorTag Pressure Sensor
 *
 * Due to the time required for the sensor to startup, this driver is meant to
 * be used in an asynchronous fashion. The caller must first activate the
 * sensor by calling SENSORS_ACTIVATE(). This will trigger the sensor's startup
 * sequence, but the call will not wait for it to complete so that the CPU can
 * perform other tasks or drop to a low power mode.
 *
 * Once the sensor is stable, the driver will generate a sensors_changed event.
 *
 * We take readings in "Forced" mode. In this mode, the BMP will take a single
 * measurement and it will then automatically go to sleep.
 *
 * SENSORS_ACTIVATE must be called again to trigger a new reading cycle
 * @{
 *
 * \file
 *        Header file for the Sensortag BMP280 Altimeter / Pressure Sensor
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef BMP_280_SENSOR_H_
#define BMP_280_SENSOR_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/sensors.h"
/*---------------------------------------------------------------------------*/
#include "board-conf.h"
/*---------------------------------------------------------------------------*/
/* The BMP-280 driver uses the I2C0 peripheral to access the senssor */
#if BOARD_SENSORS_ENABLE
#if (TI_I2C_CONF_ENABLE == 0) || (TI_I2C_CONF_I2C0_ENABLE == 0)
#error "The BMP280 requires the I2C driver (TI_I2C_CONF_ENABLE = 1)"
#endif
#endif
/*---------------------------------------------------------------------------*/
typedef enum {
  BMP_280_SENSOR_TYPE_TEMP,
  BMP_280_SENSOR_TYPE_PRESS
} BMP_280_SENSOR_TYPE;
/*---------------------------------------------------------------------------*/
#define BMP_280_READING_ERROR    -1
/*---------------------------------------------------------------------------*/
extern const struct sensors_sensor bmp_280_sensor;
/*---------------------------------------------------------------------------*/
#endif /* BMP_280_SENSOR_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
