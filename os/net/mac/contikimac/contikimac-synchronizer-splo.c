/*
 * Copyright (c) 2018, Hasso-Plattner-Institut.
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
 *         ContikiMAC's original phase-lock optimization.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "lib/random.h"
#include "net/linkaddr.h"
#include "net/mac/contikimac/contikimac-nbr.h"
#include "net/mac/contikimac/contikimac-synchronizer.h"
#include "net/nbr-table.h"
#include "net/packetbuf.h"
#include "services/akes/akes-nbr.h"
#include "sys/rtimer.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "SPLO"
#define LOG_LEVEL LOG_LEVEL_MAC

#define PHASE_LOCK_GUARD_TIME_NEGATIVE \
  (2 + 2 + CONTIKIMAC_ACKNOWLEDGMENT_WINDOW)
#define PHASE_LOCK_GUARD_TIME_POSITIVE (2 + 2)
#define FREQUENCY_TOLERANCE (15) /* ppm */
#define MAX_TIME_FOR_FRAME_CREATION (US_TO_RTIMERTICKS(2000))

#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  LOG_INFO("t_a = %lu\n", (long unsigned)CONTIKIMAC_ACKNOWLEDGMENT_WINDOW);
  LOG_INFO("t_s = %lu\n", (long unsigned)PHASE_LOCK_GUARD_TIME_NEGATIVE);
}
/*---------------------------------------------------------------------------*/
static struct contikimac_phase *
obtain_phase_lock_data(void)
{
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry) {
    LOG_ERR("no entry found\n");
    return NULL;
  }

  akes_nbr_t *nbr;
  if(contikimac_state.strobe.is_helloack) {
    nbr = entry->tentative;
  } else {
    nbr = entry->permanent;
  }

  if(!nbr) {
    LOG_ERR("could not obtain phase-lock data\n");
    return NULL;
  }

  if(contikimac_state.strobe.is_helloack) {
    return &contikimac_nbr_get_tentative(nbr->meta)->phase;
  } else {
    return &contikimac_nbr_get(nbr)->phase;
  }
}
/*---------------------------------------------------------------------------*/
static int
schedule(void)
{
  if(contikimac_state.strobe.is_broadcast) {
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    contikimac_state.strobe.next_transmission =
        wake_up_counter_shift_to_future(contikimac_get_last_wake_up_time()
                                        - (WAKE_UP_COUNTER_INTERVAL / 2));
    if(!(contikimac_get_wake_up_counter(
             contikimac_state.strobe.next_transmission).u32 & 1)) {
      contikimac_state.strobe.next_transmission += WAKE_UP_COUNTER_INTERVAL;
    }
    while(rtimer_has_timed_out(contikimac_state.strobe.next_transmission
                               - MAX_TIME_FOR_FRAME_CREATION
                               - CONTIKIMAC_STROBE_GUARD_TIME
                               - RTIMER_GUARD_TIME
                               - 1)) {
      contikimac_state.strobe.next_transmission +=
          2 * WAKE_UP_COUNTER_INTERVAL;
    }
    contikimac_state.strobe.timeout =
        contikimac_state.strobe.next_transmission + WAKE_UP_COUNTER_INTERVAL;
#else /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
    contikimac_synchronizer_strobe_soon();
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
  } else {
    struct contikimac_phase *phase = obtain_phase_lock_data();
    if(!phase) {
      return MAC_TX_ERR_FATAL;
    }
    rtimer_clock_t ticks_since_last_sync = RTIMER_NOW() - phase->t;
    rtimer_clock_t positive_uncertainty;
    rtimer_clock_t negative_uncertainty = positive_uncertainty =
        ((ticks_since_last_sync * FREQUENCY_TOLERANCE) / (1000000)) + 1;
    negative_uncertainty += PHASE_LOCK_GUARD_TIME_NEGATIVE;
    positive_uncertainty += PHASE_LOCK_GUARD_TIME_POSITIVE;
    contikimac_state.strobe.next_transmission =
        wake_up_counter_shift_to_future(phase->t - negative_uncertainty);

#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
    while(rtimer_has_timed_out(contikimac_state.strobe.next_transmission
                               - MAX_TIME_FOR_FRAME_CREATION
                               - CONTIKIMAC_STROBE_GUARD_TIME
                               - RTIMER_GUARD_TIME
                               - 1)) {
      contikimac_state.strobe.next_transmission += WAKE_UP_COUNTER_INTERVAL;
    }
    contikimac_state.strobe.receivers_wake_up_counter.u32 =
        phase->his_wake_up_counter_at_t.u32
        + wake_up_counter_increments(contikimac_state.strobe.next_transmission
                                     - phase->t,
                                     NULL)
        + 1;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */

    contikimac_state.strobe.timeout = contikimac_state.strobe.next_transmission
                                      + negative_uncertainty
                                      + positive_uncertainty;
  }
  return MAC_TX_OK;
}
/*---------------------------------------------------------------------------*/
static void
on_unicast_transmitted(void)
{
  struct contikimac_phase *phase;
  if(contikimac_state.strobe.is_helloack
     || (contikimac_state.strobe.result != MAC_TX_OK)
     || !((phase = obtain_phase_lock_data()))) {
    return;
  }

  phase->t =
      contikimac_state.strobe.t1[0]
      - (contikimac_state.strobe.acknowledgment[1] << CONTIKIMAC_DELTA_SHIFT);
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  phase->his_wake_up_counter_at_t =
      contikimac_state.strobe.receivers_wake_up_counter;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
}
/*---------------------------------------------------------------------------*/
const struct contikimac_synchronizer contikimac_synchronizer_splo = {
  init,
  schedule,
  on_unicast_transmitted,
};
/*---------------------------------------------------------------------------*/
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
