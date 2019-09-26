/*
 * Copyright (c) 2005, Swedish Institute of Computer Science
 * Copyright (c) 2018, George Oikonomou - http://www.spd.gr
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
 * \addtogroup leds
 * @{
 *
 * \file
 *     Implementation of the platform-independent aspects of the LED HAL
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"
#include "dev/leds.h"

#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#if LEDS_LEGACY_API
/*---------------------------------------------------------------------------*/
void
leds_init(void)
{
  leds_arch_init();
}
/*---------------------------------------------------------------------------*/
void
leds_blink(void)
{
  /* Blink all leds that were initially off. */
  unsigned char blink;
  blink = ~leds_arch_get();
  leds_toggle(blink);

  clock_delay(400);

  leds_toggle(blink);
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_get(void) {
  return leds_arch_get();
}
/*---------------------------------------------------------------------------*/
void
leds_set(leds_mask_t ledv)
{
  leds_arch_set(ledv);
}
/*---------------------------------------------------------------------------*/
void
leds_on(leds_mask_t ledv)
{
  leds_arch_set(leds_arch_get() | ledv);
}
/*---------------------------------------------------------------------------*/
void
leds_off(leds_mask_t ledv)
{
  leds_arch_set(leds_arch_get() & ~ledv);
}
/*---------------------------------------------------------------------------*/
void
leds_toggle(leds_mask_t ledv)
{
  leds_arch_set(leds_arch_get() ^ ledv);
}
/*---------------------------------------------------------------------------*/
#else /* LEDS_LEGACY_API */
/*---------------------------------------------------------------------------*/
#if LEDS_COUNT
extern const leds_t leds_arch_leds[];
#else
static const leds_t *leds_arch_leds = NULL;
#endif
/*---------------------------------------------------------------------------*/
#if GPIO_HAL_PORT_PIN_NUMBERING
#define LED_PORT(led) (led).port
#else
#define LED_PORT(led) GPIO_HAL_NULL_PORT
#endif
/*---------------------------------------------------------------------------*/
void
leds_init()
{
  leds_num_t led;

  for(led = 0; led < LEDS_COUNT; led++) {
    gpio_hal_arch_pin_set_output(LED_PORT(leds_arch_leds[led]),
                                 leds_arch_leds[led].pin);
  }
  leds_off(LEDS_ALL);
}
/*---------------------------------------------------------------------------*/
void
leds_single_on(leds_num_t led)
{
  if(led >= LEDS_COUNT) {
    return;
  }

  if(leds_arch_leds[led].negative_logic) {
    gpio_hal_arch_clear_pin(LED_PORT(leds_arch_leds[led]),
                                     leds_arch_leds[led].pin);
  } else {
    gpio_hal_arch_set_pin(LED_PORT(leds_arch_leds[led]),
                          leds_arch_leds[led].pin);
  }
}
/*---------------------------------------------------------------------------*/
void
leds_single_off(leds_num_t led)
{
  if(led >= LEDS_COUNT) {
    return;
  }

  if(leds_arch_leds[led].negative_logic) {
    gpio_hal_arch_set_pin(LED_PORT(leds_arch_leds[led]),
                          leds_arch_leds[led].pin);
  } else {
    gpio_hal_arch_clear_pin(LED_PORT(leds_arch_leds[led]),
                            leds_arch_leds[led].pin);
  }
}
/*---------------------------------------------------------------------------*/
void
leds_single_toggle(leds_num_t led)
{
  if(led >= LEDS_COUNT) {
    return;
  }

  gpio_hal_arch_toggle_pin(LED_PORT(leds_arch_leds[led]),
                           leds_arch_leds[led].pin);
}
/*---------------------------------------------------------------------------*/
void
leds_on(leds_mask_t leds)
{
  leds_num_t led;

  for(led = 0; led < LEDS_COUNT; led++) {
    if((1 << led) & leds) {
      if(leds_arch_leds[led].negative_logic) {
        gpio_hal_arch_clear_pin(LED_PORT(leds_arch_leds[led]),
                                leds_arch_leds[led].pin);
      } else {
        gpio_hal_arch_set_pin(LED_PORT(leds_arch_leds[led]),
                              leds_arch_leds[led].pin);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
leds_off(leds_mask_t leds)
{
  leds_num_t led;

  for(led = 0; led < LEDS_COUNT; led++) {
    if((1 << led) & leds) {
      if(leds_arch_leds[led].negative_logic) {
        gpio_hal_arch_set_pin(LED_PORT(leds_arch_leds[led]),
                              leds_arch_leds[led].pin);
      } else {
        gpio_hal_arch_clear_pin(LED_PORT(leds_arch_leds[led]),
                                leds_arch_leds[led].pin);
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
void
leds_toggle(leds_mask_t leds)
{
  leds_num_t led;

  for(led = 0; led < LEDS_COUNT; led++) {
    if((1 << led) & leds) {
      gpio_hal_arch_toggle_pin(LED_PORT(leds_arch_leds[led]),
                               leds_arch_leds[led].pin);
    }
  }
}
/*---------------------------------------------------------------------------*/
void
leds_set(leds_mask_t leds)
{
  leds_off(LEDS_ALL);
  leds_on(leds);
}
/*---------------------------------------------------------------------------*/
leds_mask_t
leds_get()
{
  leds_mask_t rv = 0;
  leds_num_t led;
  uint8_t pin_state;

  for(led = 0; led < LEDS_COUNT; led++) {
    pin_state = gpio_hal_arch_read_pin(LED_PORT(leds_arch_leds[led]),
                                       leds_arch_leds[led].pin);

    if((leds_arch_leds[led].negative_logic == false && pin_state == 1) ||
       (leds_arch_leds[led].negative_logic == true && pin_state == 0)) {
      rv |= 1 << led;
    }
  }

  return rv;
}
/*---------------------------------------------------------------------------*/
#endif /* LEDS_LEGACY_API */
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
