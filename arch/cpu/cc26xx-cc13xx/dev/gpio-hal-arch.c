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
#include "ti-lib.h"
#include "ti-lib-rom.h"
#include "dev/gpio-hal.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
#define CONFIG_MASK (IOC_IOPULL_M | IOC_INT_M | IOC_IOMODE_OPEN_SRC_INV)
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_pin_cfg_set(gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  uint32_t config;
  gpio_hal_pin_cfg_t tmp;

  /* Clear settings that we are about to change, keep everything else */
#ifdef ThisLibraryIsFor_CC26x0R2_HaltIfViolated
  config = ti_lib_ioc_port_configure_get(pin);
#else
  config = ti_lib_rom_ioc_port_configure_get(pin);
#endif
  config &= ~CONFIG_MASK;

  tmp = cfg & GPIO_HAL_PIN_CFG_EDGE_BOTH;
  if(tmp == GPIO_HAL_PIN_CFG_EDGE_NONE) {
    config |= IOC_NO_EDGE;
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_RISING) {
    config |= IOC_RISING_EDGE;
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_FALLING) {
    config |= IOC_FALLING_EDGE;
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_BOTH) {
    config |= IOC_BOTH_EDGES;
  }

  tmp = cfg & GPIO_HAL_PIN_CFG_PULL_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_PULL_NONE) {
    config |= IOC_NO_IOPULL;
  } else if(tmp == GPIO_HAL_PIN_CFG_PULL_DOWN) {
    config |= IOC_IOPULL_DOWN;
  } else if(tmp == GPIO_HAL_PIN_CFG_PULL_UP) {
    config |= IOC_IOPULL_UP;
  }

  tmp = cfg & GPIO_HAL_PIN_CFG_INT_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_INT_DISABLE) {
    config |= IOC_INT_DISABLE;
  } else if(tmp == GPIO_HAL_PIN_CFG_INT_ENABLE) {
    config |= IOC_INT_ENABLE;
  }

  ti_lib_rom_ioc_port_configure_set(pin, IOC_PORT_GPIO, config);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_pin_cfg_get(gpio_hal_pin_t pin)
{
  gpio_hal_pin_cfg_t cfg;
  uint32_t tmp;
  uint32_t config;

  cfg = 0;
#ifdef ThisLibraryIsFor_CC26x0R2_HaltIfViolated
  config = ti_lib_ioc_port_configure_get(pin);
#else
  config = ti_lib_rom_ioc_port_configure_get(pin);
#endif

  /* Pull */
  tmp = config & IOC_IOPULL_M;
  if(tmp == IOC_IOPULL_UP) {
    cfg |= GPIO_HAL_PIN_CFG_PULL_UP;
  } else if(tmp == IOC_IOPULL_DOWN) {
    cfg |= GPIO_HAL_PIN_CFG_PULL_DOWN;
  } else if(tmp == IOC_NO_IOPULL) {
    cfg |= GPIO_HAL_PIN_CFG_PULL_NONE;
  }

  /* Interrupt enable/disable */
  tmp = config & IOC_INT_ENABLE;
  if(tmp == IOC_INT_DISABLE) {
    cfg |= GPIO_HAL_PIN_CFG_INT_DISABLE;
  } else if(tmp == IOC_INT_ENABLE) {
    cfg |= GPIO_HAL_PIN_CFG_INT_ENABLE;
  }

  /* Edge detection */
  tmp = config & IOC_BOTH_EDGES;
  if(tmp == IOC_NO_EDGE) {
    cfg |= GPIO_HAL_PIN_CFG_EDGE_NONE;
  } else if(tmp == IOC_FALLING_EDGE) {
    cfg |= GPIO_HAL_PIN_CFG_EDGE_FALLING;
  } else if(tmp == IOC_RISING_EDGE) {
    cfg |= GPIO_HAL_PIN_CFG_EDGE_RISING;
  } else if(tmp == IOC_BOTH_EDGES) {
    cfg |= GPIO_HAL_PIN_CFG_EDGE_BOTH;
  }

  return cfg;
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_read_pins(gpio_hal_pin_mask_t pins)
{
  gpio_hal_pin_mask_t oe_pins;

  /* For pins configured as output we need to read DOUT31_0 */
  oe_pins = ti_lib_gpio_get_output_enable_multi_dio(pins);

  pins &= ~oe_pins;

  return (HWREG(GPIO_BASE + GPIO_O_DOUT31_0) & oe_pins) |
         ti_lib_gpio_read_multi_dio(pins);
}
/*---------------------------------------------------------------------------*/
uint8_t
gpio_hal_arch_read_pin(gpio_hal_pin_t pin)
{
  if(ti_lib_gpio_get_output_enable_dio(pin)) {
    return (HWREG(GPIO_BASE + GPIO_O_DOUT31_0) >> pin) & 1;
  }

  return ti_lib_gpio_read_dio(pin);
}
/*---------------------------------------------------------------------------*/
/** @} */
