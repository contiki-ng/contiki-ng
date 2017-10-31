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
 *         Implementation of the energy estimation module
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#include "contiki.h"
#include "sys/energest.h"

#if ENERGEST_CONF_ON

uint64_t energest_total_time[ENERGEST_TYPE_MAX];
ENERGEST_TIME_T energest_current_time[ENERGEST_TYPE_MAX];
unsigned char energest_current_mode[ENERGEST_TYPE_MAX];

/*---------------------------------------------------------------------------*/
void
energest_init(void)
{
  int i;
  for(i = 0; i < ENERGEST_TYPE_MAX; ++i) {
    energest_total_time[i] = energest_current_time[i] = 0;
    energest_current_mode[i] = 0;
  }
  ENERGEST_ON(ENERGEST_TYPE_CPU);
}
/*---------------------------------------------------------------------------*/
void
energest_flush(void)
{
  uint64_t now;
  int i;
  for(i = 0; i < ENERGEST_TYPE_MAX; i++) {
    if(energest_current_mode[i]) {
      now = ENERGEST_CURRENT_TIME();
      energest_total_time[i] +=
        (ENERGEST_TIME_T)(now - energest_current_time[i]);
      energest_current_time[i] = now;
    }
  }
}
/*---------------------------------------------------------------------------*/
uint64_t
energest_get_total_time(void)
{
  return energest_type_time(ENERGEST_TYPE_CPU) +
    energest_type_time(ENERGEST_TYPE_LPM) +
    energest_type_time(ENERGEST_TYPE_DEEP_LPM);
}
/*---------------------------------------------------------------------------*/
#else /* ENERGEST_CONF_ON */

void
energest_init(void)
{
}

void
energest_flush(void)
{
}

uint64_t
energest_get_total_time(void)
{
  return 0;
}

#endif /* ENERGEST_CONF_ON */
