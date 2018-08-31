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
 * \addtogroup sensortag-bmp-sensor
 * @{
 *
 * \file
 *        Driver for the Sensortag BMP280 Altimeter / Pressure Sensor
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/sensors.h"
#include "sys/ctimer.h"
/*---------------------------------------------------------------------------*/
#include "board-conf.h"
#include "bmp-280-sensor.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>

#include <ti/drivers/I2C.h>
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#define DEBUG 0
#if DEBUG
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif
/*---------------------------------------------------------------------------*/
/*
 * Disable the entire file if sensors are disabled, as it could potentially
 * create compile errors with missing defines from either the Board file or
 * configuration defines.
 */
#if BOARD_SENSORS_ENABLE
/*---------------------------------------------------------------------------*/
#ifndef Board_BMP280_ADDR
#error "Board file doesn't define I2C address Board_BMP280_ADDR"
#endif
/* Sensor I2C address */
#define BMP280_I2C_ADDRESS                  Board_BMP280_ADDR
/*---------------------------------------------------------------------------*/
/* Registers */
#define ADDR_CALIB                          0x88
#define ADDR_PROD_ID                        0xD0
#define ADDR_RESET                          0xE0
#define ADDR_STATUS                         0xF3
#define ADDR_CTRL_MEAS                      0xF4
#define ADDR_CONFIG                         0xF5
#define ADDR_PRESS_MSB                      0xF7
#define ADDR_PRESS_LSB                      0xF8
#define ADDR_PRESS_XLSB                     0xF9
#define ADDR_TEMP_MSB                       0xFA
#define ADDR_TEMP_LSB                       0xFB
#define ADDR_TEMP_XLSB                      0xFC
/*---------------------------------------------------------------------------*/
/* Reset values */
#define VAL_PROD_ID                         0x58
#define VAL_RESET                           0x00
#define VAL_STATUS                          0x00
#define VAL_CTRL_MEAS                       0x00
#define VAL_CONFIG                          0x00
#define VAL_PRESS_MSB                       0x80
#define VAL_PRESS_LSB                       0x00
#define VAL_TEMP_MSB                        0x80
#define VAL_TEMP_LSB                        0x00
/*---------------------------------------------------------------------------*/
/* Test values */
#define VAL_RESET_EXECUTE                   0xB6
#define VAL_CTRL_MEAS_TEST                  0x55
/*---------------------------------------------------------------------------*/
/* Misc. */
#define MEAS_DATA_SIZE                      6
#define CALIB_DATA_SIZE                     24
/*---------------------------------------------------------------------------*/
#define RES_OFF                             0
#define RES_ULTRA_LOW_POWER                 1
#define RES_LOW_POWER                       2
#define RES_STANDARD                        3
#define RES_HIGH                            5
#define RES_ULTRA_HIGH                      6
/*---------------------------------------------------------------------------*/
/* Bit fields in CTRL_MEAS register */
#define PM_OFF                              0
#define PM_FORCED                           1
#define PM_NORMAL                           3
/*---------------------------------------------------------------------------*/
#define OSRST(v)                            ((v) << 5)
#define OSRSP(v)                            ((v) << 2)
/*---------------------------------------------------------------------------*/
typedef struct {
  uint16_t dig_t1;
  int16_t dig_t2;
  int16_t dig_t3;
  uint16_t dig_p1;
  int16_t dig_p2;
  int16_t dig_p3;
  int16_t dig_p4;
  int16_t dig_p5;
  int16_t dig_p6;
  int16_t dig_p7;
  int16_t dig_p8;
  int16_t dig_p9;
} BMP_280_Calibration;
/*---------------------------------------------------------------------------*/
static BMP_280_Calibration calib_data;
/*---------------------------------------------------------------------------*/
static I2C_Handle i2c_handle;
/*---------------------------------------------------------------------------*/
typedef enum {
  SENSOR_STATUS_DISABLED,
  SENSOR_STATUS_INITIALISED,
  SENSOR_STATUS_NOT_READY,
  SENSOR_STATUS_READY
} SENSOR_STATUS;

static volatile SENSOR_STATUS sensor_status = SENSOR_STATUS_DISABLED;
/*---------------------------------------------------------------------------*/
/* Wait SENSOR_STARTUP_DELAY clock ticks for the sensor to be ready - ~80ms */
#define SENSOR_STARTUP_DELAY 3

static struct ctimer startup_timer;
/*---------------------------------------------------------------------------*/
static void
notify_ready(void *unused)
{
  (void)unused;

  sensor_status = SENSOR_STATUS_READY;
  sensors_changed(&bmp_280_sensor);
}
/*---------------------------------------------------------------------------*/
static bool
i2c_write_read(void *writeBuf, size_t writeCount, void *readBuf, size_t readCount)
{
  I2C_Transaction i2cTransaction = {
    .writeBuf = writeBuf,
    .writeCount = writeCount,
    .readBuf = readBuf,
    .readCount = readCount,
    .slaveAddress = BMP280_I2C_ADDRESS,
  };

  return I2C_transfer(i2c_handle, &i2cTransaction);
}
#define i2c_write(writeBuf, writeCount)   i2c_write_read(writeBuf, writeCount, NULL, 0)
#define i2c_read(readBuf, readCount)      i2c_write_read(NULL, 0, readBuf, readCount)
/*---------------------------------------------------------------------------*/
/**
 * \brief          Initalise the sensor.
 * \return Boolean Value descibing whether initialization were
 *                 successful or not.
 * \retval true    Successful initialization
 * \retval false   Error during initialization
 */
static bool
init(void)
{
  if(i2c_handle) {
    return true;
  }

  I2C_Params i2cParams;
  I2C_Params_init(&i2cParams);

  i2cParams.transferMode = I2C_MODE_BLOCKING;
  i2cParams.bitRate = I2C_400kHz;

  i2c_handle = I2C_open(Board_I2C0, &i2cParams);
  if(i2c_handle == NULL) {
    return false;
  }

  uint8_t reset_data[] = { ADDR_RESET, VAL_RESET_EXECUTE };

  uint8_t calib_reg = ADDR_CALIB;
  /* Read and store calibration data */
  return i2c_write_read(&calib_reg, sizeof(calib_reg), &calib_data, sizeof(calib_data))
         /* then reset the sensor */
         && i2c_write(reset_data, sizeof(reset_data));
}
/*---------------------------------------------------------------------------*/
/**
 * \brief          Enable/disable measurements.
 * \param enable   Enable if true; else, disable.
 * \return Boolean Value descibing whether initialization were
 *                 successful or not.
 * \retval true    Successful initialization
 * \retval false   Error during initialization
 */
static bool
enable_sensor(bool enable)
{
  uint8_t val = (enable)
    ? PM_FORCED | OSRSP(1) | OSRST(1)
    : PM_OFF;

  uint8_t ctrl_meas_data[] = { ADDR_CTRL_MEAS, val };
  return i2c_write(&ctrl_meas_data, sizeof(ctrl_meas_data));
}
/*---------------------------------------------------------------------------*/
/**
 * \brief          Read temperature and pressure data.
 * \param data     Pointer to a buffer where temperature and pressure will be
 *                 written.
 * \param count    Number of byes to read.
 * \return Boolean Value descibing whether initialization were
 *                 successful or not.
 * \retval true    Successful initialization
 * \retval false   Error during initialization
 */
static bool
read_data(uint8_t *data, size_t count)
{
  uint8_t press_msb_reg = ADDR_PRESS_MSB;
  return i2c_write_read(&press_msb_reg, sizeof(press_msb_reg), data, count);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief       Convert raw data to values in degrees C (temp) and Pascal
 *              (pressure).
 * \param data  Pointer to a buffer that holds raw sensor data.
 * \param temp  Pointer to a variable where the converted temperature will
 *              be written.
 * \param press Pointer to a variable where the converted pressure will be
 *              written.
 */
static void
convert(uint8_t *data, int32_t *temp, uint32_t *press)
{
  BMP_280_Calibration *p = &calib_data;

  /* Pressure */
  const int32_t upress = (int32_t)(
      (((uint32_t)data[0]) << 12) |
      (((uint32_t)data[1]) << 4) |
      (((uint32_t)data[2]) >> 4)
      );
  /* Temperature */
  const int32_t utemp = (int32_t)(
      (((uint32_t)data[3]) << 12) |
      (((uint32_t)data[4]) << 4) |
      (((uint32_t)data[5]) >> 4)
      );

  /* Compensate temperature */
  int32_t v_x1_u32r = (((utemp >> 3) - ((int32_t)p->dig_t1 << 1)) * (int32_t)p->dig_t2) >> 11;
  int32_t v_x2_u32r = (((((utemp >> 4) - (int32_t)p->dig_t1) * ((utemp >> 4) - (int32_t)p->dig_t1)) >> 12) * (int32_t)p->dig_t3) >> 14;

  const uint32_t t_fine = v_x1_u32r + v_x2_u32r;
  const int32_t temperature = (t_fine * 5 + 128) >> 8;
  *temp = temperature;

  /* Compensate pressure */
  v_x1_u32r = ((int32_t)t_fine >> 1) - (int32_t)64000;
  v_x2_u32r = (((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 11) * (int32_t)p->dig_p6;
  v_x2_u32r = ((v_x1_u32r * (int32_t)p->dig_p5) << 1) + v_x2_u32r;
  v_x2_u32r = (v_x2_u32r >> 2) + ((int32_t)p->dig_p4 << 16);
  v_x1_u32r = ((((((v_x1_u32r >> 2) * (v_x1_u32r >> 2)) >> 13) * p->dig_p3) >> 3) + (((int32_t)p->dig_p2 * v_x1_u32r) >> 1)) >> 18;
  v_x1_u32r = ((32768 + v_x1_u32r) * (int32_t)p->dig_p1) >> 15;

  if(v_x1_u32r == 0) {
    /* Avoid exception caused by division by zero */
    *press = 0;
    return;
  }

  uint32_t pressure = (((uint32_t)((int32_t)1048576 - upress)) - (v_x2_u32r >> 12)) * 3125;
  if((int32_t)pressure < 0) {
    pressure = (pressure << 1) / (uint32_t)v_x1_u32r;
  } else {
    pressure = (pressure / (uint32_t)v_x1_u32r) * 2;
  }

  v_x1_u32r = (((int32_t)(((pressure >> 3) * (pressure >> 3)) >> 13)) * (int32_t)p->dig_p9) >> 12;
  v_x2_u32r = ((int32_t)(pressure >> 2) * (int32_t)p->dig_p8) >> 13;
  pressure = (uint32_t)(((v_x1_u32r + v_x2_u32r + p->dig_p7) >> 4) + (int32_t)pressure);

  *press = pressure;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Returns a reading from the sensor.
 * \param type Parameter of type BMP_280_SENSOR_TYPE, choosing between either
 *             measuring temperature or pressure.
 * \return     Sensor data of either Temperature (centi degrees C) or
 *             Pressure (Pascal).
 */
static int
value(int type)
{
  int32_t temp = 0;
  uint32_t pres = 0;

  if(sensor_status != SENSOR_STATUS_READY) {
    PRINTF("Sensor disabled or starting up (%d)\n", sensor_status);
    return BMP_280_READING_ERROR;
  }

  /* A buffer for the raw reading from the sensor */
  uint8_t sensor_value[MEAS_DATA_SIZE];

  switch(type) {
  case BMP_280_SENSOR_TYPE_TEMP:
  case BMP_280_SENSOR_TYPE_PRESS:
    memset(sensor_value, 0, MEAS_DATA_SIZE);
    if(!read_data(sensor_value, MEAS_DATA_SIZE)) {
      return BMP_280_READING_ERROR;
    }

    PRINTF("val: %02x%02x%02x %02x%02x%02x\n",
           sensor_value[0], sensor_value[1], sensor_value[2],
           sensor_value[3], sensor_value[4], sensor_value[5]);

    convert(sensor_value, &temp, &pres);

    if(type == BMP_280_SENSOR_TYPE_TEMP) {
      return (int)temp;
    } else if(type == BMP_280_SENSOR_TYPE_PRESS) {
      return (int)pres;
    } else {
      return 0;
    }

  default:
    PRINTF("Invalid BMP 208 Sensor Type\n");
    return BMP_280_READING_ERROR;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief        Configuration function for the BMP280 sensor.
 * \param type   Activate, enable or disable the sensor. See below
 * \param enable Disable sensor if 0; else, enable sensor otherwise.
 *               When type == SENSORS_HW_INIT we turn on the hardware.
 *               When type == SENSORS_ACTIVE and enable==1 we enable the sensor.
 *               When type == SENSORS_ACTIVE and enable==0 we disable the sensor.
 */
static int
configure(int type, int enable)
{
  switch(type) {
  case SENSORS_HW_INIT:
    if(init()) {
      enable_sensor(false);
      sensor_status = SENSOR_STATUS_INITIALISED;
    } else {
      sensor_status = SENSOR_STATUS_DISABLED;
    }
    break;

  case SENSORS_ACTIVE:
    /* Must be initialised first */
    if(sensor_status == SENSOR_STATUS_DISABLED) {
      break;
    }
    if(enable) {
      enable_sensor(true);
      ctimer_set(&startup_timer, SENSOR_STARTUP_DELAY, notify_ready, NULL);
      sensor_status = SENSOR_STATUS_NOT_READY;
    } else {
      ctimer_stop(&startup_timer);
      enable_sensor(false);
      sensor_status = SENSOR_STATUS_INITIALISED;
    }
    break;

  default:
    break;
  }
  return sensor_status;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief      Returns the status of the sensor.
 * \param type SENSORS_ACTIVE or SENSORS_READY.
 * \return     Current status of the sensor.
 */
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return sensor_status;
  default:
    return SENSOR_STATUS_DISABLED;
  }
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(bmp_280_sensor, "BMP280", value, configure, status);
/*---------------------------------------------------------------------------*/
#endif /* BOARD_SENSORS_ENABLE */
/*---------------------------------------------------------------------------*/
/** @} */
