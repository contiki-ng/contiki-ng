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
 * \addtogroup cc13xx-cc26xx-batmon
 * @{
 *
 * \file
 *        Driver for the CC13xx/CC26xx AON battery monitor.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/sensors.h"

#include "batmon-sensor.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/aon_batmon.h)
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
#define SENSOR_STATUS_DISABLED 0
#define SENSOR_STATUS_ENABLED  1

static int enabled = SENSOR_STATUS_DISABLED;
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns a reading from the sensor
 * \param type BATMON_SENSOR_TYPE_TEMP or BATMON_SENSOR_TYPE_VOLT
 *
 * \return The value as returned by the respective CC26xxware function
 */
static int
value(int type)
{
  if(enabled == SENSOR_STATUS_DISABLED) {
    PRINTF("Sensor Disabled\n");
    return BATMON_SENSOR_READING_ERROR;
  }

  switch(type) {
  case BATMON_SENSOR_TYPE_TEMP: return (int)AONBatMonTemperatureGetDegC();
  case BATMON_SENSOR_TYPE_VOLT: return (int)AONBatMonBatteryVoltageGet();
  default:
    PRINTF("Invalid type\n");
    return BATMON_SENSOR_READING_ERROR;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Configuration function for the battery monitor sensor.
 *
 * \param type Activate, enable or disable the sensor. See below
 * \param enable If
 *
 * When type == SENSORS_HW_INIT we turn on the hardware
 * When type == SENSORS_ACTIVE and enable==1 we enable the sensor
 * When type == SENSORS_ACTIVE and enable==0 we disable the sensor
 */
static int
configure(int type, int enable)
{
  switch(type) {
  case SENSORS_HW_INIT:
    AONBatMonEnable();
    enabled = SENSOR_STATUS_ENABLED;
    break;
  case SENSORS_ACTIVE:
    if(enable) {
      AONBatMonEnable();
      enabled = SENSOR_STATUS_ENABLED;
    } else {
      AONBatMonDisable();
      enabled = SENSOR_STATUS_DISABLED;
    }
    break;
  default:
    break;
  }
  return enabled;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Returns the status of the sensor
 * \param type SENSORS_ACTIVE or SENSORS_READY
 * \return 1 if the sensor is enabled
 */
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return enabled;
    break;
  default:
    break;
  }
  return SENSOR_STATUS_DISABLED;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(batmon_sensor, "Battery Monitor", value, configure, status);
/*---------------------------------------------------------------------------*/
/** @} */
