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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup simplelink-platform
 * @{
 *
 * \file
 *  Driver for LaunchPad LEDs
 */
/*---------------------------------------------------------------------------*/
/* Contiki API */
#include <contiki.h>
#include <dev/leds.h>
/*---------------------------------------------------------------------------*/
/* Simplelink SDK API */
#include <Board.h>

#include <ti/drivers/GPIO.h>
/*---------------------------------------------------------------------------*/
/* Standard library */
#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
/* Available LED configuration */

/* LED */
#define LEDS_ARCH     Board_GPIO_LED0
/*---------------------------------------------------------------------------*/
static volatile unsigned char c;
/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
  static bool bHasInit = false;
  if(bHasInit) {
    return;
  }
  bHasInit = true;

  // GPIO_init will most likely be called in platform.c,
  // but call it here to be sure GPIO is initialized.
  // Calling GPIO_init multiple times is safe.
  GPIO_init();
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_arch_get(void)
{
  return c;
}
/*---------------------------------------------------------------------------*/
static inline void
write_led(const bool on, const uint_fast32_t gpioLed)
{
  const GPIO_PinConfig pinCfg = (on)
    ? Board_GPIO_LED_ON
    : Board_GPIO_LED_OFF;
  GPIO_write(gpioLed, pinCfg);
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char leds)
{
  c = leds;

  // Green LED
  uint_fast32_t gpioOn = (leds & (LEDS_ARCH)) == (LEDS_ARCH);
  write_led(gpioOn, LEDS_ARCH);
}
/*---------------------------------------------------------------------------*/
/** @} */
