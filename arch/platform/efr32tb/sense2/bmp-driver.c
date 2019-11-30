#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include "dev/i2c-hal.h"
#include "sys/clock.h"
#include "dev/bmp280/bmp280.h"
#include "em_gpio.h"
#include "bmp-driver.h"

/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "BMP"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/

#define BMP_REG_ADDR_ID                    0xD0 /* Chip ID */

#ifndef BMP_I2C_BUS
#error BMP_I2C_BUS must be set!
#endif /* BMP_I2C_BUS */

extern i2c_hal_bus_t BMP_I2C_BUS;

static i2c_hal_device_t bmp_sens = {
  .bus = &BMP_I2C_BUS,
  .speed = I2C_HAL_NORMAL_BUS_SPEED,
  .timeout = 1000,
  .address = 0x77 << 1
};

/* functions for the BMP280 code */
static int8_t  i2c_bus_read(uint8_t devAddr, uint8_t regAddr, uint8_t *regData, uint8_t count);
static int8_t  i2c_bus_write(uint8_t devAddr, uint8_t regAddr, uint8_t *regData, uint8_t count);

static uint8_t bmpDeviceId;       /* The device ID of the connected chip  */
static uint8_t bmp280_power_mode;   /* The actual power mode of the BMP280  */
static struct  bmp280_t bmp280;   /* Structure to hold BMP280 driver data */

static void
delay_ms(uint32_t ms)
{
  clock_delay_usec(ms * 1000);
}

uint32_t bmp_init(uint8_t *deviceId)
{
  int result;

  /* Enable power to the pressure sensor */
  GPIO_PinModeSet(BOARD_ENV_ENABLE_PORT, BOARD_ENV_ENABLE_PIN,
                  gpioModePushPull, 1);

  /* The device needs 2 ms startup time */
  clock_delay_usec(2000);

  if(i2c_hal_acquire(&bmp_sens) != I2C_HAL_STATUS_OK) {
    LOG_WARN("failed to acquire I2C\n");
    return BMP_ERROR_I2C_TRANSACTION_FAILED;
  }

  /* Read device ID to determine if we have a BMP280 connected */
  i2c_hal_read_register(&bmp_sens, BMP_REG_ADDR_ID, &bmpDeviceId, 1);

  LOG_INFO("REG_ID: %d\n", bmpDeviceId);

  if(bmpDeviceId != BMP_DEVICE_ID_BMP280) {
    LOG_WARN("device id mismatch: %u != %u\n", bmpDeviceId, BMP_DEVICE_ID_BMP280);
    return BMP_ERROR_DEVICE_ID_MISMATCH;
  }

  bmp280.bus_write  = i2c_bus_write;
  bmp280.bus_read   = i2c_bus_read;
  bmp280.dev_addr   = bmp_sens.address;
  bmp280.delay_msec = delay_ms;

  result = bmp280_init(&bmp280);

  if(result != BMP_OK ) {
    return result;
  }

  result = bmp280_set_power_mode(BMP280_FORCED_MODE);

  if(result != BMP_OK ) {
    return result;
  }

  result = bmp280_set_work_mode(BMP280_ULTRA_HIGH_RESOLUTION_MODE);

  if(result != BMP_OK) {
    return result;
  }

  bmp280_power_mode = BMP280_FORCED_MODE;

  *deviceId = bmpDeviceId;

  if(i2c_hal_release(&bmp_sens) != I2C_HAL_STATUS_OK) {
    return BMP_ERROR_I2C_TRANSACTION_FAILED;
  }

  return BMP_OK;
}


uint32_t
bmp_config(bmp_config_t *cfg)
{
  uint32_t result = 0;

  result += bmp280_set_work_mode(cfg->oversampling);
  result += bmp280_set_power_mode(cfg->powerMode);
  bmp280_power_mode = cfg->powerMode;
  result += bmp280_set_standby_durn(cfg->standbyTime);

  return result;
}

uint32_t bmp_get_temperature_pressure(int32_t *temp, uint32_t *pressure)
{
  int8_t result;
  int32_t uncompTemp;
  int32_t uncompPressure;
  int32_t compTemp;
  uint32_t compPressure;

  if(i2c_hal_acquire(&bmp_sens) > 0) {
    return 1;
  }

  if ( bmp280_power_mode == BMP280_NORMAL_MODE ) {
    result = bmp280_read_uncomp_pressure(&uncompPressure);
    if ( result == SUCCESS ) {
      result = bmp280_read_uncomp_temperature(&uncompTemp);
    }
  } else {
    result = bmp280_get_forced_uncomp_pressure_temperature(&uncompPressure, &uncompTemp);
  }

  if(i2c_hal_release(&bmp_sens) > 0) {
    return 1;
  }

  if ( result != SUCCESS ) {
    return (uint32_t) result;
  }

  compTemp = bmp280_compensate_temperature_int32(uncompTemp);
  *temp = compTemp * 10;

  compPressure = bmp280_compensate_pressure_int64(uncompPressure);
  compPressure = compPressure * 10;
  *pressure = compPressure / 256;

  return BMP_OK;
}


static int8_t
i2c_bus_write(uint8_t devAddr, uint8_t regAddr, uint8_t *regData, uint8_t count)
{
  int ret;
  LOG_DBG("writing data to %x, %d\n", devAddr, regAddr);
  ret = i2c_hal_write_register_buf(&bmp_sens, regAddr, regData, count);

  if(ret != 0) {
    return BMP_ERROR_I2C_TRANSACTION_FAILED;
  }

  return BMP_OK;
}

static int8_t
i2c_bus_read(uint8_t devAddr, uint8_t regAddr, uint8_t *regData, uint8_t count)
{
  int ret;
  LOG_DBG("reading data from %x, %d\n", devAddr, regAddr);
  ret = i2c_hal_read_register(&bmp_sens, regAddr, regData, count);

  if(ret != 0) {
    return BMP_ERROR_I2C_TRANSACTION_FAILED;
  }

  return BMP_OK;
}
