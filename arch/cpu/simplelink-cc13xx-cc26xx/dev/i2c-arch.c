/*
 * Copyright (c) 2018, Texas Instruments Incorporated - http://www.ti.com/
 * Copyright (c) 2020, George Oikonomou - http://www.spd.gr
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup cc13xx-cc26xx-i2c
 * @{
 *
 * \file
 *   Implementation of the I2C HAL driver for CC13xx/CC26xx.
 *
 * \author
 *   Edvard Pettersen <e.pettersen@ti.com>
 * \author
 *   George Oikonomou <george@contiki-ng.org>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "i2c-arch.h"

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
bool
i2c_arch_write_read(I2C_Handle i2c_handle, uint_least8_t slave_addr,
                    void *wbuf, size_t wcount, void *rbuf,
                    size_t rcount)
{
  I2C_Transaction i2cTransaction = {
    .writeBuf = wbuf,
    .writeCount = wcount,
    .readBuf = rbuf,
    .readCount = rcount,
    .slaveAddress = slave_addr,
  };

  if(!i2c_handle) {
    return false;
  }

  return I2C_transfer(i2c_handle, &i2cTransaction);
}
/*---------------------------------------------------------------------------*/
/* Releases the I2C Peripheral */
void
i2c_arch_release(I2C_Handle i2c_handle)
{
  if(!i2c_handle) {
    return;
  }

  I2C_close(i2c_handle);
}
/*---------------------------------------------------------------------------*/
I2C_Handle
i2c_arch_acquire(uint_least8_t index)
{
  I2C_Params i2c_params;

  I2C_Params_init(&i2c_params);

  i2c_params.transferMode = I2C_MODE_BLOCKING;
  i2c_params.bitRate = I2C_400kHz;

  return I2C_open(index, &i2c_params);
}
/*---------------------------------------------------------------------------*/
/** @} */
