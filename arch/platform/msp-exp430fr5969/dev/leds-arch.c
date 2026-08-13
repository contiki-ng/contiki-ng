/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
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
 */

/**
 * \file
 *         LED driver for MSP-EXP430FR5969 LaunchPad
 *
 *         LED1 (Red):   P1.0
 *         LED2 (Green): P4.6
 */

#include "contiki.h"
#include "dev/leds.h"
#include <msp430.h>

/*---------------------------------------------------------------------------*/
/* LED pin definitions */
#define LED_RED_BIT   (1 << 0)  /* P1.0 */
#define LED_GREEN_BIT (1 << 6)  /* P4.6 */

/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
  /* Set LED pins as outputs */
  P1DIR |= LED_RED_BIT;
  P4DIR |= LED_GREEN_BIT;

  /* Turn off all LEDs initially */
  P1OUT &= ~LED_RED_BIT;
  P4OUT &= ~LED_GREEN_BIT;
}
/*---------------------------------------------------------------------------*/
leds_mask_t
leds_arch_get(void)
{
  leds_mask_t leds = 0;

  if(P1OUT & LED_RED_BIT) {
    leds |= LEDS_RED;
  }
  if(P4OUT & LED_GREEN_BIT) {
    leds |= LEDS_GREEN;
  }

  return leds;
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(leds_mask_t leds)
{
  /* Red LED on P1.0 */
  if(leds & LEDS_RED) {
    P1OUT |= LED_RED_BIT;
  } else {
    P1OUT &= ~LED_RED_BIT;
  }

  /* Green LED on P4.6 */
  if(leds & LEDS_GREEN) {
    P4OUT |= LED_GREEN_BIT;
  } else {
    P4OUT &= ~LED_GREEN_BIT;
  }
}
/*---------------------------------------------------------------------------*/
