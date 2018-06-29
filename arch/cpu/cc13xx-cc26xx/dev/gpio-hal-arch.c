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
 * \addtogroup cc26xx-gpio-hal
 * @{
 *
 * \file
 *     Implementation file for the CC13xx/CC26xx GPIO HAL functions
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/gpio.h)
#include DeviceFamily_constructPath(driverlib/ioc.h)

#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define CONFIG_MASK (IOC_IOPULL_M | IOC_INT_M | IOC_IOMODE_OPEN_SRC_INV)
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_interrupt_enable(gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  GPIO_clearEventDio(pin);
  gpio_hal_arch_pin_cfg_set(pin, cfg);
  IOCIntEnable(pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_pin_cfg_set(gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  /* Clear settings that we are about to change, keep everything else */
  uint32_t config = IOCPortConfigureGet(pin);
  config &= ~CONFIG_MASK;

  switch (cfg & GPIO_HAL_PIN_CFG_INT_MASK) {
  case GPIO_HAL_PIN_CFG_INT_DISABLE: config |= (IOC_NO_EDGE      | IOC_INT_DISABLE); break;
  case GPIO_HAL_PIN_CFG_INT_FALLING: config |= (IOC_FALLING_EDGE | IOC_INT_ENABLE);  break;
  case GPIO_HAL_PIN_CFG_INT_RISING:  config |= (IOC_RISING_EDGE  | IOC_INT_ENABLE);  break;
  case GPIO_HAL_PIN_CFG_INT_BOTH:    config |= (IOC_BOTH_EDGES   | IOC_INT_ENABLE);  break;
  default: {}
  }

  switch (cfg & GPIO_HAL_PIN_CFG_PULL_MASK) {
  case GPIO_HAL_PIN_CFG_PULL_NONE: config |= IOC_NO_IOPULL;   break;
  case GPIO_HAL_PIN_CFG_PULL_DOWN: config |= IOC_IOPULL_DOWN; break;
  case GPIO_HAL_PIN_CFG_PULL_UP:   config |= IOC_IOPULL_UP;   break;
  default: {}
  }

  IOCPortConfigureSet(pin, IOC_PORT_GPIO, config);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_pin_cfg_get(gpio_hal_pin_t pin)
{
  gpio_hal_pin_cfg_t cfg = 0;
  uint32_t config = IOCPortConfigureGet(pin);

  switch (config & IOC_IOPULL_M) {
  case IOC_IOPULL_UP:   cfg |= GPIO_HAL_PIN_CFG_PULL_UP;   break;
  case IOC_IOPULL_DOWN: cfg |= GPIO_HAL_PIN_CFG_PULL_DOWN; break;
  case IOC_NO_IOPULL:   cfg |= GPIO_HAL_PIN_CFG_PULL_NONE; break;
  default: {}
  }

  /* Interrupt enable/disable */
  uint32_t tmp = config & IOC_INT_M;
  if (tmp & IOC_INT_ENABLE) {
    switch (tmp) {
    case IOC_FALLING_EDGE: cfg |= GPIO_HAL_PIN_CFG_INT_FALLING; break;
    case IOC_RISING_EDGE:  cfg |= GPIO_HAL_PIN_CFG_INT_RISING;  break;
    case IOC_BOTH_EDGES:   cfg |= GPIO_HAL_PIN_CFG_INT_BOTH;    break;
    default: {}
    }
  } else {
    cfg |= GPIO_HAL_PIN_CFG_INT_DISABLE;
  }

  return cfg;
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_read_pins(gpio_hal_pin_mask_t pins)
{
  /* For pins configured as output we need to read DOUT31_0 */
  gpio_hal_pin_mask_t oe_pins = GPIO_getOutputEnableMultiDio(pins);

  pins &= ~oe_pins;

  return (HWREG(GPIO_BASE + GPIO_O_DOUT31_0) & oe_pins) |
          GPIO_readMultiDio(pins);
}
/*---------------------------------------------------------------------------*/
uint8_t
gpio_hal_arch_read_pin(gpio_hal_pin_t pin)
{
  if (GPIO_getOutputEnableDio(pin)) {
    return (HWREG(GPIO_BASE + GPIO_O_DOUT31_0) >> pin) & 1;
  }

  return GPIO_readDio(pin);
}
/*---------------------------------------------------------------------------*/
/** @} */
