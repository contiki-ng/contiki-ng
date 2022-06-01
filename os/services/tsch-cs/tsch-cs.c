/*
 * Copyright (c) 2016-2018, University of Bristol - http://www.bristol.ac.uk
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
 */

/**
 * \file
 *         Source file for TSCH adaptive channel selection
 * \author
 *         Atis Elsts <atis.elsts@bristol.ac.uk>
 */

#include "tsch.h"
#include "tsch-stats.h"
#include "tsch-cs.h"

#if ! TSCH_STATS_ON
#error tsch-cs requires tsch-stats. Please enable TSCH_STATS_CONF_ON.
#endif /* ! TSCH_STATS_ON */

#if ! TSCH_STATS_SAMPLE_NOISE_RSSI
#error tsch-cs requires periodic RSSI sampling. Please enable TSCH_STATS_CONF_SAMPLE_NOISE_RSSI.
#endif /* ! TSCH_STATS_SAMPLE_NOISE_RSSI */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TSCH CS"
#define LOG_LEVEL LOG_LEVEL_MAC

/*---------------------------------------------------------------------------*/

/* Allow to change only 1 channel at once */
#define TSCH_CS_MAX_CHANNELS_CHANGED 1

/* Do not up change channels more frequently than this */
#define TSCH_CS_MIN_UPDATE_INTERVAL_SEC 60

/* Do not change channels if the difference in qualities is below this */
#define TSCH_CS_HYSTERESIS (TSCH_STATS_BINARY_SCALING_FACTOR / 10)

/* After removing a channel from the sequence, do not add it back at least this time */
#define TSCH_CS_BLACKLIST_DURATION_SEC (5 * 60)

/* A potential for change detected? */
static bool recaculation_requested;

/* Time (in seconds) when channels were marked as busy; 0 if they are not busy */
static uint32_t tsch_cs_busy_since[TSCH_STATS_NUM_CHANNELS];

/*
 * The following variables are kept in order to avoid completely migrating away
 * from the initial hopping sequence (as then new nodes would not be able to join).
 * The invariant is: tsch_cs_initial_bitmap & tsch_cs_current_bitmap != 0
 */
/* The bitmap with the initial channels */
static tsch_cs_bitmap_t tsch_cs_initial_bitmap;
/* The bitmap with the current channels */
static tsch_cs_bitmap_t tsch_cs_current_bitmap;

/* structure for sorting */
struct tsch_cs_quality {
  /* channel number */
  uint8_t channel;
  /* the higher, the better */
  tsch_stat_t metric;
};
/*---------------------------------------------------------------------------*/
static inline bool
tsch_cs_bitmap_contains(tsch_cs_bitmap_t bitmap, uint8_t channel)
{
  return (1 << (channel - TSCH_STATS_FIRST_CHANNEL)) & bitmap;
}
/*---------------------------------------------------------------------------*/
static inline tsch_cs_bitmap_t
tsch_cs_bitmap_set(tsch_cs_bitmap_t bitmap, uint8_t channel)
{
  return (1 << (channel - TSCH_STATS_FIRST_CHANNEL)) | bitmap;
}
/*---------------------------------------------------------------------------*/
static tsch_cs_bitmap_t
tsch_cs_bitmap_calc(void)
{
  tsch_cs_bitmap_t result = 0;
  int i;
  for(i = 0; i < tsch_hopping_sequence_length.val; ++i) {
    result = tsch_cs_bitmap_set(result, tsch_hopping_sequence[i]);
  }
  return result;
}
/*---------------------------------------------------------------------------*/
void
tsch_cs_adaptations_init(void)
{
  tsch_cs_initial_bitmap = tsch_cs_bitmap_calc();
  tsch_cs_current_bitmap = tsch_cs_initial_bitmap;
}
/*---------------------------------------------------------------------------*/
/* Sort the elements to that the channels with the best metrics are in the front */
static void
tsch_cs_bubble_sort(struct tsch_cs_quality *qualities)
{
  int i, j;
  struct tsch_cs_quality tmp;

  for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
    for(j = 0; j + 1 < TSCH_STATS_NUM_CHANNELS; ++j) {
      if(qualities[j].metric < qualities[j+1].metric){
        tmp = qualities[j];
        qualities[j] = qualities[j+1];
        qualities[j+1] = tmp;
      }
    }
  }
}
/*---------------------------------------------------------------------------*/
/* Select a single, currently unused, good enough channel. Returns 0xff on failure. */
static uint8_t
tsch_cs_select_replacement(uint8_t old_channel, tsch_stat_t old_ewma,
                      struct tsch_cs_quality *qualities, uint8_t is_in_sequence[])
{
  int i;
  uint32_t now = clock_seconds();
  tsch_cs_bitmap_t bitmap = tsch_cs_bitmap_set(0, old_channel);

  /* Don't want to replace a channel if the improvement is miniscule (< 10%) */
  old_ewma += TSCH_CS_HYSTERESIS;

  /* iterate up to -1 because we know that at least one of the channels is bad */
  for(i = 0; i < TSCH_STATS_NUM_CHANNELS - 1; ++i) {
    /* select a replacement candidate */
    uint8_t candidate = qualities[i].channel;

    if(qualities[i].metric < TSCH_CS_FREE_THRESHOLD) {
      /* This channel is not good enough.
       * since we know that the other channels in the sorted list are even worse,
       * it makes sense to return immediately rather than to continue t
       */
      LOG_DBG("ch %u: busy\n", candidate);
      return 0xff;
    }

    if(qualities[i].metric < old_ewma) {
      /* not good enough to replace */
      LOG_DBG("ch %u: hysteresis check failed\n", candidate);
      return 0xff;
    }

    /* already in the current TSCH hopping sequence? */
    if(is_in_sequence[candidate - TSCH_STATS_FIRST_CHANNEL] != 0xff) {
      LOG_DBG("ch %u: in seq\n", candidate);
      continue;
    }

    /* ignore this candidate if too recently blacklisted */
    if(tsch_cs_busy_since[candidate - TSCH_STATS_FIRST_CHANNEL] != 0
        && tsch_cs_busy_since[candidate - TSCH_STATS_FIRST_CHANNEL] + TSCH_CS_BLACKLIST_DURATION_SEC > now) {
      LOG_DBG("ch %u: recent bl\n", candidate);
      continue;
    }

    /* check if removing the old channel would break our hopping sequence invariant */
    if(bitmap == (tsch_cs_initial_bitmap & tsch_cs_current_bitmap)) {
      /* the channel is the only one that belongs to both */
      if(!tsch_cs_bitmap_contains(tsch_cs_initial_bitmap, candidate)) {
        /* the candidate is not in the initial sequence; not acceptable */
        continue;
      }
    }

    return candidate;
  }

  return 0xff;
}
/*---------------------------------------------------------------------------*/
bool
tsch_cs_process(void)
{
  int i;
  bool try_replace;
  bool has_replaced;
  struct tsch_cs_quality qualities[TSCH_STATS_NUM_CHANNELS];
  uint8_t is_channel_busy[TSCH_STATS_NUM_CHANNELS];
  uint8_t is_in_sequence[TSCH_STATS_NUM_CHANNELS];
  static uint32_t last_time_changed;

  if(!recaculation_requested) {
    /* nothing to do */
    return false;
  }

  if(last_time_changed != 0 && last_time_changed + TSCH_CS_MIN_UPDATE_INTERVAL_SEC > clock_seconds()) {
    /* too soon */
    return false;
  }

  /* reset the flag */
  recaculation_requested = false;

  for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
    qualities[i].channel = i + TSCH_STATS_FIRST_CHANNEL;
    qualities[i].metric = tsch_stats.channel_free_ewma[i];
  }

  /* bubble sort the channels */
  tsch_cs_bubble_sort(qualities);

  /* start with the threshold values */
  for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
    is_channel_busy[i] = (tsch_stats.channel_free_ewma[i] < TSCH_CS_FREE_THRESHOLD);
  }
  memset(is_in_sequence, 0xff, sizeof(is_in_sequence));
  for(i = 0; i < tsch_hopping_sequence_length.val; ++i) {
    uint8_t channel = tsch_hopping_sequence[i];
    is_in_sequence[channel - TSCH_STATS_FIRST_CHANNEL] = i;
  }

  /* mark the first N channels as "good" - there is nothing better to select */
  for(i = 0; i < tsch_hopping_sequence_length.val; ++i) {
     is_channel_busy[qualities[i].channel - TSCH_STATS_FIRST_CHANNEL] = 0;
  }

  for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
    uint8_t ci = qualities[i].channel - TSCH_STATS_FIRST_CHANNEL;
    (void)ci;
    LOG_DBG("ch %u q %u busy %u in seq %u\n",
        qualities[i].channel,
        qualities[i].metric,
        is_channel_busy[ci],
        is_in_sequence[ci] == 0xff ? 0 : 1);
  }

  try_replace = false;
  for(i = 0; i < tsch_hopping_sequence_length.val; ++i) {
    uint8_t channel = tsch_hopping_sequence[i];
    if(is_channel_busy[channel - TSCH_STATS_FIRST_CHANNEL]) {
      try_replace = true;
    }
  }
  if(!try_replace) {
    LOG_DBG("cs: not replacing\n");
    return false;
  }

  has_replaced = false;
  for(i = TSCH_STATS_NUM_CHANNELS - 1; i >= tsch_hopping_sequence_length.val; --i) {
    if(is_in_sequence[qualities[i].channel - TSCH_STATS_FIRST_CHANNEL] != 0xff) {
      /* found the worst channel; it must be busy */
      uint8_t channel = qualities[i].channel;
      tsch_stat_t ewma_metric = qualities[i].metric;
      uint8_t replacement = tsch_cs_select_replacement(channel, ewma_metric,
                                                  qualities, is_in_sequence);
      uint8_t position = is_in_sequence[channel - TSCH_STATS_FIRST_CHANNEL];

      if(replacement != 0xff) {
        printf("\ncs: replacing channel %u %u (%u) with %u\n",
               channel, tsch_hopping_sequence[position], position, replacement);
        /* mark the old channel as busy */
        tsch_cs_busy_since[channel - TSCH_STATS_FIRST_CHANNEL] = clock_seconds();
        /* do the actual replacement in the global TSCH HS variable */
        tsch_hopping_sequence[position] = replacement;
        has_replaced = true;
        /* recalculate the hopping sequence bitmap */
        tsch_cs_current_bitmap = tsch_cs_bitmap_calc();
      }
      break; /* replace just one at once */
    }
  }

  if(has_replaced) {
    last_time_changed = clock_seconds();
    return true;
  }

  LOG_DBG("cs: no changes\n");
  return false;
}
/*---------------------------------------------------------------------------*/
void
tsch_cs_channel_stats_updated(uint8_t updated_channel, uint16_t old_busyness_metric)
{
  uint8_t index;
  bool old_is_busy;
  bool new_is_busy;

  /* Enable this only on the coordinator node */
  if(!tsch_is_coordinator) {
    return;
  }

  /* Do not try to adapt before enough information has been learned */
  if(clock_seconds() < TSCH_CS_LEARNING_PERIOD_SEC) {
    return;
  }

  index = tsch_stats_channel_to_index(updated_channel);

  old_is_busy = (old_busyness_metric < TSCH_CS_FREE_THRESHOLD);
  new_is_busy = (tsch_stats.channel_free_ewma[index] < TSCH_CS_FREE_THRESHOLD);

  if(old_is_busy != new_is_busy) {
    /* the status of the channel has changed*/
    recaculation_requested = true;

  } else if(new_is_busy) {
    /* run the reselection algorithm iff the channel is both (1) bad and (2) in use */
    if(tsch_cs_bitmap_contains(tsch_cs_current_bitmap, updated_channel)) {
      /* the channel is in use and is busy */
      recaculation_requested = true;
    }
  }
}
