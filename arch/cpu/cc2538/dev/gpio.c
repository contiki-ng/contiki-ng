/*
 * Copyright (c) 2012, Texas Instruments Incorporated - http://www.ti.com/
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
 *
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
/**
 * \addtogroup cc2538-gpio
 * @{
 *
 * \file
 * Implementation of the cc2538 GPIO controller
 */
#include "contiki.h"
#include "dev/leds.h"
#include "dev/gpio-hal.h"
#include "dev/gpio.h"
#include "dev/nvic.h"
#include "reg.h"
#include "lpm.h"

#include <string.h>
/*---------------------------------------------------------------------------*/
/** \brief Interrupt service routine for Port \a port
 * \param port Number between 0 and 3. Port A: 0, Port B: 1, etc.
 */
static void
gpio_port_isr(uint8_t port)
{
  uint32_t base;
  uint8_t int_status, power_up_int_status;

  lpm_exit();

  base = GPIO_PORT_TO_BASE(port);
  int_status = GPIO_GET_MASKED_INT_STATUS(base);
  power_up_int_status = GPIO_GET_POWER_UP_INT_STATUS(port);

  gpio_hal_event_handler((int_status | power_up_int_status) << (port << 3));

  GPIO_CLEAR_INTERRUPT(base, int_status);
  GPIO_CLEAR_POWER_UP_INTERRUPT(port, power_up_int_status);
}
/*---------------------------------------------------------------------------*/
#define GPIO_PORT_ISR(lowercase_port, uppercase_port) \
void \
gpio_port_##lowercase_port##_isr(void) \
{ \
  gpio_port_isr(GPIO_##uppercase_port##_NUM); \
}
GPIO_PORT_ISR(a, A)
GPIO_PORT_ISR(b, B)
GPIO_PORT_ISR(c, C)
GPIO_PORT_ISR(d, D)
/*---------------------------------------------------------------------------*/
/** @} */
