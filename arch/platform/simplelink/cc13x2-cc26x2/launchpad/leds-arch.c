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

/* Green LED */
#ifdef Board_GPIO_GLED
#   define LEDS_ARCH_GREEN    Board_GPIO_GLED
#endif

/* Yellow LED */
#ifdef Board_GPIO_YLED
#   define LEDS_ARCH_YELLOW   Board_GPIO_YLED
#endif

/* Red LED */
#ifdef Board_GPIO_RLED
#   define LEDS_ARCH_RED      Board_GPIO_RLED
#endif

/* Blue LED */
#ifdef Board_GPIO_BLED
#   define LEDS_ARCH_BLUE     Board_GPIO_BLED
#endif
/*---------------------------------------------------------------------------*/
static unsigned char c;
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
    ? Board_GPIO_LED_ON : Board_GPIO_LED_OFF;
  GPIO_write(gpioLed, pinCfg);
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char leds)
{
  c = leds;

#define LED_ON(led_define)  ((leds & (led_define)) == (led_define))

  // Green LED
#ifdef LEDS_ARCH_GREEN
  write_led(LED_ON(LEDS_GREEN), LEDS_ARCH_GREEN);
#endif

  // Yellow LED
#ifdef LEDS_ARCH_YELLOW
  write_led(LED_ON(LEDS_YELLOW), LEDS_ARCH_YELLOW);
#endif

  // Red LED
#ifdef LEDS_ARCH_RED
  write_led(LED_ON(LEDS_RED), LEDS_ARCH_RED);
#endif

  // Blue LED
#ifdef LEDS_ARCH_BLUE
  write_led(LED_ON(LEDS_BLUE), LEDS_ARCH_BLUE);
#endif

#undef LED_ON
}
/*---------------------------------------------------------------------------*/
/** @} */
