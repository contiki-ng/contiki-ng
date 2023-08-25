/*
 * Copyright (c) 2004, 2008, Swedish Institute of Computer Science.
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
 * Author: Adam Dunkels <adam@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 *
 */

/**
 * \file
 * Timer of seconds library implementation.
 * \author
 * Adam Dunkels <adam@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup stimer
 * @{
 */

#include "contiki.h"
#include "sys/clock.h"
#include "sys/stimer.h"

#define SCLOCK_GEQ(a, b)	((unsigned long)((a) - (b)) < \
				((unsigned long)(~((unsigned long)0)) >> 1))

/*---------------------------------------------------------------------------*/
/**
 * Reset the timer with the same interval.
 *
 * This function resets the timer with the same interval that was
 * given to the stimer_set() function. The start point of the interval
 * is the exact time that the timer last expired. Therefore, this
 * function will cause the timer to be stable over time, unlike the
 * stimer_restart() function. If this is executed before the
 * timer expired, this function has no effect.
 *
 * \param t A pointer to the timer.
 *
 * \sa stimer_restart()
 */
void
stimer_reset(struct stimer *t)
{
  if(stimer_expired(t)) {
    t->start += t->interval;
  }
}
/*---------------------------------------------------------------------------*/
/**
 * Restart the timer from the current point in time
 *
 * This function restarts a timer with the same interval that was
 * given to the stimer_set() function. The timer will start at the
 * current time.
 *
 * \note A periodic timer will drift if this function is used to reset
 * it. For periodic timers, use the stimer_reset() function instead.
 *
 * \param t A pointer to the timer.
 *
 * \sa stimer_reset()
 */
void
stimer_restart(struct stimer *t)
{
  t->start = clock_seconds();
}
/*---------------------------------------------------------------------------*/
/**
 * Check if a timer has expired.
 *
 * This function tests if a timer has expired and returns true or
 * false depending on its status.
 *
 * \param t A pointer to the timer
 *
 * \return True if the timer has expired.
 *
 */
bool
stimer_expired(struct stimer *t)
{
  return SCLOCK_GEQ(clock_seconds(), t->start + t->interval);
}
/*---------------------------------------------------------------------------*/
/**
 * The time elapsed since the timer started
 *
 * This function returns the time elapsed.
 *
 * \param t A pointer to the timer
 *
 * \return The time elapsed since the last start of the timer
 *
 */
unsigned long
stimer_elapsed(struct stimer *t)
{
  return clock_seconds() - t->start;
}

/*---------------------------------------------------------------------------*/

/** @} */
