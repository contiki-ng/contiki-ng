/*
 * Copyright (c) 2017 - 2020, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_GLUE_H__
#define NRFX_GLUE_H__

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \addtogroup nrf
 * @{
 *
 * \file
 *      Header with nrfx stub defines
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 */

#include <soc/nrfx_irqs.h>
#include <soc/nrfx_atomic.h>
#include <soc/nrfx_coredep.h>

/*------------------------------------------------------------------------------ */

/**
 * @brief Macro for placing a runtime assertion.
 *
 * @param expression  Expression to evaluate.
 */
#define NRFX_ASSERT(expression)

/**
 * @brief Macro for placing a compile time assertion.
 *
 * @param expression  Expression to evaluate.
 */
#define NRFX_STATIC_ASSERT(expression)

/*------------------------------------------------------------------------------ */

/**
 * @brief Macro for setting the priority of a specific IRQ.
 *
 * @param irq_number IRQ number.
 * @param priority   Priority to be set.
 */
#define NRFX_IRQ_PRIORITY_SET(irq_number, priority) \
  _NRFX_IRQ_PRIORITY_SET(irq_number, priority)
static inline void
_NRFX_IRQ_PRIORITY_SET(IRQn_Type irq_number,
                       uint8_t priority)
{
  NVIC_SetPriority(irq_number, priority);
}
/**
 * @brief Macro for enabling a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_ENABLE(irq_number)  _NRFX_IRQ_ENABLE(irq_number)
static inline void
_NRFX_IRQ_ENABLE(IRQn_Type irq_number)
{
  NVIC_EnableIRQ(irq_number);
}
/**
 * @brief Macro for checking if a specific IRQ is enabled.
 *
 * @param irq_number IRQ number.
 *
 * @retval true  If the IRQ is enabled.
 * @retval false Otherwise.
 */
#define NRFX_IRQ_IS_ENABLED(irq_number)  _NRFX_IRQ_IS_ENABLED(irq_number)
static inline bool
_NRFX_IRQ_IS_ENABLED(IRQn_Type irq_number)
{
  return 0 != (NVIC->ISER[irq_number / 32] & (1UL << (irq_number % 32)));
}
/**
 * @brief Macro for disabling a specific IRQ.
 *
 * @param irq_number  IRQ number.
 */
#define NRFX_IRQ_DISABLE(irq_number)  _NRFX_IRQ_DISABLE(irq_number)
static inline void
_NRFX_IRQ_DISABLE(IRQn_Type irq_number)
{
  NVIC_DisableIRQ(irq_number);
}
/**
 * @brief Macro for clearing the pending status of a specific IRQ.
 *
 * @param irq_number IRQ number.
 */
#define NRFX_IRQ_PENDING_CLEAR(irq_number) _NVIC_ClearPendingIRQ(irq_number)
static inline void
_NVIC_ClearPendingIRQ(IRQn_Type irq_number)
{
  NVIC_ClearPendingIRQ(irq_number);
}
/**
 * @brief Macro for entering into a critical section.
 */
#define NRFX_CRITICAL_SECTION_ENTER()   __disable_irq()

/**
 * @brief Macro for exiting from a critical section.
 */
#define NRFX_CRITICAL_SECTION_EXIT()    __enable_irq()

/*------------------------------------------------------------------------------ */

#define NRFX_DELAY_US(us_time) nrfx_coredep_delay_us(us_time)

/*------------------------------------------------------------------------------ */

/** @brief Atomic 32-bit unsigned type. */
#define nrfx_atomic_t uint32_t

/**
 * @brief Macro for running a bitwise AND operation on an atomic object
 *        and returning its previous value.
 *
 * @param[in] p_data Atomic memory pointer.
 * @param[in] value  Value of the second operand in the AND operation.
 *
 * @return Previous value of the atomic object.
 */
#define NRFX_ATOMIC_FETCH_AND(p_data, value) nrfx_atomic_u32_fetch_and(p_data, value)

/*------------------------------------------------------------------------------ */

/** @} */

#ifdef __cplusplus
}
#endif

#endif // NRFX_GLUE_H__
