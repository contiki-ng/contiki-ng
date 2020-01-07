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
 * \file
 *	TSCH scheduling engine
*/

#ifndef __TSCH_SCHEDULE_H__
#define __TSCH_SCHEDULE_H__

/********** Includes **********/

#include "contiki.h"
#include "net/linkaddr.h"

/********** Functions *********/

/**
 * \brief Module initialization, call only once at init
 * \return 1 if success, 0 if failure
 */
int tsch_schedule_init(void);
/**
 * \brief Create a 6tisch minimal schedule with length TSCH_SCHEDULE_DEFAULT_LENGTH
 */
void tsch_schedule_create_minimal(void);
/**
 * \brief Prints out the current schedule (all slotframes and links)
 */
void tsch_schedule_print(void);


/**
 * \brief Creates and adds a new slotframe
 * \param handle the slotframe handle
 * \param size the slotframe size
 * \return the new slotframe, NULL if failure
 */
struct tsch_slotframe *tsch_schedule_add_slotframe(uint16_t handle, uint16_t size);

/**
 * \brief Looks up a slotframe by handle
 * \param handle the slotframe handle
 * \return the slotframe with required handle, if any. NULL otherwise.
 */
struct tsch_slotframe *tsch_schedule_get_slotframe_by_handle(uint16_t handle);

/**
 * \brief Removes a slotframe
 * \param slotframe The slotframe to be removed
 * \return 1 if success, 0 if failure
 */
int tsch_schedule_remove_slotframe(struct tsch_slotframe *slotframe);

/**
 * \brief Removes all slotframes, resulting in an empty schedule
 * \return 1 if success, 0 if failure
 */
int tsch_schedule_remove_all_slotframes(void);

/**
 * \brief Adds a link to a slotframe
 * \param slotframe The slotframe that will contain the new link
 * \param link_options The link options, as a bitfield (LINK_OPTION_* flags)
 * \param link_type The link type (advertising, normal)
 * \param address The link address of the intended destination. Use &tsch_broadcast_address for a slot towards any neighbor
 * \param timeslot The link timeslot within the slotframe
 * \param channel_offset The link channel offset
 * \param do_remove Whether to remove an old link at this timeslot and channel offset
 * \return A pointer to the new link, NULL if failure
 */
struct tsch_link *tsch_schedule_add_link(struct tsch_slotframe *slotframe,
                                         uint8_t link_options, enum link_type link_type, const linkaddr_t *address,
                                         uint16_t timeslot, uint16_t channel_offset, uint8_t do_remove);
/**
* \brief Looks for a link from a handle
* \param handle The target handle
* \return The link with required handle, if any. Otherwise, NULL
*/
struct tsch_link *tsch_schedule_get_link_by_handle(uint16_t handle);

/**
 * \brief Looks within a slotframe for a link with a given timeslot
 * \param slotframe The desired slotframe
 * \param timeslot The desired timeslot
 * \param channel_offset The desired channel offset 
 * \return The link if found, NULL otherwise
 */
struct tsch_link *tsch_schedule_get_link_by_timeslot(struct tsch_slotframe *slotframe,
                                                     uint16_t timeslot, uint16_t channel_offset);

/**
 * \brief Removes a link
 * \param slotframe The slotframe the link belongs to
 * \param l The link to be removed
 * \return 1 if success, 0 if failure
 */
int tsch_schedule_remove_link(struct tsch_slotframe *slotframe, struct tsch_link *l);

/**
 * \brief Removes a link from a slotframe and timeslot
 * \param slotframe The slotframe where to look for the link
 * \param timeslot The timeslot where to look for the link within the target slotframe
 * \param channel_offset The channel offset where to look for the link within the target slotframe
 * \return 1 if success, 0 if failure
 */
int tsch_schedule_remove_link_by_timeslot(struct tsch_slotframe *slotframe,
                                          uint16_t timeslot, uint16_t channel_offset);

/**
 * \brief Returns the next active link after a given ASN, and a backup link (for the same ASN, with Rx flag)
 * \param asn The base ASN, from which we look for the next active link
 * \param time_offset A pointer to uint16_t where to store the time offset between base ASN and link found
 * \param backup_link A pointer where to write the address of a backup link, to be executed should the original be no longer active at wakeup
 * \return The next active link if any, NULL otherwise
 */
struct tsch_link * tsch_schedule_get_next_active_link(struct tsch_asn_t *asn, uint16_t *time_offset,
    struct tsch_link **backup_link);

/**
 * \brief Access the first item in the list of slotframes
 * \return The first slotframe in the schedule if any, NULL otherwise
 */
struct tsch_slotframe *tsch_schedule_slotframe_head(void);

/**
 * \brief Access the next item in the list of slotframes
 * \param sf The current slotframe (item in the list)
 * \return The next slotframe if any, NULL otherwise
 */
struct tsch_slotframe *tsch_schedule_slotframe_next(struct tsch_slotframe *sf);

#endif /* __TSCH_SCHEDULE_H__ */
/** @} */
