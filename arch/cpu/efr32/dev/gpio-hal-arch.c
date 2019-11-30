/*
 * Copyright (c) 2018, Joakim Eriksson
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

#include "contiki.h"
#include <em_device.h>
#include <em_cmu.h>
#include <em_gpio.h>
#include "gpio-hal.h"
#include "sys/log.h"
/*---------------------------------------------------------------------------*/
/* Log configuration */
#define LOG_MODULE "GPIO HAL"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
void
GPIO_EVEN_IRQHandler(void)
{
  uint32_t iflags;
  /* Get all even interrupts. */
  iflags = GPIO_IntGetEnabled() & 0x00005555;
  /* Clean only even interrupts. */
  GPIO_IntClear(iflags);

  gpio_hal_event_handler(0, iflags);
}
/*---------------------------------------------------------------------------*/
void
GPIO_ODD_IRQHandler(void)
{
  uint32_t iflags;
  /* Get all even interrupts. */
  iflags = GPIO_IntGetEnabled() & 0x0000AAAA;
  /* Clean only even interrupts. */
  GPIO_IntClear(iflags);
  gpio_hal_event_handler(0, iflags);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_init(void)
{
  CMU_ClockEnable(cmuClock_GPIO, true);
  /* enable GPIO interrupts */
  NVIC_ClearPendingIRQ(GPIO_ODD_IRQn);
  NVIC_EnableIRQ(GPIO_ODD_IRQn);
  NVIC_ClearPendingIRQ(GPIO_EVEN_IRQn);
  NVIC_EnableIRQ(GPIO_EVEN_IRQn);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_interrupt_enable(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  /* somehow the pins are the only thing that is enabling interrupt */
  GPIO_IntEnable(1 << pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_interrupt_disable(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  GPIO_IntDisable(1 << pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_pin_cfg_set(gpio_hal_port_t port,
                               gpio_hal_pin_t pin,
                               gpio_hal_pin_cfg_t cfg)
{
  /* configure GPIO ports */
  GPIO_Mode_TypeDef def;
  def = GPIO_PinModeGet(port, pin);
  /* handle pull up / down */
  GPIO_PinModeSet(port, pin, def, (cfg & GPIO_HAL_PIN_CFG_PULL_UP) ? 1 : 0);

  LOG_DBG("set config %d.%d = 0x%04x\n", port, (int)pin, (unsigned int)cfg);
  /* for now - assume that second and third arg is same */
  GPIO_ExtIntConfig(port, pin, pin,
                    (cfg & GPIO_HAL_PIN_CFG_EDGE_RISING) != 0,
                    (cfg & GPIO_HAL_PIN_CFG_EDGE_FALLING) != 0,
                    (cfg & GPIO_HAL_PIN_CFG_INT_ENABLE) != 0);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_port_pin_cfg_get(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  /* not sure how to get these out easily... probably cache...*/
  return 0;
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_pin_set_input(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  LOG_DBG("set input %d.%d\n", port, (int)pin);
  GPIO_PinModeSet(port, pin, gpioModeInput, 0);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_pin_set_output(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  LOG_DBG("set output %d.%d\n", port, (int)pin);
  GPIO_PinModeSet(port, pin, gpioModePushPull, 0);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_set_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  LOG_DBG("set pin %d.%d\n", port, (int)pin);
  GPIO_PinOutSet(port, pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_clear_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  LOG_DBG("clear pin %d.%d\n", port, (int)pin);
  GPIO_PinOutClear(port, pin);
}
/*---------------------------------------------------------------------------*/
uint8_t
gpio_hal_arch_port_read_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  return GPIO_PinInGet(port, pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_write_pin(gpio_hal_port_t port,
                             gpio_hal_pin_t pin,
                             uint8_t value)
{
  LOG_DBG("write pin %d.%d = %u\n", port, (int)pin, value);
  if(value) {
    GPIO_PinOutSet(port, pin);
  } else {
    GPIO_PinOutClear(port, pin);
  }
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_toggle_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  LOG_DBG("toggle pin %d.%d\n", port, (int)pin);
  GPIO_PinOutToggle(port, pin);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_set_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  uint32_t val;
  LOG_DBG("set pins %d.%d\n", port, (int)pins);
  val = GPIO_PortOutGet(port);
  val |= pins;
  GPIO_PortOutSet(port, val);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_clear_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  LOG_DBG("clear pins %d.%d\n", port, (int)pins);
  GPIO_PortOutClear(port, pins);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_toggle_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  LOG_DBG("toggle pins %d.%d\n", port, (int)pins);
  GPIO_PortOutToggle(port, pins);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_port_read_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  uint32_t val;
  val = GPIO_PortOutGet(port);
  val = (val & pins);
  return (gpio_hal_pin_mask_t)val;
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_write_pins(gpio_hal_port_t port,
                              gpio_hal_pin_mask_t pins,
                              gpio_hal_pin_mask_t value)
{
  GPIO_PortOutSetVal(port, value, pins);
}
/*---------------------------------------------------------------------------*/
