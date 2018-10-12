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
 * \addtogroup cc2538
 * @{
 *
 * \defgroup cc2538-gpio-hal CC2538 GPIO HAL implementation
 *
 * @{
 *
 * \file
 *     Header file for the CC2538 GPIO HAL functions
 *
 * \note
 *     Do not include this header directly
 */
/*---------------------------------------------------------------------------*/
#ifndef GPIO_HAL_ARCH_H_
#define GPIO_HAL_ARCH_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define PIN_TO_PORT(pin) (pin >> 3)
#define PIN_TO_NUM(pin) (pin % 8)
#define PIN_TO_PORT_BASE(pin) GPIO_PORT_TO_BASE(PIN_TO_PORT(pin))
/*---------------------------------------------------------------------------*/
#define gpio_hal_arch_init() do { /* Do nothing */ } while(0)

#define gpio_hal_arch_interrupt_enable(p) do { \
  GPIO_ENABLE_INTERRUPT(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8)); \
  NVIC_EnableIRQ(PIN_TO_PORT(p)); \
} while(0);

#define gpio_hal_arch_interrupt_disable(p) \
  GPIO_DISABLE_INTERRUPT(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8))

#define gpio_hal_arch_pin_set_input(p) do { \
  GPIO_SOFTWARE_CONTROL(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8)); \
  GPIO_SET_INPUT(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8)); \
} while(0);

#define gpio_hal_arch_pin_set_output(p) do { \
  GPIO_SOFTWARE_CONTROL(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8)); \
  GPIO_SET_OUTPUT(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8)); \
} while(0);

#define gpio_hal_arch_set_pin(p) \
  GPIO_SET_PIN(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8))

#define gpio_hal_arch_clear_pin(p) \
  GPIO_CLR_PIN(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8))

#define gpio_hal_arch_read_pin(p) \
  (GPIO_READ_PIN(PIN_TO_PORT_BASE(p), GPIO_PIN_MASK((p) % 8)) == 0 ? 0 : 1)
/*---------------------------------------------------------------------------*/
#endif /* GPIO_HAL_ARCH_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
