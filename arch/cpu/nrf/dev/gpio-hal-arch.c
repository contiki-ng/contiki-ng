/*
 * Copyright (c) 2020, George Oikonomou - https://spd.gr
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-dev Device drivers
 * @{
 *
 * \addtogroup nrf-gpio GPIO HAL driver
 * @{
 *
 * \file
 *     GPIO HAL implementation for the nRF
 * \author
 *     Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "dev/gpio-hal.h"

#include "nrfx_gpiote.h"

#include "hal/nrf_gpio.h"
/*---------------------------------------------------------------------------*/
#define PIN_TO_PORT(pin) (pin >> 5)
#define PIN_TO_NUM(pin) (pin & 0x1F)
/*---------------------------------------------------------------------------*/
/* Newer nrfx uses either a multi-instance or a single-instance GPIOTE API. */
#if NRFX_API_VER_AT_LEAST(3, 2, 0)
#if defined(NRF54L15_XXAA)
/* nRF54L15 has two GPIOTE instances: GPIOTE20 for P1, GPIOTE30 for P0. */
static const nrfx_gpiote_t gpiote_instance_p0 = NRFX_GPIOTE_INSTANCE(30);
static const nrfx_gpiote_t gpiote_instance_p1 = NRFX_GPIOTE_INSTANCE(20);

/* Older nRF54L15 MDK snapshots lacked these aliases. */
#ifndef GPIOTE20_CH_NUM
#define GPIOTE20_CH_NUM 8
#endif
#ifndef GPIOTE30_CH_NUM
#define GPIOTE30_CH_NUM 4
#endif

/* Get the appropriate GPIOTE instance for a given pin */
static inline const nrfx_gpiote_t *
get_gpiote_instance(uint32_t pin)
{
  uint8_t port = PIN_TO_PORT(pin);
  return (port == 0) ? &gpiote_instance_p0 : &gpiote_instance_p1;
}
#else
static const nrfx_gpiote_t gpiote_instance = NRFX_GPIOTE_INSTANCE(0);

static inline const nrfx_gpiote_t *
get_gpiote_instance(uint32_t pin)
{
  (void)pin;
  return &gpiote_instance;
}
#endif

void
gpio_hal_arch_interrupt_enable_nrfx_v3(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  uint32_t pin_number = NRF_GPIO_PIN_MAP(port, pin);
  const nrfx_gpiote_t *p_instance = get_gpiote_instance(pin_number);
  nrfx_gpiote_trigger_enable(p_instance, pin_number, true);
}

void
gpio_hal_arch_interrupt_disable_nrfx_v3(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  uint32_t pin_number = NRF_GPIO_PIN_MAP(port, pin);
  const nrfx_gpiote_t *p_instance = get_gpiote_instance(pin_number);
  nrfx_gpiote_trigger_disable(p_instance, pin_number);
}
#endif /* NRFX_API_VER_AT_LEAST(3, 2, 0) */
/*---------------------------------------------------------------------------*/
/**
 * @brief GPIO event handler
 *
 * @param pin GPIO pin
 * @param action Action
 */
#if NRFX_API_VER_AT_LEAST(3, 2, 0)
static void
pin_event_handler(nrfx_gpiote_pin_t pin, nrfx_gpiote_trigger_t trigger, void *p_context)
{
  gpio_hal_port_t port;
  gpio_hal_pin_mask_t pin_mask;

  port = PIN_TO_PORT(pin);
  pin_mask = gpio_hal_pin_to_mask(PIN_TO_NUM(pin));

  gpio_hal_event_handler(port, pin_mask);
}
#else
static void
pin_event_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
  gpio_hal_port_t port;
  gpio_hal_pin_mask_t pin_mask;

  port = PIN_TO_PORT(pin);
  pin_mask = gpio_hal_pin_to_mask(PIN_TO_NUM(pin));

  gpio_hal_event_handler(port, pin_mask);
}
#endif
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_init(void)
{
#if NRFX_API_VER_AT_LEAST(3, 2, 0)
#if defined(NRF54L15_XXAA)
  if(!nrfx_gpiote_init_check(&gpiote_instance_p0)) {
    nrfx_gpiote_init(&gpiote_instance_p0, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
  }
  if(!nrfx_gpiote_init_check(&gpiote_instance_p1)) {
    nrfx_gpiote_init(&gpiote_instance_p1, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
  }
#else
  if(!nrfx_gpiote_init_check(&gpiote_instance)) {
    nrfx_gpiote_init(&gpiote_instance, NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
  }
#endif
#else
  if(!nrfx_gpiote_is_init()) {
    nrfx_gpiote_init(NRFX_GPIOTE_DEFAULT_CONFIG_IRQ_PRIORITY);
  }
#endif
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_pin_cfg_set(gpio_hal_port_t port, gpio_hal_pin_t pin, gpio_hal_pin_cfg_t cfg)
{
  gpio_hal_pin_cfg_t tmp;
  uint32_t pin_number = NRF_GPIO_PIN_MAP(port, pin);

#if NRFX_API_VER_AT_LEAST(3, 2, 0)
  const nrfx_gpiote_t *p_instance = get_gpiote_instance(pin_number);
  uint8_t channel;
  nrfx_err_t err;

  nrf_gpio_pin_pull_t pull_config = NRF_GPIO_PIN_NOPULL;

  nrfx_gpiote_trigger_config_t trigger_config = {
    .trigger = NRFX_GPIOTE_TRIGGER_NONE,
  };

  nrfx_gpiote_handler_config_t handler_config = {
    .handler = pin_event_handler,
    .p_context = NULL,
  };

  tmp = cfg & GPIO_HAL_PIN_CFG_EDGE_BOTH;
  if(tmp == GPIO_HAL_PIN_CFG_EDGE_RISING) {
    trigger_config.trigger = NRFX_GPIOTE_TRIGGER_LOTOHI;
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_FALLING) {
    trigger_config.trigger = NRFX_GPIOTE_TRIGGER_HITOLO;
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_BOTH) {
    trigger_config.trigger = NRFX_GPIOTE_TRIGGER_TOGGLE;
  }

  if(trigger_config.trigger != NRFX_GPIOTE_TRIGGER_NONE) {
    err = nrfx_gpiote_channel_get(p_instance, pin_number, &channel);
    if(err == NRFX_ERROR_INVALID_PARAM) {
      err = nrfx_gpiote_channel_alloc(p_instance, &channel);
    }
    if(err == NRFX_SUCCESS) {
      trigger_config.p_in_channel = &channel;
    }
  }

  tmp = cfg & GPIO_HAL_PIN_CFG_PULL_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_PULL_DOWN) {
    pull_config = NRF_GPIO_PIN_PULLDOWN;
  } else if(tmp == GPIO_HAL_PIN_CFG_PULL_UP) {
    pull_config = NRF_GPIO_PIN_PULLUP;
  }

  nrfx_gpiote_input_pin_config_t input_pin_config = {
    .p_pull_config = &pull_config,
    .p_trigger_config = &trigger_config,
    .p_handler_config = &handler_config,
  };

  err = nrfx_gpiote_input_configure(p_instance, pin_number, &input_pin_config);
  if(err != NRFX_SUCCESS) {
    return;
  }

  tmp = cfg & GPIO_HAL_PIN_CFG_INT_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_INT_ENABLE) {
    gpio_hal_arch_interrupt_enable_nrfx_v3(port, pin);
  } else {
    gpio_hal_arch_interrupt_disable_nrfx_v3(port, pin);
  }
#else /* NRFX_API_VER_AT_LEAST(3, 2, 0) */
  nrfx_gpiote_in_config_t gpiote_config = {
    .is_watcher = false,
    .hi_accuracy = true,
  };

  tmp = cfg & GPIO_HAL_PIN_CFG_EDGE_BOTH;
  if(tmp == GPIO_HAL_PIN_CFG_EDGE_NONE) {
    gpiote_config.sense = GPIOTE_CONFIG_POLARITY_None;
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_RISING) {
    gpiote_config.sense = NRF_GPIOTE_POLARITY_LOTOHI;
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_FALLING) {
    gpiote_config.sense = NRF_GPIOTE_POLARITY_HITOLO;
  } else if(tmp == GPIO_HAL_PIN_CFG_EDGE_BOTH) {
    gpiote_config.sense = NRF_GPIOTE_POLARITY_TOGGLE;
  }

  tmp = cfg & GPIO_HAL_PIN_CFG_PULL_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_PULL_NONE) {
    gpiote_config.pull = NRF_GPIO_PIN_NOPULL;
  } else if(tmp == GPIO_HAL_PIN_CFG_PULL_DOWN) {
    gpiote_config.pull = NRF_GPIO_PIN_PULLDOWN;
  } else if(tmp == GPIO_HAL_PIN_CFG_PULL_UP) {
    gpiote_config.pull = NRF_GPIO_PIN_PULLUP;
  }

  nrfx_gpiote_in_init(pin_number, &gpiote_config, pin_event_handler);

  tmp = cfg & GPIO_HAL_PIN_CFG_INT_MASK;
  if(tmp == GPIO_HAL_PIN_CFG_INT_DISABLE) {
    nrfx_gpiote_in_event_disable(pin_number);
  } else if(tmp == GPIO_HAL_PIN_CFG_INT_ENABLE) {
    nrfx_gpiote_in_event_enable(pin_number, true);
  }
#endif /* NRFX_API_VER_AT_LEAST(3, 2, 0) */
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_cfg_t
gpio_hal_arch_port_pin_cfg_get(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  uint8_t i;
  uint32_t pin_number;
  gpio_hal_pin_cfg_t cfg = GPIO_HAL_PIN_CFG_PULL_NONE |
    GPIO_HAL_PIN_CFG_EDGE_NONE |
    GPIO_HAL_PIN_CFG_INT_DISABLE;
  nrf_gpio_pin_pull_t pull;
  nrf_gpiote_polarity_t polarity;

  pin_number = NRF_GPIO_PIN_MAP(port, pin);

  /* First, check if the pin is configured as output */
  if(nrf_gpio_pin_dir_get(pin_number) == NRF_GPIO_PIN_DIR_OUTPUT) {
    return 0;
  }

#if defined(NRF54L15_XXAA)
  /* nRF54L15: Select correct GPIOTE instance based on port */
  NRF_GPIOTE_Type *gpiote_reg = (port == 0) ? NRF_GPIOTE30 : NRF_GPIOTE20;
  uint8_t ch_num = (port == 0) ? GPIOTE30_CH_NUM : GPIOTE20_CH_NUM;

  /*
   * Input pin. Check all GPIOTE channel configurations and figure out which
   * channel corresponds to our pin of interest. For that channel, read out
   * the GPIOTE configuration
   */
  for(i = 0; i < ch_num; i++) {
    if(nrf_gpiote_event_pin_get(gpiote_reg, i) == pin_number) {
      polarity = nrf_gpiote_event_polarity_get(gpiote_reg, i);

      if(polarity == NRF_GPIOTE_POLARITY_LOTOHI) {
        cfg |= GPIO_HAL_PIN_CFG_EDGE_RISING;
      } else if(polarity == NRF_GPIOTE_POLARITY_HITOLO) {
        cfg |= GPIO_HAL_PIN_CFG_EDGE_FALLING;
      } else if(polarity == NRF_GPIOTE_POLARITY_TOGGLE) {
        cfg |= GPIO_HAL_PIN_CFG_EDGE_BOTH;
      }

      pull = nrf_gpio_pin_pull_get(pin_number);

      if(pull == NRF_GPIO_PIN_PULLDOWN) {
        cfg |= GPIO_HAL_PIN_CFG_PULL_DOWN;
      } else if(pull == NRF_GPIO_PIN_PULLUP) {
        cfg |= GPIO_HAL_PIN_CFG_PULL_UP;
      }

      if(nrf_gpiote_int_enable_check(gpiote_reg, 1 << i)) {
        cfg |= GPIO_HAL_PIN_CFG_INT_ENABLE;
      }
      return cfg;
    }
  }
#else
  /*
   * Input pin. Check all GPIOTE channel configurations and figure out which
   * channel corresponds to our pin of interest. For that channel, read out
   * the GPIOTE configuration
   */
  for(i = 0; i < GPIOTE_CH_NUM; i++) {
    if(nrf_gpiote_event_pin_get(NRF_GPIOTE, i) == pin_number) {
      polarity = nrf_gpiote_event_polarity_get(NRF_GPIOTE, i);

      if(polarity == NRF_GPIOTE_POLARITY_LOTOHI) {
        cfg |= GPIO_HAL_PIN_CFG_EDGE_BOTH;
      } else if(polarity == NRF_GPIOTE_POLARITY_HITOLO) {
        cfg |= GPIO_HAL_PIN_CFG_EDGE_BOTH;
      } else if(polarity == NRF_GPIOTE_POLARITY_TOGGLE) {
        cfg |= GPIO_HAL_PIN_CFG_EDGE_BOTH;
      }

      pull = nrf_gpio_pin_pull_get(pin_number);

      if(pull == NRF_GPIO_PIN_PULLDOWN) {
        cfg |= GPIO_HAL_PIN_CFG_PULL_DOWN;
      } else if(pull == NRF_GPIO_PIN_PULLUP) {
        cfg |= GPIO_HAL_PIN_CFG_PULL_UP;
      }

      if(nrf_gpiote_int_enable_check(NRF_GPIOTE, 1 << i)) {
        cfg |= GPIO_HAL_PIN_CFG_INT_ENABLE;
      }
      return cfg;
    }
  }
#endif

  /* Did not find a GPIOTE channel configured for this pin */
  return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
gpio_hal_arch_port_read_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  return (uint8_t)nrf_gpio_pin_read(NRF_GPIO_PIN_MAP(port, pin));
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_set_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  NRF_GPIO_Type *gpio_regs[GPIO_COUNT] = GPIO_REG_LIST;

  if(port >= GPIO_COUNT) {
    return;
  }

  nrf_gpio_port_out_set(gpio_regs[port], pins);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_clear_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  NRF_GPIO_Type *gpio_regs[GPIO_COUNT] = GPIO_REG_LIST;

  if(port >= GPIO_COUNT) {
    return;
  }

  nrf_gpio_port_out_clear(gpio_regs[port], pins);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_toggle_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  if(port >= GPIO_COUNT) {
    return;
  }
  gpio_hal_arch_write_pins(port, pins, ~gpio_hal_arch_read_pins(port, pins));
}
/*---------------------------------------------------------------------------*/
gpio_hal_pin_mask_t
gpio_hal_arch_port_read_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  NRF_GPIO_Type *gpio_regs[GPIO_COUNT] = GPIO_REG_LIST;

  if(port >= GPIO_COUNT) {
    return 0;
  }

  return nrf_gpio_port_in_read(gpio_regs[port]);
}
/*---------------------------------------------------------------------------*/
void
gpio_hal_arch_port_write_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins,
                              gpio_hal_pin_mask_t value)
{
  NRF_GPIO_Type *gpio_regs[GPIO_COUNT] = GPIO_REG_LIST;

  if(port >= GPIO_COUNT) {
    return;
  }

  nrf_gpio_port_out_write(gpio_regs[port], value);
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
