/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
 * \addtogroup cc26xx
 * @{
 *
 * \defgroup cc26xx-gpio-hal CC13xx/CC26xx GPIO HAL implementation
 *
 * @{
 *
 * \file
 *     Header file for the CC13xx/CC26xx GPIO HAL functions
 *
 * \note
 *     Do not include this header directly
 */
/*---------------------------------------------------------------------------*/
#ifndef GPIO_HAL_ARCH_H_
#define GPIO_HAL_ARCH_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "ti-lib.h"
#include "ti-lib-rom.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define gpio_hal_arch_init()               do { /* Do nothing */ } while(0)

#define gpio_hal_arch_interrupt_enable(p)  interrupt_enable(p)

#ifdef ThisLibraryIsFor_CC26x0R2_HaltIfViolated
#define gpio_hal_arch_interrupt_disable(p) ti_lib_ioc_int_disable(p)
#define gpio_hal_arch_pin_set_input(p)     ti_lib_ioc_pin_type_gpio_input(p)
#define gpio_hal_arch_pin_set_output(p)    ti_lib_ioc_pin_type_gpio_output(p)
#else
#define gpio_hal_arch_interrupt_disable(p) ti_lib_rom_ioc_int_disable(p)
#define gpio_hal_arch_pin_set_input(p)     ti_lib_rom_ioc_pin_type_gpio_input(p)
#define gpio_hal_arch_pin_set_output(p)    ti_lib_rom_ioc_pin_type_gpio_output(p)
#endif
#define gpio_hal_arch_set_pin(p)           ti_lib_gpio_set_dio(p)
#define gpio_hal_arch_clear_pin(p)         ti_lib_gpio_clear_dio(p)
#define gpio_hal_arch_toggle_pin(p)        ti_lib_gpio_toggle_dio(p)
#define gpio_hal_arch_write_pin(p, v)      ti_lib_gpio_write_dio(p, v)

#define gpio_hal_arch_set_pins(p)          ti_lib_gpio_set_multi_dio(p)
#define gpio_hal_arch_clear_pins(p)        ti_lib_gpio_clear_multi_dio(p)
#define gpio_hal_arch_toggle_pins(p)       ti_lib_gpio_toggle_multi_dio(p)
#define gpio_hal_arch_write_pins(p, v)     ti_lib_gpio_write_multi_dio(p, v)
/*---------------------------------------------------------------------------*/
static inline void
interrupt_enable(gpio_hal_pin_t pin)
{
  ti_lib_gpio_clear_event_dio(pin);
#ifndef ThisLibraryIsFor_CC26x0R2_HaltIfViolated
  ti_lib_rom_ioc_int_enable(pin);
#else
  ti_lib_ioc_int_enable(pin);
#endif
}
/*---------------------------------------------------------------------------*/
#endif /* GPIO_HAL_ARCH_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
