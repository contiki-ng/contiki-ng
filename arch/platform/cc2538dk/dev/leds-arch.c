/*
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
#include "contiki.h"
#include "dev/leds.h"
#include "dev/gpio-hal.h"
#include "dev/gpio-hal-arch.h"

#include <stdbool.h>
/*---------------------------------------------------------------------------*/
const leds_t leds_arch_leds[] = {
  {
    .pin = GPIO_PORT_PIN_TO_GPIO_HAL_PIN(LEDS_ARCH_L1_PORT, LEDS_ARCH_L1_PIN),
    .negative_logic = false
  },
  {
    .pin = GPIO_PORT_PIN_TO_GPIO_HAL_PIN(LEDS_ARCH_L2_PORT, LEDS_ARCH_L2_PIN),
    .negative_logic = false
  },
  {
    .pin = GPIO_PORT_PIN_TO_GPIO_HAL_PIN(LEDS_ARCH_L3_PORT, LEDS_ARCH_L3_PIN),
    .negative_logic = false
  },
#if !USB_SERIAL_CONF_ENABLE
  {
    .pin = GPIO_PORT_PIN_TO_GPIO_HAL_PIN(LEDS_ARCH_L4_PORT, LEDS_ARCH_L4_PIN),
    .negative_logic = false
  },
#endif
};
/*---------------------------------------------------------------------------*/
