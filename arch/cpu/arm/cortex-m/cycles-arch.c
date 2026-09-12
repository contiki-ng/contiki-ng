/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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
 * \addtogroup cycles
 * @{
 *
 * \file
 *         Cycle counter on the Cortex-M Data Watchpoint and Trace unit.
 * \author
 *         Niclas Finne <niclas.finne@ri.se>
 */
#include "contiki.h"
#include "cycles-arch.h"
/*---------------------------------------------------------------------------*/
void
cycles_arch_start(void)
{
  /*
   * Off after a power-on reset, and otherwise as a warm reset or a
   * debugger left it. TRCENA is set first because it gates the counting
   * as well as the register definitions: a core found with CYCCNTENA
   * set and TRCENA clear keeps its counter frozen. TRCENA stays set
   * once enabled, being the global enable for the whole DWT and ITM.
   *
   * Only enable bits are touched, so a call from a second user does not
   * disturb a measurement in progress, and a counter that is already
   * running keeps running.
   */
  CYCLES_ARCH_DEMCR |= CYCLES_ARCH_DEMCR_TRCENA;
  CYCLES_ARCH_DWT_CTRL |= CYCLES_ARCH_DWT_CTRL_CYCCNTENA;
}
/*---------------------------------------------------------------------------*/
void
cycles_arch_stop(void)
{
  /*
   * Only the counter stops; TRCENA is left alone so that the count stays
   * readable and a trace session set up by a debugger survives.
   */
  CYCLES_ARCH_DWT_CTRL &= ~CYCLES_ARCH_DWT_CTRL_CYCCNTENA;
}
/*---------------------------------------------------------------------------*/
/** @} */
