/*
 * Copyright (c) 2017, George Oikonomou - http://www.spd.gr
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
 * \defgroup mutex Mutexes
 * @{
 *
 * This library provides an API and generic implementation of mutexes.
 *
 * Calling code should manipulate mutexes through the mutex_try_lock() and
 * mutex_unlock() macros. By default, those macros will expand to the generic
 * mutex manipulation implementations provided here. While these will work,
 * they do reply on disabling the master interrupt in order to perform the
 * lock/unlock operation.
 *
 * It is possible to override those generic implementation with CPU-specific
 * implementations that exploit synchronisation instructions. To do so,
 * create a CPU-specific header file. In this file, define mutex_try_lock()
 * and mutex_unlock() to expand to the respective CPU function names. These
 * can (but do not have to) be inlined. Then define MUTEX_CONF_ARCH_HEADER_PATH
 * as this header's filename.
 */
/*---------------------------------------------------------------------------*/
#ifndef MUTEX_H_
#define MUTEX_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#define MUTEX_STATUS_LOCKED   1 /** The mutex is locked */
#define MUTEX_STATUS_UNLOCKED 0 /** The mutex is not locked */
/*---------------------------------------------------------------------------*/
#ifdef MUTEX_CONF_ARCH_HEADER_PATH
#include MUTEX_CONF_ARCH_HEADER_PATH
#endif /* MUTEX_CONF_ARCH_HEADER_PATH */
/*---------------------------------------------------------------------------*/
#if !MUTEX_CONF_HAS_MUTEX_T
/**
 * \brief Mutex data type
 *
 * It is possible for the platform to override this with its own typedef. In
 * this scenario, make sure to also define MUTEX_CONF_HAS_MUTEX_T as 1.
 */
typedef uint_fast8_t mutex_t;
#endif
/*---------------------------------------------------------------------------*/
#ifndef mutex_try_lock
/**
 * \brief Try to lock a mutex
 * \param m A pointer to the mutex to be locked
 * \retval true Locking succeeded
 * \retval false Locking failed (the mutex is already locked)
 *
 * This macro will expand to mutex_generic_try_lock() or to a CPU-provided
 * implementation. Platform-independent code should use this macro instead
 * of mutex_generic_try_lock().
 */
#define mutex_try_lock(m) mutex_generic_try_lock(m)
#endif

#ifndef mutex_unlock
/**
 * \brief Unlock a previously acquired mutex
 * \param m A pointer to the mutex to be unlocked
 *
 * This macro will expand to mutex_generic_unlock() or to a CPU-provided
 * implementation. Platform-independent code should use this macro instead
 * of mutex_generic_unlock().
 */
#define mutex_unlock(m)   mutex_generic_unlock(m)
#endif
/*---------------------------------------------------------------------------*/
/**
 * \brief Try to lock a mutex
 * \param mutex A pointer to the mutex to be locked
 * \retval true Locking succeeded
 * \retval false Locking failed (the mutex is already locked)
 *
 * Do not call this function directly. Use the mutex_try_lock() macro instead.
 */
bool mutex_generic_try_lock(volatile mutex_t *mutex);

/**
 * \brief Unlock a previously acquired mutex
 * \param mutex A pointer to the mutex to be unlocked
 *
 * Do not call this function directly. Use the mutex_unlock() macro instead.
 */
void mutex_generic_unlock(volatile mutex_t *mutex);
/*---------------------------------------------------------------------------*/
#endif /* MUTEX_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
