/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
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
 * \file
 *         GPIO HAL definitions for MSP-EXP430FR5969 LaunchPad
 *
 *         Ports are numbered 1..4 to mirror the MSP430 hardware naming
 *         (P1, P2, P3, P4). PJ is not exposed as it is reserved for JTAG
 *         and the LFXT crystal.
 */

#ifndef GPIO_HAL_ARCH_H_
#define GPIO_HAL_ARCH_H_

#include "contiki.h"
#include <msp430.h>

/*---------------------------------------------------------------------------*/
#define GPIO_HAL_ARCH_PORT_1 1
#define GPIO_HAL_ARCH_PORT_2 2
#define GPIO_HAL_ARCH_PORT_3 3
#define GPIO_HAL_ARCH_PORT_4 4

/*---------------------------------------------------------------------------*/
/* Inline implementations for the operations called from timing-critical
 * paths (e.g. the DHT11 bit-banging driver). The switch lets the compiler
 * fold to a single bis/bic/xor when the port is a compile-time constant. */
/*---------------------------------------------------------------------------*/
#define gpio_hal_arch_set_pin(port, pin)        \
  gpio_hal_arch_msp430_set_pin(port, pin)
#define gpio_hal_arch_clear_pin(port, pin)      \
  gpio_hal_arch_msp430_clear_pin(port, pin)
#define gpio_hal_arch_toggle_pin(port, pin)     \
  gpio_hal_arch_msp430_toggle_pin(port, pin)
#define gpio_hal_arch_read_pin(port, pin)       \
  gpio_hal_arch_msp430_read_pin(port, pin)
#define gpio_hal_arch_write_pin(port, pin, v)   \
  gpio_hal_arch_msp430_write_pin(port, pin, v)

#define gpio_hal_arch_set_pins(port, pins)      \
  gpio_hal_arch_msp430_set_pins(port, pins)
#define gpio_hal_arch_clear_pins(port, pins)    \
  gpio_hal_arch_msp430_clear_pins(port, pins)
#define gpio_hal_arch_toggle_pins(port, pins)   \
  gpio_hal_arch_msp430_toggle_pins(port, pins)
#define gpio_hal_arch_read_pins(port, pins)     \
  gpio_hal_arch_msp430_read_pins(port, pins)
#define gpio_hal_arch_write_pins(port, pins, v) \
  gpio_hal_arch_msp430_write_pins(port, pins, v)

#define gpio_hal_arch_pin_set_input(port, pin)  \
  gpio_hal_arch_msp430_pin_set_input(port, pin)
#define gpio_hal_arch_pin_set_output(port, pin) \
  gpio_hal_arch_msp430_pin_set_output(port, pin)
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_set_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  uint8_t m = (uint8_t)pins;
  switch(port) {
  case 1: P1OUT |= m; break;
  case 2: P2OUT |= m; break;
  case 3: P3OUT |= m; break;
  case 4: P4OUT |= m; break;
  }
}
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_clear_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  uint8_t m = (uint8_t)pins;
  switch(port) {
  case 1: P1OUT &= ~m; break;
  case 2: P2OUT &= ~m; break;
  case 3: P3OUT &= ~m; break;
  case 4: P4OUT &= ~m; break;
  }
}
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_toggle_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  uint8_t m = (uint8_t)pins;
  switch(port) {
  case 1: P1OUT ^= m; break;
  case 2: P2OUT ^= m; break;
  case 3: P3OUT ^= m; break;
  case 4: P4OUT ^= m; break;
  }
}
/*---------------------------------------------------------------------------*/
static inline gpio_hal_pin_mask_t
gpio_hal_arch_msp430_read_pins(gpio_hal_port_t port, gpio_hal_pin_mask_t pins)
{
  uint8_t m = (uint8_t)pins;
  uint8_t in = 0;
  switch(port) {
  case 1: in = P1IN; break;
  case 2: in = P2IN; break;
  case 3: in = P3IN; break;
  case 4: in = P4IN; break;
  }
  return (gpio_hal_pin_mask_t)(in & m);
}
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_write_pins(gpio_hal_port_t port,
                                gpio_hal_pin_mask_t pins,
                                gpio_hal_pin_mask_t value)
{
  uint8_t m = (uint8_t)pins;
  uint8_t v = (uint8_t)value;
  switch(port) {
  case 1: P1OUT = (P1OUT & ~m) | (v & m); break;
  case 2: P2OUT = (P2OUT & ~m) | (v & m); break;
  case 3: P3OUT = (P3OUT & ~m) | (v & m); break;
  case 4: P4OUT = (P4OUT & ~m) | (v & m); break;
  }
}
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_set_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  gpio_hal_arch_msp430_set_pins(port, (gpio_hal_pin_mask_t)1 << pin);
}
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_clear_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  gpio_hal_arch_msp430_clear_pins(port, (gpio_hal_pin_mask_t)1 << pin);
}
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_toggle_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  gpio_hal_arch_msp430_toggle_pins(port, (gpio_hal_pin_mask_t)1 << pin);
}
/*---------------------------------------------------------------------------*/
static inline uint8_t
gpio_hal_arch_msp430_read_pin(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  return gpio_hal_arch_msp430_read_pins(port, (gpio_hal_pin_mask_t)1 << pin)
         ? 1 : 0;
}
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_write_pin(gpio_hal_port_t port,
                               gpio_hal_pin_t pin,
                               uint8_t value)
{
  gpio_hal_pin_mask_t mask = (gpio_hal_pin_mask_t)1 << pin;
  gpio_hal_arch_msp430_write_pins(port, mask, value ? mask : 0);
}
/*---------------------------------------------------------------------------*/
/* SEL0/SEL1 = 0/0 selects the GPIO function on every FR5969 pin. The
 * helpers below also clear those bits so a pin that was previously routed
 * to a peripheral (e.g. UART) becomes a plain GPIO. */
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_pin_set_input(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  uint8_t m = (uint8_t)1 << pin;
  uint8_t nm = (uint8_t)~m;
  switch(port) {
  case 1: P1SEL0 &= nm; P1SEL1 &= nm; P1DIR &= nm; break;
  case 2: P2SEL0 &= nm; P2SEL1 &= nm; P2DIR &= nm; break;
  case 3: P3SEL0 &= nm; P3SEL1 &= nm; P3DIR &= nm; break;
  case 4: P4SEL0 &= nm; P4SEL1 &= nm; P4DIR &= nm; break;
  }
}
/*---------------------------------------------------------------------------*/
static inline void
gpio_hal_arch_msp430_pin_set_output(gpio_hal_port_t port, gpio_hal_pin_t pin)
{
  uint8_t m = (uint8_t)1 << pin;
  uint8_t nm = (uint8_t)~m;
  switch(port) {
  case 1: P1SEL0 &= nm; P1SEL1 &= nm; P1DIR |= m; break;
  case 2: P2SEL0 &= nm; P2SEL1 &= nm; P2DIR |= m; break;
  case 3: P3SEL0 &= nm; P3SEL1 &= nm; P3DIR |= m; break;
  case 4: P4SEL0 &= nm; P4SEL1 &= nm; P4DIR |= m; break;
  }
}
/*---------------------------------------------------------------------------*/
#endif /* GPIO_HAL_ARCH_H_ */
