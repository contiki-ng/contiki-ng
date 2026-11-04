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
 * \defgroup cc13xx-cc26xx-gpio-hal CC13xx/CC26xx GPIO HAL implementation
 *
 * @{
 *
 * \file
 *        Header file for the CC13xx/CC26xx GPIO HAL functions.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 * \note
 *        Do not include this header directly.
 */
/*---------------------------------------------------------------------------*/
#ifndef GPIO_HAL_ARCH_H_
#define GPIO_HAL_ARCH_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/gpio.h)

#include <ti/drivers/pin/PINCC26XX.h>
/*---------------------------------------------------------------------------*/
#define gpio_hal_arch_set_pin(port, pin)        PINCC26XX_setOutputValue(pin, 1)
#define gpio_hal_arch_clear_pin(port, pin)      PINCC26XX_setOutputValue(pin, 0)
#define gpio_hal_arch_toggle_pin(port, pin)     PINCC26XX_setOutputValue(pin, \
                                                  PINCC26XX_getOutputValue(pin) \
                                                  ? 0 : 1)
#define gpio_hal_arch_write_pin(port, pin, v)   PINCC26XX_setOutputValue(pin, v)

/*
 * Multi-pin access goes straight to the GPIO registers: the CC13x2x7/CC26x2x7
 * driverlib no longer has the GPIO_*MultiDio() helpers.
 */
#define gpio_hal_arch_set_pins(port, pin) \
  (HWREG(GPIO_BASE + GPIO_O_DOUTSET31_0) = (pin))
#define gpio_hal_arch_clear_pins(port, pin) \
  (HWREG(GPIO_BASE + GPIO_O_DOUTCLR31_0) = (pin))
#define gpio_hal_arch_toggle_pins(port, pin) \
  (HWREG(GPIO_BASE + GPIO_O_DOUTTGL31_0) = (pin))
#define gpio_hal_arch_write_pins(port, pin, v) \
  gpio_hal_arch_dout_write((pin), (v))

/* A function, not a macro, so that pin is evaluated once. */
static inline void
gpio_hal_arch_dout_write(uint32_t pins, uint32_t value)
{
  HWREG(GPIO_BASE + GPIO_O_DOUT31_0) =
    (HWREG(GPIO_BASE + GPIO_O_DOUT31_0) & ~pins) | (value & pins);
}
/*---------------------------------------------------------------------------*/
#endif /* GPIO_HAL_ARCH_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
