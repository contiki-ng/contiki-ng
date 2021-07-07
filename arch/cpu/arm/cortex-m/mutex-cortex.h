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
 * \addtogroup arm
 *
 * Arm Cortex-M implementation of mutexes using the LDREX, STREX and DMB
 * instructions.
 *
 * @{
 */
/*---------------------------------------------------------------------------*/
#ifndef MUTEX_CORTEX_H_
#define MUTEX_CORTEX_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#ifdef CMSIS_CONF_HEADER_PATH
#include CMSIS_CONF_HEADER_PATH
#endif

#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#define mutex_try_lock(m) mutex_cortex_try_lock(m)
#define mutex_unlock(m)   mutex_cortex_unlock(m)
/*---------------------------------------------------------------------------*/
#define MUTEX_CONF_HAS_MUTEX_T 1
typedef uint8_t mutex_t;
/*---------------------------------------------------------------------------*/
static inline bool
mutex_cortex_try_lock(volatile mutex_t *mutex)
{
  do {
    if(__LDREXB(mutex) != 0) {
      return false;
    }
  } while(__STREXB(1, mutex) != 0);

  __DMB();

  return true;
}
/*---------------------------------------------------------------------------*/
static inline void
mutex_cortex_unlock(volatile mutex_t *mutex)
{
  __DMB();
  *mutex = 0;
}
/*---------------------------------------------------------------------------*/
#endif /* MUTEX_CORTEX_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
