/*
 * Copyright (c) 2016, Texas Instruments Incorporated - http://www.ti.com/
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
#include "inc/hw_types.h"
#include "driverlib/setup.h"
/*---------------------------------------------------------------------------*/
#define WEAK_ALIAS(x) __attribute__ ((weak, alias(#x)))
/*---------------------------------------------------------------------------*/
void ResetISR(void);
static void default_handler(void);
extern int main(void);
/*---------------------------------------------------------------------------*/
void NmiSR(void) WEAK_ALIAS(default_handler);
void FaultISR(void) WEAK_ALIAS(default_handler);
void MPUFaultIntHandler(void) WEAK_ALIAS(default_handler);
void BusFaultIntHandler(void) WEAK_ALIAS(default_handler);
void UsageFaultIntHandler(void) WEAK_ALIAS(default_handler);
void GPIOIntHandler(void) WEAK_ALIAS(default_handler);
void I2CIntHandler(void) WEAK_ALIAS(default_handler);
void AONRTCIntHandler(void) WEAK_ALIAS(default_handler);
void SSI0IntHandler(void) WEAK_ALIAS(default_handler);
void FlashIntHandler(void) WEAK_ALIAS(default_handler);
/*---------------------------------------------------------------------------*/
/*
 * The following are constructs created by the linker, indicating where the
 * "data" and "bss" segments reside in memory.
 */
extern uint32_t _etext;
extern uint32_t _data;
extern uint32_t _edata;
extern uint32_t _bss;
extern uint32_t _ebss;
extern uint32_t _estack;
/*---------------------------------------------------------------------------*/
__attribute__ ((section(".vectors"), used))
void(*const vectors[]) (void) =
{
  (void (*)(void)) & _estack,           /* The initial stack pointer */
  ResetISR,                             /* The reset handler */
  NmiSR,                                /* The NMI handler */
  FaultISR,                             /* The hard fault handler */
  MPUFaultIntHandler,                   /* The MPU fault handler */
  BusFaultIntHandler,                   /* The bus fault handler */
  UsageFaultIntHandler,                 /* The usage fault handler */
  0,                                    /* Reserved */
  0,                                    /* Reserved */
  0,                                    /* Reserved */
  0,                                    /* Reserved */
  default_handler,                      /* SVCall handler */
  default_handler,                      /* Debug monitor handler */
  0,                                    /* Reserved */
  default_handler,                      /* The PendSV handler */
  default_handler,                      /* The SysTick handler */
  GPIOIntHandler,                       /* AON edge detect */
  I2CIntHandler,                        /* I2C */
  default_handler,                      /* RF Core Command & Packet Engine 1 */
  default_handler,                      /* AON SpiSplave Rx, Tx and CS */
  AONRTCIntHandler,                     /* AON RTC */
  default_handler,                      /* UART0 Rx and Tx */
  default_handler,                      /* AUX software event 0 */
  SSI0IntHandler,                       /* SSI0 Rx and Tx */
  default_handler,                      /* SSI1 Rx and Tx */
  default_handler,                      /* RF Core Command & Packet Engine 0 */
  default_handler,                      /* RF Core Hardware */
  default_handler,                      /* RF Core Command Acknowledge */
  default_handler,                      /* I2S */
  default_handler,                      /* AUX software event 1 */
  default_handler,                      /* Watchdog timer */
  default_handler,                      /* Timer 0 subtimer A */
  default_handler,                      /* Timer 0 subtimer B */
  default_handler,                      /* Timer 1 subtimer A */
  default_handler,                      /* Timer 1 subtimer B */
  default_handler,                      /* Timer 2 subtimer A */
  default_handler,                      /* Timer 2 subtimer B */
  default_handler,                      /* Timer 3 subtimer A */
  default_handler,                      /* Timer 3 subtimer B */
  default_handler,                      /* Crypto Core Result available */
  default_handler,                      /* uDMA Software */
  default_handler,                      /* uDMA Error */
  FlashIntHandler,                      /* Flash controller */
  default_handler,                      /* Software Event 0 */
  default_handler,                      /* AUX combined event */
  default_handler,                      /* AON programmable 0 */
  default_handler,                      /* Dynamic Programmable interrupt
                                         * source (Default: PRCM) */
  default_handler,                      /* AUX Comparator A */
  default_handler,                      /* AUX ADC new sample or ADC DMA
                                         * done, ADC underflow, ADC overflow */
  default_handler                       /* TRNG event */
};
/*---------------------------------------------------------------------------*/
void
ResetISR(void)
{
  uint32_t *pui32Src, *pui32Dest;

  /* Final trim of device */
  SetupTrimDevice();

  /* Copy the data segment initializers from flash to SRAM. */
  pui32Src = &_etext;
  for(pui32Dest = &_data; pui32Dest < &_edata;) {
    *pui32Dest++ = *pui32Src++;
  }

  /* Zero fill the bss segment. */
  __asm("    ldr     r0, =_bss\n"
        "    ldr     r1, =_ebss\n"
        "    mov     r2, #0\n"
        "    .thumb_func\n"
        "zero_loop:\n"
        "        cmp     r0, r1\n"
        "        it      lt\n"
        "        strlt   r2, [r0], #4\n"
        "        blt     zero_loop");

  /* Call the application's entry point. */
  main();

  /* If we ever return signal Error */
  FaultISR();
}
/*---------------------------------------------------------------------------*/
static void
default_handler(void)
{
  while(1);
}
/*---------------------------------------------------------------------------*/
