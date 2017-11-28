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
 *         Header file for TSCH adaptive channel selection
 * \author
 *         Atis Elsts <atis.elsts@bristol.ac.uk>
 */

#ifndef __TSCH_CS_H__
#define __TSCH_CS_H__

#include "contiki.h"
#include <stdbool.h>

/* If `channel_free_ewma` value is less than this, the channel is considered busy */
#ifdef TSCH_CS_CONF_FREE_THRESHOLD
#define TSCH_CS_FREE_THRESHOLD TSCH_CS_CONF_FREE_THRESHOLD
#else
/* < 85% free */
#define TSCH_CS_FREE_THRESHOLD ((tsch_stat_t)(85ul * TSCH_STATS_BINARY_SCALING_FACTOR / 100))
#endif

#define TSCH_CS_LEARNING_PERIOD_SEC 30

/**
 * \brief Initializes the TSCH hopping sequence selection module.
 */
void tsch_cs_adaptations_init(void);
    
/**
 * \brief Signal the need to potentially update the TSCH hopping sequence.
 * \param updated_channel     The channel with the updated RSSI measurement
 * \param old_busyness_metric The EWMA value of the "channel busy" status before the last RSSI measurement
 */
void tsch_cs_channel_stats_updated(uint8_t updated_channel, uint16_t old_busyness_metric);

/**
 * \brief Potentially update the TSCH hopping sequence
 * \return true if the hopping sequence was updated, false otherwise
 */
bool tsch_cs_process(void);


/* A bit corresponds to a channel; `uint16_t` value is OK for up to 16 channels. */
typedef uint16_t tsch_cs_bitmap_t;


#endif /* __TSCH_CS_H__ */
