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
 *         Interface to the channel selection strategy.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CSL_CHANNEL_SELECTOR_H_
#define CSL_CHANNEL_SELECTOR_H_

#include "contiki.h"
#include <stdbool.h>

#ifdef CSL_CHANNEL_SELECTOR_CONF_SW_UCB_WINDOW_SIZE
#define CSL_CHANNEL_SELECTOR_SW_UCB_WINDOW_SIZE \
  CSL_CHANNEL_SELECTOR_CONF_SW_UCB_WINDOW_SIZE
#else /* CSL_CHANNEL_SELECTOR_CONF_SW_UCB_WINDOW_SIZE */
#define CSL_CHANNEL_SELECTOR_SW_UCB_WINDOW_SIZE (100)
#endif /* CSL_CHANNEL_SELECTOR_CONF_SW_UCB_WINDOW_SIZE */

#if CSL_CHANNEL_SELECTOR_CONF_WITH_D_UCB
#define CSL_CHANNEL_SELECTOR_WITH_D_UCB CSL_CHANNEL_SELECTOR_CONF_WITH_D_UCB
#else /* CSL_CHANNEL_SELECTOR_CONF_WITH_D_UCB */
#define CSL_CHANNEL_SELECTOR_WITH_D_UCB (0)
#endif /* CSL_CHANNEL_SELECTOR_CONF_WITH_D_UCB */

#if CSL_CHANNEL_SELECTOR_WITH_D_UCB \
    || !defined(CSL_CHANNEL_SELECTOR_CONF_WITH_SW_UCB)
#define CSL_CHANNEL_SELECTOR_WITH_SW_UCB (0)
#else
#define CSL_CHANNEL_SELECTOR_WITH_SW_UCB CSL_CHANNEL_SELECTOR_CONF_WITH_SW_UCB
#endif

#ifdef CSL_CHANNEL_SELECTOR_CONF
#define CSL_CHANNEL_SELECTOR CSL_CHANNEL_SELECTOR_CONF
#elif CSL_CHANNEL_SELECTOR_WITH_D_UCB
#define CSL_CHANNEL_SELECTOR csl_channel_selector_d_ucb
#elif CSL_CHANNEL_SELECTOR_WITH_SW_UCB
#define CSL_CHANNEL_SELECTOR csl_channel_selector_sw_ucb
#else
#define CSL_CHANNEL_SELECTOR csl_channel_selector_null
#endif

struct csl_nbr;
typedef struct csl_nbr csl_nbr_t;

/** Strategy for channel selection */
struct csl_channel_selector {
  void (* init)(csl_nbr_t *csl_nbr);
  uint_fast16_t (* propose_channels)(csl_nbr_t *csl_nbr);
  void (* take_feedback)(csl_nbr_t *csl_nbr, bool successful, uint8_t channel);
  bool (* is_exploring)(csl_nbr_t *csl_nbr);
};

void csl_channel_selector_take_feedback(bool successful,
                                        uint_fast8_t burst_index);
bool csl_channel_selector_take_feedback_is_exploring(void);

extern const struct csl_channel_selector CSL_CHANNEL_SELECTOR;

#endif /* CSL_CHANNEL_SELECTOR_H_ */

/** @} */
