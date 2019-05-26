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
 * \addtogroup NRF52
 * @{
 *
 * \defgroup nrf52-gpio-hal NRF52 GPIO HAL implementation
 *
 * @{
 *
 * \file
 *     Header file for the NRF52 GPIO HAL functions
 *
 * \note
 *     Do not include this header directly
 */
/*---------------------------------------------------------------------------*/
#ifndef GPIO_HAL_ARCH_H_
#define GPIO_HAL_ARCH_H_
/*---------------------------------------------------------------------------*/
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"

/*---------------------------------------------------------------------------*/
#define gpio_hal_arch_init()               do { /* Do nothing */ } while(0)

#define gpio_hal_arch_interrupt_enable(p)  nrf_drv_gpiote_in_event_enable(p, true)
#define gpio_hal_arch_interrupt_disable(p) nrf_drv_gpiote_in_event_disable(p)

#define gpio_hal_arch_pin_set_input(p)     nrf_gpio_cfg_input(p, NRF_GPIO_PIN_PULLUP);
#define gpio_hal_arch_pin_set_output(p)    nrf_gpio_cfg_output(p)

#define gpio_hal_arch_set_pin(p)           nrf_gpio_pin_set(p)
#define gpio_hal_arch_clear_pin(p)         nrf_gpio_pin_clear(p)
#define gpio_hal_arch_write_pin(p, v)      nrf_gpio_pin_write(p, v)
#define gpio_hal_arch_read_pin(p)		   nrf_gpio_pin_read(p)
#define gpio_hal_arch_toggle_pin(p)        nrf_gpio_pin_toggle(p)

#endif /* GPIO_HAL_ARCH_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
