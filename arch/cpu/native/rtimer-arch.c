/*
 * Copyright (c) 2007, Swedish Institute of Computer Science.
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
 *         Native (non-specific) code for the Contiki real-time module rt
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include <signal.h>
#include <sys/time.h>
#include <stddef.h>

#include "sys/rtimer.h"
#include "sys/clock.h"

#define DEBUG 0
#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/*---------------------------------------------------------------------------*/
static void
interrupt(int sig)
{
  signal(sig, interrupt);
  rtimer_run_next();
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_init(void)
{
  signal(SIGALRM, interrupt);
}
/*---------------------------------------------------------------------------*/
void
rtimer_arch_schedule(rtimer_clock_t t)
{
  struct itimerval val;
  rtimer_clock_t c;

  c = t - clock_time();
  
  val.it_value.tv_sec = c / CLOCK_SECOND;
  val.it_value.tv_usec = (c % CLOCK_SECOND) * CLOCK_SECOND;

  PRINTF("rtimer_arch_schedule time %"PRIu32 " %"PRIu32 " in %ld.%ld seconds\n",
         t, c, (long)val.it_value.tv_sec, (long)val.it_value.tv_usec);

  val.it_interval.tv_sec = val.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &val, NULL);
}
/*---------------------------------------------------------------------------*/
