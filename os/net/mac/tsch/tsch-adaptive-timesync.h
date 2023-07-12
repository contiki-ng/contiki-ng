/*
 * Copyright (c) 2015, SICS Swedish ICT.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \addtogroup tsch
 * @{
 * \file
 *	TSCH adaptive time synchronization
*/

#ifndef TSCH_ADAPTIVE_TIMESYNC_H_
#define TSCH_ADAPTIVE_TIMESYNC_H_

/********** Includes **********/

#include "contiki.h"

/********** Functions *********/

/**
 * \brief Updates timesync information for a given neighbor
 * \param n The neighbor
 * \param time_delta_asn ASN time delta since last synchronization, i.e. number of slots elapsed
 * \param drift_correction The measured drift in ticks since last synchronization
 */
void tsch_timesync_update(struct tsch_neighbor *n, uint16_t time_delta_asn, int32_t drift_correction);

/**
 * \brief Computes time compensation for a given point in the future
 * \param delta_ticks The number of ticks in the future we want to calculate compensation for
 * \return The time compensation
 */
int32_t tsch_timesync_adaptive_compensate(rtimer_clock_t delta_ticks);

/**
 * \brief Gives the estimated clock drift w.r.t. the time source in PPM (parts per million)
 * \return The time drift in PPM
 */
long int tsch_adaptive_timesync_get_drift_ppm(void);

/**
 * \brief Reset the status of the module
 */
void tsch_adaptive_timesync_reset(void);


#endif /* TSCH_ADAPTIVE_TIMESYNC_H_ */
/** @} */
