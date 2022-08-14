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
#include "sys/log.h"

#define LOG_MODULE "NRF HARDFAULT"
#define LOG_LEVEL LOG_LEVEL_INFO
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
 * @brief Hard fault c handler
 * 
 * @param p_stack_address Pointer to hard fault stack
 */
void
HardFault_c_handler(uint32_t *p_stack_address)
{
#ifndef CFSR_MMARVALID
#define CFSR_MMARVALID (1 << (0 + 7))
#endif

#ifndef CFSR_BFARVALID
#define CFSR_BFARVALID (1 << (8 + 7))
#endif

  HardFault_stack_t *p_stack = (HardFault_stack_t *)p_stack_address;
  static const char *cfsr_msgs[] = {
    [0] = "The processor has attempted to execute an undefined instruction",
    [1] = "The processor attempted a load or store at a location that does not permit the operation",
    [2] = NULL,
    [3] = "Unstack for an exception return has caused one or more access violations",
    [4] = "Stacking for an exception entry has caused one or more access violations",
    [5] = "A MemManage fault occurred during floating-point lazy state preservation",
    [6] = NULL,
    [7] = NULL,
    [8] = "Instruction bus error",
    [9] = "Data bus error (PC value stacked for the exception return points to the instruction that caused the fault)",
    [10] = "Data bus error (return address in the stack frame is not related to the instruction that caused the error)",
    [11] = "Unstack for an exception return has caused one or more BusFaults",
    [12] = "Stacking for an exception entry has caused one or more BusFaults",
    [13] = "A bus fault occurred during floating-point lazy state preservation",
    [14] = NULL,
    [15] = NULL,
    [16] = "The processor has attempted to execute an undefined instruction",
    [17] = "The processor has attempted to execute an instruction that makes illegal use of the EPSR",
    [18] = "The processor has attempted an illegal load of EXC_RETURN to the PC, as a result of an invalid context, or an invalid EXC_RETURN value",
    [19] = "The processor has attempted to access a coprocessor",
    [20] = NULL,
    [21] = NULL,
    [22] = NULL,
    [23] = NULL,
    [24] = "The processor has made an unaligned memory access",
    [25] = "The processor has executed an SDIV or UDIV instruction with a divisor of 0",
  };

  uint32_t cfsr = SCB->CFSR;

  if(p_stack != NULL) {
    /* Print information about error. */
    LOG_INFO("HARD FAULT at 0x%08lX\n", p_stack->pc);
    LOG_INFO("  R0:  0x%08lX  R1:  0x%08lX  R2:  0x%08lX  R3:  0x%08lX\n",
             p_stack->r0, p_stack->r1, p_stack->r2, p_stack->r3);
    LOG_INFO("  R12: 0x%08lX  LR:  0x%08lX  PSR: 0x%08lX\n",
             p_stack->r12, p_stack->lr, p_stack->psr);
  } else {
    LOG_INFO("Stack violation: stack pointer outside stack area.\n");
  }

  if(SCB->HFSR & SCB_HFSR_VECTTBL_Msk) {
    LOG_INFO("Cause: BusFault on a vector table read during exception processing.\n");
  }

  for(uint32_t i = 0; i < sizeof(cfsr_msgs) / sizeof(cfsr_msgs[0]); i++) {
    if(((cfsr & (1 << i)) != 0) && (cfsr_msgs[i] != NULL)) {
      LOG_INFO("Cause: %s.\n", cfsr_msgs[i]);
    }
  }

  if(cfsr & CFSR_MMARVALID) {
    LOG_INFO("MemManage Fault Address: 0x%08lX\n", SCB->MMFAR);
  }

  if(cfsr & CFSR_BFARVALID) {
    LOG_INFO("Bus Fault Address: 0x%08lX\n", SCB->BFAR);
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
/**
 * @}
 * @}
 * @}
 */
