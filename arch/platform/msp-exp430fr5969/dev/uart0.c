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
 *         eUSCI_A0 UART driver for MSP430FR5969
 *
 *         Uses eUSCI_A0 module with P2.0 (TXD) and P2.1 (RXD) for
 *         backchannel UART communication.
 */

#include "contiki.h"
#include "dev/uart0.h"
#include "sys/energest.h"
#include "isr_compat.h"
#include <msp430.h>

static int (*uart0_input_handler)(unsigned char c);

/*---------------------------------------------------------------------------*/
int
uart0_active(void)
{
  return (UCA0STATW & UCBUSY) != 0;
}
/*---------------------------------------------------------------------------*/
void
uart0_set_input(int (*input)(unsigned char c))
{
  uart0_input_handler = input;
}
/*---------------------------------------------------------------------------*/
void
uart0_writeb(unsigned char c)
{
  /* Wait for previous transmission to complete */
  while(!(UCA0IFG & UCTXIFG));

  UCA0TXBUF = c;
}
/*---------------------------------------------------------------------------*/
void
uart0_init(unsigned long ubr)
{
  /* The baud rate is fixed at 115200 with an 8 MHz SMCLK; ubr is
   * accepted for API compatibility with other MSP430 UART drivers but
   * is not used to derive the divider values. */
  (void)ubr;

  /* Put eUSCI in reset */
  UCA0CTLW0 = UCSWRST;

  /* Configure eUSCI_A0: 8N1, SMCLK source */
  UCA0CTLW0 |= UCSSEL__SMCLK;

  /* Set baud rate at 115200 with oversampling (8 MHz SMCLK):
   * 8000000 / 115200 = 69.44 → UCBRx = 4, UCBRF = 5, UCBRS = 0x55. */
  UCA0BRW = 4;
  UCA0MCTLW = (5 << 4) | (0x55 << 8) | UCOS16;

  /* Configure pins: P2.0 = TXD (output), P2.1 = RXD (input).
   * msp430_cpu_init() previously drove every port pin as an output
   * low, so the RX direction must be reset explicitly before the
   * eUSCI peripheral function is selected. */
  P2DIR |= BIT0;
  P2DIR &= ~BIT1;
  P2SEL0 &= ~(BIT0 | BIT1);
  P2SEL1 |= BIT0 | BIT1;

  /* Release from reset */
  UCA0CTLW0 &= ~UCSWRST;

  /* Enable RX interrupt */
  UCA0IE |= UCRXIE;
}
/*---------------------------------------------------------------------------*/
ISR(USCI_A0, uart0_rx_interrupt)
{
  uint8_t c;

  ENERGEST_ON(ENERGEST_TYPE_IRQ);

  switch(UCA0IV) {
  case USCI_UART_UCRXIFG:
    c = UCA0RXBUF;
    if(uart0_input_handler != NULL) {
      if(uart0_input_handler(c)) {
        LPM4_EXIT;
      }
    }
    break;
  }

  ENERGEST_OFF(ENERGEST_TYPE_IRQ);
}
/*---------------------------------------------------------------------------*/
