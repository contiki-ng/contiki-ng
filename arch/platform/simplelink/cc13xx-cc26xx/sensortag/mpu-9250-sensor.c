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
 * \addtogroup sensortag-mpu
 * @{
 *
 * \file
 *        Driver for the Sensortag Invensense MPU9250 motion processing unit
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "lib/sensors.h"
#include "sys/rtimer.h"
#include "dev/i2c-arch.h"
/*---------------------------------------------------------------------------*/
#include "board-conf.h"
#include "mpu-9250-sensor.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/cpu.h)

#include <ti/drivers/PIN.h>
#include <ti/drivers/I2C.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
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
#ifndef Board_MPU9250_ADDR
#error "Board file doesn't define I2C address Board_MPU9250_ADDR"
#endif
#ifndef Board_MPU9250_MAG_ADDR
#error "Board file doesn't define I2C address Board_MPU9250_MAG_ADDR"
#endif

/* Sensor I2C address */
#define MPU_9250_I2C_ADDRESS          Board_MPU9250_ADDR
#define MPU_9250_MAG_I2C_ADDRESS      Board_MPU9250_MAG_ADDR
/*-------------a--------------------------------------------------------------*/
/* Self Test Registers */
#define REG_SELF_TEST_X_GYRO          0x00 /* R/W */
#define REG_SELF_TEST_Y_GYRO          0x01 /* R/W */
#define REG_SELF_TEST_Z_GYRO          0x02 /* R/W */
#define REG_SELF_TEST_X_ACCEL         0x0D /* R/W */
#define REG_SELF_TEST_Z_ACCEL         0x0E /* R/W */
#define REG_SELF_TEST_Y_ACCEL         0x0F /* R/W */
/*---------------------------------------------------------------------------*/
/* Axis Registers */
#define REG_XG_OFFSET_H               0x13 /* R/W */
#define REG_XG_OFFSET_L               0x14 /* R/W */
#define REG_YG_OFFSET_H               0x15 /* R/W */
#define REG_YG_OFFSET_L               0x16 /* R/W */
#define REG_ZG_OFFSET_H               0x17 /* R/W */
#define REG_ZG_OFFSET_L               0x18 /* R/W */
/*---------------------------------------------------------------------------*/
/* Control Registers */
#define REG_SMPLRT_DIV                0x19 /* R/W */
#define REG_CONFIG                    0x1A /* R/W */
#define REG_GYRO_CONFIG               0x1B /* R/W */
#define REG_ACCEL_CONFIG              0x1C /* R/W */
#define REG_ACCEL_CONFIG_2            0x1D /* R/W */
#define REG_LP_ACCEL_ODR              0x1E /* R/W */
#define REG_WOM_THR                   0x1F /* R/W */
#define REG_FIFO_EN                   0x23 /* R/W */
/*---------------------------------------------------------------------------*/
/*
 * Registers 0x24 - 0x36 are not applicable to the SensorTag HW configuration
 * (IC2 Master)
 */
#define REG_INT_PIN_CFG               0x37 /* R/W */
#define REG_INT_ENABLE                0x38 /* R/W */
#define REG_INT_STATUS                0x3A /* R */
#define REG_ACCEL_XOUT_H              0x3B /* R */
#define REG_ACCEL_XOUT_L              0x3C /* R */
#define REG_ACCEL_YOUT_H              0x3D /* R */
#define REG_ACCEL_YOUT_L              0x3E /* R */
#define REG_ACCEL_ZOUT_H              0x3F /* R */
#define REG_ACCEL_ZOUT_L              0x40 /* R */
#define REG_TEMP_OUT_H                0x41 /* R */
#define REG_TEMP_OUT_L                0x42 /* R */
#define REG_GYRO_XOUT_H               0x43 /* R */
#define REG_GYRO_XOUT_L               0x44 /* R */
#define REG_GYRO_YOUT_H               0x45 /* R */
#define REG_GYRO_YOUT_L               0x46 /* R */
#define REG_GYRO_ZOUT_H               0x47 /* R */
#define REG_GYRO_ZOUT_L               0x48 /* R */
/*---------------------------------------------------------------------------*/
/*
 * Registers 0x49 - 0x60 are not applicable to the SensorTag HW configuration
 * (external sensor data)
 *
 * Registers 0x63 - 0x67 are not applicable to the SensorTag HW configuration
 * (I2C master)
 */
#define REG_SIG_PATH_RST              0x68 /* R/W */
#define REG_ACC_INTEL_CTRL            0x69 /* R/W */
#define REG_USER_CTRL                 0x6A /* R/W */
#define REG_PWR_MGMT_1                0x6B /* R/W */
#define REG_PWR_MGMT_2                0x6C /* R/W */
#define REG_FIFO_COUNT_H              0x72 /* R/W */
#define REG_FIFO_COUNT_L              0x73 /* R/W */
#define REG_FIFO_R_W                  0x74 /* R/W */
#define REG_WHO_AM_I                  0x75 /* R/W */
/*---------------------------------------------------------------------------*/
/* Masks is mpuConfig valiable */
#define ACC_CONFIG_MASK               0x38
#define GYRO_CONFIG_MASK              0x07
/*---------------------------------------------------------------------------*/
/* Values PWR_MGMT_1 */
#define PWR_MGMT_1_VAL_MPU_SLEEP      0x4F  /* Sleep + stop all clocks */
#define PWR_MGMT_1_VAL_MPU_WAKE_UP    0x09  /* Disable temp. + intern osc */
/*---------------------------------------------------------------------------*/
/* Values PWR_MGMT_2 */
#define PWR_MGMT_2_VAL_ALL_AXES       0x3F
#define PWR_MGMT_2_VAL_GYRO_AXES      0x07
#define PWR_MGMT_2_VAL_ACC_AXES       0x38
/*---------------------------------------------------------------------------*/
/* Output data rates */
#define INV_LPA_0_3125HZ              0
#define INV_LPA_0_625HZ               1
#define INV_LPA_1_25HZ                2
#define INV_LPA_2_5HZ                 3
#define INV_LPA_5HZ                   4
#define INV_LPA_10HZ                  5
#define INV_LPA_20HZ                  6
#define INV_LPA_40HZ                  7
#define INV_LPA_80HZ                  8
#define INV_LPA_160HZ                 9
#define INV_LPA_320HZ                 10
#define INV_LPA_640HZ                 11
#define INV_LPA_STOPPED               255
/*---------------------------------------------------------------------------*/
/* Bit values */
#define BIT_ANY_RD_CLR                0x10
#define BIT_RAW_RDY_EN                0x01
#define BIT_WOM_EN                    0x40
#define BIT_LPA_CYCLE                 0x20
#define BIT_STBY_XA                   0x20
#define BIT_STBY_YA                   0x10
#define BIT_STBY_ZA                   0x08
#define BIT_STBY_XG                   0x04
#define BIT_STBY_YG                   0x02
#define BIT_STBY_ZG                   0x01
#define BIT_STBY_XYZA                 (BIT_STBY_XA | BIT_STBY_YA | BIT_STBY_ZA)
#define BIT_STBY_XYZG                 (BIT_STBY_XG | BIT_STBY_YG | BIT_STBY_ZG)
/*---------------------------------------------------------------------------*/
static PIN_Config mpu_9250_pin_table[] = {
  Board_MPU_INT | PIN_INPUT_EN | PIN_PULLDOWN | PIN_HYSTERESIS,
  Board_MPU_POWER | PIN_GPIO_OUTPUT_EN | PIN_DRVSTR_MAX | PIN_GPIO_LOW,
  PIN_TERMINATE
};

static PIN_State pin_state;
static PIN_Handle pin_handle;
static I2C_Handle i2c_handle;

/*---------------------------------------------------------------------------*/
typedef struct {
  volatile MPU_9250_SENSOR_STATUS status;
  volatile MPU_9250_SENSOR_TYPE type;
  MPU_9250_SENSOR_ACC_RANGE acc_range;
} MPU_9250_Object;

static MPU_9250_Object mpu_9250;
/*---------------------------------------------------------------------------*/
/* 3 16-byte words for all sensor readings */
#define SENSOR_DATA_BUF_SIZE   3
/* Data sizes */
#define DATA_SIZE              6
/*---------------------------------------------------------------------------*/
/*
 * Wait SENSOR_BOOT_DELAY ticks for the sensor to boot and
 * SENSOR_STARTUP_DELAY for readings to be ready
 * Gyro is a little slower than Acc
 */
#define SENSOR_BOOT_DELAY     8
#define SENSOR_STARTUP_DELAY  5

static struct ctimer startup_timer;
/*---------------------------------------------------------------------------*/

/*
 * Wait timeout in rtimer ticks. This is just a random low number, since the
 * first time we read the sensor status, it should be ready to return data
 */
#define READING_WAIT_TIMEOUT 10
/*---------------------------------------------------------------------------*/
/* Code in flash, cache disabled: 7 cycles per loop */
/* ui32Count = [delay in us] * [CPU clock in MHz] / [cycles per loop] */
#define delay_ms(ms)    CPUdelay((ms) * 1000 * 48 / 7)
/*---------------------------------------------------------------------------*/
/**
 * \brief   Initialize the MPU-9250 sensor driver.
 * \return  true if I2C operation successful; else, return false.
 */
static bool
sensor_init(void)
{
  pin_handle = PIN_open(&pin_state, mpu_9250_pin_table);
  if(pin_handle == NULL) {
    return false;
  }

  mpu_9250.type = MPU_9250_SENSOR_TYPE_NONE;
  mpu_9250.status = MPU_9250_SENSOR_STATUS_DISABLED;
  mpu_9250.acc_range = MPU_9250_SENSOR_ACC_RANGE_ARG;

  return true;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Place the sensor in low-power mode.
 */
static void
sensor_sleep(void)
{
  {
    uint8_t all_axes_data[] = { REG_PWR_MGMT_2, PWR_MGMT_2_VAL_ALL_AXES };
    i2c_arch_write(i2c_handle, MPU_9250_I2C_ADDRESS, all_axes_data, sizeof(all_axes_data));
  }
  {
    uint8_t mpu_sleep_data[] = { REG_PWR_MGMT_1, PWR_MGMT_1_VAL_MPU_SLEEP };
    i2c_arch_write(i2c_handle, MPU_9250_I2C_ADDRESS, mpu_sleep_data, sizeof(mpu_sleep_data));
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief  Wakeup the sensor from low-power mode.
 */
static void
sensor_wakeup(void)
{
  {
    uint8_t mpu_wakeup_data[] = { REG_PWR_MGMT_1, PWR_MGMT_1_VAL_MPU_WAKE_UP };
    i2c_arch_write(i2c_handle, MPU_9250_I2C_ADDRESS, mpu_wakeup_data, sizeof(mpu_wakeup_data));
  }
  {
    /* All axis initially disabled */
    uint8_t all_axes_data[] = { REG_PWR_MGMT_2, PWR_MGMT_2_VAL_ALL_AXES };
    i2c_arch_write(i2c_handle, MPU_9250_I2C_ADDRESS, all_axes_data, sizeof(all_axes_data));
  }
  {
    /* Restore the range */
    uint8_t accel_cfg_data[] = { REG_ACCEL_CONFIG, mpu_9250.acc_range };
    i2c_arch_write(i2c_handle, MPU_9250_I2C_ADDRESS, accel_cfg_data, sizeof(accel_cfg_data));
  }
  {
    /* Clear interrupts */
    uint8_t int_status_data[] = { REG_INT_STATUS };
    uint8_t dummy;
    i2c_arch_write_read(i2c_handle, MPU_9250_I2C_ADDRESS, int_status_data, sizeof(int_status_data), &dummy, 1);
  }
}
/*---------------------------------------------------------------------------*/
static void
sensor_set_acc_range(MPU_9250_SENSOR_ACC_RANGE acc_range)
{
  /* Apply the range */
  uint8_t accel_cfg_data[] = { REG_ACCEL_CONFIG, acc_range };
  i2c_arch_write(i2c_handle, MPU_9250_I2C_ADDRESS, accel_cfg_data, sizeof(accel_cfg_data));
}
/*---------------------------------------------------------------------------*/
static void
sensor_set_axes(MPU_9250_SENSOR_TYPE sensor_type)
{
  uint8_t _data[] = { REG_PWR_MGMT_2, ~(uint8_t)sensor_type };
  i2c_arch_write(i2c_handle, MPU_9250_I2C_ADDRESS, _data, sizeof(_data));
}
/*---------------------------------------------------------------------------*/
static void
convert_to_le(uint8_t *data, uint8_t len)
{
  int i;
  for(i = 0; i < len; i += 2) {
    uint8_t tmp;
    tmp = data[i];
    data[i] = data[i + 1];
    data[i + 1] = tmp;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief   Check whether a data or wake on motion interrupt has occurred.
 * \return  Return the interrupt status.
 *
 *          This driver does not use interrupts, however this function allows
 *          us to determine whether a new sensor reading is available.
 */
static bool
sensor_data_ready(uint8_t *int_status)
{
  uint8_t int_status_data[] = { REG_INT_STATUS };
  const bool spi_ok = i2c_arch_write_read(i2c_handle, MPU_9250_I2C_ADDRESS, int_status_data, sizeof(int_status_data), int_status, 1);

  return spi_ok && (*int_status != 0);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief   Read data from the accelerometer, total of 3 words (X, Y, Z).
 * \return  true if a valid reading could be taken; otherwise, false.
 */
static bool
acc_read(uint8_t int_status, uint16_t *data)
{
  if(!(int_status & BIT_RAW_RDY_EN)) {
    return false;
  }

  /* Burst read of all accelerometer values */
  uint8_t accel_xout_h[] = { REG_ACCEL_XOUT_H };
  bool spi_ok = i2c_arch_write_read(i2c_handle, MPU_9250_I2C_ADDRESS, accel_xout_h, sizeof(accel_xout_h), data, DATA_SIZE);
  if(!spi_ok) {
    return false;
  }

  convert_to_le((uint8_t *)data, DATA_SIZE);

  return true;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief   Read data from the accelerometer, total of 3 words (X, Y, Z).
 * \return  true if a valid reading could be taken; otherwise, false.
 */
static bool
gyro_read(uint8_t int_status, uint16_t *data)
{
  if(!(int_status & BIT_RAW_RDY_EN)) {
    return false;
  }

  /* Burst read of all accelerometer values */
  uint8_t gyro_xout_h[] = { REG_GYRO_XOUT_H };
  bool spi_ok = i2c_arch_write_read(i2c_handle, MPU_9250_I2C_ADDRESS, gyro_xout_h, sizeof(gyro_xout_h), data, DATA_SIZE);
  if(!spi_ok) {
    return false;
  }

  convert_to_le((uint8_t *)data, DATA_SIZE);

  return true;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief           Convert accelerometer raw reading to a value in G.
 * \param raw_data  The raw accelerometer reading.
 * \return          The converted value.
 */
static int32_t
acc_convert(int32_t raw_data)
{
  switch(mpu_9250.acc_range) {
  case MPU_9250_SENSOR_ACC_RANGE_2G:  return raw_data * 100 * 2 / 32768;
  case MPU_9250_SENSOR_ACC_RANGE_4G:  return raw_data * 100 * 4 / 32768;
  case MPU_9250_SENSOR_ACC_RANGE_8G:  return raw_data * 100 * 8 / 32768;
  case MPU_9250_SENSOR_ACC_RANGE_16G: return raw_data * 100 * 16 / 32768;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief           Convert gyro raw reading to a value in deg/sec.
 * \param raw_data  The raw accelerometer reading.
 * \return          The converted value.
 */
static int32_t
gyro_convert(int32_t raw_data)
{
  /* calculate rotation, unit deg/s, range -250, +250 */
  return raw_data * 100 * 500 / 65536;
}
/*---------------------------------------------------------------------------*/
static void
notify_ready_cb(void *unused)
{
  (void)unused;

  mpu_9250.status = MPU_9250_SENSOR_STATUS_READY;
  sensors_changed(&mpu_9250_sensor);
}
/*---------------------------------------------------------------------------*/
static void
initialise_cb(void *unused)
{
  (void)unused;

  if(mpu_9250.type == MPU_9250_SENSOR_TYPE_NONE) {
    return;
  }

  i2c_handle = i2c_arch_acquire(Board_I2C1);

  if(!i2c_handle) {
    return;
  }

  /* Wake up the sensor */
  sensor_wakeup();

  /* Configure the accelerometer range */
  if((mpu_9250.type & MPU_9250_SENSOR_TYPE_ACC) != 0) {
    sensor_set_acc_range(mpu_9250.acc_range);
  }

  /* Enable gyro + accelerometer readout */
  sensor_set_axes(mpu_9250.type);
  delay_ms(10);

  i2c_arch_release(i2c_handle);

  ctimer_set(&startup_timer, SENSOR_STARTUP_DELAY, notify_ready_cb, NULL);
}
/*---------------------------------------------------------------------------*/
/**
 * \brief       Returns a reading from the sensor.
 * \param type  MPU_9250_SENSOR_TYPE_ACC_[XYZ] or
 *              MPU_9250_SENSOR_TYPE_GYRO_[XYZ].
 * \return      Centi-G (ACC) or centi-Deg/Sec (Gyro).
 */
static int
value(int type)
{
  if(mpu_9250.status == MPU_9250_SENSOR_STATUS_DISABLED) {
    PRINTF("MPU: Sensor Disabled\n");
    return MPU_9250_READING_ERROR;
  }

  if(mpu_9250.type == MPU_9250_SENSOR_TYPE_NONE) {
    return MPU_9250_READING_ERROR;
  }

  i2c_handle = i2c_arch_acquire(Board_I2C1);

  if(!i2c_handle) {
    return MPU_9250_READING_ERROR;
  }

  uint8_t int_status = 0;
  const rtimer_clock_t t0 = RTIMER_NOW();
  while(!sensor_data_ready(&int_status)) {
    if(!(RTIMER_CLOCK_LT(RTIMER_NOW(), t0 + READING_WAIT_TIMEOUT))) {
      i2c_arch_release(i2c_handle);
      return MPU_9250_READING_ERROR;
    }
  }

  uint16_t sensor_value[SENSOR_DATA_BUF_SIZE];
  memset(sensor_value, 0, sizeof(sensor_value));

  /* Read accel data */
  if((type & MPU_9250_SENSOR_TYPE_ACC) != 0) {

    if(!acc_read(int_status, sensor_value)) {
      i2c_arch_release(i2c_handle);
      return MPU_9250_READING_ERROR;
    }

    i2c_arch_release(i2c_handle);

    PRINTF("MPU: ACC = 0x%04x 0x%04x 0x%04x = ",
           sensor_value[0], sensor_value[1], sensor_value[2]);

    /* Convert */
    switch(type) {
    case MPU_9250_SENSOR_TYPE_ACC_X: return acc_convert(sensor_value[0]);
    case MPU_9250_SENSOR_TYPE_ACC_Y: return acc_convert(sensor_value[1]);
    case MPU_9250_SENSOR_TYPE_ACC_Z: return acc_convert(sensor_value[2]);
    default:                         return MPU_9250_READING_ERROR;
    }

    /* Read gyro data */
  } else if((type & MPU_9250_SENSOR_TYPE_GYRO) != 0) {

    if(!gyro_read(int_status, sensor_value)) {
      i2c_arch_release(i2c_handle);
      return MPU_9250_READING_ERROR;
    }

    i2c_arch_release(i2c_handle);

    PRINTF("MPU: Gyro = 0x%04x 0x%04x 0x%04x = ",
           sensor_value[0], sensor_value[1], sensor_value[2]);

    /* Convert */
    switch(type) {
    case MPU_9250_SENSOR_TYPE_GYRO_X: return gyro_convert(sensor_value[0]);
    case MPU_9250_SENSOR_TYPE_GYRO_Y: return gyro_convert(sensor_value[1]);
    case MPU_9250_SENSOR_TYPE_GYRO_Z: return gyro_convert(sensor_value[2]);
    default:                          return MPU_9250_READING_ERROR;
    }

    /* Invalid sensor type */
  } else {
    PRINTF("MPU: Invalid type\n");
    return MPU_9250_READING_ERROR;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * \brief         Configuration function for the MPU9250 sensor.
 * \param type    Activate, enable or disable the sensor. See below.
 * \param enable  Enable or disable sensor.
 *                When type == SENSORS_HW_INIT we turn on the hardware.
 *                When type == SENSORS_ACTIVE and enable==1 we enable the sensor.
 *                When type == SENSORS_ACTIVE and enable==0 we disable the sensor.
 */
static int
configure(int type, int enable)
{
  /* Mask enable */
  const MPU_9250_SENSOR_TYPE enable_type = enable & MPU_9250_SENSOR_TYPE_ALL;

  switch(type) {
  case SENSORS_HW_INIT:
    if(sensor_init()) {
      mpu_9250.status = MPU_9250_SENSOR_STATUS_ENABLED;
    } else {
      mpu_9250.status = MPU_9250_SENSOR_STATUS_DISABLED;
    }
    break;

  case SENSORS_ACTIVE:
    if(enable_type != MPU_9250_SENSOR_TYPE_NONE) {
      PRINTF("MPU: Enabling\n");

      mpu_9250.type = enable_type;
      mpu_9250.status = MPU_9250_SENSOR_STATUS_BOOTING;

      PIN_setOutputValue(pin_handle, Board_MPU_POWER, 1);

      ctimer_set(&startup_timer, SENSOR_BOOT_DELAY, initialise_cb, NULL);
    } else {
      PRINTF("MPU: Disabling\n");

      ctimer_stop(&startup_timer);

      if(PIN_getOutputValue(Board_MPU_POWER)) {
        i2c_handle = i2c_arch_acquire(Board_I2C1);

        if(!i2c_handle) {
          PIN_setOutputValue(pin_handle, Board_MPU_POWER, 0);

          return MPU_9250_SENSOR_STATUS_DISABLED;
        }

        sensor_sleep();

        i2c_arch_release(i2c_handle);

        PIN_setOutputValue(pin_handle, Board_MPU_POWER, 0);
      }

      mpu_9250.type = MPU_9250_SENSOR_TYPE_NONE;
      mpu_9250.status = MPU_9250_SENSOR_STATUS_DISABLED;
    }
    break;

  default:
    break;
  }
  return mpu_9250.status;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief       Returns the status of the sensor
 * \param type  SENSORS_ACTIVE or SENSORS_READY
 * \return      1 if the sensor is enabled, else 0.
 */
static int
status(int type)
{
  switch(type) {
  case SENSORS_ACTIVE:
  case SENSORS_READY:
    return mpu_9250.status;

  default:
    return MPU_9250_SENSOR_STATUS_DISABLED;
  }
}
/*---------------------------------------------------------------------------*/
SENSORS_SENSOR(mpu_9250_sensor, "MPU9250", value, configure, status);
/*---------------------------------------------------------------------------*/
#endif /* BOARD_SENSORS_ENABLE */
/*---------------------------------------------------------------------------*/
/** @} */
