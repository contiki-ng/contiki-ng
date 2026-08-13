/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 * \addtogroup nrf
 * @{
 *
 * \addtogroup nrf-arm ARM Handler
 * @{
 *
 * \addtogroup nrf-hardfault Hardfault Handler
 * @{
 * 
 * \file
 *         Hardfault Handler implementation for the nRF.
 * \author
 *         Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include "cmsis_compiler.h"
#include "nrf.h"

/*---------------------------------------------------------------------------*/
#if NRF_HARDFAULT_HANDLER_EXTENDED
/*---------------------------------------------------------------------------*/
typedef struct HardFault_stack { /**< HardFault Stack */
  uint32_t r0;    /**< R0 register. */
  uint32_t r1;    /**< R1 register. */
  uint32_t r2;    /**< R2 register. */
  uint32_t r3;    /**< R3 register. */
  uint32_t r12;   /**< R12 register. */
  uint32_t lr;    /**< Link register. */
  uint32_t pc;    /**< Program counter. */
  uint32_t psr;   /**< Program status register. */
} HardFault_stack_t;
/*---------------------------------------------------------------------------*/
/*
 * Crash info saved to .noinit RAM — survives NVIC_SystemReset().
 * The startup code does not zero .noinit, so the magic value persists.
 * At boot, the radio driver checks this and prints the crash dump.
 */
#define CRASH_MAGIC 0xDEADF007UL
#define HF_CANARY_VALUE 0xFA17FA17UL

typedef struct {
  uint32_t magic;
  uint32_t pc;
  uint32_t lr;
  uint32_t psr;
  uint32_t cfsr;
  uint32_t hfsr;
  uint32_t r0;
  uint32_t r1;
  uint32_t r2;
  uint32_t r3;
  uint32_t r12;
  uint32_t mmfar;
  uint32_t bfar;
} crash_info_t;

__attribute__((section(".noinit"))) crash_info_t crash_info;
/*---------------------------------------------------------------------------*/
/**
 * @brief Check and print any saved crash info from a previous fault.
 *        Call this early in boot (after UART is initialized).
 */
void
hardfault_print_saved_crash(void)
{
  extern int dbg_putchar(int c);
  static const char hx[] = "0123456789abcdef";
  extern volatile uint32_t hf_canary;

  /* Stay quiet on normal boots: .noinit contains arbitrary stale RAM. */
  if(hf_canary == HF_CANARY_VALUE) {
    dbg_putchar('C');
    dbg_putchar('=');
    { uint32_t c = hf_canary;
      for(int s = 28; s >= 0; s -= 4) dbg_putchar(hx[(c >> s) & 0xf]);
    }
    dbg_putchar(' ');
    dbg_putchar('M');
    dbg_putchar('=');
    { uint32_t m = crash_info.magic;
      for(int s = 28; s >= 0; s -= 4) dbg_putchar(hx[(m >> s) & 0xf]);
    }
    dbg_putchar('\n');
    dbg_putchar('!'); dbg_putchar('H'); dbg_putchar('F'); dbg_putchar('\n');
    hf_canary = 0; /* Clear canary */
  } else if(crash_info.magic != CRASH_MAGIC) {
    return;
  } else {
    dbg_putchar('C');
    dbg_putchar('=');
    { uint32_t c = hf_canary;
      for(int s = 28; s >= 0; s -= 4) dbg_putchar(hx[(c >> s) & 0xf]);
    }
    dbg_putchar(' ');
    dbg_putchar('M');
    dbg_putchar('=');
    { uint32_t m = crash_info.magic;
      for(int s = 28; s >= 0; s -= 4) dbg_putchar(hx[(m >> s) & 0xf]);
    }
    dbg_putchar('\n');
  }

  /* Clear magic so we don't print again on next boot */
  crash_info.magic = 0;

  static const char hex[] = "0123456789abcdef";

  /* Helper: print a 32-bit hex value using dbg_putchar */
#define FAULT_PUTHEX(val) do { \
    uint32_t _v = (val); \
    for(int _s = 28; _s >= 0; _s -= 4) \
      dbg_putchar(hex[(_v >> _s) & 0xf]); \
  } while(0)
#define FAULT_PUTS(s) do { \
    const char *_p = (s); \
    while(*_p) dbg_putchar(*_p++); \
  } while(0)

  FAULT_PUTS("\n*** PREVIOUS CRASH ***\n");
  FAULT_PUTS("PC=");   FAULT_PUTHEX(crash_info.pc);
  FAULT_PUTS(" LR=");  FAULT_PUTHEX(crash_info.lr);
  FAULT_PUTS(" PSR="); FAULT_PUTHEX(crash_info.psr);
  dbg_putchar('\n');
  FAULT_PUTS("R0=");   FAULT_PUTHEX(crash_info.r0);
  FAULT_PUTS(" R1=");  FAULT_PUTHEX(crash_info.r1);
  FAULT_PUTS(" R2=");  FAULT_PUTHEX(crash_info.r2);
  FAULT_PUTS(" R3=");  FAULT_PUTHEX(crash_info.r3);
  dbg_putchar('\n');
  FAULT_PUTS("R12=");  FAULT_PUTHEX(crash_info.r12);
  FAULT_PUTS(" CFSR="); FAULT_PUTHEX(crash_info.cfsr);
  FAULT_PUTS(" HFSR="); FAULT_PUTHEX(crash_info.hfsr);
  dbg_putchar('\n');

  if(crash_info.cfsr & (1 << 7)) {
    FAULT_PUTS("MMFAR="); FAULT_PUTHEX(crash_info.mmfar); dbg_putchar('\n');
  }
  if(crash_info.cfsr & (1 << 15)) {
    FAULT_PUTS("BFAR="); FAULT_PUTHEX(crash_info.bfar); dbg_putchar('\n');
  }
  FAULT_PUTS("*** END CRASH ***\n");

#undef FAULT_PUTHEX
#undef FAULT_PUTS
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Hard fault final handling
 *
 */
__WEAK void
HardFault_process()
{
  NVIC_SystemReset();
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Hard fault c handler — saves crash info to .noinit RAM and resets.
 *
 * @param p_stack_address Pointer to hard fault stack
 */
/* Canary to detect if HardFault_c_handler ever runs */
__attribute__((section(".noinit"))) volatile uint32_t hf_canary;

void
HardFault_c_handler(uint32_t *p_stack_address)
{
  extern int dbg_putchar(int c);
  static const char hex[] = "0123456789abcdef";

  /* Write canary FIRST */
  hf_canary = HF_CANARY_VALUE;

  /* Immediately print "HF!" via the platform debug putchar so this
   * works on every nrf board, not just those with a uarte console. */
  dbg_putchar('\n');
  dbg_putchar('H');
  dbg_putchar('F');
  dbg_putchar('!');
  dbg_putchar(' ');

  HardFault_stack_t *p_stack = (HardFault_stack_t *)p_stack_address;

  crash_info.cfsr = SCB->CFSR;
  crash_info.hfsr = SCB->HFSR;
  crash_info.mmfar = SCB->MMFAR;
  crash_info.bfar = SCB->BFAR;

  if(p_stack != NULL) {
    crash_info.pc  = p_stack->pc;
    crash_info.lr  = p_stack->lr;
    crash_info.psr = p_stack->psr;
    crash_info.r0  = p_stack->r0;
    crash_info.r1  = p_stack->r1;
    crash_info.r2  = p_stack->r2;
    crash_info.r3  = p_stack->r3;
    crash_info.r12 = p_stack->r12;
  }

  /* Write magic last — signals that crash_info is valid */
  crash_info.magic = CRASH_MAGIC;

  /* Print crash info via dbg_putchar so this works on every nrf board. */
  {
#define HF_PUTHEX(val) do { \
    uint32_t _v = (val); \
    for(int _s = 28; _s >= 0; _s -= 4) { \
      dbg_putchar(hex[(_v >> _s) & 0xf]); \
    } \
  } while(0)
    dbg_putchar('P');
    dbg_putchar('C');
    dbg_putchar('=');
    HF_PUTHEX(crash_info.pc);
    dbg_putchar(' ');
    dbg_putchar('L');
    dbg_putchar('R');
    dbg_putchar('=');
    HF_PUTHEX(crash_info.lr);
    dbg_putchar(' ');
    dbg_putchar('C');
    dbg_putchar('F');
    dbg_putchar('=');
    HF_PUTHEX(crash_info.cfsr);
    dbg_putchar('\n');
#undef HF_PUTHEX
  }

  HardFault_process();
}
/*---------------------------------------------------------------------------*/
/**
 * @brief Hardfault handler
 * 
 */
void HardFault_Handler(void) __attribute__((naked));
/*---------------------------------------------------------------------------*/
/**
 * @brief Hardfault handler
 * 
 */
void
HardFault_Handler(void)
{
  __ASM volatile (
    "   .syntax unified                        \n"

    "   ldr   r0, =0xFFFFFFFD                  \n"
    "   cmp   r0, lr                           \n"
    "   bne   HardFault_Handler_ChooseMSP      \n"
    /* Reading PSP into R0 */
    "   mrs   r0, PSP                          \n"
    "   b     HardFault_Handler_Continue       \n"
    "HardFault_Handler_ChooseMSP:              \n"
    /* Reading MSP into R0 */
    "   mrs   r0, MSP                          \n"
    /* -----------------------------------------------------------------
     * If we have selected MSP check if we may use stack safetly.
     * If not - reset the stack to the initial value. */
    "   ldr   r1, =__StackTop                  \n"
    "   ldr   r2, =__StackLimit                \n"

    /* MSP is in the range of the stack area */
    "   cmp   r0, r1                           \n"
    "   bhi   HardFault_MoveSP                 \n"
    "   cmp   r0, r2                           \n"
    "   bhi   HardFault_Handler_Continue       \n"
    /* ----------------------------------------------------------------- */
    "HardFault_MoveSP:                         \n"
    "   mov   SP, r1                           \n"
    "   movs  r0, #0                           \n"

    "HardFault_Handler_Continue:               \n"
    "   ldr r3, =%0                            \n"
    "   bx r3                                  \n"

    "   .ltorg                                 \n"
    : : "X" (HardFault_c_handler)
    );
}
/*---------------------------------------------------------------------------*/
#else /* NRF_HARDFAULT_HANDLER_EXTENDED */
/*---------------------------------------------------------------------------*/
/**
 * @brief Hardfault handler
 * 
 */
void HardFault_Handler(void);
/*---------------------------------------------------------------------------*/
/**
 * @brief Hardfault handler
 * 
 */
void
HardFault_Handler(void)
{
  NVIC_SystemReset();
}
#endif /* NRF_HARDFAULT_HANDLER_EXTENDED */
/*---------------------------------------------------------------------------*/
/*
 * Override default fault handlers that loop forever (b .) in the startup
 * assembly.  Without these overrides, a BusFault/UsageFault/MemManage/
 * SecureFault silently freezes the CPU.
 *
 * On Cortex-M33 these faults escalate to HardFault by default (unless
 * individually enabled in SCB->SHCSR).  However, if any code enables them
 * (or if TrustZone routing sends SecureFault directly), these handlers
 * ensure the system resets with a diagnostic message instead of hanging.
 */
/*---------------------------------------------------------------------------*/
static void
fault_print_and_reset(char f1, char f2)
{
  extern int dbg_putchar(int c);
  dbg_putchar('\n');
  dbg_putchar(f1);
  dbg_putchar(f2);
  dbg_putchar('!');
  dbg_putchar('\n');
  /* Brief delay for the debug output to flush */
  for(volatile int i = 0; i < 50000; i++) {
  }
  NVIC_SystemReset();
}
/*---------------------------------------------------------------------------*/
void BusFault_Handler(void)       { fault_print_and_reset('B', 'F'); }
void UsageFault_Handler(void)     { fault_print_and_reset('U', 'F'); }
void MemoryManagement_Handler(void) { fault_print_and_reset('M', 'M'); }
#ifndef TRUSTZONE_SECURE
/* In a TrustZone secure build, tz-fault.c owns SecureFault_Handler. */
void SecureFault_Handler(void)    { fault_print_and_reset('S', 'F'); }
#endif
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 * @}
 */
