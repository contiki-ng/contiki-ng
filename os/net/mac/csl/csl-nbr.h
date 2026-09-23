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
 * \addtogroup csl
 * @{
 *
 * \file
 *         Stores CSL-specific metadata of L2-neighbors.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CSL_NBR_H_
#define CSL_NBR_H_

#include "lib/ufix.h"
#include "net/mac/csl/csl-channel-selector.h"
#include "net/mac/csl/csl-synchronizer-compliant.h"
#include "net/mac/csl/csl-synchronizer-splo.h"
#include "net/mac/csl/csl.h"
#include "services/akes/akes-nbr.h"
#include <stdbool.h>

typedef struct {
  uint8_t arm : 4;
  bool reward : 1;
} csl_nbr_sw_ucb_window_entry_t;

typedef struct {
#if !CSL_COMPLIANT
  uint8_t q[AKES_NBR_CHALLENGE_LEN];
  rtimer_clock_t helloack_sfd_timestamp;
  wake_up_counter_t predicted_wake_up_counter;
#endif /* !CSL_COMPLIANT */
} csl_nbr_tentative_t;

typedef struct csl_nbr {
#if CSL_COMPLIANT
  struct csl_synchronizer_compliant_data sync_data;
#else /* CSL_COMPLIANT */
  struct csl_synchronizer_splo_data sync_data;
  int32_t drift;
  struct csl_synchronizer_splo_data historical_sync_data;
#if CSL_CHANNEL_SELECTOR_WITH_D_UCB
  ufix22_t discounted_pulls[CSL_CHANNELS_COUNT];
  ufix22_t discounted_rewards[CSL_CHANNELS_COUNT];
#elif CSL_CHANNEL_SELECTOR_WITH_SW_UCB
  uint32_t time_step;
  csl_nbr_sw_ucb_window_entry_t
      window[CSL_CHANNEL_SELECTOR_SW_UCB_WINDOW_SIZE];
#endif /* CSL_CHANNEL_SELECTOR_WITH_SW_UCB */
#endif /* CSL_COMPLIANT */
} csl_nbr_t;

csl_nbr_tentative_t *csl_nbr_get_tentative(
    const akes_nbr_tentative_t *tentative);
csl_nbr_t *csl_nbr_get(const akes_nbr_t *nbr);
csl_nbr_t *csl_nbr_get_receiver(void);
uint8_t csl_nbr_get_index_of(csl_nbr_t *nbr);

#endif /* CSL_NBR_H_ */

/** @} */
