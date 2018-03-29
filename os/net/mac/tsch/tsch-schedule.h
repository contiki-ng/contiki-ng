/*
 * Copyright (c) 2014, SICS Swedish ICT.
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
*/

#ifndef __TSCH_SCHEDULE_H__
#define __TSCH_SCHEDULE_H__

/********** Includes **********/

#include "contiki.h"
#include "net/linkaddr.h"

/********** Functions *********/

/* Module initialization, call only once at startup. Returns 1 is success, 0 if failure. */
int tsch_schedule_init(void);
/* Create a 6TiSCH minimal schedule */
void tsch_schedule_create_minimal(void);
/* Prints out the current schedule (all slotframes and links) */
void tsch_schedule_print(void);

/* Adds and returns a slotframe (NULL if failure) */
struct tsch_slotframe *tsch_schedule_add_slotframe(uint16_t handle, uint16_t size);
/* Looks for a slotframe from a handle */
struct tsch_slotframe *tsch_schedule_get_slotframe_by_handle(uint16_t handle);
/* Removes a slotframe Return 1 if success, 0 if failure */
int tsch_schedule_remove_slotframe(struct tsch_slotframe *slotframe);
/* Removes all slotframes, resulting in an empty schedule */
int tsch_schedule_remove_all_slotframes(void);

/* Returns next slotframe */
struct tsch_slotframe *tsch_schedule_slotframes_next(struct tsch_slotframe *sf);
/* Adds a link to a slotframe, return a pointer to it (NULL if failure) */
struct tsch_link *tsch_schedule_add_link(struct tsch_slotframe *slotframe,
                                         uint8_t link_options, enum link_type link_type, const linkaddr_t *address,
                                         uint16_t timeslot, uint16_t channel_offset);
/* Looks for a link from a handle */
struct tsch_link *tsch_schedule_get_link_by_handle(uint16_t handle);
/* Looks within a slotframe for a link with a given timeslot */
struct tsch_link *tsch_schedule_get_link_by_timeslot(struct tsch_slotframe *slotframe, uint16_t timeslot);
/* Removes a link. Return 1 if success, 0 if failure */
int tsch_schedule_remove_link(struct tsch_slotframe *slotframe, struct tsch_link *l);
/* Removes a link from slotframe and timeslot. Return a 1 if success, 0 if failure */
int tsch_schedule_remove_link_by_timeslot(struct tsch_slotframe *slotframe, uint16_t timeslot);

/* Returns the next active link after a given ASN, and a backup link (for the same ASN, with Rx flag) */
struct tsch_link * tsch_schedule_get_next_active_link(struct tsch_asn_t *asn, uint16_t *time_offset,
    struct tsch_link **backup_link);
/* Access to slotframe list */
struct tsch_slotframe *tsch_schedule_slotframe_head(void);
struct tsch_slotframe *tsch_schedule_slotframe_next(struct tsch_slotframe *sf);

#endif /* __TSCH_SCHEDULE_H__ */
/** @} */
