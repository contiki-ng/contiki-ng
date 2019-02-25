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
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \defgroup cc13xx-cc26xx-spi CC13xx/CC26xx SPI HAL
 *
 * @{
 * \file
 *        Implementation of the SPI HAL driver for CC13xx/CC26xx.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/cc.h"
#include "dev/spi.h"
/*---------------------------------------------------------------------------*/
#include <ti/drivers/SPI.h>
#include <ti/drivers/dpl/HwiP.h>
#include <ti/drivers/pin/PINCC26XX.h>
/*---------------------------------------------------------------------------*/
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
typedef struct {
  SPI_Handle handle;
  const spi_device_t *owner;
} spi_arch_t;
/*---------------------------------------------------------------------------*/
#if (SPI_CONTROLLER_COUNT > 0)
static spi_arch_t spi_arches[SPI_CONTROLLER_COUNT];
#else
static spi_arch_t *spi_arches = NULL;
#endif
/*---------------------------------------------------------------------------*/
static inline spi_arch_t *
get_handle(uint8_t spi_controller)
{
  if(spi_controller < SPI_CONTROLLER_COUNT) {
    return &spi_arches[spi_controller];
  } else {
    return NULL;
  }
}
/*---------------------------------------------------------------------------*/
static SPI_FrameFormat
convert_frame_format(uint8_t pol, uint8_t pha)
{
  pol = (pol) ? 1 : 0;
  pha = (pha) ? 1 : 0;
  /*
   * Convert pol/pha to a single byte on the following format:
   *   xxxA xxxB
   * where A is the polarity bit and B is the phase bit.
   * Note that any other value deviating from this format will
   * default to the SPI_POL1_PHA1 format.
   */
  uint8_t pol_pha = (pol << 4) | (pha << 0);
  switch(pol_pha) {
  case 0x00: return SPI_POL0_PHA0;
  case 0x01: return SPI_POL0_PHA1;
  case 0x10: return SPI_POL1_PHA0;
  default: /* fallthrough */
  case 0x11: return SPI_POL1_PHA1;
  }
}
/*---------------------------------------------------------------------------*/
bool
spi_arch_has_lock(const spi_device_t *dev)
{
  /*
   * The SPI device is the owner if the SPI controller returns a valid
   * SPI arch object and the SPI device is owner of that object.
   */
  spi_arch_t *spi_arch = get_handle(dev->spi_controller);
  return (spi_arch != NULL) && (spi_arch->owner == dev);
}
/*---------------------------------------------------------------------------*/
bool
spi_arch_is_bus_locked(const spi_device_t *dev)
{
  /*
   * The SPI controller is locked by any device if the SPI controller returns
   * a valid SPI arch object and the SPI handle of that object is valid.
   */
  spi_arch_t *spi_arch = get_handle(dev->spi_controller);
  return (spi_arch != NULL) && (spi_arch->handle != NULL);
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_lock_and_open(const spi_device_t *dev)
{
  uint_least8_t spi_index;
  spi_arch_t *spi_arch;
  SPI_Params spi_params;

  const uintptr_t hwi_key = HwiP_disable();

  spi_index = dev->spi_controller;
  spi_arch = get_handle(spi_index);

  /* Ensure the provided SPI index is valid. */
  if(spi_arch == NULL) {
    HwiP_restore(hwi_key);
    return SPI_DEV_STATUS_EINVAL;
  }

  /* Ensure the corresponding SPI interface is not already locked. */
  if(spi_arch_is_bus_locked(dev)) {
    HwiP_restore(hwi_key);
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  SPI_Params_init(&spi_params);

  spi_params.transferMode = SPI_MODE_BLOCKING;
  spi_params.mode = SPI_MASTER;
  spi_params.bitRate = dev->spi_bit_rate;
  spi_params.dataSize = 8;
  spi_params.frameFormat = convert_frame_format(dev->spi_pol, dev->spi_pha);

  /*
   * Try to open the SPI driver. Accessing the SPI driver also ensures
   * atomic access to the SPI interface.
   */
  spi_arch->handle = SPI_open(spi_index, &spi_params);

  if(spi_arch->handle == NULL) {
    HwiP_restore(hwi_key);
    return SPI_DEV_STATUS_BUS_LOCKED;
  }

  spi_arch->owner = dev;

  HwiP_restore(hwi_key);
  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_close_and_unlock(const spi_device_t *dev)
{
  spi_arch_t *spi_arch;

  const uintptr_t hwi_key = HwiP_disable();

  /* Ensure the provided SPI index is valid. */
  spi_arch = get_handle(dev->spi_controller);
  if(spi_arch == NULL) {
    HwiP_restore(hwi_key);
    return SPI_DEV_STATUS_EINVAL;
  }

  /* Ensure the corresponding SPI device owns the SPI controller. */
  if(!spi_arch_has_lock(dev)) {
    HwiP_restore(hwi_key);
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  SPI_close(spi_arch->handle);

  spi_arch->handle = NULL;
  spi_arch->owner = NULL;

  HwiP_restore(hwi_key);
  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
spi_status_t
spi_arch_transfer(const spi_device_t *dev,
                  const uint8_t *write_buf, int wlen,
                  uint8_t *inbuf, int rlen, int ignore_len)
{
  spi_arch_t *spi_arch;
  size_t totlen;
  SPI_Transaction spi_transaction;
  bool transfer_ok;

  /* Ensure the provided SPI index is valid. */
  spi_arch = get_handle(dev->spi_controller);
  if(spi_arch == NULL) {
    return SPI_DEV_STATUS_EINVAL;
  }

  if(!spi_arch_has_lock(dev)) {
    return SPI_DEV_STATUS_BUS_NOT_OWNED;
  }

  totlen = MAX((size_t)(rlen + ignore_len), (size_t)wlen);

  if(totlen == 0) {
    /* Nothing to do */
    return SPI_DEV_STATUS_OK;
  }

  spi_transaction.count = totlen;
  spi_transaction.txBuf = (void *)write_buf;
  spi_transaction.rxBuf = (void *)inbuf;

  transfer_ok = SPI_transfer(spi_arch->handle, &spi_transaction);

  if(!transfer_ok) {
    return SPI_DEV_STATUS_TRANSFER_ERR;
  }

  return SPI_DEV_STATUS_OK;
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
