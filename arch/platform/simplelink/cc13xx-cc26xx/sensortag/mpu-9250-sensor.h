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
 * \defgroup sensortag-mpu SensorTag Motion Processing Unit
 *
 *      Driver for the Invensense MPU-9250 Motion Processing Unit.
 *
 *      Due to the time required between triggering a reading and the
 *      reading becoming available, this driver is meant to be used in an
 *      asynchronous fashion. The caller must first activate the sensor by
 *      calling
 *
 *        mpu_9250_sensor.configure(SENSORS_ACTIVE, xyz);
 *
 *      The value for the xyz arguments depends on the required readings. If
 *      the caller intends to read both the accelerometer as well as the gyro
 *      then xyz should be MPU_9250_SENSOR_TYPE_ALL. If the caller only needs
 *      to take a reading from one of the two elements, xyz should be one of
 *      MPU_9250_SENSOR_TYPE_ACC or MPU_9250_SENSOR_TYPE_GYRO
 *
 *      Calling configure() will power up the sensor and initialise it. When
 *      the sensor is ready to provide readings, the driver will generate a
 *      sensors_changed event.
 *
 *      Calls to status() will return the driver's state which could indicate
 *      that the sensor is off, booting or on.
 *
 *      Once a reading has been taken, the caller has two options:
 *      - Turn the sensor off by calling SENSORS_DEACTIVATE, but in order to
 *        take subsequent readings the sensor must be started up all over
 *      - Leave the sensor on. In this scenario, the caller can simply keep
 *        calling value() for subsequent readings, but having the sensor on
 *        will consume more energy, especially if both accelerometer and the
 *        gyro are on.
 * @{
 *
 * \file
 *        Header file for the Sensortag Invensense MPU-9250 motion processing unit
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#ifndef MPU_9250_SENSOR_H_
#define MPU_9250_SENSOR_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/sensors.h"
/*---------------------------------------------------------------------------*/
#include "board-conf.h"
/*---------------------------------------------------------------------------*/
#if BOARD_SENSORS_ENABLE
#if (TI_I2C_CONF_ENABLE == 0) || (TI_I2C_CONF_I2C0_ENABLE == 0)
#error "The MPU-9250 requires the I2C driver (TI_I2C_CONF_ENABLE = 1)"
#endif
#endif
/*---------------------------------------------------------------------------*/
#define MPU_9250_READING_ERROR    -1
/*---------------------------------------------------------------------------*/
/* Accelerometer / Gyro Axes */
typedef enum {
  MPU_9250_SENSOR_TYPE_NONE = (0),          /**< 0b000000 = 0x00 */
  MPU_9250_SENSOR_TYPE_GYRO_X = (1 << 0),   /**< 0b000001 = 0x01 */
  MPU_9250_SENSOR_TYPE_GYRO_Y = (1 << 1),   /**< 0b000010 = 0x02 */
  MPU_9250_SENSOR_TYPE_GYRO_Z = (1 << 2),   /**< 0b000100 = 0x04 */
  MPU_9250_SENSOR_TYPE_ACC_X = (1 << 3),    /**< 0b001000 = 0x08 */
  MPU_9250_SENSOR_TYPE_ACC_Y = (1 << 4),    /**< 0b010000 = 0x10 */
  MPU_9250_SENSOR_TYPE_ACC_Z = (1 << 5),    /**< 0b100000 = 0x20 */
  MPU_9250_SENSOR_TYPE_GYRO = MPU_9250_SENSOR_TYPE_GYRO_X
    | MPU_9250_SENSOR_TYPE_GYRO_Y
    | MPU_9250_SENSOR_TYPE_GYRO_Z,
  /**< 0b000111 = 0x07 */
  MPU_9250_SENSOR_TYPE_ACC = MPU_9250_SENSOR_TYPE_ACC_X
    | MPU_9250_SENSOR_TYPE_ACC_Y
    | MPU_9250_SENSOR_TYPE_ACC_Z,
  /**< 0b111000 = 0x38 */
  MPU_9250_SENSOR_TYPE_ALL = MPU_9250_SENSOR_TYPE_GYRO
    | MPU_9250_SENSOR_TYPE_ACC
    /**< 0b111111 = 0x3F */
} MPU_9250_SENSOR_TYPE;
/*---------------------------------------------------------------------------*/
/* Accelerometer range */
typedef enum {
  MPU_9250_SENSOR_ACC_RANGE_2G = 0,
  MPU_9250_SENSOR_ACC_RANGE_4G = 1,
  MPU_9250_SENSOR_ACC_RANGE_8G = 2,
  MPU_9250_SENSOR_ACC_RANGE_16G = 3
} MPU_9250_SENSOR_ACC_RANGE;
/*---------------------------------------------------------------------------*/
/* Sensor status */
typedef enum {
  MPU_9250_SENSOR_STATUS_DISABLED,
  MPU_9250_SENSOR_STATUS_ENABLED,
  MPU_9250_SENSOR_STATUS_BOOTING,
  MPU_9250_SENSOR_STATUS_READY
} MPU_9250_SENSOR_STATUS;
/*---------------------------------------------------------------------------*/
/* Accelerometer range configuration, type MPU_9250_SENSOR_ACC_RANGE */
#ifdef MPU_9250_SENSOR_CONF_ACC_RANGE_ARG
#define MPU_9250_SENSOR_ACC_RANGE_ARG  MPU_9250_SENSOR_CONF_ACC_RANGE
#else
#define MPU_9250_SENSOR_ACC_RANGE_ARG  MPU_9250_SENSOR_ACC_RANGE_2G
#endif
/*---------------------------------------------------------------------------*/
extern const struct sensors_sensor mpu_9250_sensor;
/*---------------------------------------------------------------------------*/
#endif /* MPU_9250_SENSOR_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
