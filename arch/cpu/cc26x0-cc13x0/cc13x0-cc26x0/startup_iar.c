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
/**
 * \addtogroup cc13xx-cc26xx-cpu
 * @{
 *
 * \file
 *        Startup file for IAR for CC13xx/CC26xx.
 */
/*---------------------------------------------------------------------------*/
/* Check if compiler is IAR. */
#if !(defined(__IAR_SYSTEMS_ICC__))
#error "startup_cc13xx_cc26xx_iar.c: Unsupported compiler!"
#endif
/*---------------------------------------------------------------------------*/
/* We need intrinsic functions for IAR (if used in source code). */
#include <intrinsics.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(driverlib/setup.h)
#include DeviceFamily_constructPath(driverlib/interrupt.h)
/*---------------------------------------------------------------------------*/
/* Forward declaration of the reset ISR and the default fault handlers. */
static void nmiISR(void);
static void faultISR(void);
static void intDefaultHandler(void);
extern int  main(void);

extern void MPUFaultIntHandler(void);
extern void BusFaultIntHandler(void);
extern void UsageFaultIntHandler(void);
extern void SVCallIntHandler(void);
extern void DebugMonIntHandler(void);
extern void PendSVIntHandler(void);
extern void SysTickIntHandler(void);
extern void GPIOIntHandler(void);
extern void I2CIntHandler(void);
extern void RFCCPE1IntHandler(void);
extern void AONRTCIntHandler(void);
extern void UART0IntHandler(void);
extern void AUXSWEvent0IntHandler(void);
extern void SSI0IntHandler(void);
extern void SSI1IntHandler(void);
extern void RFCCPE0IntHandler(void);
extern void RFCHardwareIntHandler(void);
extern void RFCCmdAckIntHandler(void);
extern void I2SIntHandler(void);
extern void AUXSWEvent1IntHandler(void);
extern void WatchdogIntHandler(void);
extern void Timer0AIntHandler(void);
extern void Timer0BIntHandler(void);
extern void Timer1AIntHandler(void);
extern void Timer1BIntHandler(void);
extern void Timer2AIntHandler(void);
extern void Timer2BIntHandler(void);
extern void Timer3AIntHandler(void);
extern void Timer3BIntHandler(void);
extern void CryptoIntHandler(void);
extern void uDMAIntHandler(void);
extern void uDMAErrIntHandler(void);
extern void FlashIntHandler(void);
extern void SWEvent0IntHandler(void);
extern void AUXCombEventIntHandler(void);
extern void AONProgIntHandler(void);
extern void DynProgIntHandler(void);
extern void AUXCompAIntHandler(void);
extern void AUXADCIntHandler(void);
extern void TRNGIntHandler(void);

/* Default interrupt handlers */
#pragma weak MPUFaultIntHandler     = intDefaultHandler
#pragma weak BusFaultIntHandler     = intDefaultHandler
#pragma weak UsageFaultIntHandler   = intDefaultHandler
#pragma weak SVCallIntHandler       = intDefaultHandler
#pragma weak DebugMonIntHandler     = intDefaultHandler
#pragma weak PendSVIntHandler       = intDefaultHandler
#pragma weak SysTickIntHandler      = intDefaultHandler
#pragma weak GPIOIntHandler         = intDefaultHandler
#pragma weak I2CIntHandler          = intDefaultHandler
#pragma weak RFCCPE1IntHandler      = intDefaultHandler
#pragma weak AONRTCIntHandler       = intDefaultHandler
#pragma weak UART0IntHandler        = intDefaultHandler
#pragma weak AUXSWEvent0IntHandler  = intDefaultHandler
#pragma weak SSI0IntHandler         = intDefaultHandler
#pragma weak SSI1IntHandler         = intDefaultHandler
#pragma weak RFCCPE0IntHandler      = intDefaultHandler
#pragma weak RFCHardwareIntHandler  = intDefaultHandler
#pragma weak RFCCmdAckIntHandler    = intDefaultHandler
#pragma weak I2SIntHandler          = intDefaultHandler
#pragma weak AUXSWEvent1IntHandler  = intDefaultHandler
#pragma weak WatchdogIntHandler     = intDefaultHandler
#pragma weak Timer0AIntHandler      = intDefaultHandler
#pragma weak Timer0BIntHandler      = intDefaultHandler
#pragma weak Timer1AIntHandler      = intDefaultHandler
#pragma weak Timer1BIntHandler      = intDefaultHandler
#pragma weak Timer2AIntHandler      = intDefaultHandler
#pragma weak Timer2BIntHandler      = intDefaultHandler
#pragma weak Timer3AIntHandler      = intDefaultHandler
#pragma weak Timer3BIntHandler      = intDefaultHandler
#pragma weak CryptoIntHandler       = intDefaultHandler
#pragma weak uDMAIntHandler         = intDefaultHandler
#pragma weak uDMAErrIntHandler      = intDefaultHandler
#pragma weak FlashIntHandler        = intDefaultHandler
#pragma weak SWEvent0IntHandler     = intDefaultHandler
#pragma weak AUXCombEventIntHandler = intDefaultHandler
#pragma weak AONProgIntHandler      = intDefaultHandler
#pragma weak DynProgIntHandler      = intDefaultHandler
#pragma weak AUXCompAIntHandler     = intDefaultHandler
#pragma weak AUXADCIntHandler       = intDefaultHandler
#pragma weak TRNGIntHandler         = intDefaultHandler
/*---------------------------------------------------------------------------*/
/* The entry point for the application startup code. */
extern void __iar_program_start(void);

/* Get stack start (highest address) symbol from linker file. */
extern const void *STACK_TOP;
/*---------------------------------------------------------------------------*/
/*
 * It is required to place something in the CSTACK segment to get the stack
 * check feature in IAR to work as expected
 */
__root static void *dummy_stack @ ".stack";

/*
 * The vector table. Note that the proper constructs must be placed on this to
 * ensure that it ends up at physical address 0x0000.0000 or at the start of
 * the program if located at a start address other than 0.
 */
__root void(*const __vector_table[])(void) @ ".intvec" =
{
  (void (*)(void)) & STACK_TOP,           /*  0 The initial stack pointer */
  __iar_program_start,                    /*  1 The reset handler */
  nmiISR,                                 /*  2 The NMI handler */
  faultISR,                               /*  3 The hard fault handler */
  MPUFaultIntHandler,                     /*  4 The MPU fault handler */
  BusFaultIntHandler,                     /*  5 The bus fault handler */
  UsageFaultIntHandler,                   /*  6 The usage fault handler */
  0,                                      /*  7 Reserved */
  0,                                      /*  8 Reserved */
  0,                                      /*  9 Reserved */
  0,                                      /* 10 Reserved */
  SVCallIntHandler,                       /* 11 SVCall handler */
  DebugMonIntHandler,                     /* 12 Debug monitor handler */
  0,                                      /* 13 Reserved */
  PendSVIntHandler,                       /* 14 The PendSV handler */
  SysTickIntHandler,                      /* 15 The SysTick handler */
  /* --- External interrupts --- */
  GPIOIntHandler,                         /* 16 AON edge detect */
  I2CIntHandler,                          /* 17 I2C */
  RFCCPE1IntHandler,                      /* 18 RF Core Command & Packet Engine 1 */
  intDefaultHandler,                      /* 19 Reserved */
  AONRTCIntHandler,                       /* 20 AON RTC */
  UART0IntHandler,                        /* 21 UART0 Rx and Tx */
  AUXSWEvent0IntHandler,                  /* 22 AUX software event 0 */
  SSI0IntHandler,                         /* 23 SSI0 Rx and Tx */
  SSI1IntHandler,                         /* 24 SSI1 Rx and Tx */
  RFCCPE0IntHandler,                      /* 25 RF Core Command & Packet Engine 0 */
  RFCHardwareIntHandler,                  /* 26 RF Core Hardware */
  RFCCmdAckIntHandler,                    /* 27 RF Core Command Acknowledge */
  I2SIntHandler,                          /* 28 I2S */
  AUXSWEvent1IntHandler,                  /* 29 AUX software event 1 */
  WatchdogIntHandler,                     /* 30 Watchdog timer */
  Timer0AIntHandler,                      /* 31 Timer 0 subtimer A */
  Timer0BIntHandler,                      /* 32 Timer 0 subtimer B */
  Timer1AIntHandler,                      /* 33 Timer 1 subtimer A */
  Timer1BIntHandler,                      /* 34 Timer 1 subtimer B */
  Timer2AIntHandler,                      /* 35 Timer 2 subtimer A */
  Timer2BIntHandler,                      /* 36 Timer 2 subtimer B */
  Timer3AIntHandler,                      /* 37 Timer 3 subtimer A */
  Timer3BIntHandler,                      /* 38 Timer 3 subtimer B */
  CryptoIntHandler,                       /* 39 Crypto Core Result available */
  uDMAIntHandler,                         /* 40 uDMA Software */
  uDMAErrIntHandler,                      /* 41 uDMA Error */
  FlashIntHandler,                        /* 42 Flash controller */
  SWEvent0IntHandler,                     /* 43 Software Event 0 */
  AUXCombEventIntHandler,                 /* 44 AUX combined event */
  AONProgIntHandler,                      /* 45 AON programmable 0 */
  DynProgIntHandler,                      /* 46 Dynamic Programmable interrupt */
                                          /*    source (Default: PRCM) */
  AUXCompAIntHandler,                     /* 47 AUX Comparator A */
  AUXADCIntHandler,                       /* 48 AUX ADC new sample or ADC DMA */
                                          /*    done, ADC underflow, ADC overflow */
  TRNGIntHandler                          /* 49 TRNG event */
}
/*---------------------------------------------------------------------------*/
/*
 * \brief   Setup trim device.
 * \return  Return value determines whether to omit seg_init or not.
 *       0 => omit seg_init
 *       1 => run seg_init
 *
 * This function is called by __iar_program_start() early in the boot sequence.
 * Copy the first 16 vectors from the read-only/reset table to the runtime
 * RAM table. Fill the remaining vectors with a stub. This vector table will
 * be updated at runtime.
 */
int
__low_level_init(void)
{
  IntMasterDisable();

  /* Final trim of device */
  SetupTrimDevice();

  /* Run seg_init */
  return 1;
}
/*---------------------------------------------------------------------------*/
/*
 * \brief  Non-Maskable Interrupt (NMI) ISR.
 *
 * This is the code that gets called when the processor receives a NMI. This
 * simply enters an infinite loop, preserving the system state for examination
 * by a debugger.
 */
static void
nmiISR(void)
{
  /* Enter an infinite loop. */
  for(;;) { /* hang */ }
}
/*---------------------------------------------------------------------------*/
/*
 * \brief     Debug stack pointer.
 * \param sp  Stack pointer.
 *
 * Provide a view into the CPU state from the provided stack pointer.
 */
void
debugStackPointer(uint32_t *sp)
{
  volatile uint32_t r0;  /**< R0 register */
  volatile uint32_t r1;  /**< R1 register */
  volatile uint32_t r2;  /**< R2 register */
  volatile uint32_t r3;  /**< R3 register */
  volatile uint32_t r12; /**< R12 register */
  volatile uint32_t lr;  /**< LR register */
  volatile uint32_t pc;  /**< PC register */
  volatile uint32_t psr; /**< PSR register */

  /* Cast to void to disable warnings of unused variables */
  (void)(r0  = sp[0]);
  (void)(r1  = sp[1]);
  (void)(r2  = sp[2]);
  (void)(r3  = sp[3]);
  (void)(r12 = sp[4]);
  (void)(lr  = sp[5]);
  (void)(pc  = sp[6]);
  (void)(psr = sp[7]);

  /* Enter an infinite loop. */
  for(;;) { /* hang */ }
}
/*---------------------------------------------------------------------------*/
/*
 * \brief  CPU Fault ISR.
 *
 * This is the code that gets called when the processor receives a fault
 * interrupt. Setup a call to debugStackPointer with the current stack pointer.
 * The stack pointer in this case would be the CPU state which caused the CPU
 * fault.
 */
static void
faultISR(void)
{
  __asm__ __volatile__
  (
    "tst lr, #4           \n"
    "ite eq               \n"
    "mrseq r0, msp        \n"
    "mrsne r0, psp        \n"
    "b debugStackPointer  \n"
  );
}
/*---------------------------------------------------------------------------*/
/*
 * \brief  Interrupt Default Handler.
 *
 * This is the code that gets called when the processor receives an unexpected
 * interrupt. This simply enters an infinite loop, preserving the system state
 * for examination by a debugger.
 */
static void
intDefaultHandler(void)
{
  /* Enter an infinite loop. */
  for(;;) { /* hang */ }
}
/*---------------------------------------------------------------------------*/
/** @} */
