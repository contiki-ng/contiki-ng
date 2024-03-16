/*
 * Copyright (c) 2021, Uppsala universitet.
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
 *         D-UCB-based channel selection.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 *         Alex Kangas <alex.kangas.5644@student.uu.se>
 */

#include "lib/assert.h"
#include "net/mac/csl/csl-channel-selector.h"
#include "net/mac/csl/csl-nbr.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "CSL"
#define LOG_LEVEL LOG_LEVEL_MAC

struct result {
  uint8_t channel;
  ufix22_t ucb;
};

#if CSL_CHANNEL_SELECTOR_WITH_D_UCB
#define MAX_PROPOSED_CHANNELS (4)
#define DISCOUNT_FACTOR (UFIX22_ONE - (UFIX22_ONE >> 10))
#define EXPLORATION_TENDENCY_EXP (13)
#define LOG_2_E_INV_XI (UFIX22_LOG_2_E_INV >> EXPLORATION_TENDENCY_EXP)

/*---------------------------------------------------------------------------*/
static void
init(csl_nbr_t *csl_nbr)
{
  memset(csl_nbr->discounted_pulls, 0, sizeof(csl_nbr->discounted_pulls));
  memset(csl_nbr->discounted_rewards, 0, sizeof(csl_nbr->discounted_rewards));
}
/*---------------------------------------------------------------------------*/
static uint_fast16_t
propose_channels(csl_nbr_t *csl_nbr)
{
  uint_fast16_t proposed_channels = 0;
  ufix22_t intermediate = 0;
  for(uint_fast8_t i = 0; i < CSL_CHANNELS_COUNT; i++) {
    if(csl_nbr->discounted_pulls[i]) {
      intermediate += csl_nbr->discounted_pulls[i];
    } else {
      proposed_channels |= 1 << i;
    }
  }
  if(proposed_channels) {
    return proposed_channels;
  }
  intermediate = ufix22_multiply(LOG_2_E_INV_XI, ufix22_log2(intermediate));

  struct result max_results[MAX_PROPOSED_CHANNELS];
  for(uint_fast8_t i = 0; i < CSL_CHANNELS_COUNT; i++) {
    ufix22_t exploitation = ufix22_divide(csl_nbr->discounted_rewards[i],
                                          csl_nbr->discounted_pulls[i]);
    ufix22_t exploration =
        ufix22_sqrt(ufix22_divide(intermediate, csl_nbr->discounted_pulls[i]));
    ufix22_t ucb = exploitation + exploration;

    bool inserted = false;
    uint_fast8_t already_inserted_results = MIN(i, MAX_PROPOSED_CHANNELS);
    for(uint_fast8_t j = 0; j < already_inserted_results; j++) {
      if(ucb > max_results[j].ucb) {
        memmove(max_results + j + 1,
                max_results + j,
                MIN(already_inserted_results - j, MAX_PROPOSED_CHANNELS - j - 1)
                * sizeof(max_results[0]));
        max_results[j].channel = i;
        max_results[j].ucb = ucb;
        inserted = true;
        break;
      }
    }
    if(!inserted && (i < MAX_PROPOSED_CHANNELS)) {
      max_results[i].channel = i;
      max_results[i].ucb = ucb;
    }
  }

  for(uint_fast8_t j = 0; j < MAX_PROPOSED_CHANNELS; j++) {
    assert(!j || (max_results[j - 1].ucb >= max_results[j].ucb));
    proposed_channels |= 1 << max_results[j].channel;
  }
  LOG_DBG("D-UCB: proposed_channels = %u%u%u%u%u%u%u%u%u%u%u%u%u%u%u%u\n",
          proposed_channels & 1 << 15 ? 1 : 0,
          proposed_channels & 1 << 14 ? 1 : 0,
          proposed_channels & 1 << 13 ? 1 : 0,
          proposed_channels & 1 << 12 ? 1 : 0,
          proposed_channels & 1 << 11 ? 1 : 0,
          proposed_channels & 1 << 10 ? 1 : 0,
          proposed_channels & 1 << 9 ? 1 : 0,
          proposed_channels & 1 << 8 ? 1 : 0,
          proposed_channels & 1 << 7 ? 1 : 0,
          proposed_channels & 1 << 6 ? 1 : 0,
          proposed_channels & 1 << 5 ? 1 : 0,
          proposed_channels & 1 << 4 ? 1 : 0,
          proposed_channels & 1 << 3 ? 1 : 0,
          proposed_channels & 1 << 2 ? 1 : 0,
          proposed_channels & 1 << 1 ? 1 : 0,
          proposed_channels & 1 << 0 ? 1 : 0);
  return proposed_channels;
}
/*---------------------------------------------------------------------------*/
static void
take_feedback(csl_nbr_t *csl_nbr, bool successful, uint8_t channel)
{
  for(uint_fast8_t i = 0; i < CSL_CHANNELS_COUNT; i++) {
    csl_nbr->discounted_pulls[i] =
        ufix22_multiply(csl_nbr->discounted_pulls[i], DISCOUNT_FACTOR);
    csl_nbr->discounted_rewards[i] =
        ufix22_multiply(csl_nbr->discounted_rewards[i], DISCOUNT_FACTOR);
  }
  csl_nbr->discounted_pulls[channel] += UFIX22_ONE;
  if(successful) {
    csl_nbr->discounted_rewards[channel] += UFIX22_ONE;
  }
}
/*---------------------------------------------------------------------------*/
static bool
is_exploring(csl_nbr_t *csl_nbr)
{
  for(uint_fast8_t i = 0; i < CSL_CHANNELS_COUNT; i++) {
    if(!csl_nbr->discounted_pulls[i]) {
      return true;
    }
  }
  return false;
}
/*---------------------------------------------------------------------------*/
const struct csl_channel_selector csl_channel_selector_d_ucb = {
  init,
  propose_channels,
  take_feedback,
  is_exploring,
};
/*---------------------------------------------------------------------------*/
#endif /* CSL_CHANNEL_SELECTOR_WITH_D_UCB */

/** @} */
