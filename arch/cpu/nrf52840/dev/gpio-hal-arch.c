/*
 * Copyright (c) 2019, Carlo Vallati - http://www.unipi.it
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
 * \addtogroup nrf52-gpio-hal
 * @{
 *
 * \file
 *     Implementation file for the NRF52 GPIO HAL functions
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"

#include <stdint.h>

/*---------------------------------------------------------------------------*/

#define GPIO_PIN_MASK(PIN) (1 << (PIN))

gpio_hal_pin_cfg_t cfgs[GPIO_HAL_PIN_COUNT];

/*---------------------------------------------------------------------------*/
void
gpio_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  gpio_hal_pin_mask_t pin_mask = 0;

  pin_mask |= gpio_hal_pin_to_mask(pin);

  gpio_hal_event_handler(pin_mask);
}

/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_pin_cfg_set(gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  gpio_hal_pin_cfg_t tmp;
  uint32_t err_code;

  // Initialize, if uninitialized
  if(!nrf_drv_gpiote_is_init())
  {
      err_code = nrf_drv_gpiote_init();
      if( err_code != NRF_SUCCESS )
      {
    	  return;
      }
  }

  // Remove prev config
  nrf_drv_gpiote_in_uninit(pin);
  cfgs[pin] = cfg;

  tmp = cfg & GPIO_HAL_PIN_CFG_EDGE_BOTH;
  if(tmp == GPIO_HAL_PIN_CFG_EDGE_RISING) {
	nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_LOTOHI(false);

	if((cfg & GPIO_HAL_PIN_CFG_PULL_MASK) == GPIO_HAL_PIN_CFG_PULL_UP)
		in_config.pull = NRF_GPIO_PIN_PULLUP;

	err_code = nrf_drv_gpiote_in_init(pin, &in_config, gpio_handler);
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_FALLING) {
	nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_HITOLO(false);

	if((cfg & GPIO_HAL_PIN_CFG_PULL_MASK) == GPIO_HAL_PIN_CFG_PULL_DOWN)
		in_config.pull = NRF_GPIO_PIN_PULLDOWN;

	err_code = nrf_drv_gpiote_in_init(pin, &in_config, gpio_handler);
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_BOTH) {
	nrf_drv_gpiote_in_config_t in_config = NRFX_GPIOTE_CONFIG_IN_SENSE_TOGGLE(false);

	if((cfg & GPIO_HAL_PIN_CFG_PULL_MASK) == GPIO_HAL_PIN_CFG_PULL_DOWN)
		in_config.pull = NRF_GPIO_PIN_PULLDOWN;
	else if((cfg & GPIO_HAL_PIN_CFG_PULL_MASK) == GPIO_HAL_PIN_CFG_PULL_UP)
		in_config.pull = NRF_GPIO_PIN_PULLUP;

	err_code = nrf_drv_gpiote_in_init(pin, &in_config, gpio_handler);
  }

  tmp = cfg & GPIO_HAL_PIN_CFG_INT_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_INT_DISABLE) {
	nrf_drv_gpiote_in_event_disable(pin);
  } else if(tmp == GPIO_HAL_PIN_CFG_INT_ENABLE) {
	nrf_drv_gpiote_in_event_enable(pin, true);
  }

}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_pin_cfg_get(gpio_hal_pin_t pin)
{
  if(nrfx_gpiote_in_is_set(pin))
	  return cfgs[pin];
  else
	  return 0;

}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_read_pins(gpio_hal_pin_mask_t pins)
{
  gpio_hal_pin_mask_t oe_pins = 0;
  uint32_t i;

  for( i=0; i < GPIO_HAL_PIN_COUNT; i++ ){
	  uint32_t p = gpio_hal_pin_to_mask(i);
	  if((p & pins) && gpio_hal_arch_read_pin(i)){
		  oe_pins &= p;
	  }
  }

  return oe_pins;
}
/*---------------------------------------------------------------------------*/
void gpio_hal_arch_write_pins(gpio_hal_pin_mask_t pins,
                              gpio_hal_pin_mask_t value){
  uint32_t i;

  for( i=0; i < GPIO_HAL_PIN_COUNT; i++ ){
	  uint32_t p = gpio_hal_pin_to_mask(i);
	  if(p & pins){
		  gpio_hal_arch_write_pin(i,(p & value));
	  }
  }
}
/*---------------------------------------------------------------------------*/
void gpio_hal_arch_toggle_pins(gpio_hal_pin_mask_t pins){
  uint32_t i;

  for( i=0; i < GPIO_HAL_PIN_COUNT; i++ ){
	  uint32_t p = gpio_hal_pin_to_mask(i);
	  if(p & pins){
		  gpio_hal_arch_toggle_pin(i);
	  }
  }
}
/*---------------------------------------------------------------------------*/
void gpio_hal_arch_clear_pins(gpio_hal_pin_mask_t pins){
  uint32_t i;

  for( i=0; i < GPIO_HAL_PIN_COUNT; i++ ){
	  uint32_t p = gpio_hal_pin_to_mask(i);
	  if(p & pins){
		  gpio_hal_arch_clear_pin(i);
	  }
  }
}
/*---------------------------------------------------------------------------*/
void gpio_hal_arch_set_pins(gpio_hal_pin_mask_t pins){

  uint32_t i;

  for( i=0; i < GPIO_HAL_PIN_COUNT; i++ ){
	  uint32_t p = gpio_hal_pin_to_mask(i);
	  if(p & pins){
		  gpio_hal_arch_set_pin(i);
	  }
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
