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
#include "contiki.h"
#include "cycles-arch.h"

#include <err.h>
#include <stdbool.h>
#include <stdlib.h>

/* The same guard as clock_time() in arch/platform/native/clock.c. */
#if defined(__linux__) || (defined(__MACH__) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101200)
#define CYCLES_ARCH_HAVE_CPUTIME_CLOCK 1
#include <time.h>
#else /* CYCLES_ARCH_HAVE_CPUTIME_CLOCK */
#define CYCLES_ARCH_HAVE_CPUTIME_CLOCK 0
#include <sys/resource.h>
#include <sys/time.h>
#endif /* CYCLES_ARCH_HAVE_CPUTIME_CLOCK */
/*---------------------------------------------------------------------------*/
static bool running;    /* the counter is enabled */
static uint32_t offset; /* the free-running clock minus the count */
static uint32_t frozen; /* the count kept while stopped */
/*---------------------------------------------------------------------------*/
/*
 * The low 32 bits of the process CPU time in nanoseconds. Truncating
 * each term to 32 bits is exact, because the low bits of a sum are the
 * sum of the low bits.
 */
static uint32_t
cputime_ns(void)
{
#if CYCLES_ARCH_HAVE_CPUTIME_CLOCK
  struct timespec ts;

  if(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts) == -1) {
    err(EXIT_FAILURE, "clock_gettime");
  }

  return (uint32_t)ts.tv_sec * 1000000000u + (uint32_t)ts.tv_nsec;
#else /* CYCLES_ARCH_HAVE_CPUTIME_CLOCK */
  struct rusage ru;
  uint32_t us;

  /*
   * The same quantity at microsecond resolution, for systems without the
   * CPU-time clock.
   */
  if(getrusage(RUSAGE_SELF, &ru) == -1) {
    err(EXIT_FAILURE, "getrusage");
  }

  us = (uint32_t)ru.ru_utime.tv_sec * 1000000u + (uint32_t)ru.ru_utime.tv_usec
    + (uint32_t)ru.ru_stime.tv_sec * 1000000u + (uint32_t)ru.ru_stime.tv_usec;

  return us * 1000u;
#endif /* CYCLES_ARCH_HAVE_CPUTIME_CLOCK */
}
/*---------------------------------------------------------------------------*/
void
cycles_arch_start(void)
{
  if(running) {
    /*
     * Idempotent, so a call from a second user does not disturb a
     * measurement in progress.
     */
    return;
  }
  offset = cputime_ns() - frozen;
  running = true;
}
/*---------------------------------------------------------------------------*/
void
cycles_arch_stop(void)
{
  if(!running) {
    return;
  }
  frozen = cputime_ns() - offset;
  running = false;
}
/*---------------------------------------------------------------------------*/
uint32_t
cycles_arch_now(void)
{
  return running ? cputime_ns() - offset : frozen;
}
/*---------------------------------------------------------------------------*/
/** @} */
