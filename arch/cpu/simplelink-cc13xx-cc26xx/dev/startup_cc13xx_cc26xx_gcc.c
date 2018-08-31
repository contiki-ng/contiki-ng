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
 *        Startup file for GCC for CC13xx/CC26xx.
 */
/*---------------------------------------------------------------------------*/
/* Check if compiler is GNU Compiler. */
#if !(defined(__GNUC__))
#error "startup_cc13xx_cc26xx_gcc.c: Unsupported compiler!"
#endif
/*---------------------------------------------------------------------------*/
#include <string.h>

#include <ti/devices/DeviceFamily.h>
#include DeviceFamily_constructPath(inc/hw_types.h)
#include DeviceFamily_constructPath(driverlib/interrupt.h)
#include DeviceFamily_constructPath(driverlib/setup.h)
/*---------------------------------------------------------------------------*/
/* Forward declaration of the default fault handlers. */
void resetISR(void);
static void nmiISR(void);
static void faultISR(void);
static void defaultHandler(void);
static void busFaultHandler(void);
/*---------------------------------------------------------------------------*/
/*
 * External declaration for the reset handler that is to be called when the
 * processor is started.
 */
extern void _c_int00(void);

/* The entry point for the application. */
extern int main(void);
/*---------------------------------------------------------------------------*/
/* Linker variable that marks the top of stack. */
extern unsigned long _stack_end;

/*
 * The vector table. Note that the proper constructs must be placed on this to
 * ensure that it ends up at physical address 0x0000.0000.
 */
__attribute__((section(".resetVecs"))) __attribute__((used))
static void(*const resetVectors[16])(void) =
{
  (void(*)(void))((uint32_t)&_stack_end),
  /* The initial stack pointer */
  resetISR,                            /* The reset handler */
  nmiISR,                              /* The NMI handler */
  faultISR,                            /* The hard fault handler */
  defaultHandler,                      /* The MPU fault handler */
  busFaultHandler,                     /* The bus fault handler */
  defaultHandler,                      /* The usage fault handler */
  0,                                   /* Reserved */
  0,                                   /* Reserved */
  0,                                   /* Reserved */
  0,                                   /* Reserved */
  defaultHandler,                      /* SVCall handler */
  defaultHandler,                      /* Debug monitor handler */
  0,                                   /* Reserved */
  defaultHandler,                      /* The PendSV handler */
  defaultHandler                       /* The SysTick handler */
};
/*---------------------------------------------------------------------------*/
/*
 * The following are arrays of pointers to constructor functions that need to
 * be called during startup to initialize global objects.
 */
extern void (*__init_array_start[])(void);
extern void (*__init_array_end[])(void);

/* The following global variable is required for C++ support. */
void *__dso_handle = (void *)&__dso_handle;
/*---------------------------------------------------------------------------*/
/*
 * The following are constructs created by the linker, indicating where the
 * the "data" and "bss" segments reside in memory. The initializers for the
 * for the "data" segment resides immediately following the "text" segment.
 */
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __data_load__;
extern uint32_t __data_start__;
extern uint32_t __data_end__;
/*---------------------------------------------------------------------------*/
/*
 * \brief  Entry point of the startup code.
 *
 * Initialize the .data and .bss sections, and call main.
 */
void
localProgramStart(void)
{
  uint32_t *bs;
  uint32_t *be;
  uint32_t *dl;
  uint32_t *ds;
  uint32_t *de;
  uint32_t count;
  uint32_t i;

  IntMasterDisable();

  /* Final trim of device */
  SetupTrimDevice();

  /* initiailize .bss to zero */
  bs = &__bss_start__;
  be = &__bss_end__;
  while(bs < be) {
    *bs = 0;
    bs++;
  }

  /* relocate the .data section */
  dl = &__data_load__;
  ds = &__data_start__;
  de = &__data_end__;
  if(dl != ds) {
    while(ds < de) {
      *ds = *dl;
      dl++;
      ds++;
    }
  }

  /* Run any constructors */
  count = (uint32_t)(__init_array_end - __init_array_start);
  for(i = 0; i < count; i++) {
    __init_array_start[i]();
  }

  /* Call the application's entry point. */
  main();

  /* If we ever return signal Error */
  faultISR();
}
/*---------------------------------------------------------------------------*/
/*
 * \brief  Reset ISR.
 *
 * This is the code that gets called when the processor first starts execution
 * following a reset event. Only the absolutely necessary set is performed,
 * after which the application supplied entry() routine is called.  Any fancy
 * actions (such as making decisions based on the reset cause register, and
 * resetting the bits in that register) are left solely in the hands of the
 * application.
 */
void __attribute__((naked))
resetISR(void)
{
  __asm__ __volatile__
  (
    "movw r0, #:lower16:resetVectors  \n"
    "movt r0, #:upper16:resetVectors  \n"
    "ldr r0, [r0]                     \n"
    "mov sp, r0                       \n"
    "bx %0                            \n"
    : /* output */
    : /* input */
    "r"(localProgramStart)
  );
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
static void
debugHardfault(uint32_t *sp)
{
  volatile uint32_t r0;  /**< R0 register */
  volatile uint32_t r1;  /**< R1 register */
  volatile uint32_t r2;  /**< R2 register */
  volatile uint32_t r3;  /**< R3 register */
  volatile uint32_t r12; /**< R12 register */
  volatile uint32_t lr;  /**< LR register */
  volatile uint32_t pc;  /**< PC register */
  volatile uint32_t psr; /**< PSR register */

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
    "tst lr, #4        \n"
    "ite eq            \n"
    "mrseq r0, msp     \n"
    "mrsne r0, psp     \n"
    "bx %0             \n"
    : /* output */
    : /* input */
    "r"(debugHardfault)
  );
}
/*---------------------------------------------------------------------------*/
/* Dummy variable */
volatile int x__;

/*
 * \brief  Bus Fault Handler.
 *
 * This is the code that gets called when the processor receives an unexpected
 * interrupt. This simply enters an infinite loop, preserving the system state
 * for examination by a debugger.
 */
static void
busFaultHandler(void)
{
  x__ = 0;

  /* Enter an infinite loop. */
  for(;;) { /* hang */ }
}
/*---------------------------------------------------------------------------*/
/*
 * \brief  Default Handler.
 *
 * This is the code that gets called when the processor receives an unexpected
 * interrupt.  This simply enters an infinite loop, preserving the system state
 * for examination by a debugger.
 */
static void
defaultHandler(void)
{
  /* Enter an infinite loop. */
  for(;;) { /* hang */ }
}
/*---------------------------------------------------------------------------*/
/*
 * \brief  Finalize object function.
 *
 * This function is called by __libc_fini_array which gets called when exit()
 * is called. In order to support exit(), an empty _fini() stub function is
 * required.
 */
void
_fini(void)
{
  /* Function body left empty intentionally */
}
/*---------------------------------------------------------------------------*/
/** @} */
