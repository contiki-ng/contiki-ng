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
 * \addtogroup cc13xx-cc26xx-gpio-hal
 * @{
 *
 * \file
 *        Implementation of the GPIO HAL module for CC13xx/CC26xx. The GPIO
 *        HAL module is implemented by using the PINCC26XX module, except
 *        for multi-dio functions which use the GPIO driverlib module.
 * \author
 *        Edvard Pettersen <e.pettersen@ti.com>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"
/*---------------------------------------------------------------------------*/
#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(driverlib/gpio.h)

#include <ti/drivers/GPIO.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
static void
from_hal_cfg(gpio_hal_pin_cfg_t cfg, GPIO_PinConfig *pin_cfg)
{
  /* TODO */
}
/*---------------------------------------------------------------------------*/
static void
to_hal_cfg(GPIO_PinConfig pin_cfg, gpio_hal_pin_cfg_t *cfg)
{
  /* TODO */
}
/*---------------------------------------------------------------------------*/
static void
gpio_int_cb(uint_least8_t pin_id)
{
  /* Notify the GPIO HAL driver */
  gpio_hal_event_handler(gpio_hal_pin_to_mask(pin_id));
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_init(void)
{
}
/*---------------------------------------------------------------------------*/

void
gpio_hal_arch_no_port_pin_set_input(gpio_hal_pin_t pin)
{
  GPIO_setConfig(pin, GPIO_CFG_INPUT);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_pin_set_output(gpio_hal_pin_t pin)
{
  GPIO_setConfig(pin, GPIO_CFG_OUTPUT);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_interrupt_enable(gpio_hal_pin_t pin)
{
  GPIO_setCallback(pin, gpio_int_cb);
  GPIO_enableInt(pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_interrupt_disable(gpio_hal_pin_t pin)
{
  GPIO_disableInt(pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_pin_cfg_set(gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  GPIO_PinConfig pin_cfg = 0;
  from_hal_cfg(cfg, &pin_cfg);
  GPIO_setConfig(pin, pin_cfg);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_no_port_pin_cfg_get(gpio_hal_pin_t pin)
{
  GPIO_PinConfig pin_cfg;
  GPIO_getConfig(pin, &pin_cfg);
  gpio_hal_pin_cfg_t cfg = 0;
  to_hal_cfg(pin_cfg, &cfg);
  return cfg;
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_no_port_read_pins(gpio_hal_pin_mask_t pins)
{
  gpio_hal_pin_mask_t result = 0;
  for(gpio_hal_pin_t pin = 0; pin < (sizeof(gpio_hal_pin_mask_t) * 8); pin++) {
    if(pins & (1 << pin)) {
      result |= gpio_hal_arch_no_port_read_pin(pin);
    }
  }
  return result;
}
/*---------------------------------------------------------------------------*/
uint8_t
gpio_hal_arch_no_port_read_pin(gpio_hal_pin_t pin)
{
  return GPIO_read(pin);
}
/*---------------------------------------------------------------------------*/
/** @} */
