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

#include <ti/drivers/PIN.h>
/*---------------------------------------------------------------------------*/
/* Standard library */
#include <stdbool.h>
#include <stdint.h>
/*---------------------------------------------------------------------------*/
static const PIN_Config pin_table[] = {
  Board_PIN_LED0 | PIN_GPIO_OUTPUT_EN | PIN_GPIO_LOW | PIN_PUSHPULL | PIN_DRVSTR_MAX,
  PIN_TERMINATE
};

static PIN_State pin_state;
static PIN_Handle pin_handle;

static volatile unsigned char c;
/*---------------------------------------------------------------------------*/
void
leds_arch_init(void)
{
  static bool bHasInit = false;
  if(bHasInit) {
    return;
  }

  // PIN_init() called from Board_initGeneral()
  pin_handle = PIN_open(&pin_state, pin_table);
  if (!pin_handle) {
    return;
  }

  bHasInit = true;
}
/*---------------------------------------------------------------------------*/
unsigned char
leds_arch_get(void)
{
  return c;
}
/*---------------------------------------------------------------------------*/
void
leds_arch_set(unsigned char leds)
{
  c = leds;

  PIN_setPortOutputValue(pin_handle, 0);

  if (leds & LEDS_RED) {
    PIN_setOutputValue(pin_handle, Board_PIN_LED0, 1);
  }
}
/*---------------------------------------------------------------------------*/
/** @} */
