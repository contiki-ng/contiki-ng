/*
 * Copyright (c) 2015, Nordic Semiconductor
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

/**
 * \addtogroup nrf52840dongle
 * @{
 *
 * \addtogroup nrf52840dongle-devices Device drivers
 * @{
 *
 * \addtogroup nrf52840dongle-devices-led LED driver
 * @{
 *
 * \file
 *         Architecture specific LED driver implementation for nRF52 DK.
 * \author
 *         Wojciech Bober <wojciech.bober@nordicsemi.no>
 */
#include "boards.h"
#include "contiki.h"
#include "dev/leds.h"

/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
  nrf_gpio_cfg_output(LED1_G);
  nrf_gpio_cfg_output(LED2_R);
  nrf_gpio_cfg_output(LED2_G);
  nrf_gpio_cfg_output(LED2_B);
}
/*---------------------------------------------------------------------------*/
leds_mask_t
leds_arch_get(void)
{
  leds_mask_t mask = (leds_mask_t)
    nrf_gpio_pin_read(LED1_G) |
    nrf_gpio_pin_read(LED2_R) << 1 |
    nrf_gpio_pin_read(LED2_G) << 2 |
    nrf_gpio_pin_read(LED2_B) << 3;

  return mask;
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(leds_mask_t leds)
{
  nrf_gpio_pin_set(LED1_G);
  nrf_gpio_pin_set(LED2_R);
  nrf_gpio_pin_set(LED2_G);
  nrf_gpio_pin_set(LED2_B);

  if(leds & 1) {
    nrf_gpio_pin_clear(LED1_G);
  }
  if(leds & 2) {
    nrf_gpio_pin_clear(LED2_R);
  }
  if(leds & 4) {
    nrf_gpio_pin_clear(LED2_G);
  }
  if(leds & 8) {
    nrf_gpio_pin_clear(LED2_B);
  }
}
/*---------------------------------------------------------------------------*/

/**
 * @}
 * @}
 * @}
 */
