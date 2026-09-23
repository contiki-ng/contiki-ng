/*
 * Copyright (c) 2026, Konrad-Felix Krentz
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
 *         Estimates the noise floor based on recent RSSIs securely.
 *         See https://doi.org/10.1109/JIOT.2020.3045462.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "contiki.h"
#include <stdint.h>
#include <string.h>

#ifdef NOISE_FLOOR_CONF_INITIAL_ESTIMATE
#define INITIAL_ESTIMATE NOISE_FLOOR_CONF_INITIAL_ESTIMATE
#else /* NOISE_FLOOR_CONF_INITIAL_ESTIMATE */
#define INITIAL_ESTIMATE (-90)
#endif /* NOISE_FLOOR_CONF_INITIAL_ESTIMATE */

#if MAC_CONF_WITH_CSL
#include "net/mac/csl/csl.h"
#define CHANNELS_COUNT (CSL_CHANNELS_COUNT)
#else /* MAC_CONF_WITH_CSL */
#define CHANNELS_COUNT (1)
#endif /* MAC_CONF_WITH_CSL */

#define HISTORY_LENGTH (5)

static int8_t history[CHANNELS_COUNT][HISTORY_LENGTH];
static uint8_t indices[CHANNELS_COUNT];

/*---------------------------------------------------------------------------*/
void
noise_floor_init(void)
{
  memset(history, (int8_t)INITIAL_ESTIMATE, sizeof(history));
}
/*---------------------------------------------------------------------------*/
void
noise_floor_add(uint_fast8_t channel_index, int_fast8_t rssi)
{
  history[channel_index][indices[channel_index]++] = rssi;
  indices[channel_index] %= HISTORY_LENGTH;
}
/*---------------------------------------------------------------------------*/
int8_t
noise_floor_estimate(uint_fast8_t channel_index)
{
  /* copy the channel-specific noise floor history */
  int8_t local[HISTORY_LENGTH];
  memcpy(local, history[channel_index], sizeof(local));

  /* compute median using partial selection sort */
  const uint_fast8_t median_index = HISTORY_LENGTH / 2;

  for(uint_fast8_t i = 0; i <= median_index; i++) {
    uint_fast8_t minimum_index = i;
    for(uint_fast8_t j = i + 1; j < HISTORY_LENGTH; j++) {
      if(local[j] < local[minimum_index]) {
        minimum_index = j;
      }
    }
    int8_t temp = local[i];
    local[i] = local[minimum_index];
    local[minimum_index] = temp;
  }

  return local[median_index];
}
/*---------------------------------------------------------------------------*/
