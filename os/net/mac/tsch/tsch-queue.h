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
 *	TSCH queues
*/

#ifndef TSCH_QUEUE_H_
#define TSCH_QUEUE_H_

/********** Includes **********/

#include "contiki.h"
#include "lib/ringbufindex.h"
#include "net/linkaddr.h"
#include "net/mac/mac.h"

/***** External Variables *****/

/* Broadcast and EB virtual neighbors */
extern struct tsch_neighbor *n_broadcast;
extern struct tsch_neighbor *n_eb;

/********** Functions *********/

/**
 * \brief Add a TSCH neighbor queue
 * \param addr The link-layer address of the neighbor to be added
 */
struct tsch_neighbor *tsch_queue_add_nbr(const linkaddr_t *addr);
/**
 * \brief Get a TSCH neighbor
 * \param addr The link-layer address of the neighbor we are looking for
 * \return A pointer to the neighbor queue, NULL if not found
 */
struct tsch_neighbor *tsch_queue_get_nbr(const linkaddr_t *addr);
/**
 * \brief Get the TSCH time source (we currently assume there is only one)
 * \return The neighbor queue associated to the time source
 */
struct tsch_neighbor *tsch_queue_get_time_source(void);
/**
 * \brief Get the address of a neighbor.
 * \return The link-layer address of the neighbor.
 */
linkaddr_t *tsch_queue_get_nbr_address(const struct tsch_neighbor *);
/**
 * \brief Update TSCH time source
 * \param new_addr The address of the new TSCH time source
 */
int tsch_queue_update_time_source(const linkaddr_t *new_addr);
/**
 * \brief Add packet to neighbor queue. Use same lockfree implementation as ringbuf.c (put is atomic)
 * \param addr The address of the targetted neighbor, &tsch_broadcast_address for broadcast
 * \param max_transmissions The number of MAC retries
 * \param sent The MAC packet sent callback
 * \param ptr The MAC packet send callback parameter
 * \return The newly created packet if any, NULL otherwise
 */
struct tsch_packet *tsch_queue_add_packet(const linkaddr_t *addr, uint8_t max_transmissions,
                                          mac_callback_t sent, void *ptr);
/**
 * \brief Returns the number of packets currently in all TSCH queues
 * \return The number of packets currently in all TSCH queues
 */
int tsch_queue_global_packet_count(void);
/**
 * \brief Returns the number of packets currently a given neighbor queue (by pointer)
 * \param n The neighbor we are interested in
 * \return The number of packets in the neighbor's queue
 */
int tsch_queue_nbr_packet_count(const struct tsch_neighbor *n);
/**
 * \brief Remove first packet from a neighbor queue. The packet is stored in a separate
 * dequeued packet list, for later processing.
 * \param n The neighbor queue
 * \return The packet that was removed if any, NULL otherwise
 */
struct tsch_packet *tsch_queue_remove_packet_from_queue(struct tsch_neighbor *n);
/**
 * \brief Free a packet
 * \param p The packet to be freed
 */
void tsch_queue_free_packet(struct tsch_packet *p);
/**
 * \brief Flush packets to a specific address
 * \param addr The address of the neighbor whose packets to free
 */
void tsch_queue_free_packets_to(const linkaddr_t *addr);
/**
 * \brief Updates neighbor queue state after a transmission
 * \param n The neighbor queue we just sent from
 * \param p The packet that was just sent
 * \param link The TSCH link used for Tx
 * \param mac_tx_status The MAC status (see mac.h)
 * \return 1 if the packet remains in queue after the call, 0 if it was removed
 */
int tsch_queue_packet_sent(struct tsch_neighbor *n, struct tsch_packet *p, struct tsch_link *link, uint8_t mac_tx_status);
/**
 * \brief Reset neighbor queues module
 */
void tsch_queue_reset(void);
/**
 * \brief Deallocate all neighbors with empty queue
 */
void tsch_queue_free_unused_neighbors(void);
/**
 * \brief Is the neighbor queue empty?
 * \param n The neighbor queue
 * \return 1 if empty, 0 otherwise
 */
int tsch_queue_is_empty(const struct tsch_neighbor *n);
/**
 * \brief Returns the first packet that can be sent from a queue on a given link
 * \param n The neighbor queue
 * \param link The link
 * \return The next packet to be sent for the neighbor on the given link, if any, else NULL
 */
struct tsch_packet *tsch_queue_get_packet_for_nbr(const struct tsch_neighbor *n, struct tsch_link *link);
/**
 * \brief Returns the first packet that can be sent to a given address on a given link
 * \param addr The target link-layer address
 * \param link The link
 * \return The next packet to be sent for to the given address on the given link, if any, else NULL
 */
struct tsch_packet *tsch_queue_get_packet_for_dest_addr(const linkaddr_t *addr, struct tsch_link *link);
/**
 * \brief Gets the head packet of any neighbor queue with zero backoff counter.
 * \param n A pointer where to store the neighbor queue to be used for Tx
 * \param link The link to be used for Tx
 * \return The packet if any, else NULL
 */
struct tsch_packet *tsch_queue_get_unicast_packet_for_any(struct tsch_neighbor **n, struct tsch_link *link);
/**
 * \brief Is the neighbor backoff timer expired?
 * \param n The neighbor queue
 * \return 1 if the backoff has expired (neighbor ready to transmit on a shared link), 0 otherwise
 */
int tsch_queue_backoff_expired(const struct tsch_neighbor *n);
/**
 * \brief Reset neighbor backoff
 * \param n The neighbor queue
 */
void tsch_queue_backoff_reset(struct tsch_neighbor *n);
/**
 * \brief Increment backoff exponent of a given neighbor queue, pick a new window
 * \param n The neighbor queue
 */
void tsch_queue_backoff_inc(struct tsch_neighbor *n);
/**
 * \brief Decrement backoff window for the queue(s) able to Tx to a given address
 * \param dest_addr The target address, &tsch_broadcast_address for broadcast
 */
void tsch_queue_update_all_backoff_windows(const linkaddr_t *dest_addr);
/**
 * \brief Initialize TSCH queue module
 */
void tsch_queue_init(void);

#endif /* TSCH_QUEUE_H_ */
/** @} */
