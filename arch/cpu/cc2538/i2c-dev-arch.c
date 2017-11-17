/*
 * Copyright (c) 2017, Yanzi Networks.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include "dev/i2c-dev.h"
#include "dev/i2c.h"

#ifdef PLATFORM_HAS_I2C_DEV_ARCH

static volatile uint8_t is_locked = 0;
/*---------------------------------------------------------------------------*/
i2c_dev_status_t
i2c_arch_lock(i2c_device_t *dev)
{
  i2c_bus_config_t *conf;

  /* The underlying CC2538 I2C API only allows single access */
  if(is_locked) {
    return I2C_DEV_STATUS_BUS_LOCKED;
  }
  is_locked = 1;

  if(dev->speed != I2C_NORMAL_BUS_SPEED &&
     dev->speed != I2C_FAST_BUS_SPEED) {
    return I2C_DEV_STATUS_EINVAL;
  }

  conf = &dev->bus->config;
  i2c_init(conf->sda_port, conf->sda_pin, conf->scl_port, conf->scl_pin,
           dev->speed);

  i2c_start_timeout(dev->timeout);
  return I2C_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
i2c_dev_status_t
i2c_arch_unlock(i2c_device_t *dev)
{
  i2c_master_disable();
  i2c_stop_timeout();
  is_locked = 0;
  return I2C_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
i2c_dev_status_t
i2c_arch_restart_timeout(i2c_device_t *dev)
{
  i2c_start_timeout(dev->timeout);
  return I2C_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
static i2c_dev_status_t
convert_status(uint8_t status)
{
  switch(status) {
  case I2C_MASTER_ERR_NONE:
    return I2C_DEV_STATUS_OK;
  case I2C_MASTER_ERR_TIMEOUT:
    return I2C_DEV_STATUS_TIMEOUT;
  }
  if(status & I2CM_STAT_ADRACK) {
    return I2C_DEV_STATUS_ADDRESS_NACK;
  }
  if(status & I2CM_STAT_DATACK) {
    return I2C_DEV_STATUS_ADDRESS_NACK;
  }
  return I2C_DEV_STATUS_TIMEOUT;
}
/*---------------------------------------------------------------------------*/
i2c_dev_status_t
i2c_arch_write(i2c_device_t *dev, const uint8_t *data, int len)
{
  i2c_dev_status_t status;
  /* Convert the device address from 8 bit to 7 bit */
  status = i2c_burst_send(dev->address >> 1, (uint8_t *)data, len);
  /* Translate status into something sensible... */
  return convert_status(status);
}
/*---------------------------------------------------------------------------*/
i2c_dev_status_t
i2c_arch_read(i2c_device_t *dev, uint8_t *data, int len)
{
  i2c_dev_status_t status;
  /* Convert the device address from 8 bit to 7 bit */
  status = i2c_burst_receive(dev->address >> 1, data, len);
  /* Translate status into the defined status */
  return convert_status(status);
}
/*---------------------------------------------------------------------------*/
i2c_dev_status_t
i2c_arch_stop(i2c_device_t *dev)
{
  return I2C_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
#endif /* PLATFORM_HAS_I2C_DEV_ARCH */
