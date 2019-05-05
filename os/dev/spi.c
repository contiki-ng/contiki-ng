/*
 * Copyright (c) 2016-2017, Yanzi Networks.
 * Copyright (c) 2017, University of Bristol - http://www.bristol.ac.uk/
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup spi-hal
 * @{
 *
 * \file
 *     Implementation of the platform-independent aspects of the SPI HAL
 */
/*---------------------------------------------------------------------------*/
#include <dev/spi.h>
#include "contiki.h"
#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
spi_status_t
spi_acquire(const spi_device_t *dev)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  /* lock and open the bus */
  return spi_arch_lock_and_open(dev);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_release(const spi_device_t *dev)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  /* close and unlock the bus */
  return spi_arch_close_and_unlock(dev);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_select(const spi_device_t *dev)
{
  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  gpio_hal_arch_clear_pin(SPI_DEVICE_PORT(cs, dev), dev->pin_spi_cs);

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_deselect(const spi_device_t *dev)
{
  gpio_hal_arch_set_pin(SPI_DEVICE_PORT(cs, dev), dev->pin_spi_cs);

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
bool
spi_has_bus(const spi_device_t *dev)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return false;
  }

  return spi_arch_has_lock(dev);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_write_byte(const spi_device_t *dev, uint8_t data)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  return spi_arch_transfer(dev, &data, 1, 0, 0, 0);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_write(const spi_device_t *dev, const uint8_t *data, int size)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  return spi_arch_transfer(dev, data, size, 0, 0, 0);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_read_byte(const spi_device_t *dev, uint8_t *buf)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  return spi_arch_transfer(dev, NULL, 0, buf, 1, 0);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_read(const spi_device_t *dev, uint8_t *buf, int size)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  return spi_arch_transfer(dev, NULL, 0, buf, size, 0);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_read_skip(const spi_device_t *dev, int size)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  return spi_arch_transfer(dev, NULL, 0, NULL, 0, size);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_transfer(const spi_device_t *dev,
             const uint8_t *wdata, int wsize,
             uint8_t *rbuf, int rsize, int ignore)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  if(wdata == NULL && wsize > 0) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(rbuf == NULL && rsize > 0) {
    return SPI_DEV_STATUS_EINVAL;
  }

  return spi_arch_transfer(dev, wdata, wsize, rbuf, rsize, ignore);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_read_register(const spi_device_t *dev, uint8_t reg, uint8_t *data, int size)
{
  spi_status_t status;
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  /* write the register first (will read a status) */
  status = spi_write_byte(dev, reg);
  if(status != SPI_DEV_STATUS_OK) {
    return status;
  }

  /* then read the value  (will read the value) */
  status = spi_read(dev, data, size);
  if(status != SPI_DEV_STATUS_OK) {
    return status;
  }

  return status;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_strobe(const spi_device_t *dev, uint8_t strobe, uint8_t *result)
{
  if(dev == NULL || dev->spi_controller >= SPI_CONTROLLER_COUNT) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  return spi_arch_transfer(dev, &strobe, 1, result, 1, 0);
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
