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
 * \addtogroup sensortag-opt-sensor
 * @{
 *
 * \file
 *        Driver for the Sensortag OPT-3001 light sensor.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/sensors.h"
#include "sys/ctimer.h"
/*---------------------------------------------------------------------------*/
#include "board-conf.h"
#include "opt-3001-sensor.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>

#include <ti/drivers/I2C.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
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
#ifndef Board_OPT3001_ADDR
#error "Board file doesn't define I2C address Board_OPT3001_ADDR"
#endif
/* Slave address */
#define OPT_3001_I2C_ADDRESS            Board_OPT3001_ADDR
/*---------------------------------------------------------------------------*/
/* Register addresses */
#define REG_RESULT                  0x00
#define REG_CONFIGURATION           0x01
#define REG_LOW_LIMIT               0x02
#define REG_HIGH_LIMIT              0x03

#define REG_MANUFACTURER_ID         0x7E
#define REG_DEVICE_ID               0x7F
/*---------------------------------------------------------------------------*/
/*
 * Configuration Register Bits and Masks.
 * We use uint16_t to read from / write to registers, meaning that the
 * register's MSB is the variable's LSB.
 */
#define CFG_RN                    0x00F0 /**< [15..12] Range Number */
#define CFG_CT                    0x0008 /**< [11] Conversion Time */
#define CFG_M                     0x0006 /**< [10..9] Mode of Conversion */
#define CFG_OVF                   0x0001 /**< [8] Overflow */
#define CFG_CRF                   0x8000 /**< [7] Conversion Ready Field */
#define CFG_FH                    0x4000 /**< [6] Flag High */
#define CFG_FL                    0x2000 /**< [5] Flag Low */
#define CFG_L                     0x1000 /**< [4] Latch */
#define CFG_POL                   0x0800 /**< [3] Polarity */
#define CFG_ME                    0x0400 /**< [2] Mask Exponent */
#define CFG_FC                    0x0300 /**< [1..0] Fault Count */
/*---------------------------------------------------------------------------*/
/* Possible Values for CT */
#define CFG_CT_100                0x0000
#define CFG_CT_800                CFG_CT
/*---------------------------------------------------------------------------*/
/* Possible Values for M */
#define CFG_M_CONTI               0x0004
#define CFG_M_SINGLE              0x0002
#define CFG_M_SHUTDOWN            0x0000
/*---------------------------------------------------------------------------*/
/* Reset Value for the register 0xC810. All zeros except: */
#define CFG_RN_RESET              0x00C0
#define CFG_CT_RESET              CFG_CT_800
#define CFG_L_RESET               0x1000
#define CFG_DEFAULTS              (CFG_RN_RESET | CFG_CT_100 | CFG_L_RESET)
/*---------------------------------------------------------------------------*/
/* Enable / Disable */
#define CFG_ENABLE_CONTINUOUS     (CFG_M_CONTI | CFG_DEFAULTS)
#define CFG_ENABLE_SINGLE_SHOT    (CFG_M_SINGLE | CFG_DEFAULTS)
#define CFG_DISABLE               CFG_DEFAULTS
/*---------------------------------------------------------------------------*/
/* Register length */
#define REGISTER_LENGTH           2
/*---------------------------------------------------------------------------*/
/* Sensor data size */
#define DATA_LENGTH               2
/*---------------------------------------------------------------------------*/
/* Byte swap of 16-bit register value */
#define HI_UINT16(a)              (((a) >> 8) & 0xFF)
#define LO_UINT16(a)              (((a) >> 0) & 0xFF)

#define SWAP16(v)                 ((LO_UINT16(v) << 8) | (HI_UINT16(v) << 0))

#define LSB16(v)                  (LO_UINT16(v)), (HI_UINT16(v))
#define MSB16(v)                  (HI_UINT16(v)), (LO_UINT16(v))
/*---------------------------------------------------------------------------*/
typedef struct {
  volatile OPT_3001_STATUS status;
} OPT_3001_Object;

static OPT_3001_Object opt_3001;
/*---------------------------------------------------------------------------*/
/* Wait SENSOR_STARTUP_DELAY for the sensor to be ready - 125ms */
#define SENSOR_STARTUP_DELAY (CLOCK_SECOND >> 3)

static struct ctimer startup_timer;
/*---------------------------------------------------------------------------*/
static I2C_Handle i2c_handle;
/*---------------------------------------------------------------------------*/
/**
 * \brief         Setup and peform an I2C transaction.
 * \param wbuf    Output buffer during the I2C transation.
 * \param wcount  How many bytes in the wbuf.
 * \param rbuf    Input buffer during the I2C transation.
 * \param rcount  How many bytes to read into rbuf.
 * \return        true if the I2C operation was successful;
 *                else, return false.
 */
static bool
i2c_write_read(void *wbuf, size_t wcount, void *rbuf, size_t rcount)
{
  I2C_Transaction i2c_transaction = {
    .writeBuf = wbuf,
    .writeCount = wcount,
    .readBuf = rbuf,
    .readCount = rcount,
    .slaveAddress = OPT_3001_I2C_ADDRESS,
  };

  return I2C_transfer(i2c_handle, &i2c_transaction);
}
/**
 * \brief         Peform a write only I2C transaction.
 * \param wbuf    Output buffer during the I2C transation.
 * \param wcount  How many bytes in the wbuf.
 * \return        true if the I2C operation was successful;
 *                else, return false.
 */
static inline bool
i2c_write(void *wbuf, size_t wcount)
{
  return i2c_write_read(wbuf, wcount, NULL, 0);
}
/**
 * \brief         Peform a read only I2C transaction.
 * \param rbuf    Input buffer during the I2C transation.
 * \param rcount  How many bytes to read into rbuf.
 * \return        true if the I2C operation was successful;
 *                else, return false.
 */
static inline bool
i2c_read(void *rbuf, size_t rcount)
{
  return i2c_write_read(NULL, 0, rbuf, rcount);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief   Initialize the OPT-3001 sensor driver.
 * \return  true if I2C operation successful; else, return false.
 */
static bool
sensor_init(void)
{
  if(i2c_handle) {
    return true;
  }

  I2C_Params i2c_params;
  I2C_Params_init(&i2c_params);

  i2c_params.transferMode = I2C_MODE_BLOCKING;
  i2c_params.bitRate = I2C_400kHz;

  i2c_handle = I2C_open(Board_I2C0, &i2c_params);
  if(i2c_handle == NULL) {
    return false;
  }

  opt_3001.status = OPT_3001_STATUS_DISABLED;

  return true;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief         Turn the sensor on/off
 * \param enable  Enable sensor if true; else, disable sensor.
 */
static bool
sensor_enable(bool enable)
{
  uint16_t data = (enable)
    ? CFG_ENABLE_SINGLE_SHOT
    : CFG_DISABLE;

  uint8_t cfg_data[] = { REG_CONFIGURATION, LSB16(data) };
  return i2c_write(cfg_data, sizeof(cfg_data));
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Callback when sensor is ready to read data from.
 */
static void
notify_ready_cb(void *unused)
{
  /* Unused args */
  (void)unused;

  /*
   * Depending on the CONFIGURATION.CONVERSION_TIME bits, a conversion will
   * take either 100 or 800 ms. Here we inspect the CONVERSION_READY bit and
   * if the reading is ready we notify, otherwise we just reschedule ourselves
   */

  uint8_t cfg_data[] = { REG_CONFIGURATION };
  uint16_t cfg_value = 0;

  bool spi_ok = i2c_write_read(cfg_data, sizeof(cfg_data), &cfg_value, sizeof(cfg_value));
  if(!spi_ok) {
    opt_3001.status = OPT_3001_STATUS_I2C_ERROR;
    return;
  }

  if(cfg_value & CFG_CRF) {
    opt_3001.status = OPT_3001_STATUS_DATA_READY;
    sensors_changed(&opt_3001_sensor);
  } else {
    ctimer_set(&startup_timer, SENSOR_STARTUP_DELAY, notify_ready_cb, NULL);
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief       Returns a reading from the sensor.
 * \param type  Ignored.
 * \return      Illuminance in centilux.
 */
static int
value(int type)
{
  /* Unused args */
  (void)type;

  if(opt_3001.status != OPT_3001_STATUS_DATA_READY) {
    return OPT_3001_READING_ERROR;
  }

  uint8_t cfg_data[] = { REG_CONFIGURATION };
  uint16_t cfg_value = 0;

  bool spi_ok = i2c_write_read(cfg_data, sizeof(cfg_data), &cfg_value, sizeof(cfg_value));
  if(!spi_ok) {
    opt_3001.status = OPT_3001_STATUS_I2C_ERROR;
    return OPT_3001_READING_ERROR;
  }

  uint8_t result_data[] = { REG_RESULT };
  uint16_t result_value = 0;

  spi_ok = i2c_write_read(result_data, sizeof(result_data), &result_value, sizeof(result_value));
  if(!spi_ok) {
    opt_3001.status = OPT_3001_STATUS_I2C_ERROR;
    return OPT_3001_READING_ERROR;
  }

  result_value = SWAP16(result_value);

  /* formula for computing lux: lux = 0.01 * 2^e * m
   * scale up by 100 to avoid floating point, then require
   * users to scale down by same.
   */
  uint32_t m = (result_value & 0x0FFF) >> 0;
  uint32_t e = (result_value & 0xF000) >> 12;
  uint32_t converted = m * (1 << e);

  PRINTF("OPT: %04X            r=%d (centilux)\n", result_value,
         (int)(converted));

  return (int)converted;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief         Configuration function for the OPT3001 sensor.
 * \param type    Activate, enable or disable the sensor. See below.
 * \param enable  Enable or disable sensor.
 *
 *                When type == SENSORS_HW_INIT we turn on the hardware.
 *                When type == SENSORS_ACTIVE and enable==1 we enable the sensor.
 *                When type == SENSORS_ACTIVE and enable==0 we disable the sensor.
 */
static int
configure(int type, int enable)
{
  int rv = 0;
  switch(type) {
  case SENSORS_HW_INIT:
    if(sensor_init()) {
      opt_3001.status = OPT_3001_STATUS_STANDBY;
    } else {
      opt_3001.status = OPT_3001_STATUS_DISABLED;
      rv = OPT_3001_READING_ERROR;
    }
    break;

  case SENSORS_ACTIVE:
    if(enable) {
      sensor_enable(true);
      ctimer_set(&startup_timer, SENSOR_STARTUP_DELAY, notify_ready_cb, NULL);

      opt_3001.status = OPT_3001_STATUS_BOOTING;
    } else {
      ctimer_stop(&startup_timer);
      sensor_enable(false);
    }
    break;

  default:
    break;
  }

  return rv;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief       Returns the status of the sensor.
 * \param type  Ignored.
 * \return      The state of the sensor SENSOR_STATE_xyz.
 */
static int
status(int type)
{
  /* Unused args */
  (void)type;

  return opt_3001.status;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(opt_3001_sensor, "OPT3001", value, configure, status);
/*---------------------------------------------------------------------------*/
#endif /* BOARD_SENSORS_ENABLE */
/*---------------------------------------------------------------------------*/
/** @} */
