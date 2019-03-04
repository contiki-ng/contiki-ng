/*
 * Copyright (c) 2019, Toshiba Corporation
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
/**
 * \addtogroup sys
 * @{
 *
 * \defgroup atomic Atomic operations
 * @{
 *
 * This library provides an API and generic implementation of atomic
 * operations.
 *
 * The structure of this library is more or less the same as
 * sys/mutex. By default, atomic operations are implemented by
 * disabling interrupts temporarily. Platforms can provide better
 * implementation using platform-specific features.
 *
 */
#ifndef ATOMIC_H_
#define ATOMIC_H_

#include <contiki.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef ATOMIC_CONF_ARCH_HEADER_PATH
#include ATOMIC_CONF_ARCH_HEADER_PATH
#endif /* ATOMIC_CONF_ARCH_HEADER_PATH */

/**
 * \brief Atomic compare-and-swap (CAS) on a byte.
 * 
 * This macro expands to atomic_generic_cas_uint8() or CPU-provided
 * implementation. Platform-independent code should use this macro
 * instead of atomic_generic_cas_uint8(). 
 */  
#ifndef atomic_cas_uint8
#define atomic_cas_uint8(t,o,n) atomic_generic_cas_uint8((t),(o),(n))
#endif /* atomic_cas_uint8 */

/**
 * \brief Atomic compare-and-swap (CAS) on a byte (generic impl.)
 * \param target Pointer to the target byte to manipulate.
 * \param old_val Value that is expected to be stored in the target.
 * \param new_val Value that it stores to the target.
 *
 * If value of target is equal to old_val, store new_val to
 * target. If the store operation succeeds, it returns true.
 * Otherwise, it just returns false without storing.
 */
bool atomic_generic_cas_uint8(uint8_t *target, uint8_t old_val, uint8_t new_val);

#endif /* ATOMIC_H_ */
/**
 * @}
 * @}
 */
