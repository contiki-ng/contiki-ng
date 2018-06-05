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
 *	TSCH runtime operation within timeslots
*/

#ifndef __TSCH_SLOT_OPERATION_H__
#define __TSCH_SLOT_OPERATION_H__

/********** Includes **********/

#include "contiki.h"
#include "lib/ringbufindex.h"

/***** External Variables *****/

/* A ringbuf storing outgoing packets after they were dequeued.
 * Will be processed layer by tsch_tx_process_pending */
extern struct ringbufindex dequeued_ringbuf;
extern struct tsch_packet *dequeued_array[TSCH_DEQUEUED_ARRAY_SIZE];
/* A ringbuf storing incoming packets.
 * Will be processed layer by tsch_rx_process_pending */
extern struct ringbufindex input_ringbuf;
extern struct input_packet input_array[TSCH_MAX_INCOMING_PACKETS];
/* Last clock_time_t where synchronization happened */
extern clock_time_t last_sync_time;
/* Counts the length of the current burst */
extern int tsch_current_burst_count;

/********** Functions *********/

/**
 * Returns a 802.15.4 channel from an ASN and channel offset. Basically adds
 * The offset to the ASN and performs a hopping sequence lookup.
 *
 * \param asn A given ASN
 * \param channel_offset A given channel offset
 * \return The resulting channel
 */
uint8_t tsch_calculate_channel(struct tsch_asn_t *asn, uint8_t channel_offset);
/**
 * Checks if the TSCH lock is set. Accesses to global structures outside of
 * interrupts must be done through the lock, unless the sturcutre has
 * atomic read/write
 *
 * \return 1 if the lock is taken, 0 otherwise
 */
int tsch_is_locked(void);
/**
 * Takes the TSCH lock. When the lock is taken, slot operation will be skipped
 * until release.
 *
 * \return 1 if the lock was successfully taken, 0 otherwise
 */
int tsch_get_lock(void);
/**
 * Releases the TSCH lock.
 */
void tsch_release_lock(void);
/**
 * Set global time before starting slot operation, with a rtimer time and an ASN
 *
 * \param next_slot_start the time to the start of the next slot, in rtimer ticks
 * \param next_slot_asn the ASN of the next slot
 */
void tsch_slot_operation_sync(rtimer_clock_t next_slot_start,
    struct tsch_asn_t *next_slot_asn);
/**
 * Start actual slot operation
 */
void tsch_slot_operation_start(void);

#endif /* __TSCH_SLOT_OPERATION_H__ */
/** @} */
