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
 *         Header file for the energy estimation mechanism
 * \author
 *         Adam Dunkels <adam@sics.se>
 */

#ifndef ENERGEST_H_
#define ENERGEST_H_

#include "contiki.h"

#ifndef ENERGEST_CONF_ON
/* Energest is disabled by default */
#define ENERGEST_CONF_ON 0
#endif /* ENERGEST_CONF_ON */

#ifndef ENERGEST_CURRENT_TIME
#ifdef ENERGEST_CONF_CURRENT_TIME
#define ENERGEST_CURRENT_TIME ENERGEST_CONF_CURRENT_TIME
#else
#define ENERGEST_CURRENT_TIME RTIMER_NOW
#define ENERGEST_SECOND RTIMER_SECOND
#define ENERGEST_TIME_T rtimer_clock_t
#endif /* ENERGEST_CONF_TIME */
#endif /* ENERGEST_TIME */

#ifndef ENERGEST_TIME_T
#ifdef ENERGEST_CONF_TIME_T
#define ENERGEST_TIME_T ENERGEST_CONF_TIME_T
#else
#define ENERGEST_TIME_T rtimer_clock_t
#endif /* ENERGEST_CONF_TIME_T */
#endif /* ENERGEST_TIME_T */

#ifndef ENERGEST_SECOND
#ifdef ENERGEST_CONF_SECOND
#define ENERGEST_SECOND ENERGEST_CONF_SECOND
#else /* ENERGEST_CONF_SECOND */
#define ENERGEST_SECOND RTIMER_SECOND
#endif /* ENERGEST_CONF_SECOND */
#endif /* ENERGEST_SECOND */

#ifndef ENERGEST_GET_TOTAL_TIME
#if ENERGEST_CONF_ON
#ifdef ENERGEST_CONF_GET_TOTAL_TIME
#define ENERGEST_GET_TOTAL_TIME ENERGEST_CONF_GET_TOTAL_TIME
#else /* ENERGEST_CONF_GET_TOTAL_TIME */
#define ENERGEST_GET_TOTAL_TIME energest_get_total_time
#endif /* ENERGEST_CONF_GET_TOTAL_TIME */
#else /* ENERGEST_CONF_ON */
#define ENERGEST_GET_TOTAL_TIME()
#endif /* ENERGEST_CONF_ON */
#endif /* ENERGEST_GET_TOTAL_TIME */

/*
 * Optional support for more energest types.
 *
 * #define ENERGEST_CONF_PLATFORM_ADDITIONS TYPE_NAME1, TYPE_NAME2
 *
 * #define ENERGEST_CONF_ADDITIONS TYPE_NAME3, TYPE_NAME4
 */
typedef enum energest_type {
  ENERGEST_TYPE_CPU,
  ENERGEST_TYPE_LPM,
  ENERGEST_TYPE_DEEP_LPM,
  ENERGEST_TYPE_TRANSMIT,
  ENERGEST_TYPE_LISTEN,

#ifdef ENERGEST_CONF_PLATFORM_ADDITIONS
  ENERGEST_CONF_PLATFORM_ADDITIONS,
#endif /* ENERGEST_CONF_PLATFORM_ADDITIONS */

#ifdef ENERGEST_CONF_ADDITIONS
  ENERGEST_CONF_ADDITIONS,
#endif /* ENERGEST_CONF_ADDITIONS */

  ENERGEST_TYPE_MAX
} energest_type_t;

#if ENERGEST_CONF_ON

void energest_init(void);
void energest_flush(void);

uint64_t ENERGEST_GET_TOTAL_TIME(void);

extern uint64_t energest_total_time[ENERGEST_TYPE_MAX];
extern ENERGEST_TIME_T energest_current_time[ENERGEST_TYPE_MAX];
extern unsigned char energest_current_mode[ENERGEST_TYPE_MAX];

static inline uint64_t
energest_type_time(energest_type_t type)
{
  return energest_total_time[type];
}

static inline void
energest_type_set(energest_type_t type, uint64_t value)
{
  energest_total_time[type] = value;
}

static inline void
energest_on(energest_type_t type)
{
  if(energest_current_mode[type] == 0) {
    energest_current_time[type] = ENERGEST_CURRENT_TIME();
    energest_current_mode[type] = 1;
  }
}
#define ENERGEST_ON(type) energest_on(type)

static inline void
energest_off(energest_type_t type)
{
 if(energest_current_mode[type] != 0) {
   energest_total_time[type] +=
     (ENERGEST_TIME_T)(ENERGEST_CURRENT_TIME() - energest_current_time[type]);
   energest_current_mode[type] = 0;
 }
}
#define ENERGEST_OFF(type) energest_off(type)

static inline void
energest_switch(energest_type_t type_off, energest_type_t type_on)
{
  ENERGEST_TIME_T energest_local_variable_now = ENERGEST_CURRENT_TIME();
  if(energest_current_mode[type_off] != 0) {
    energest_total_time[type_off] += (ENERGEST_TIME_T)
      (energest_local_variable_now - energest_current_time[type_off]);
    energest_current_mode[type_off] = 0;
  }
  if(energest_current_mode[type_on] == 0) {
    energest_current_time[type_on] = energest_local_variable_now;
    energest_current_mode[type_on] = 1;
  }
}
#define ENERGEST_SWITCH(type_off, type_on) energest_switch(type_off, type_on)

#else /* ENERGEST_CONF_ON */

static inline void energest_init(void) { }

static inline void energest_flush(void) { }

static inline uint64_t energest_type_time(energest_type_t type) { return 0; }

static inline void energest_type_set(energest_type_t type, uint64_t time) { }

static inline void energest_on(energest_type_t type) { }

static inline void energest_off(energest_type_t type) { }

static inline void energest_switch(energest_type_t type_off,
                                   energest_type_t type_on)
{
}

#define ENERGEST_ON(type) do { } while(0)
#define ENERGEST_OFF(type) do { } while(0)
#define ENERGEST_SWITCH(type_off, type_on) do { } while(0)

#endif /* ENERGEST_CONF_ON */

#endif /* ENERGEST_H_ */
