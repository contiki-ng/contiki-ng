/*
 * Copyright (c) 2005, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Clock implementation for Unix.
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "sys/clock.h"
#include <time.h>
#include <sys/time.h>

/*---------------------------------------------------------------------------*/
typedef struct clock_timespec_s {
  time_t  tv_sec;
  long  tv_nsec;
} clock_timespec_t;
/*---------------------------------------------------------------------------*/
static void
get_time(clock_timespec_t *spec)
{
#if defined(__linux__) || (defined(__MACH__) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 101200)
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);

  spec->tv_sec = ts.tv_sec;
  spec->tv_nsec = ts.tv_nsec;
#else
  struct timeval tv;

  gettimeofday(&tv, NULL);

  spec->tv_sec = tv.tv_sec;
  spec->tv_nsec = tv.tv_usec * 1000;
#endif
}
/*---------------------------------------------------------------------------*/
clock_time_t
clock_time(void)
{
  clock_timespec_t ts;

  get_time(&ts);

  return ts.tv_sec * CLOCK_SECOND + ts.tv_nsec / (1000000000 / CLOCK_SECOND);
}
/*---------------------------------------------------------------------------*/
unsigned long
clock_seconds(void)
{
  clock_timespec_t ts;

  get_time(&ts);

  return ts.tv_sec;
}
/*---------------------------------------------------------------------------*/
void
clock_delay(unsigned int d)
{
  /* Does not do anything. */
}
/*---------------------------------------------------------------------------*/
void
clock_init(void)
{
  /* Provide this function, required by main() */
  return;
}
/*---------------------------------------------------------------------------*/
