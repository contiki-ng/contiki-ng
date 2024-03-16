/*
 * Copyright (c) 2023, Uppsala universitet.
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
 *         Common functionality for scheduling retransmissions.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef FRAME_QUEUE_H_
#define FRAME_QUEUE_H_

#include "contiki.h"
#include "net/linkaddr.h"
#include "net/mac/mac.h"
#include "sys/clock.h"
#include <stdbool.h>
#include <stdint.h>

#ifdef FRAME_QUEUE_CONF_BROADCASTS_AS_UNICASTS
#define FRAME_QUEUE_BROADCASTS_AS_UNICASTS \
  FRAME_QUEUE_CONF_BROADCASTS_AS_UNICASTS
#else /* FRAME_QUEUE_CONF_BROADCASTS_AS_UNICASTS */
#define FRAME_QUEUE_BROADCASTS_AS_UNICASTS 0
#endif /* FRAME_QUEUE_CONF_BROADCASTS_AS_UNICASTS */

typedef struct frame_queue_entry {
  struct frame_queue_entry *next;
  struct queuebuf *qb;
  mac_callback_t sent;
  void *ptr;
#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
  uint32_t neighbor_bitmap;
  bool is_broadcast_as_unicast;
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */
} frame_queue_entry_t;

/**
 * \brief Initializes.
 */
void frame_queue_init(void);

/**
 * \brief Buffers outgoing frames.
 */
bool frame_queue_add(mac_callback_t sent, void *ptr);

/**
 * \brief Selects the next frame to transmit.
 */
frame_queue_entry_t *frame_queue_pick(void);

/**
 * \brief Returns the first entry in the queue.
 */
frame_queue_entry_t *frame_queue_head(void);

/**
 * \brief Returns the next entry in the queue.
 */
frame_queue_entry_t *frame_queue_next(frame_queue_entry_t *fqe);

/**
 * \brief Selects the next frame to burst.
 */
frame_queue_entry_t *frame_queue_burst(frame_queue_entry_t *previous);

/**
 * \brief Delays the transmission of any frames toward the same receiver.
 */
void frame_queue_postpone(clock_time_t duration);

/**
 * \brief Handles a completed transmission.
 */
void frame_queue_on_transmitted(int result, frame_queue_entry_t *fqe);

/**
 * \brief Cancels the transmission of an 802.15.4 MAC command frame.
 * \param receiver The receiver's MAC address.
 * \param cmd_id The 802.15.4 command frame identifier.
 */
void frame_queue_cancel_command(const linkaddr_t *receiver,
                                uint_fast8_t cmd_id);

#endif /* FRAME_QUEUE_H_ */
