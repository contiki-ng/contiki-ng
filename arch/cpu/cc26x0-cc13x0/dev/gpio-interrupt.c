/*
 * Copyright (c) 2014, Texas Instruments Incorporated - http://www.ti.com/
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
 * \addtogroup cc26xx
 * @{
 *
 * \file
 *     CC13xx/CC26xx GPIO interrupt ISR.
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/gpio-hal.h"
#include "ti-lib.h"

#include <string.h>
/*---------------------------------------------------------------------------*/
#define gpio_interrupt_isr GPIOIntHandler
/*---------------------------------------------------------------------------*/
void
gpio_interrupt_isr(void)
{
  gpio_hal_pin_mask_t pin_mask;

  /* Read interrupt flags */
  pin_mask = (HWREG(GPIO_BASE + GPIO_O_EVFLAGS31_0) & GPIO_DIO_ALL_MASK);

  /* Clear the interrupt flags */
  HWREG(GPIO_BASE + GPIO_O_EVFLAGS31_0) = pin_mask;

  gpio_hal_event_handler(pin_mask);
}
/*---------------------------------------------------------------------------*/
/** @} */
