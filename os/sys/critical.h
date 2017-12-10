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
 * \defgroup critical Critical sections
 * @{
 *
 * Platform-independent functions for critical section entry and exit
 */
/*---------------------------------------------------------------------------*/
#ifndef CRITICAL_H_
#define CRITICAL_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/memory-barrier.h"
#include "sys/int-master.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/**
 * \brief Enter a critical section
 * \return The status of the master interrupt before entering the critical
 *
 * This function will return the status of the master interrupt as it was
 * before entering the critical section.
 *
 * The semantics of the return value are entirely platform-specific. The
 * calling code should not try to determine whether the master interrupt was
 * previously enabled/disabled by interpreting the return value of this
 * function. The return value should only be used as an argument to
 * critical_exit().
 */
static inline int_master_status_t
critical_enter()
{
  int_master_status_t status = int_master_read_and_disable();
  memory_barrier();
  return status;
}
/*---------------------------------------------------------------------------*/
/**
 * \brief Exit a critical section and restore the master interrupt
 * \param status The new status of the master interrupt
 *
 * The semantics of \e status are platform-dependent. Normally, the argument
 * provided to this function will be a value previously retrieved through a
 * call to critical_enter().
 */
static inline void
critical_exit(int_master_status_t status)
{
  memory_barrier();
  int_master_status_set(status);
}
/*---------------------------------------------------------------------------*/
#endif /* CRITICAL_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
