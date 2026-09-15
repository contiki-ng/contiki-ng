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
 *         Cycle counter on the process CPU-time clock.
 * \author
 *         Niclas Finne <niclas.finne@ri.se>
 */
#ifndef CYCLES_ARCH_H_
#define CYCLES_ARCH_H_

#include <stdint.h>

/*
 * A cycle is a nanosecond of process CPU time, which is what a hosted
 * port has that matches the contract in os/sys/cycles.h: the count does
 * not advance while the process is not running on a CPU. The count is
 * the low 32 bits of that time, so it wraps after about 4.3 s of CPU
 * time, in the same way as a 32-bit hardware counter.
 *
 * Unlike a hardware cycle counter this is host CPU time measured under
 * a host scheduler, so the same code path does not cost the same twice:
 * readings vary by tens of percent between runs. Use them for logging
 * and profiling, never to decide what the code does, and do not assert
 * on their value.
 */
#define CYCLES_ARCH_HZ 1000000000

void cycles_arch_start(void);
void cycles_arch_stop(void);
uint32_t cycles_arch_now(void);

#endif /* CYCLES_ARCH_H_ */
/** @} */
