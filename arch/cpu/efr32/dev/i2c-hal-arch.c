/*
 * Copyright (c) 2017, Yanzi Networks, 2018, RISE SICS
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
 *
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

#include "contiki.h"
#include "dev/i2c-hal.h"
#include "em_cmu.h"
#include "em_i2c.h"
#include "em_gpio.h"
#include <stdint.h>

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif /* DEBUG */

static i2c_hal_status_t
decode_status(I2C_TransferReturn_TypeDef status)
{
  switch(status) {
    case i2cTransferInProgress:
    case i2cTransferDone:
      return I2C_HAL_STATUS_OK;
    case i2cTransferNack:
      /* i2cTransferNack means NACK for address or data */
      return I2C_HAL_STATUS_DATA_NACK;
    case i2cTransferBusErr:
    case i2cTransferArbLost:
    case i2cTransferUsageFault:
    case i2cTransferSwFault:
      return I2C_HAL_STATUS_EINVAL;
    default:
      /* should not reach here */
      return I2C_HAL_STATUS_EINVAL;
  }
}
/*---------------------------------------------------------------------------*/

uint8_t
i2c_hal_arch_lock(i2c_hal_device_t *dev)
{
  i2c_hal_bus_config_t *conf = &dev->bus->config;
  uint32_t scl_port = 0, scl_pin = 0;
  uint32_t sda_port = 0, sda_pin = 0;
  bool was_locked;

  was_locked = dev->bus->lock != 0;

  if(dev->bus->lock && dev->bus->lock_device != dev) {
    PRINTF("I2C (%s): bus is locked\n", __func__);
    return I2C_HAL_STATUS_BUS_LOCKED;
  }

  if(dev->speed != I2C_HAL_NORMAL_BUS_SPEED &&
     dev->speed != I2C_HAL_FAST_BUS_SPEED) {
    PRINTF("I2C (%s): speed %" PRIu32 " is invalid\n", __func__, dev->speed);
    return I2C_HAL_STATUS_EINVAL;
  }

  dev->bus->lock = 1;
  dev->bus->lock_device = dev;

  I2C_Init_TypeDef i2cInit = I2C_INIT_DEFAULT;

  /* enable required clocks (GPIO for pins routing) */
  if(conf->I2Cx == I2C0) {
    CMU_ClockEnable(cmuClock_I2C0, true);

    sda_port = AF_I2C0_SDA_PORT(conf->sda_loc);
    sda_pin = AF_I2C0_SDA_PIN(conf->sda_loc);

    scl_port = AF_I2C0_SCL_PORT(conf->sda_loc);
    scl_pin = AF_I2C0_SCL_PIN(conf->sda_loc);
#ifdef I2C1
  } else if(conf->I2Cx == I2C1) {
    CMU_ClockEnable(cmuClock_I2C1, true);
    sda_port = AF_I2C1_SDA_PORT(conf->sda_loc);
    sda_pin = AF_I2C1_SDA_PIN(conf->sda_loc);

    scl_port = AF_I2C1_SCL_PORT(conf->sda_loc);
    scl_pin = AF_I2C1_SCL_PIN(conf->sda_loc);
#endif /* I2C1 */
  } else {
    PRINTF("I2C: lock: unsupported I2Cx: %p\n", conf->I2Cx);
    /* Only keep the lock if locked before the call */
    if(!was_locked) {
      dev->bus->lock = 0;
      dev->bus->lock_device = NULL;
    }
    return I2C_HAL_STATUS_EINVAL;
  }

  CMU_ClockEnable(cmuClock_GPIO, true);

  PRINTF("I2C: SDA PORT: %d, PIN: %d\n", (int)sda_port, (int)sda_pin);
  PRINTF("I2C: SCL PORT: %d, PIN: %d\n", (int)scl_port, (int)scl_pin);

  /* route I2C0 to correct pins */
  GPIO_PinModeSet(sda_port, sda_pin, gpioModeWiredAndPullUpFilter, 1);
  GPIO_PinModeSet(scl_port, scl_pin, gpioModeWiredAndPullUpFilter, 1);

  /* enable pin routing in I2C */
  conf->I2Cx->ROUTEPEN = I2C_ROUTEPEN_SDAPEN | I2C_ROUTEPEN_SCLPEN;
  conf->I2Cx->ROUTELOC0 = (conf->sda_loc << _I2C_ROUTELOC0_SDALOC_SHIFT) |
    (conf->scl_loc << _I2C_ROUTELOC0_SCLLOC_SHIFT);

  /* initialize I2C controller */
  /* Set emlib init parameters */
  i2cInit.enable = true;
  i2cInit.master = true; /* master mode only */

  I2C_Init(conf->I2Cx, &i2cInit);
  I2C_BusFreqSet(conf->I2Cx, 0, dev->speed, i2cClockHLRStandard);
  conf->I2Cx->CTRL |= I2C_CTRL_AUTOACK | I2C_CTRL_AUTOSN;

  return I2C_HAL_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

uint8_t
i2c_hal_arch_unlock(i2c_hal_device_t *dev)
{
  /* disable I2C controller */
  I2C_Enable(dev->bus->config.I2Cx, false);

  /* TODO: turn off clocks to reduce power consumption? */

  /* unlock the bus */
  dev->bus->lock = 0;
  dev->bus->lock_device = NULL;

  return I2C_HAL_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

uint8_t
i2c_hal_arch_restart_timeout(i2c_hal_device_t *dev)
{
  /* TODO: implement timeout */
  return I2C_HAL_STATUS_OK;
}
/*---------------------------------------------------------------------------*/

uint8_t
i2c_hal_arch_read(i2c_hal_device_t *dev, uint8_t *data, int len)
{
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef ret;
  int timeout = 100; /* 10 loops? */

  i2cTransfer.addr          = dev->address;
  i2cTransfer.flags         = I2C_FLAG_READ;
  i2cTransfer.buf[0].data   = data;
  i2cTransfer.buf[0].len    = len;

  /* I2C0 is hardoced as the EFR32MG has only one I2C controller */
  ret = I2C_TransferInit(dev->bus->config.I2Cx, &i2cTransfer);
  if((ret != i2cTransferInProgress) && (ret != i2cTransferDone)) {
    PRINTF("I2C (%s): RX init error (%d)\n", __func__, ret);
    return decode_status(ret);
  }

  PRINTF("I2C Init RX: %d Addr:%x (len: %d)\n", ret, i2cTransfer.addr, len);

  while(ret == i2cTransferInProgress && timeout > 0) {
    //PRINTF("I2C RX: %d (%d)\n", ret, timeout);
    clock_delay_usec(1000); /* 1 msec wait */
    ret = I2C_Transfer(dev->bus->config.I2Cx);
    timeout--;
  }

  if((ret != i2cTransferInProgress) && (ret != i2cTransferDone)) {
    PRINTF("I2C (%s): RX error (%d)\n", __func__, ret);
    return decode_status(ret);
  } else {
    return I2C_HAL_STATUS_OK;
  }
}
/*---------------------------------------------------------------------------*/

uint8_t
i2c_hal_arch_write(i2c_hal_device_t *dev, const uint8_t *data, int len)
{
  I2C_TransferSeq_TypeDef i2cTransfer;
  I2C_TransferReturn_TypeDef ret;
  int timeout = 100;

  i2cTransfer.addr          = dev->address;
  i2cTransfer.flags         = I2C_FLAG_WRITE;
  i2cTransfer.buf[0].data   = (uint8_t *)data;
  i2cTransfer.buf[0].len    = len;

  if(dev->bus->config.I2Cx == NULL) {
    PRINTF("I2C: No I2C module configured\n");
    return I2C_HAL_STATUS_EINVAL;
  }

  /* I2C0 is hardoced as the EFR32MG has only one I2C controller */
  ret = I2C_TransferInit(dev->bus->config.I2Cx, &i2cTransfer);
  if((ret != i2cTransferInProgress) && (ret != i2cTransferDone)) {
    PRINTF("I2C (%s): TX init error (%d)\n", __func__, ret);
    return decode_status(ret);
  }

  //PRINTF("I2C Init TX: %d Addr:%x (len: %d)\n", ret, i2cTransfer.addr, len);

  while(ret == i2cTransferInProgress && timeout > 0) {
    clock_delay_usec(1000);
    ret = I2C_Transfer(dev->bus->config.I2Cx);
    timeout--;
  }
  PRINTF("I2C TX: timeout: %d\n", timeout);

  if((ret != i2cTransferInProgress) && (ret != i2cTransferDone)) {
    PRINTF("I2C (%s): TX error (%d)\n", __func__, ret);
    return decode_status(ret);
  } else {
    return I2C_HAL_STATUS_OK;
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
i2c_hal_arch_stop(i2c_hal_device_t *dev)
{
  /* emlib driver doesn't allow for manual STOP conditions */
  return I2C_HAL_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
