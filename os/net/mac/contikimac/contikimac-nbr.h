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
 *         Stores ContikiMAC-specific metadata of L2-neighbors.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CONTIKIMAC_NBR_H_
#define CONTIKIMAC_NBR_H_

#include "net/mac/anti-replay.h"
#include "net/mac/contikimac/contikimac-framer-potr.h"
#include "net/mac/contikimac/contikimac.h"
#include "services/akes/akes-nbr.h"

typedef struct {
#if CONTIKIMAC_FRAMER_POTR_ENABLED
#if CONTIKIMAC_WITH_SECURE_PHASE_LOCK
  struct contikimac_phase phase;
  rtimer_clock_t t1;
#if CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED
  wake_up_counter_t predicted_wake_up_counter;
#endif /* CONTIKIMAC_FRAMER_POTR_ILOS_ENABLED */
  uint8_t q[CONTIKIMAC_Q_LEN];
  uint8_t strobe_index;
#endif /* CONTIKIMAC_WITH_SECURE_PHASE_LOCK */
#endif /* CONTIKIMAC_FRAMER_POTR_ENABLED */
} contikimac_nbr_tentative_t;

typedef struct {
#if CONTIKIMAC_WITH_PHASE_LOCK
  struct contikimac_phase phase;
#endif /* CONTIKIMAC_WITH_PHASE_LOCK */
#if ANTI_REPLAY_WITH_SUPPRESSION
  uint16_t broadcast_expiration_time;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
} contikimac_nbr_t;

contikimac_nbr_tentative_t *contikimac_nbr_get_tentative(
    const akes_nbr_tentative_t *tentative);
contikimac_nbr_t *contikimac_nbr_get(const akes_nbr_t *nbr);

#endif /* CONTIKIMAC_NBR_H_ */
