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
#ifndef CYCLES_ARCH_H_
#define CYCLES_ARCH_H_

#include <stdint.h>

/*
 * The Data Watchpoint and Trace unit and the Debug Control Block sit at
 * addresses the ARMv7-M and ARMv8-M architectures fix for every core, in
 * the private peripheral bus, which TrustZone leaves accessible to both
 * worlds. Only a power-on reset clears them: a warm reset or a debugger
 * can leave the counter running, see os/sys/cycles.h.
 *
 * They are addressed directly rather than through CMSIS. CMSIS is on
 * the include path of every Cortex-M port here, but in scope from
 * contiki.h only on the nrf one; core_cmX.h cannot be included on its
 * own, because it needs IRQn_Type and __NVIC_PRIO_BITS from a device
 * header first; and the Debug Control Block is not spelled the same
 * across these cores, core_cm33.h deprecating CoreDebug in favour of
 * DCB while core_cm3.h and core_cm4.h have only CoreDebug.
 */
#define CYCLES_ARCH_DWT_CTRL    (*(volatile uint32_t *)0xE0001000UL)
#define CYCLES_ARCH_DWT_CYCCNT  (*(volatile uint32_t *)0xE0001004UL)
#define CYCLES_ARCH_DEMCR       (*(volatile uint32_t *)0xE000EDFCUL)

#define CYCLES_ARCH_DWT_CTRL_CYCCNTENA 0x00000001UL
#define CYCLES_ARCH_DEMCR_TRCENA       0x01000000UL

/*
 * NOCYCCNT reads as 1 on a core that implements no cycle counter, so it
 * is the check to make when bringing up a port. Nothing here reads it:
 * the cores this file is built for all have CYCCNT.
 */
#define CYCLES_ARCH_DWT_CTRL_NOCYCCNT  0x02000000UL

void cycles_arch_start(void);
void cycles_arch_stop(void);

static inline uint32_t
cycles_arch_now(void)
{
  return CYCLES_ARCH_DWT_CYCCNT;
}

/*
 * Fixed at compile time from the CPU port's configured core frequency,
 * because a run-time SystemCoreClock is not readable from a TrustZone
 * normal world on every SoC. A CPU or board header sets CYCLES_CONF_HZ;
 * where none does, CYCLES_HZ and the CYCLES_TO_US helpers stay
 * undefined, see os/sys/cycles.h.
 */
#ifdef CYCLES_CONF_HZ
#define CYCLES_ARCH_HZ CYCLES_CONF_HZ
#endif /* CYCLES_CONF_HZ */
#endif /* CYCLES_ARCH_H_ */
/** @} */
