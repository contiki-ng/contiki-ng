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

#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
/*---------------------------------------------------------------------------*/
#include <stdint.h>
/*---------------------------------------------------------------------------*/
static PIN_Config pin_config[] = { PIN_TERMINATE };
static PIN_State pin_state;
static PIN_Handle pin_handle;
/*---------------------------------------------------------------------------*/
/**< PIN standard input configuration mimicking IOC_STD_INPUT */
#define GPIO_HAL_PIN_STD_INPUT      (PIN_INPUT_EN | PIN_GPIO_OUTPUT_DIS | \
                                     PIN_NOPULL | PIN_DRVSTR_MIN | PIN_IRQ_DIS)

/**< PIN standard input configuration bitmask */
#define GPIO_HAL_PIN_STD_INPUT_BM   (PIN_BM_INPUT_EN | PIN_BM_GPIO_OUTPUT_EN | \
                                     PIN_BM_PULLING | PIN_BM_INV_INOUT | \
                                     PIN_BM_DRVSTR | PIN_BM_SLEWCTRL | \
                                     PIN_BM_HYSTERESIS | PIN_BM_IRQ)

/**< PIN standard input configuration mimicking IOC_STD_OUTPUT */
#define GPIO_HAL_PIN_STD_OUTPUT     (PIN_INPUT_DIS | PIN_GPIO_OUTPUT_EN | \
                                     PIN_NOPULL | PIN_DRVSTR_MIN | PIN_IRQ_DIS)

/**< PIN standard output configuration bitmask */
#define GPIO_HAL_PIN_STD_OUTPUT_BM  (PIN_BM_INPUT_EN | PIN_BM_GPIO_OUTPUT_EN | \
                                     PIN_BM_PULLING | PIN_BM_INV_INOUT | \
                                     PIN_BM_DRVSTR | PIN_BM_SLEWCTRL | \
                                     PIN_BM_HYSTERESIS | PIN_BM_IRQ)
/*---------------------------------------------------------------------------*/
static void
from_hal_cfg(gpio_hal_pin_cfg_t cfg, PIN_Config *pin_cfg, PIN_Config *pin_mask)
{
  /* Pulling config */
  *pin_mask |= PIN_BM_PULLING;

  switch(cfg & GPIO_HAL_PIN_CFG_PULL_MASK) {
  default: /* Default to no pullup/pulldown */
  case GPIO_HAL_PIN_CFG_PULL_NONE: *pin_cfg |= PIN_NOPULL;   break;
  case GPIO_HAL_PIN_CFG_PULL_UP:   *pin_cfg |= PIN_PULLUP;   break;
  case GPIO_HAL_PIN_CFG_PULL_DOWN: *pin_cfg |= PIN_PULLDOWN; break;
  }

  /* Hysteresis config */
  *pin_mask |= PIN_BM_HYSTERESIS;

  if(cfg & GPIO_HAL_PIN_CFG_HYSTERESIS) {
    *pin_cfg |= PIN_HYSTERESIS;
  }

  /* Interrupt config */
  *pin_mask |= PIN_BM_IRQ;

  if((cfg & GPIO_HAL_PIN_CFG_INT_MASK) == GPIO_HAL_PIN_CFG_INT_ENABLE) {
    /* Interrupt edge config */
    switch(cfg & GPIO_HAL_PIN_CFG_EDGE_BOTH) {
    case GPIO_HAL_PIN_CFG_EDGE_NONE:    *pin_cfg |= PIN_IRQ_DIS;       break;
    case GPIO_HAL_PIN_CFG_EDGE_FALLING: *pin_cfg |= PIN_IRQ_NEGEDGE;   break;
    case GPIO_HAL_PIN_CFG_EDGE_RISING:  *pin_cfg |= PIN_IRQ_POSEDGE;   break;
    case GPIO_HAL_PIN_CFG_EDGE_BOTH:    *pin_cfg |= PIN_IRQ_BOTHEDGES; break;
    }
  } else {
    *pin_cfg |= PIN_IRQ_DIS;
  }
}
/*---------------------------------------------------------------------------*/
static void
to_hal_cfg(PIN_Config pin_cfg, gpio_hal_pin_cfg_t *cfg)
{
  /* Input config */
  if(pin_cfg & PIN_BM_INPUT_MODE) {
    /* Hysteresis config */
    if((pin_cfg & PIN_BM_HYSTERESIS) == PIN_BM_HYSTERESIS) {
      *cfg |= GPIO_HAL_PIN_CFG_HYSTERESIS;
    }

    /* Pulling config */
    switch(pin_cfg & (PIN_GEN | PIN_BM_PULLING)) {
    case PIN_NOPULL:   *cfg |= GPIO_HAL_PIN_CFG_PULL_NONE; break;
    case PIN_PULLUP:   *cfg |= GPIO_HAL_PIN_CFG_PULL_UP;   break;
    case PIN_PULLDOWN: *cfg |= GPIO_HAL_PIN_CFG_PULL_DOWN; break;
    }
  }

  /* Interrupt config */
  if(pin_cfg & PIN_BM_IRQ) {
    /* Interrupt edge config */
    switch(pin_cfg & (PIN_GEN | PIN_BM_IRQ)) {
    case PIN_IRQ_DIS:       *cfg |= GPIO_HAL_PIN_CFG_EDGE_NONE;
                            *cfg |= GPIO_HAL_PIN_CFG_INT_DISABLE;
                            break;
    case PIN_IRQ_NEGEDGE:   *cfg |= GPIO_HAL_PIN_CFG_EDGE_FALLING;
                            *cfg |= GPIO_HAL_PIN_CFG_INT_ENABLE;
                            break;
    case PIN_IRQ_POSEDGE:   *cfg |= GPIO_HAL_PIN_CFG_EDGE_RISING;
                            *cfg |= GPIO_HAL_PIN_CFG_INT_ENABLE;
                            break;
    case PIN_IRQ_BOTHEDGES: *cfg |= GPIO_HAL_PIN_CFG_EDGE_BOTH;
                            *cfg |= GPIO_HAL_PIN_CFG_INT_ENABLE;
                            break;
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
gpio_int_cb(PIN_Handle handle, PIN_Id pin_id)
{
  /* Unused args */
  (void)handle;

  /* Notify the GPIO HAL driver */
  gpio_hal_event_handler(gpio_hal_pin_to_mask(pin_id));
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_init(void)
{
  /* No error checking */
  pin_handle = PIN_open(&pin_state, pin_config);
  PIN_registerIntCb(pin_handle, gpio_int_cb);
}
/*---------------------------------------------------------------------------*/

void
gpio_hal_arch_no_port_pin_set_input(gpio_hal_pin_t pin)
{
  PIN_add(pin_handle, PIN_getConfig(pin));

  /* Configure pin as standard input */
  PIN_Config pin_cfg = GPIO_HAL_PIN_STD_INPUT | pin;
  PIN_Config pin_mask = GPIO_HAL_PIN_STD_INPUT_BM;

  PIN_setConfig(pin_handle, pin_mask, pin_cfg);
}
/*---------------------------------------------------------------------------*/
void gpio_hal_arch_no_port_pin_set_output(gpio_hal_pin_t pin)
{
  PIN_add(pin_handle, PIN_getConfig(pin));

  /* Configure pin as standard output */
  PIN_Config pin_cfg = GPIO_HAL_PIN_STD_OUTPUT | pin;
  PIN_Config pin_mask = GPIO_HAL_PIN_STD_OUTPUT_BM;
  PIN_setConfig(pin_handle, pin_mask, pin_cfg);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_interrupt_enable(gpio_hal_pin_t pin)
{
  /*
   * This requires pin cfg already set with GPIO_HAL_PIN_CFG_INT_ENABLE,
   * i.e. enabled interrupts, thus making this function redundant.
   * Might be fixed if switching to GPIO or GPIO++ lib. instead of PIN.
   */
  PIN_Config pin_cfg;
  PIN_Config irq_cfg;

  pin_cfg = PIN_getConfig(pin);
  PIN_add(pin_handle, pin_cfg);

  irq_cfg = pin_cfg & PIN_BM_IRQ;
  PIN_setInterrupt(pin_handle, pin | irq_cfg);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_interrupt_disable(gpio_hal_pin_t pin)
{
  PIN_add(pin_handle, PIN_getConfig(pin));

  /*
   * This removes all IRQ config., thus pin cfg must be set again to
   * re-enable interrupts.
   */
  PIN_setInterrupt(pin_handle, pin | PIN_IRQ_DIS);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_no_port_pin_cfg_set(gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  PIN_add(pin_handle, PIN_getConfig(pin));

  /* Clear settings that we are about to change, keep everything else. */
  PIN_Config pin_cfg = 0;
  PIN_Config pin_mask = 0;

  from_hal_cfg(cfg, &pin_cfg, &pin_mask);

  PIN_setConfig(pin_handle, pin_mask, pin | pin_cfg);
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_no_port_pin_cfg_get(gpio_hal_pin_t pin)
{
  PIN_Config pin_cfg = PIN_getConfig(pin);
  gpio_hal_pin_cfg_t cfg = 0;

  to_hal_cfg(pin_cfg, &cfg);

  return cfg;
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_no_port_read_pins(gpio_hal_pin_mask_t pins)
{
  /* For pins configured as output we need to read DOUT31_0 */
  gpio_hal_pin_mask_t oe_pins = GPIO_getOutputEnableMultiDio(pins);

  pins &= ~oe_pins;

  return (HWREG(GPIO_BASE + GPIO_O_DOUT31_0) & oe_pins) |
         GPIO_readMultiDio(pins);
}
/*---------------------------------------------------------------------------*/
uint8_t
gpio_hal_arch_no_port_read_pin(gpio_hal_pin_t pin)
{
  return (GPIO_getOutputEnableDio(pin))
         ? PINCC26XX_getOutputValue(pin)
         : PINCC26XX_getInputValue(pin);
}
/*---------------------------------------------------------------------------*/
/** @} */
