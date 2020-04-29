/*
 * Copyright (c) 2020, Institute of Electronics and Computer Science (EDI)
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
 * \addtogroup cc13xx-cc26xx-rat
 * @{
 *
 * \file
 *        Implementation of the CC13xx/CC26xx RAT (Radio Timer) upkeep.
 * \author
 *        Atis Elsts <atis.elsts@edi.lv>
 */
/*---------------------------------------------------------------------------*/
#include "rf/rat.h"
#include "rf/radio-mode.h"
/*---------------------------------------------------------------------------*/
#include <ti/drivers/rf/RF.h>
/*---------------------------------------------------------------------------*/
/* RAT has 32-bit register, overflows once ~18 minutes */
#define RAT_RANGE             4294967296ull
#define RAT_ONE_QUARTER       (RAT_RANGE / (uint32_t)4)
#define RAT_ONE_THIRD         (RAT_RANGE / (uint32_t)3)
#define RAT_THREE_QUARTERS    (RAT_ONE_QUARTER * 3)
#define RTIMER_TO_RAT(x)      ((x) * (RAT_SECOND / RTIMER_SECOND))
/*---------------------------------------------------------------------------*/
static void
check_rat_overflow(void)
{
  const bool was_off = !radio_mode->rx_is_active();

  if(was_off) {
    RF_runDirectCmd(radio_mode->rf_handle, CMD_NOP);
  }

  const uint32_t current_value = RF_getCurrentTime();

  static bool initial_iteration = true;
  static uint32_t last_value;

  if(initial_iteration) {
    /* First time checking overflow will only store the current value */
    initial_iteration = false;
  } else {
    /* Overflow happens when the previous reading is bigger than the current one */
    if(current_value < last_value) {
      /* Overflow detected */
      radio_mode->rat.last_overflow = RTIMER_NOW();
      radio_mode->rat.overflow_count += 1;
    }
  }

  last_value = current_value;

  if(was_off) {
    RF_yield(radio_mode->rf_handle);
  }
}
/*---------------------------------------------------------------------------*/
static void
rat_overflow_cb(void *arg)
{
  check_rat_overflow();
  /* Check next time after half of the RAT interval */
  const clock_time_t two_quarters = (2 * RAT_ONE_QUARTER * CLOCK_SECOND) / RAT_SECOND;
  ctimer_set(&radio_mode->rat.overflow_timer, two_quarters, rat_overflow_cb, NULL);
}
/*---------------------------------------------------------------------------*/
void
rat_init(void)
{
  check_rat_overflow();
  const clock_time_t one_third = (RAT_ONE_THIRD * CLOCK_SECOND) / RAT_SECOND;
  ctimer_set(&radio_mode->rat.overflow_timer, one_third, rat_overflow_cb, NULL);
}
/*---------------------------------------------------------------------------*/
uint32_t
rat_to_timestamp(const uint32_t rat_ticks, int32_t offset_ticks)
{
  check_rat_overflow();

  uint64_t adjusted_overflow_count = radio_mode->rat.overflow_count;

  /* If the timestamp is in the 4th quarter and the last overflow was recently,
   * assume that the timestamp refers to the time before the overflow */
  if(rat_ticks > RAT_THREE_QUARTERS) {
    const rtimer_clock_t one_quarter = (RAT_ONE_QUARTER * RTIMER_SECOND) / RAT_SECOND;
    if(RTIMER_CLOCK_LT(RTIMER_NOW(), radio_mode->rat.last_overflow + one_quarter)) {
      adjusted_overflow_count -= 1;
    }
  }

  /* Add the overflowed time to the timestamp */
  const uint64_t rat_ticks_adjusted = (uint64_t)rat_ticks + (uint64_t)RAT_RANGE * adjusted_overflow_count;

  return RAT_TO_RTIMER(rat_ticks_adjusted + offset_ticks);
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
