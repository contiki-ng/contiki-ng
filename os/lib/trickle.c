/*
 * Copyright (c) 2023, Uppsala universitet.
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
 * \file
 *         Implements the Trickle algorithm as per RFC 6206.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "lib/trickle.h"
#include "lib/assert.h"
#include "sys/cc.h"
#include "sys/clock.h"

#include "sys/log.h"
#define LOG_MODULE "Trickle"
#define LOG_LEVEL LOG_LEVEL_NONE

enum event {
  START,
  TIMER,
  RESET,
};

static char on_event(struct trickle *trickle, enum event event);

/*---------------------------------------------------------------------------*/
static void
on_timeout(void *ptr)
{
  on_event((struct trickle *)ptr, TIMER);
}
/*---------------------------------------------------------------------------*/
static char
on_event(struct trickle *trickle, enum event event)
{
  PT_BEGIN(&trickle->protothread);

  while(1) {
    /* deviating from the RFC, we start with I = I_min to speed things up */
    trickle->interval_size = trickle->imin;
    while(1) {
      if(trickle->on_new_interval) {
        trickle->on_new_interval();
      }
      trickle->counter = 0;

      /* wait until t */
      ctimer_set(&trickle->timer,
                 (trickle->interval_size / 2)
                 + clock_random((trickle->interval_size / 2) - 1),
                 on_timeout,
                 trickle);
      LOG_INFO("I=%" CLOCK_PRI "s t=%" CLOCK_PRI "s\n",
               trickle->interval_size / CLOCK_SECOND,
               trickle->timer.etimer.timer.interval / CLOCK_SECOND);
      PT_YIELD(&trickle->protothread);
      if(event == RESET) {
        break;
      }

      /* suppress? */
      if(trickle->counter >= trickle->redundancy_constant) {
        LOG_INFO("Suppressed\n");
      } else {
        LOG_INFO("Broadcasting\n");
        trickle->on_broadcast();
      }

      /* wait until the interval ends */
      ctimer_set(&trickle->timer,
                 trickle->interval_size - trickle->timer.etimer.timer.interval,
                 on_timeout,
                 trickle);
      PT_YIELD(&trickle->protothread);
      if(event == RESET) {
        break;
      }

      /* new interval */
      trickle->interval_size = MIN(2 * trickle->interval_size,
                                   trickle->imin << trickle->max_doublings);
    }
    LOG_INFO("Resetting Trickle\n");
  }

  PT_END(&trickle->protothread);
}
/*---------------------------------------------------------------------------*/
void
trickle_start(struct trickle *trickle,
              clock_time_t imin,
              uint8_t max_doublings,
              uint8_t redundancy_constant,
              trickle_callback_t on_broadcast,
              trickle_callback_t on_new_interval)
{
  assert(trickle);
  assert(on_broadcast);
  trickle->imin = imin;
  trickle->max_doublings = max_doublings;
  trickle->redundancy_constant = redundancy_constant;
  trickle->on_broadcast = on_broadcast;
  trickle->on_new_interval = on_new_interval;
  on_event(trickle, START);
}
/*---------------------------------------------------------------------------*/
void
trickle_increment_counter(struct trickle *trickle)
{
  trickle->counter++;
}
/*---------------------------------------------------------------------------*/
void
trickle_reset(struct trickle *trickle)
{
  if(trickle->interval_size == trickle->imin) {
    LOG_INFO("Not resetting Trickle since I = I_min\n");
    return;
  }
  on_event(trickle, RESET);
}
/*---------------------------------------------------------------------------*/
void
trickle_stop(struct trickle *trickle)
{
  ctimer_stop(&trickle->timer);
}
/*---------------------------------------------------------------------------*/
