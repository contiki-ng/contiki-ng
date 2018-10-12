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
 * \defgroup sensortag-tmp-sensor SensorTag IR Thermophile sensor
 *
 *        Due to the time required for the sensor to startup, this driver is
 *        meant to be used in an asynchronous fashion. The caller must first
 *        activate the sensor by calling SENSORS_ACTIVATE(). This will trigger
 *        the sensor's startup sequence, but the call will not wait for it to
 *        complete so that the CPU can perform other tasks or drop to a low
 *        power mode.
 *
 *        Once the sensor is stable, the driver will generate a
 *        sensors_changed event.
 *
 *        The caller should then use value(TMP_007_SENSOR_TYPE_ALL) to
 *        read sensor values and latch them. Once completed successfully,
 *        individual readings can be retrieved with calls to
 *        value(TMP_007_SENSOR_TYPE_OBJECT) or
 *        value(TMP_007_SENSOR_TYPE_AMBIENT).
 *
 *        Once required readings have been taken, the caller has two options:
 *        - Turn the sensor off by calling SENSORS_DEACTIVATE, but in order
 *          to take subsequent readings SENSORS_ACTIVATE must be called again.
 *        - Leave the sensor on. In this scenario, the caller can simply keep
 *          calling value(TMP_007_SENSOR_TYPE_ALL) to read and latch new
 *          values. However keeping the sensor on will consume more energy.
 * @{
 *
 * \file
 *        Header file for the Sensortag TMP-007 IR Thermophile sensor.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef TMP_007_SENSOR_H_
#define TMP_007_SENSOR_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include "board-conf.h"
/*---------------------------------------------------------------------------*/
#if BOARD_SENSORS_ENABLE
#if (TI_I2C_CONF_ENABLE == 0) || (TI_I2C_CONF_I2C0_ENABLE == 0)
#error "The BMP280 requires the I2C driver to be enabled (TI_I2C_CONF_ENABLE = 1)"
#endif
#endif
/*---------------------------------------------------------------------------*/
typedef enum {
  TMP_007_TYPE_OBJECT = (1 << 0),
  TMP_007_TYPE_AMBIENT = (1 << 1),

  TMP_007_TYPE_ALL = (TMP_007_TYPE_OBJECT |
                      TMP_007_TYPE_AMBIENT),
} TMP_007_TYPE;
/*---------------------------------------------------------------------------*/
typedef enum {
  TMP_007_STATUS_DISABLED,
  TMP_007_STATUS_INITIALIZED,
  TMP_007_STATUS_NOT_READY,
  TMP_007_STATUS_READY,
  TMP_007_STATUS_I2C_ERROR,
} TMP_007_STATUS;
/*---------------------------------------------------------------------------*/
#define TMP_007_READING_ERROR    -1
/*---------------------------------------------------------------------------*/
extern const struct sensors_sensor tmp_007_sensor;
/*---------------------------------------------------------------------------*/
#endif /* TMP_007_SENSOR_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
