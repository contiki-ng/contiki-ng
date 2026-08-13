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
 *         MSP430FR5969 CPU initialization
 *
 *         Configures the Clock System (CS) for 8MHz DCO operation.
 */

#include "contiki.h"
#include <msp430.h>

#if defined(__MSP430__) && defined(__GNUC__)
#define asmv(arg) __asm__ __volatile__(arg)
#endif

/* Flag for timers/serial to indicate DCO must be kept on in LPM */
int msp430_dco_required;

/*---------------------------------------------------------------------------*/
/**
 * Initialize the Clock System (CS) module.
 *
 * Configure:
 * - DCO at 8 MHz
 * - MCLK = SMCLK = DCO
 * - ACLK = LFXT (32.768 kHz on-board crystal), with VLOCLK fallback
 *   if the crystal fails to stabilize.
 *
 * Also configures FRAM wait states for 8MHz operation.
 */
static void
msp430_init_dco(void)
{
  /* Route PJ.4/PJ.5 to the LFXT crystal before unlocking the LPM5
   * port latch so the pins come up in peripheral function. */
  PJSEL0 |= BIT4 | BIT5;
  PJSEL1 &= ~(BIT4 | BIT5);

  /* Disable the GPIO power-on default high-impedance mode to activate
   * previously configured port settings */
  PM5CTL0 &= ~LOCKLPM5;

  /* Configure FRAM wait state (required for >8MHz operation)
   * FWPW = FRAM write password
   * NACCESS_0 = 0 wait states for up to 8MHz operation
   */
  FRCTL0 = FWPW | NACCESS_0;

  /* Unlock CS registers (CSKEY >> 8 to get high byte) */
  CSCTL0_H = (CSKEY >> 8);

  /* Configure DCO to 8MHz */
  CSCTL1 = DCOFSEL_6;  /* DCO = 8 MHz */

  /* Start LFXT with default drive strength. Source ACLK from VLO
   * initially so the system can run even before the crystal
   * stabilizes; we switch to LFXT below once OFIFG stays cleared. */
  CSCTL4 = (CSCTL4 & ~(LFXTOFF | LFXTBYPASS)) | LFXTDRIVE_3;
  CSCTL2 = SELA__VLOCLK | SELS__DCOCLK | SELM__DCOCLK;

  /* Set clock dividers (all divide by 1) */
  CSCTL3 = DIVA__1 | DIVS__1 | DIVM__1;

  /* Wait up to ~500 ms for the LFXT to stabilize. Each iteration
   * clears LFXTOFFG and the system oscillator fault flag, then
   * delays ~10 ms at 8 MHz; 50 iterations gives a 500 ms cap. If
   * the crystal never stabilizes (e.g. unpopulated or broken), we
   * leave ACLK on VLOCLK so etimer/rtimer still tick. */
  for(int retries = 0; retries < 50; retries++) {
    CSCTL5 &= ~LFXTOFFG;
    SFRIFG1 &= ~OFIFG;
    __delay_cycles(80000);
    if((CSCTL5 & LFXTOFFG) == 0 && (SFRIFG1 & OFIFG) == 0) {
      break;
    }
  }
  if((SFRIFG1 & OFIFG) == 0) {
    CSCTL2 = SELA__LFXTCLK | SELS__DCOCLK | SELM__DCOCLK;
  }

  /* Lock CS registers */
  CSCTL0_H = 0;

  msp430_dco_required = 0;
}
/*---------------------------------------------------------------------------*/
/**
 * Initialize all ports to output low to reduce power consumption.
 *
 * PJ is intentionally left at its reset default (all inputs):
 *   - PJ.0..PJ.3 carry the JTAG signals used by the eZ-FET probe.
 *   - PJ.4/PJ.5 are the LFXT crystal pins; driving them as outputs
 *     would jam the 32.768 kHz oscillator that sources ACLK.
 */
static void
init_ports(void)
{
  /* Configure all GPIO to output low */
  P1OUT = 0;
  P1DIR = 0xFF;
  P2OUT = 0;
  P2DIR = 0xFF;
  P3OUT = 0;
  P3DIR = 0xFF;
  P4OUT = 0;
  P4DIR = 0xFF;
}
/*---------------------------------------------------------------------------*/
void
msp430_cpu_init(void)
{
  /* Stop watchdog timer */
  WDTCTL = WDTPW | WDTHOLD;

  /* Initialize ports first (reduces power) */
  init_ports();

  /* Initialize DCO and clock system */
  msp430_init_dco();

  /* Enable global interrupts */
  eint();
}
/*---------------------------------------------------------------------------*/
/**
 * Synchronize DCO (no-op for FR5969, DCO is automatically calibrated).
 */
void
msp430_sync_dco(void)
{
  /* FR5969 DCO is factory calibrated, no sync needed */
}
/*---------------------------------------------------------------------------*/
/**
 * Mask all interrupts that can be masked.
 */
int
splhigh_(void)
{
  int sr;
  /* Clear the GIE (General Interrupt Enable) flag. */
#ifdef __IAR_SYSTEMS_ICC__
  sr = __get_SR_register();
  __bic_SR_register(GIE);
#else
  asmv("mov r2, %0" : "=r" (sr));
  asmv("bic %0, r2" : : "i" (GIE));
  /* GCC 9 warns about risk of incorrect execution without nop after
     interrupt state changes. */
  asmv("nop");
#endif
  return sr & GIE;  /* Ignore other sr bits. */
}
/*---------------------------------------------------------------------------*/
#ifdef __IAR_SYSTEMS_ICC__
int __low_level_init(void)
{
  /* turn off watchdog so that C-init will run */
  WDTCTL = WDTPW + WDTHOLD;
  return 1;
}
#endif
/*---------------------------------------------------------------------------*/
