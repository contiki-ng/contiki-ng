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
 * \addtogroup sys
 * @{
 *
 * \defgroup cycles CPU cycle counter
 *
 * Cycle-accurate timing of short code paths, for measurements that the
 * rtimer is too coarse for. The counter is a 32-bit CPU cycle count
 * that stops while the CPU sleeps, so a difference of two readings is
 * the CPU time spent between them, not the wall-clock time. It wraps
 * after 2^32 cycles (about a minute at 64 MHz).
 *
 * The counter is shared. Starting it is idempotent and never disturbs
 * the count, so every measurement site calls cycles_start() when it
 * begins measuring, however many there are, and takes the difference
 * of two readings after that. Stopping is global: it stops the counter
 * for every user, so it is for diagnostics, and only when no
 * measurement is in progress. A stopped counter reads a constant,
 * which shows up as 0 cycles in a measurement.
 *
 * The counter is off after a power-on reset. On a Cortex-M it belongs
 * to the debug logic, which a warm reset leaves alone, so a watchdog
 * or software reset, or a debugger with trace enabled, can leave it
 * running with a stale count. A measurement does not notice, since it
 * starts the counter and takes a difference.
 *
 * On a CPU with a cycle counter the count is exact, and the same code
 * path repeats to within the interrupts, flash wait states and cache
 * effects it sees. A hosted port has no such counter and reports host
 * CPU time instead, which varies by tens of percent between runs;
 * those readings are for logging and profiling, and must never decide
 * what the code does.
 *
 * Provided by the CPU port through cycles-arch.h when the port sets
 * CYCLES_CONF_ARCH_SUPPORTED, which an application can override to 0,
 * for instance to leave a Cortex-M DWT to a debugger; elsewhere
 * cycles_now() reads 0 so that measurement code compiles everywhere.
 * Converting a count to time takes the core frequency, CYCLES_HZ, which
 * the port sets through CYCLES_CONF_HZ. On a port that has a counter
 * but sets no frequency, CYCLES_HZ and the CYCLES_TO_US helpers are
 * undefined, so that a wrong time is a compile error rather than a
 * number.
 * @{
 *
 * \file
 *         CPU cycle counter API.
 * \author
 *         Niclas Finne <niclas.finne@ri.se>
 */
#ifndef CYCLES_H_
#define CYCLES_H_

#include "contiki.h"
#include <stdint.h>

/*
 * A CPU port that has a counter sets CYCLES_CONF_ARCH_SUPPORTED, and an
 * application can override it to 0, for instance to leave a Cortex-M
 * DWT to a debugger.
 */
#ifdef CYCLES_CONF_ARCH_SUPPORTED
#define CYCLES_ARCH_SUPPORTED CYCLES_CONF_ARCH_SUPPORTED
#else /* CYCLES_CONF_ARCH_SUPPORTED */
#define CYCLES_ARCH_SUPPORTED 0
#endif /* CYCLES_CONF_ARCH_SUPPORTED */

#if CYCLES_ARCH_SUPPORTED
#include "cycles-arch.h"

/** \brief Start the cycle counter; idempotent, keeps the current count. */
static inline void
cycles_start(void)
{
  cycles_arch_start();
}
/** \brief Stop the cycle counter for every user; the count is kept. */
static inline void
cycles_stop(void)
{
  cycles_arch_stop();
}
/** \brief The current cycle count. */
static inline uint32_t
cycles_now(void)
{
  return cycles_arch_now();
}

#ifdef CYCLES_ARCH_HZ
/** \brief Cycles per second, from the port's configured core frequency. */
#define CYCLES_HZ CYCLES_ARCH_HZ
#endif /* CYCLES_ARCH_HZ */
#else /* CYCLES_ARCH_SUPPORTED */

/** \brief Start the cycle counter; no counter on this port, so a no-op. */
static inline void
cycles_start(void)
{
}
/** \brief Stop the cycle counter; no counter on this port, so a no-op. */
static inline void
cycles_stop(void)
{
}
/** \brief The current cycle count; always 0 without a counter. */
static inline uint32_t
cycles_now(void)
{
  return 0;
}

/*
 * A port without a counter converts too, so that measurement code
 * compiles everywhere. The only count it can obtain is the constant 0
 * that cycles_now() returns, and that is 0 us at any rate, so the value
 * here only has to be one the conversions can divide by.
 */
#define CYCLES_HZ 1
#endif /* CYCLES_ARCH_SUPPORTED */

/*
 * The count is 32-bit, so it is narrowed before it is widened: an int
 * holding a count would otherwise sign-extend and convert to a time far
 * outside the documented range.
 */
#ifdef CYCLES_HZ
/** \brief Whole microseconds of a cycle count. */
#define CYCLES_TO_US(cycles) \
  ((unsigned long)((uint64_t)(uint32_t)(cycles) * 1000000 / CYCLES_HZ))
/** \brief Hundredths of a microsecond beyond CYCLES_TO_US(). */
#define CYCLES_TO_US_FRAC(cycles) \
  ((unsigned long)((uint64_t)(uint32_t)(cycles) * 100000000 / CYCLES_HZ % 100))
#endif /* CYCLES_HZ */

#endif /* CYCLES_H_ */
/**
 * @}
 * @}
 */
