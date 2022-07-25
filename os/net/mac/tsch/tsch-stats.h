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
 *         Header file for TSCH statistics
 * \author
 *         Atis Elsts <atis.elsts@bristol.ac.uk>
 */

/**
 * \addtogroup tsch
 * @{
*/

#ifndef TSCH_STATS_H_
#define TSCH_STATS_H_

/********** Includes **********/

#include "contiki.h"
#include "net/linkaddr.h"
#include "net/mac/tsch/tsch-conf.h"
#include "net/mac/tsch/tsch-queue.h"

/************ Constants ***********/

/* Enable the collection of TSCH statistics? */
#ifdef TSCH_STATS_CONF_ON
#define TSCH_STATS_ON TSCH_STATS_CONF_ON
#else
#define TSCH_STATS_ON 0
#endif

/* Enable the collection background noise RSSI? */
#ifdef TSCH_STATS_CONF_SAMPLE_NOISE_RSSI
#define TSCH_STATS_SAMPLE_NOISE_RSSI TSCH_STATS_CONF_SAMPLE_NOISE_RSSI
#else
#define TSCH_STATS_SAMPLE_NOISE_RSSI 0
#endif

/*
 * How to update a TSCH statistic.
 * Uses a hardcoded EWMA alpha value equal to 0.125 by default.
 */
#ifdef TSCH_STATS_CONF_EWMA_UPDATE
#define TSCH_STATS_EWMA_UPDATE TSCH_STATS_CONF_EWMA_UPDATE
#else
#define TSCH_STATS_EWMA_UPDATE(x, v) (x) = (((x) * 7 / 8) + (v) / 8)
#endif

/* 
 * A channel is considered busy if at the sampling instant
 * it has RSSI higher or equal to this limit.
 */
#ifdef TSCH_STATS_CONF_BUSY_CHANNEL_RSSI
#define TSCH_STATS_BUSY_CHANNEL_RSSI TSCH_STATS_CONF_BUSY_CHANNEL_RSSI
#else 
#define TSCH_STATS_BUSY_CHANNEL_RSSI -85
#endif

/* The period after which stat values are decayed towards the default values */
#ifdef TSCH_STATS_CONF_DECAY_INTERVAL
#define TSCH_STATS_DECAY_INTERVAL TSCH_STATS_CONF_DECAY_INTERVAL
#else
#define TSCH_STATS_DECAY_INTERVAL (20ul * 60 * CLOCK_SECOND)
#endif

/*
 * The total number of MAC-layer channels.
 * Sixteen for the IEEE802.15.4 2.4 GHz band.
 */
#ifdef TSCH_STATS_CONF_NUM_CHANNELS
#define TSCH_STATS_NUM_CHANNELS TSCH_STATS_CONF_NUM_CHANNELS
#else
#define TSCH_STATS_NUM_CHANNELS 16
#endif

/* The number of the first MAC-layer channel. */
#ifdef TSCH_STATS_CONF_FIRST_CHANNEL
#define TSCH_STATS_FIRST_CHANNEL TSCH_STATS_CONF_FIRST_CHANNEL
#else
#define TSCH_STATS_FIRST_CHANNEL 11
#endif

/* Internal: the scaling of the various stats */
#define TSCH_STATS_RSSI_SCALING_FACTOR    -16
#define TSCH_STATS_LQI_SCALING_FACTOR      16
#define TSCH_STATS_BINARY_SCALING_FACTOR   4096

/*
 * Transform a statistic from external form to the internal representation.
 * To transform back, simply divide by the factor.
 */
#define TSCH_STATS_TRANSFORM(x, factor) ((int16_t)(x) * factor)

/* The default value for RSSI statistics: -90 dBm */
#define TSCH_STATS_DEFAULT_RSSI TSCH_STATS_TRANSFORM(-90, TSCH_STATS_RSSI_SCALING_FACTOR)
/* The default value for LQI statistics: 100 */
#define TSCH_STATS_DEFAULT_LQI  TSCH_STATS_TRANSFORM(100, TSCH_STATS_LQI_SCALING_FACTOR)
/* The default value for P_tx (packet transmission probability) statistics: 50% */
#define TSCH_STATS_DEFAULT_P_TX (TSCH_STATS_BINARY_SCALING_FACTOR / 2)
/* The default value for channel free status: 100% */
#define TSCH_STATS_DEFAULT_CHANNEL_FREE TSCH_STATS_BINARY_SCALING_FACTOR

/* #define these callbacks to do the adaptive channel selection based on RSSI */
/* TSCH_CALLBACK_CHANNEL_STATS_UPDATED(channel, previous_metric); */
/* TSCH_CALLBACK_SELECT_CHANNELS(); */


/************ Types ***********/

typedef uint16_t tsch_stat_t;

struct tsch_global_stats {
  /* the maximum synchronization error */
  uint32_t max_sync_error;
  /* number of disassociations */
  uint16_t num_disassociations;
#if TSCH_STATS_SAMPLE_NOISE_RSSI
  /* per-channel noise estimates */
  tsch_stat_t noise_rssi[TSCH_STATS_NUM_CHANNELS];
  /* derived from `noise_rssi` and BUSY_CHANNEL_RSSI */
  tsch_stat_t channel_free_ewma[TSCH_STATS_NUM_CHANNELS];
#endif /* TSCH_STATS_SAMPLE_NOISE_RSSI */
};

struct tsch_channel_stats {
  /* EWMA, from receptions */
  tsch_stat_t rssi;
  /* EWMA, from receptions */
  tsch_stat_t lqi;
  /* EWMA of probability, for unicast transmissions only */
  tsch_stat_t p_tx_success;
};

struct tsch_neighbor_stats {
  struct tsch_channel_stats channel_stats[TSCH_STATS_NUM_CHANNELS];
};

struct tsch_neighbor; /* Forward declaration */


/************ External variables ***********/

#if TSCH_STATS_ON

/* Statistics for the local node */
extern struct tsch_global_stats tsch_stats;

/* For the timesource neighbor */
extern struct tsch_neighbor_stats tsch_neighbor_stats;


/************ Functions ***********/

void tsch_stats_init(void);

void tsch_stats_tx_packet(struct tsch_neighbor *, uint8_t mac_status, uint8_t channel);

void tsch_stats_rx_packet(struct tsch_neighbor *, int8_t rssi, uint8_t lqi, uint8_t channel);

void tsch_stats_on_time_synchronization(int32_t sync_error);

void tsch_stats_sample_rssi(void);

struct tsch_neighbor_stats *tsch_stats_get_from_neighbor(struct tsch_neighbor *);

void tsch_stats_reset_neighbor_stats(void);

#else /* TSCH_STATS_ON */

#define tsch_stats_init()
#define tsch_stats_tx_packet(n, mac_status, channel)
#define tsch_stats_rx_packet(n, rssi, lqi, channel)
#define tsch_stats_on_time_synchronization(sync_error)
#define tsch_stats_sample_rssi()
#define tsch_stats_get_from_neighbor(neighbor) NULL
#define tsch_stats_reset_neighbor_stats()

#endif /* TSCH_STATS_ON */

static inline uint8_t
tsch_stats_channel_to_index(uint8_t channel)
{
  return channel - TSCH_STATS_FIRST_CHANNEL;
}

static inline uint8_t
tsch_stats_index_to_channel(uint8_t channel_index)
{
  return channel_index + TSCH_STATS_FIRST_CHANNEL;
}


#endif /* TSCH_STATS_H_ */
/** @} */
