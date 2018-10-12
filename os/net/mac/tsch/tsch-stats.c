/*
 * Copyright (c) 2016-2017, University of Bristol - http://www.bristol.ac.uk
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
 *         Source file for TSCH statistics
 * \author
 *         Atis Elsts <atis.elsts@bristol.ac.uk>
 */

#include "contiki.h"
#include "net/mac/tsch/tsch.h"
#include "net/netstack.h"
#include "dev/radio.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TSCH Stats"
#define LOG_LEVEL LOG_LEVEL_MAC

/*---------------------------------------------------------------------------*/
#if TSCH_STATS_ON
/*---------------------------------------------------------------------------*/

struct tsch_global_stats tsch_stats;
struct tsch_neighbor_stats tsch_neighbor_stats;

/* Called every TSCH_STATS_DECAY_INTERVAL ticks */
static struct ctimer periodic_timer;

static void periodic(void *);

/*---------------------------------------------------------------------------*/
void
tsch_stats_init(void)
{    
#if TSCH_STATS_SAMPLE_NOISE_RSSI
  int i;

  for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
    tsch_stats.noise_rssi[i] = TSCH_STATS_DEFAULT_RSSI;
    tsch_stats.channel_free_ewma[i] = TSCH_STATS_DEFAULT_CHANNEL_FREE;
  }
#endif

  tsch_stats_reset_neighbor_stats();

  /* Start the periodic processing soonish */
  ctimer_set(&periodic_timer, TSCH_STATS_DECAY_INTERVAL / 10, periodic, NULL);
}
/*---------------------------------------------------------------------------*/
void
tsch_stats_reset_neighbor_stats(void)
{
  int i;
  struct tsch_channel_stats *ch_stats;

  ch_stats = tsch_neighbor_stats.channel_stats;
  for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
    ch_stats[i].rssi = TSCH_STATS_DEFAULT_RSSI;
    ch_stats[i].lqi = TSCH_STATS_DEFAULT_LQI;
    ch_stats[i].p_tx_success = TSCH_STATS_DEFAULT_P_TX;
  }
}
/*---------------------------------------------------------------------------*/
struct tsch_neighbor_stats *
tsch_stats_get_from_neighbor(struct tsch_neighbor *n)
{
  /* Due to RAM limitations, this module only collects neighbor stats about the time source */
  if(n != NULL && n->is_time_source) {
    return &tsch_neighbor_stats;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
tsch_stats_tx_packet(struct tsch_neighbor *n, uint8_t mac_status, uint8_t channel)
{
  struct tsch_neighbor_stats *stats;

  stats = tsch_stats_get_from_neighbor(n);
  if(stats != NULL) {
    uint8_t index = tsch_stats_channel_to_index(channel);
    uint16_t new_tx_value = (mac_status == MAC_TX_OK ? 1 : 0);
    new_tx_value *= TSCH_STATS_BINARY_SCALING_FACTOR;
    TSCH_STATS_EWMA_UPDATE(stats->channel_stats[index].p_tx_success, new_tx_value);
  }
}
/*---------------------------------------------------------------------------*/
void
tsch_stats_rx_packet(struct tsch_neighbor *n, int8_t rssi, uint8_t lqi, uint8_t channel)
{
  struct tsch_neighbor_stats *stats;

  stats = tsch_stats_get_from_neighbor(n);
  if(stats != NULL) {
    uint8_t index = tsch_stats_channel_to_index(channel);

    TSCH_STATS_EWMA_UPDATE(stats->channel_stats[index].rssi,
        TSCH_STATS_TRANSFORM(rssi, TSCH_STATS_RSSI_SCALING_FACTOR));
    TSCH_STATS_EWMA_UPDATE(stats->channel_stats[index].lqi,
        TSCH_STATS_TRANSFORM(lqi, TSCH_STATS_LQI_SCALING_FACTOR));
  }
}
/*---------------------------------------------------------------------------*/
void
tsch_stats_on_time_synchronization(int32_t sync_error)
{
  /* Update the maximal error so far if the absolute value of the new one is larger */
  tsch_stats.max_sync_error = MAX(tsch_stats.max_sync_error, ABS(sync_error));
}
/*---------------------------------------------------------------------------*/
void
tsch_stats_sample_rssi(void)
{
#if TSCH_STATS_SAMPLE_NOISE_RSSI
  uint8_t index;
  radio_value_t value;
  radio_result_t rv;

  static uint8_t measurement_channel = TSCH_STATS_FIRST_CHANNEL;

  index = tsch_stats_channel_to_index(measurement_channel);

  /* Select the measurement channel */
  NETSTACK_RADIO.set_value(RADIO_PARAM_CHANNEL, measurement_channel);

  /* Need to explicitly turn on for Coojamotes */
  NETSTACK_RADIO.on();

  /* Measure the background noise RSSI and act on it */
  rv = NETSTACK_RADIO.get_value(RADIO_PARAM_RSSI, &value);
  if(rv == RADIO_RESULT_OK) {
    tsch_stat_t prev_busyness_metric;
    uint16_t is_free;

    is_free = (((int)value <= TSCH_STATS_BUSY_CHANNEL_RSSI) ? 1 : 0) * TSCH_STATS_BINARY_SCALING_FACTOR;

    /* LOG_DBG("noise RSSI on %u: %d\n", measurement_channel, (int)value); */

    TSCH_STATS_EWMA_UPDATE(tsch_stats.noise_rssi[index],
        TSCH_STATS_TRANSFORM((int)value, TSCH_STATS_RSSI_SCALING_FACTOR));

    prev_busyness_metric = tsch_stats.channel_free_ewma[index];
    (void)prev_busyness_metric;
    TSCH_STATS_EWMA_UPDATE(tsch_stats.channel_free_ewma[index], is_free);

    /* potentially select a new TSCH hopping sequence */
#ifdef TSCH_CALLBACK_CHANNEL_STATS_UPDATED
    TSCH_CALLBACK_CHANNEL_STATS_UPDATED(measurement_channel, prev_busyness_metric);
#endif
  } else {
    LOG_ERR("! sampling RSSI failed: %d\n", (int)rv);
  }

  /* Increment the channel index for the next time */
  measurement_channel++;
  if(measurement_channel >= TSCH_STATS_FIRST_CHANNEL + TSCH_STATS_NUM_CHANNELS) {
    measurement_channel = TSCH_STATS_FIRST_CHANNEL;
  }
#endif /* TSCH_STATS_SAMPLE_NOISE_RSSI */
}
/*---------------------------------------------------------------------------*/
/* Periodic timer called every TSCH_STATS_DECAY_INTERVAL ticks */
static void
periodic(void *ptr)
{
  int i;
  struct tsch_neighbor *timesource;
  struct tsch_channel_stats *stats = tsch_neighbor_stats.channel_stats;

#if TSCH_STATS_SAMPLE_NOISE_RSSI
  LOG_DBG("Noise RSSI:\n");
  for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
    LOG_DBG("  channel %u: %d rssi, %u/%u free\n",
        TSCH_STATS_FIRST_CHANNEL + i,
        tsch_stats.noise_rssi[i] / TSCH_STATS_RSSI_SCALING_FACTOR,
        tsch_stats.channel_free_ewma[i],
        TSCH_STATS_BINARY_SCALING_FACTOR);
  }
#endif

  timesource = tsch_queue_get_time_source();
  if(timesource != NULL) {
    LOG_DBG("Time source neighbor:\n");

    for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
      LOG_DBG("  channel %u: %d rssi, %u lqi, %u/%u P(tx)\n",
          TSCH_STATS_FIRST_CHANNEL + i,
          stats[i].rssi / TSCH_STATS_RSSI_SCALING_FACTOR,
          stats[i].lqi / TSCH_STATS_LQI_SCALING_FACTOR,
          stats[i].p_tx_success,
          TSCH_STATS_BINARY_SCALING_FACTOR);
    }
  }

  /* Do not decay the periodic global stats, as they are updated independely of packet rate */
  for(i = 0; i < TSCH_STATS_NUM_CHANNELS; ++i) {
    /* decay Rx stats */
    TSCH_STATS_EWMA_UPDATE(stats[i].rssi, TSCH_STATS_DEFAULT_RSSI);
    TSCH_STATS_EWMA_UPDATE(stats[i].lqi, TSCH_STATS_DEFAULT_LQI);
    /* decay Tx stats */
    TSCH_STATS_EWMA_UPDATE(stats[i].p_tx_success, TSCH_STATS_DEFAULT_P_TX);
  }

  ctimer_set(&periodic_timer, TSCH_STATS_DECAY_INTERVAL, periodic, NULL);
}
/*---------------------------------------------------------------------------*/
#endif /* TSCH_STATS_ON */
/*---------------------------------------------------------------------------*/
