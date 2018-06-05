/*
 * Copyright (c) 2018, RISE SICS
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

#include "contiki.h"
#include "em_device.h"
#include "em_gpio.h"
#include "em_cmu.h"
#include "em_timer.h"

/**
 * RGB LEDS API.
 * Configuration inspired by Silabs demo code for TB Sense 2 - see
 * Simplicity Studio TB Sense 2 BLE Demo software for more advanced
 * RGB LED driver with tables for more correct intensity and more
 * linear RGB mapping.
 */

static TIMER_TypeDef     *pwm_timer;
/*---------------------------------------------------------------------------*/
void
rgbleds_init(void)
{
  TIMER_Init_TypeDef   timerInit      = TIMER_INIT_DEFAULT;
  TIMER_InitCC_TypeDef ccInit         = TIMER_INITCC_DEFAULT;

  /* Configure all the pins for the RGB LEDs */
  CMU_ClockEnable(BOARD_RGBLED_CMU_CLK, true);
  GPIO_PinModeSet(BOARD_RGBLED_RED_PORT, BOARD_RGBLED_RED_PIN,
                  gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_RGBLED_GREEN_PORT, BOARD_RGBLED_GREEN_PIN,
                  gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_RGBLED_BLUE_PORT, BOARD_RGBLED_BLUE_PIN,
                  gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_RGBLED_PWR_EN_PORT, BOARD_RGBLED_PWR_EN_PIN,
                  gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_RGBLED_COM0_PORT, BOARD_RGBLED_COM0_PIN,
                  gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_RGBLED_COM1_PORT, BOARD_RGBLED_COM1_PIN,
                  gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_RGBLED_COM2_PORT, BOARD_RGBLED_COM2_PIN,
                  gpioModePushPull, 0);
  GPIO_PinModeSet(BOARD_RGBLED_COM3_PORT, BOARD_RGBLED_COM3_PIN,
                  gpioModePushPull, 0);

  pwm_timer = BOARD_RGBLED_TIMER;

  timerInit.debugRun = true;
  timerInit.prescale = timerPrescale2;
  TIMER_Init(pwm_timer, &timerInit);

  /* 65536 counts at 38.4 MHz / 2 =  293 Hz. */
  TIMER_TopSet(pwm_timer, 0xFFFF);

  /* Initialize pwm CC registers */
  ccInit.mode  = timerCCModePWM;
  TIMER_InitCC(pwm_timer, 0, &ccInit);
  TIMER_InitCC(pwm_timer, 1, &ccInit);
  TIMER_InitCC(pwm_timer, 2, &ccInit);

  pwm_timer->ROUTEPEN  = 0;
  pwm_timer->ROUTELOC0 =
    (BOARD_RGBLED_RED_CCLOC << _TIMER_ROUTELOC0_CC0LOC_SHIFT)
    | (BOARD_RGBLED_GREEN_CCLOC << _TIMER_ROUTELOC0_CC1LOC_SHIFT)
    | (BOARD_RGBLED_BLUE_CCLOC  << _TIMER_ROUTELOC0_CC2LOC_SHIFT);
}
/*---------------------------------------------------------------------------*/
void
rgbleds_enable(uint8_t leds)
{
  int i;

  if(pwm_timer == NULL) {
    return;
  }

  if(leds != 0) {
    GPIO_PinOutSet(BOARD_RGBLED_PWR_EN_PORT, BOARD_RGBLED_PWR_EN_PIN);
  } else {
    GPIO_PinOutClear(BOARD_RGBLED_PWR_EN_PORT, BOARD_RGBLED_PWR_EN_PIN);
  }

  /* pins happen to be 0 - 3 on the TB Sense 2 - any other board might
     need mapping here */
  for(i = 0; i < 4; i++) {
    if(((leds >> i) & 1) == 1) {
      GPIO_PinOutSet(BOARD_RGBLED_COM_PORT, i);
    } else {
      GPIO_PinOutClear(BOARD_RGBLED_COM_PORT, i);
    }
  }
}
/*---------------------------------------------------------------------------*/
/* set color for all RGB LEDs */
void
rgbleds_setcolor(uint16_t red, uint16_t green, uint16_t blue)
{
  if(pwm_timer == NULL) {
    return;
  }

  if((red == 0) && (green == 0) && (blue == 0) ) {
    TIMER_Enable(pwm_timer, false);
    TIMER_CompareBufSet(pwm_timer, 0, 0);
    TIMER_CompareBufSet(pwm_timer, 1, 0);
    TIMER_CompareBufSet(pwm_timer, 2, 0);
    /* Ensure LED pins are disabled before changing ROUTE */
    rgbleds_enable(0);
    pwm_timer->ROUTEPEN  = 0;
  } else {
    TIMER_Enable(pwm_timer, true);
    TIMER_CompareBufSet(pwm_timer, 0, red);
    TIMER_CompareBufSet(pwm_timer, 1, green);
    TIMER_CompareBufSet(pwm_timer, 2, blue);
    pwm_timer->ROUTEPEN  =
      TIMER_ROUTEPEN_CC0PEN |
      TIMER_ROUTEPEN_CC1PEN |
      TIMER_ROUTEPEN_CC2PEN;
  }
}
/*---------------------------------------------------------------------------*/
