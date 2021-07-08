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
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \defgroup cc13xx-cc26xx-i2c CC13xx/CC26xx I2C HAL
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
#ifndef I2C_ARCH_H_
#define I2C_ARCH_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "board-conf.h"
/*---------------------------------------------------------------------------*/
#include <Board.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/cpu.h)

#include <ti/drivers/I2C.h>

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/**
 * \brief One-time initialisation of the I2C Driver
 *
 * This function must be called before any other I2C driver calls.
 */
static inline void
i2c_arch_init(void)
{
  I2C_init();
}

/**
 * \brief Open and lock the I2C Peripheral for use
 * \param index The index of the I2C controller
 * \return An I2C Handle if successful, or NULL if an error occurs
 *
 * Must be called before each I2C transaction.
 *
 * When the function returns successfully, i2c_handle will be non-NULL and can
 * be used in subsequent calls to perform an I2C transaction, for example with
 * i2c_arch_write_read().
 *
 * index can take values among Board_I2Cx e.g. Board_I2C0
 *
 * At the end of the transaction, the caller should call i2c_arch_release() in
 * order to allow other code files to use the I2C module
 */
I2C_Handle i2c_arch_acquire(uint_least8_t index);

/**
 * \brief Release the I2C Peripheral for other modules to use
 * \param i2c_handle A pointer to an I2C handle
 *
 * Must be called after the end of each I2C transaction in order to allow
 * other modules to use the I2C controller. The i2c_handle is obtained by
 * an earlier call to i2c_arch_acquire()
 */
void i2c_arch_release(I2C_Handle i2c_handle);

/**
 * \brief              Setup and peform an I2C transaction.
 * \param  i2c_handle  The I2C handle to use for this transaction
 * \param  slave_addr  The address of the slave device on the I2C bus
 * \param  wbuf        Write buffer during the I2C transation.
 * \param  wcount      How many bytes in the write buffer
 * \param  rbuf        Input buffer during the I2C transation.
 * \param  rcount      How many bytes to read into rbuf.
 * \retval true        The I2C operation was successful
 * \retval false       The I2C operation failed
 */
bool i2c_arch_write_read(I2C_Handle i2c_handle, uint_least8_t slave_addr,
                         void *wbuf, size_t wcount, void *rbuf, size_t rcount);

/**
 * \brief             Perform a write-only I2C transaction.
 * \param  i2c_handle The I2C handle to use for this transaction
 * \param  slave_addr The address of the slave device on the I2C bus
 * \param  wbuf       Write buffer during the I2C transaction.
 * \param  wcount     How many bytes in the write buffer
 * \retval true       The I2C operation was successful
 * \retval false      The I2C operation failed
 */
static inline bool
i2c_arch_write(I2C_Handle i2c_handle, uint_least8_t slave_addr,
               void *wbuf, size_t wcount)
{
  return i2c_arch_write_read(i2c_handle, slave_addr, wbuf, wcount, NULL, 0);
}

/**
 * \brief             Perform a read-only I2C transaction.
 * \param  i2c_handle The I2C handle to use for this transaction
 * \param  slave_addr The address of the slave device on the I2C bus
 * \param  rbuf       Input buffer during the I2C transaction.
 * \param  rcount     How many bytes to read into rbuf.
 * \retval true       The I2C operation was successful
 * \retval false      The I2C operation failed
 */
static inline bool
i2c_arch_read(I2C_Handle i2c_handle, uint_least8_t slave_addr, void *rbuf,
              size_t rcount)
{
  return i2c_arch_write_read(i2c_handle, slave_addr, NULL, 0, rbuf, rcount);
}
/*---------------------------------------------------------------------------*/
#endif /* I2C_ARCH_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
