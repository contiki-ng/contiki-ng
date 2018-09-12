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
 * \addtogroup sensortag-tmp-sensor
 * @{
 *
 * \file
 *        Driver for the Sensortag TI TMP-007 IR Thermophile sensor.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/sensors.h"
#include "sys/ctimer.h"
/*---------------------------------------------------------------------------*/
#include "board-conf.h"
#include "tmp-007-sensor.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>

#include <ti/drivers/I2C.h>
#include <ti/drivers/PIN.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
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
/* Slave address */
#ifndef Board_TMP_ADDR
#error "Board file doesn't define I2C address Board_TMP_ADDR"
#endif
#define TMP_007_I2C_ADDRESS     Board_TMP_ADDR

/* Sensor Interrupt pin */
#ifndef Board_TMP_RDY
#error "Board file doesn't define interrupt pin Board_TMP_RDY"
#endif
#define TMP_007_TMP_RDY         Board_TMP_RDY
/*---------------------------------------------------------------------------*/
/* TMP-007 register addresses */
#define REG_VOLTAGE             0x00
#define REG_LOCAL_TEMP          0x01
#define REG_CONFIG              0x02
#define REG_OBJ_TEMP            0x03
#define REG_STATUS              0x04
#define REG_PROD_ID             0x1F
/*---------------------------------------------------------------------------*/
/* TMP-007 register values */
#define VAL_CONFIG_ON           0x1000  /**< Sensor on state */
#define VAL_CONFIG_OFF          0x0000  /**< Sensor off state */
#define VAL_CONFIG_RESET        0x8000
#define VAL_PROD_ID             0x0078  /**< Product ID */
/*---------------------------------------------------------------------------*/
/* Conversion ready (status register) bit values */
#define CONV_RDY_BIT            0x4000
/*---------------------------------------------------------------------------*/
/* Register length */
#define REGISTER_LENGTH         2
/*---------------------------------------------------------------------------*/
/* Sensor data size */
#define DATA_SIZE               4
/*---------------------------------------------------------------------------*/
/* Byte swap of 16-bit register value */
#define HI_UINT16(a)            (((a) >> 8) & 0xFF)
#define LO_UINT16(a)            (((a) >> 0) & 0xFF)

#define SWAP16(v)               ((LO_UINT16(v) << 8) | (HI_UINT16(v) << 0))

#define LSB16(v)                (LO_UINT16(v)), (HI_UINT16(v))
#define MSB16(v)                (HI_UINT16(v)), (LO_UINT16(v))
/*---------------------------------------------------------------------------*/
static const PIN_Config pin_table[] = {
  TMP_007_TMP_RDY | PIN_INPUT_EN | PIN_PULLUP | PIN_HYSTERESIS | PIN_IRQ_NEGEDGE,
  PIN_TERMINATE
};

static PIN_State pin_state;
static PIN_Handle pin_handle;

static I2C_Handle i2c_handle;
/*---------------------------------------------------------------------------*/
typedef struct {
  TMP_007_TYPE type;
  volatile TMP_007_STATUS status;
  uint16_t local_tmp_latched;
  uint16_t obj_tmp_latched;
} TMP_007_Object;

static TMP_007_Object tmp_007;
/*---------------------------------------------------------------------------*/
/* Wait SENSOR_STARTUP_DELAY clock ticks for the sensor to be ready - 275ms */
#define SENSOR_STARTUP_DELAY    36

static struct ctimer startup_timer;
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
    .slaveAddress = TMP_007_I2C_ADDRESS,
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
 * \brief   Initialize the TMP-007 sensor driver.
 * \return  true if I2C operation successful; else, return false.
 */
static bool
sensor_init(void)
{
  if(pin_handle && i2c_handle) {
    return true;
  }

  pin_handle = PIN_open(&pin_state, pin_table);
  if(!pin_handle) {
    return false;
  }

  I2C_Params i2c_params;
  I2C_Params_init(&i2c_params);

  i2c_params.transferMode = I2C_MODE_BLOCKING;
  i2c_params.bitRate = I2C_400kHz;

  i2c_handle = I2C_open(Board_I2C0, &i2c_params);
  if(i2c_handle == NULL) {
    PIN_close(pin_handle);
    return false;
  }

  tmp_007.status = TMP_007_STATUS_DISABLED;

  return true;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Callback when sensor is ready to read data from.
 */
static void
notify_ready_cb(void *not_used)
{
  tmp_007.status = TMP_007_STATUS_READY;
  sensors_changed(&tmp_007_sensor);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Turn the sensor on or off.
 */
static bool
enable_sensor(bool enable)
{
  uint16_t cfg_value = (enable)
    ? VAL_CONFIG_ON
    : VAL_CONFIG_OFF;

  uint8_t cfg_data[] = { REG_CONFIG, LSB16(cfg_value) };

  return i2c_write(cfg_data, sizeof(cfg_data));
}
/*---------------------------------------------------------------------------*/
/**
 * \brief            Read the sensor value registers.
 * \param local_tmp  Output variable holding the Temperature in
 *                   16-bit format.
 * \param obj_tmp    Output variable holding the Object temperature in
 *                   16-bit format.
 * \return           true if valid data could be retrieved; else, false.
 */
static bool
read_data(uint16_t *local_tmp, uint16_t *obj_tmp)
{
  bool spi_ok = false;

  uint8_t status_data[] = { REG_STATUS };
  uint16_t status_value = 0;

  spi_ok = i2c_write_read(status_data, sizeof(status_data),
                          &status_value, sizeof(status_value));
  if(!spi_ok) {
    return false;
  }
  status_value = SWAP16(status_value);

  if((status_value & CONV_RDY_BIT) == 0) {
    return false;
  }

  uint8_t local_temp_data[] = { REG_LOCAL_TEMP };
  uint16_t local_temp_value = 0;

  spi_ok = i2c_write_read(local_temp_data, sizeof(local_temp_data),
                          &local_temp_value, sizeof(local_temp_value));
  if(!spi_ok) {
    return false;
  }

  uint8_t obj_temp_data[] = { REG_OBJ_TEMP };
  uint16_t obj_temp_value = 0;

  spi_ok = i2c_write_read(obj_temp_data, sizeof(obj_temp_data),
                          &obj_temp_value, sizeof(obj_temp_value));
  if(!spi_ok) {
    return false;
  }

  *local_tmp = SWAP16(local_temp_value);
  *obj_tmp = SWAP16(obj_temp_value);

  return true;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief            Convert raw data to values in degrees Celsius.
 * \param local_tmp  Output variable holding the raw ambient temperature
 *                   from sensor.
 * \param obj_tmp    Output variable holding the raw object temperature
 *                   from sensor.
 */
static void
convert(uint16_t *local_tmp, uint16_t *obj_tmp)
{
  uint32_t local = (uint32_t)*local_tmp;
  uint32_t obj = (uint32_t)*obj_tmp;

  local = (local >> 2) * 3125 / 100;
  obj = (obj >> 2) * 3125 / 100;

  *local_tmp = (uint16_t)local;
  *obj_tmp = (uint16_t)obj;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief       Returns a reading from the sensor.
 * \param type  TMP_007_SENSOR_TYPE_OBJECT or TMP_007_SENSOR_TYPE_AMBIENT.
 * \return      Object or Ambient temperature in milli degrees Celsius.
 */
static int
value(int type)
{
  uint16_t raw_local_tmp = 0, local_tmp = 0;
  uint16_t raw_obj_tmp = 0, obj_tmp = 0;

  if(tmp_007.status != TMP_007_STATUS_READY) {
    PRINTF("Sensor disabled or starting up (%d)\n", tmp_007.status);
    return TMP_007_READING_ERROR;
  }

  switch(type) {
  case TMP_007_TYPE_OBJECT:  return tmp_007.obj_tmp_latched;
  case TMP_007_TYPE_AMBIENT: return tmp_007.local_tmp_latched;

  case TMP_007_TYPE_ALL:
    if(!read_data(&raw_local_tmp, &raw_obj_tmp)) {
      return TMP_007_READING_ERROR;
    }

    local_tmp = raw_local_tmp;
    obj_tmp = raw_obj_tmp;
    convert(&local_tmp, &obj_tmp);

    PRINTF("TMP: %04X %04X       o=%d a=%d\n", raw_local_tmp, raw_obj_tmp,
           (int)(local_tmp), (int)(obj_tmp));

    tmp_007.local_tmp_latched = (int)(local_tmp);
    tmp_007.obj_tmp_latched = (int)(obj_tmp);

    return 0;

  default:
    PRINTF("Invalid type (%d)\n", type);
    return TMP_007_READING_ERROR;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief         Configuration function for the TMP-007 sensor.
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
  switch(type) {
  case SENSORS_HW_INIT:
    if(!sensor_init()) {
      return TMP_007_STATUS_DISABLED;
    }

    enable_sensor(false);

    tmp_007.status = TMP_007_STATUS_INITIALIZED;
    break;

  case SENSORS_ACTIVE:
    /* Must be initialised first */
    if(tmp_007.status == TMP_007_STATUS_DISABLED) {
      return TMP_007_STATUS_DISABLED;
    }
    if(enable) {
      enable_sensor(true);
      ctimer_set(&startup_timer, SENSOR_STARTUP_DELAY, notify_ready_cb, NULL);
      tmp_007.status = TMP_007_STATUS_NOT_READY;
    } else {
      ctimer_stop(&startup_timer);
      enable_sensor(false);
      tmp_007.status = TMP_007_STATUS_INITIALIZED;
    }
    break;

  default:
    break;
  }

  return tmp_007.status;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief       Returns the status of the sensor.
 * \param type  Ignored.
 * \return      Status of the sensor.
 */
static int
status(int type)
{
  (void)type;

  return tmp_007.status;
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(tmp_007_sensor, "TMP-007", value, configure, status);
/*---------------------------------------------------------------------------*/
#endif /* BOARD_SENSORS_ENABLE */
/*---------------------------------------------------------------------------*/
/** @} */
