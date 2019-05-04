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
 * \addtogroup cc2538-gpio-hal
 * @{
 *
 * \file
 *     Implementation file for the CC2538 GPIO HAL functions
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"
#include "dev/gpio.h"
#include "dev/ioc.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_pin_cfg_set(gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  uint8_t port, pin_num, pin_mask;
  uint32_t port_base;

  port = PIN_TO_PORT(pin);
  port_base = PIN_TO_PORT_BASE(pin);
  pin_num = pin % 8;
  pin_mask = GPIO_PIN_MASK(pin_num);

  gpio_hal_pin_cfg_t tmp;

  tmp = cfg & GPIO_HAL_PIN_CFG_EDGE_BOTH;
  if(tmp == GPIO_HAL_PIN_CFG_EDGE_NONE) {
    GPIO_DISABLE_INTERRUPT(port_base, pin_mask);
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_RISING) {
    GPIO_DETECT_EDGE(port_base, pin_mask);
    GPIO_TRIGGER_SINGLE_EDGE(port_base, pin_mask);
    GPIO_DETECT_RISING(port_base, pin_mask);
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_FALLING) {
    GPIO_DETECT_EDGE(port_base, pin_mask);
    GPIO_TRIGGER_SINGLE_EDGE(port_base, pin_mask);
    GPIO_DETECT_FALLING(port_base, pin_mask);
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_BOTH) {
    GPIO_DETECT_EDGE(port_base, pin_mask);
    GPIO_TRIGGER_BOTH_EDGES(port_base, pin_mask);
  }

  tmp = cfg & GPIO_HAL_PIN_CFG_PULL_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_PULL_NONE) {
    ioc_set_over(port, pin_num, IOC_OVERRIDE_DIS);
  } else if(tmp == GPIO_HAL_PIN_CFG_PULL_DOWN) {
    ioc_set_over(port, pin_num, IOC_OVERRIDE_PDE);
  } else if(tmp == GPIO_HAL_PIN_CFG_PULL_UP) {
    ioc_set_over(port, pin_num, IOC_OVERRIDE_PUE);
  }

  tmp = cfg & GPIO_HAL_PIN_CFG_INT_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_INT_DISABLE) {
    GPIO_DISABLE_INTERRUPT(port_base, pin_mask);
  } else if(tmp == GPIO_HAL_PIN_CFG_INT_ENABLE) {
    GPIO_ENABLE_INTERRUPT(port_base, pin_mask);
    NVIC_EnableIRQ(port);
  }

  GPIO_SOFTWARE_CONTROL(port_base, pin_mask);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_no_port_pin_cfg_get(gpio_hal_pin_t pin)
{
  uint8_t port, pin_num, pin_mask;
  uint32_t port_base;
  gpio_hal_pin_cfg_t cfg;
  uint32_t tmp;

  port = PIN_TO_PORT(pin);
  port_base = PIN_TO_PORT_BASE(pin);
  pin_num = pin % 8;
  pin_mask = GPIO_PIN_MASK(pin_num);

  cfg = 0;

  /* Pull */
  tmp = ioc_get_over(port, pin_num);
  if(tmp == IOC_OVERRIDE_PUE) {
    cfg |= GPIO_HAL_PIN_CFG_PULL_UP;
  } else if(tmp == IOC_OVERRIDE_PDE) {
    cfg |= GPIO_HAL_PIN_CFG_PULL_DOWN;
  } else {
    cfg |= GPIO_HAL_PIN_CFG_PULL_NONE;
  }

  /* Interrupt enable/disable */
  tmp = REG((port_base) + GPIO_IE) & pin_mask;
  if(tmp == 0) {
    cfg |= GPIO_HAL_PIN_CFG_INT_DISABLE;
  } else {
    cfg |= GPIO_HAL_PIN_CFG_INT_ENABLE;
  }

  /* Edge detection */
  if(REG((port_base) + GPIO_IS) & pin_mask) {
    cfg |= GPIO_HAL_PIN_CFG_EDGE_NONE;
  } else {
    if(REG((port_base) + GPIO_IBE) & pin_mask) {
      cfg |= GPIO_HAL_PIN_CFG_EDGE_BOTH;
    } else {
      if(REG((port_base) + GPIO_IEV) & pin_mask) {
        cfg |= GPIO_HAL_PIN_CFG_EDGE_RISING;
      } else {
        cfg |= GPIO_HAL_PIN_CFG_EDGE_FALLING;
      }
    }
  }

  return cfg;
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_write_pin(gpio_hal_pin_t pin, uint8_t value)
{
  if(value == 1) {
    gpio_hal_arch_set_pin(GPIO_HAL_NULL_PORT, pin);
    return;
  }
  gpio_hal_arch_clear_pin(GPIO_HAL_NULL_PORT, pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_set_pins(gpio_hal_pin_mask_t pins)
{
  GPIO_SET_PIN(GPIO_A_BASE, pins & 0xFF);
  GPIO_SET_PIN(GPIO_B_BASE, (pins >> 8) & 0xFF);
  GPIO_SET_PIN(GPIO_C_BASE, (pins >> 16) & 0xFF);
  GPIO_SET_PIN(GPIO_D_BASE, (pins >> 24) & 0xFF);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_clear_pins(gpio_hal_pin_mask_t pins)
{
  GPIO_CLR_PIN(GPIO_A_BASE, pins & 0xFF);
  GPIO_CLR_PIN(GPIO_B_BASE, (pins >> 8) & 0xFF);
  GPIO_CLR_PIN(GPIO_C_BASE, (pins >> 16) & 0xFF);
  GPIO_CLR_PIN(GPIO_D_BASE, (pins >> 24) & 0xFF);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_no_port_read_pins(gpio_hal_pin_mask_t pins)
{
  gpio_hal_pin_mask_t rv = 0;

  rv |= GPIO_READ_PIN(GPIO_A_BASE, pins & 0xFF);
  rv |= GPIO_READ_PIN(GPIO_B_BASE, (pins >> 8) & 0xFF) << 8;
  rv |= GPIO_READ_PIN(GPIO_C_BASE, (pins >> 16) & 0xFF) << 16;
  rv |= GPIO_READ_PIN(GPIO_D_BASE, (pins >> 24) & 0xFF) << 24;

  return rv;
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_write_pins(gpio_hal_pin_mask_t pins,
                                 gpio_hal_pin_mask_t value)
{
  GPIO_WRITE_PIN(GPIO_A_BASE, pins & 0xFF, value & 0xFF);
  GPIO_WRITE_PIN(GPIO_B_BASE, (pins >> 8) & 0xFF, (value >> 8) & 0xFF);
  GPIO_WRITE_PIN(GPIO_C_BASE, (pins >> 16) & 0xFF, (value >> 16) & 0xFF);
  GPIO_WRITE_PIN(GPIO_D_BASE, (pins >> 24) & 0xFF, (value >> 24) & 0xFF);
}
/*---------------------------------------------------------------------------*/
/** @} */
