/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
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
#include "dev/gpio-hal.h"
#include "sys/etimer.h"
#include "lib/sensors.h"
#include "dev/button-hal.h"

#include <stdio.h>
#include <inttypes.h>
/*---------------------------------------------------------------------------*/
extern gpio_hal_pin_t out_pin1, out_pin2, out_pin3;
extern gpio_hal_pin_t btn_pin;

#if GPIO_HAL_PORT_PIN_NUMBERING
extern gpio_hal_port_t out_port1, out_port2_3;
extern gpio_hal_port_t btn_port;
#else
#define out_port1   GPIO_HAL_NULL_PORT
#define out_port2_3 GPIO_HAL_NULL_PORT
#define btn_port    GPIO_HAL_NULL_PORT
#endif
/*---------------------------------------------------------------------------*/
static struct etimer et;
static uint8_t counter;
/*---------------------------------------------------------------------------*/
/* Print gpio_hal_pin_mask_t using the correct format */
#if GPIO_HAL_PIN_COUNT > 32
#define PIN_MASK_FMT "0x%016" PRIx64
#else
#define PIN_MASK_FMT "0x%08" PRIx32
#endif
/*---------------------------------------------------------------------------*/
PROCESS(gpio_hal_example, "GPIO HAL Example");
AUTOSTART_PROCESSES(&gpio_hal_example);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(gpio_hal_example, ev, data)
{
  PROCESS_BEGIN();

  counter = 0;

  etimer_set(&et, CLOCK_SECOND);

  while(1) {

    PROCESS_YIELD();

    if(ev == PROCESS_EVENT_TIMER && data == &et) {
      if((counter & 7) == 0) {
        /* Set output and test write, high */
        gpio_hal_arch_pin_set_output(out_port1, out_pin1);
        gpio_hal_arch_pin_set_output(out_port2_3, out_pin2);
        gpio_hal_arch_pin_set_output(out_port2_3, out_pin3);

        gpio_hal_arch_write_pin(out_port1, out_pin1, 1);
        gpio_hal_arch_write_pins(out_port2_3,
          gpio_hal_pin_to_mask(out_pin2) | gpio_hal_pin_to_mask(out_pin3),
          gpio_hal_pin_to_mask(out_pin2) | gpio_hal_pin_to_mask(out_pin3));
      } else if((counter & 7) == 1) {
        /* Test write, low */
        gpio_hal_arch_write_pin(out_port1, out_pin1, 0);
        gpio_hal_arch_write_pins(out_port2_3,
          gpio_hal_pin_to_mask(out_pin2) | gpio_hal_pin_to_mask(out_pin3), 0);
      } else if((counter & 7) == 2) {
        /* Test set */
        gpio_hal_arch_set_pin(out_port1, out_pin1);
        gpio_hal_arch_set_pins(out_port2_3,
          gpio_hal_pin_to_mask(out_pin2) | gpio_hal_pin_to_mask(out_pin3));
      } else if((counter & 7) == 3) {
        /* Test clear */
        gpio_hal_arch_clear_pin(out_port1, out_pin1);
        gpio_hal_arch_clear_pins(out_port2_3,
          gpio_hal_pin_to_mask(out_pin2) | gpio_hal_pin_to_mask(out_pin3));
      } else if((counter & 7) == 4) {
        /* Test toggle (should go high) */
        gpio_hal_arch_toggle_pin(out_port1, out_pin1);
        gpio_hal_arch_toggle_pins(out_port2_3,
          gpio_hal_pin_to_mask(out_pin2) | gpio_hal_pin_to_mask(out_pin3));
      } else if((counter & 7) == 5) {
        /* Test toggle (should go low) */
        gpio_hal_arch_toggle_pin(out_port1, out_pin1);
        gpio_hal_arch_toggle_pins(out_port2_3,
          gpio_hal_pin_to_mask(out_pin2) | gpio_hal_pin_to_mask(out_pin3));
      } else if((counter & 7) == 6) {
        /* Set to input and then set. Should stay off */
        gpio_hal_arch_pin_set_input(out_port1, out_pin1);
        gpio_hal_arch_pin_set_input(out_port2_3, out_pin2);
        gpio_hal_arch_pin_set_input(out_port2_3, out_pin3);
        gpio_hal_arch_set_pin(out_port1, out_pin1);
        gpio_hal_arch_set_pins(out_port2_3,
          gpio_hal_pin_to_mask(out_pin2) | gpio_hal_pin_to_mask(out_pin3));
      } else if((counter & 7) == 7) {
        /* Toggle button interrupt */
        gpio_hal_pin_cfg_t interrupt;

        interrupt = gpio_hal_arch_pin_cfg_get(btn_port, btn_pin) &
          GPIO_HAL_PIN_CFG_INT_ENABLE;

        if(interrupt == 0) {
          printf("Enabling button interrupt\n");
          gpio_hal_arch_interrupt_enable(btn_port, btn_pin);
        } else {
          printf("Disabling button interrupt\n");
          gpio_hal_arch_interrupt_disable(btn_port, btn_pin);
        }
      }

      /* Test read */
      printf("%u: Pins are 1=%u, 2=%u, 3=%u, mask=" PIN_MASK_FMT "\n",
             counter & 7,
             gpio_hal_arch_read_pin(out_port1, out_pin1),
             gpio_hal_arch_read_pin(out_port2_3, out_pin2),
             gpio_hal_arch_read_pin(out_port2_3, out_pin3),
             gpio_hal_arch_read_pins(out_port2_3,
                                     gpio_hal_pin_to_mask(out_pin2) |
                                     gpio_hal_pin_to_mask(out_pin3)));

      counter++;
      etimer_set(&et, CLOCK_SECOND);
    } else if(ev == button_hal_release_event) {
      printf("Button release event %s\n",
             BUTTON_HAL_GET_DESCRIPTION((button_hal_button_t *)data));
    }
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
