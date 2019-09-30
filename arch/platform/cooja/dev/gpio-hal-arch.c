/*
 * Copyright (c) 2019, George Oikonomou - http://www.spd.gr
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
#include "contiki.h"
#include "dev/gpio-hal.h"

#include <string.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "GPIO arch"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
static gpio_hal_pin_cfg_t pin_cfg[GPIO_HAL_PIN_COUNT];
static uint8_t pin_state[GPIO_HAL_PIN_COUNT];
/*---------------------------------------------------------------------------*/
/*
 * Callbacks used to notify other modules (e.g. leds_arch) that the state of
 * a pin has changed. Mainly used for output pins, since input pins can always
 * simply generate a virtual interrupt
 */
static void (*pin_callbacks[GPIO_HAL_PIN_COUNT])(void);
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_register_pin_callback(gpio_hal_pin_t pin,
                                    gpio_hal_arch_pin_callback cb)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    return;
  }
  pin_callbacks[pin] = cb;
}
/*---------------------------------------------------------------------------*/
static void
set_pin_state(gpio_hal_pin_t pin, uint8_t state)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    return;
  }
  pin_state[pin] = state;

  if(pin_callbacks[pin] != NULL) {
    pin_callbacks[pin]();
  }
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_init(void)
{
  memset(pin_cfg, 0, sizeof(pin_cfg));
  memset(pin_state, 0, sizeof(pin_state));
  memset(pin_callbacks, 0, sizeof(pin_callbacks));
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_interrupt_enable(gpio_hal_pin_t pin)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    LOG_ERR("Pin %u out of bounds\n", pin);
    return;
  }

  pin_cfg[pin] |= GPIO_HAL_PIN_CFG_INT_ENABLE;

  LOG_DBG("Pin %u: Enabled interrupt\n", pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_interrupt_disable(gpio_hal_pin_t pin)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    LOG_ERR("Pin %u out of bounds\n", pin);
    return;
  }

  pin_cfg[pin] &= ~GPIO_HAL_PIN_CFG_INT_ENABLE;

  LOG_DBG("Pin %u: Disabled interrupt\n", pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_pin_cfg_set(gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    LOG_ERR("Pin %u out of bounds\n", pin);
    return;
  }

  pin_cfg[pin] = cfg;
  LOG_DBG("Pin %u: Set config=0x%02x\n", pin, pin_cfg[pin]);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_no_port_pin_cfg_get(gpio_hal_pin_t pin)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    LOG_ERR("Pin %u out of bounds\n", pin);
    return 0;
  }

  LOG_DBG("Pin %u: Config=0x%02x\n", pin, pin_cfg[pin]);
  return pin_cfg[pin];
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_pin_set_input(gpio_hal_pin_t pin)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    LOG_ERR("Pin %u out of bounds\n", pin);
    return;
  }

  LOG_DBG("Pin %u: Set input\n", pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_pin_set_output(gpio_hal_pin_t pin)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    LOG_ERR("Pin %u out of bounds\n", pin);
    return;
  }

  LOG_DBG("Pin %u: Set output\n", pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_set_pin(gpio_hal_pin_t pin)
{
  set_pin_state(pin, 1);
  LOG_DBG("Pin %u: Set\n", pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_clear_pin(gpio_hal_pin_t pin)
{
  set_pin_state(pin, 0);
  LOG_DBG("Pin %u: Clear\n", pin);
}
/*---------------------------------------------------------------------------*/
uint8_t
gpio_hal_arch_no_port_read_pin(gpio_hal_pin_t pin)
{
  if(pin >= GPIO_HAL_PIN_COUNT) {
    LOG_ERR("Pin %u out of bounds\n", pin);
    return 0;
  }

  LOG_DBG("Pin %u: Read=%u\n", pin, pin_state[pin]);
  return pin_state[pin];
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_write_pin(gpio_hal_pin_t pin, uint8_t value)
{
  set_pin_state(pin, value);
  LOG_DBG("Pin %u: Write=%u\n", pin, pin_state[pin]);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_set_pins(gpio_hal_pin_mask_t pins)
{
  gpio_hal_pin_t pin;

  for(pin = 0; pin < GPIO_HAL_PIN_COUNT; pin++) {
    if(pins & (1 << pin)) {
      set_pin_state(pin, 1);
    }
  }

  LOG_DBG("Set pins 0x%08" PRIx32 "\n", pins);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_clear_pins(gpio_hal_pin_mask_t pins)
{
  gpio_hal_pin_t pin;

  for(pin = 0; pin < GPIO_HAL_PIN_COUNT; pin++) {
    if(pins & (1 << pin)) {
      set_pin_state(pin, 0);
    }
  }

  LOG_DBG("Clear pins 0x%08" PRIx32 "\n", pins);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_no_port_read_pins(gpio_hal_pin_mask_t pins)
{
  gpio_hal_pin_t pin;
  gpio_hal_pin_mask_t state = 0;

  for(pin = 0; pin < GPIO_HAL_PIN_COUNT; pin++) {
    state |= (pin_state[pin] << pin);
  }

  LOG_DBG("Read pins 0x%08" PRIx32 "\n", state);
  return state;
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_write_pins(gpio_hal_pin_mask_t pins,
                                 gpio_hal_pin_mask_t value)
{
  gpio_hal_pin_t pin;

  for(pin = 0; pin < GPIO_HAL_PIN_COUNT; pin++) {
    if(pins & (1 << pin)) {
      set_pin_state(pin, (value & (1 << pin)) == 0 ? 0 : 1);
    }
  }

  LOG_DBG("Write pins 0x%08" PRIx32 "->0x%08" PRIx32 "\n", pins, value);
}
/*---------------------------------------------------------------------------*/
