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
 *     GPIO HAL header file for the nRF
 * \author
 *     Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#ifndef GPIO_HAL_ARCH_H_
#define GPIO_HAL_ARCH_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "nrfx_gpiote.h"
/*---------------------------------------------------------------------------*/
#define gpio_hal_arch_interrupt_enable(port, pin)  nrfx_gpiote_in_event_enable(NRF_GPIO_PIN_MAP(port, pin), true)
#define gpio_hal_arch_interrupt_disable(port, pin) nrfx_gpiote_in_event_disable(NRF_GPIO_PIN_MAP(port, pin))
/*---------------------------------------------------------------------------*/
#define gpio_hal_arch_pin_set_input(port, pin)     nrf_gpio_cfg_input(NRF_GPIO_PIN_MAP(port, pin), NRF_GPIO_PIN_NOPULL)
#define gpio_hal_arch_pin_set_output(port, pin)    nrf_gpio_cfg_output(NRF_GPIO_PIN_MAP(port, pin))
/*---------------------------------------------------------------------------*/
#define gpio_hal_arch_set_pin(port, pin)           nrf_gpio_pin_set(NRF_GPIO_PIN_MAP(port, pin))
#define gpio_hal_arch_clear_pin(port, pin)         nrf_gpio_pin_clear(NRF_GPIO_PIN_MAP(port, pin))
#define gpio_hal_arch_toggle_pin(port, pin)        nrf_gpio_pin_toggle(NRF_GPIO_PIN_MAP(port, pin))
#define gpio_hal_arch_write_pin(port, pin, v)      nrf_gpio_pin_write(NRF_GPIO_PIN_MAP(port, pin), v)
/*---------------------------------------------------------------------------*/
#endif /* GPIO_HAL_ARCH_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
