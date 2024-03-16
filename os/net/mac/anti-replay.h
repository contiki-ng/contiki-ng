/*
 * Copyright (c) 2014, Hasso-Plattner-Institut.
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
 * \addtogroup llsec802154
 * @{
 *
 * \file
 *         Interface to anti-replay mechanisms.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef ANTI_REPLAY_H
#define ANTI_REPLAY_H

#include "contiki.h"
#include "net/mac/framer/frame802154.h"
#include <stdbool.h>

#ifdef ANTI_REPLAY_CONF_WITH_SUPPRESSION
#define ANTI_REPLAY_WITH_SUPPRESSION ANTI_REPLAY_CONF_WITH_SUPPRESSION
#else /* ANTI_REPLAY_CONF_WITH_SUPPRESSION */
#define ANTI_REPLAY_WITH_SUPPRESSION 0
#endif /* ANTI_REPLAY_CONF_WITH_SUPPRESSION */

struct anti_replay_info {
  uint32_t last_broadcast_counter;
  uint32_t last_unicast_counter;
#if ANTI_REPLAY_WITH_SUPPRESSION
  uint32_t my_unicast_counter;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */
};

#if ANTI_REPLAY_WITH_SUPPRESSION
extern uint32_t anti_replay_my_broadcast_counter;
extern uint32_t anti_replay_my_unicast_counter;
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */

/**
 * \brief      Sets the frame counter packetbuf attributes.
 * \param info Anti-replay information about the receiver (NULL if broadcast)
 */
void anti_replay_set_counter(struct anti_replay_info *info);

/**
 * \brief Gets the frame counter from packetbuf.
 */
uint32_t anti_replay_get_counter(void);

/**
 * \brief      Initializes the anti-replay information about the sender
 * \param info Anti-replay information about the sender
 */
void anti_replay_init_info(struct anti_replay_info *info);

/**
 * \brief      Checks if received frame was replayed
 * \param info Anti-replay information about the sender
 */
bool anti_replay_was_replayed(struct anti_replay_info *info);

/**
 * \brief Parses the frame counter to packetbuf attributes
 */
void anti_replay_parse_counter(const uint8_t *p);

/**
 * \brief Writes the frame counter of packetbuf to dst
 */
void anti_replay_write_counter(uint8_t *dst);

/**
 * \brief Reads the frame counter from the specified destination.
 */
uint32_t anti_replay_read_counter(const uint8_t *src);

/**
 * \brief Gets the LSBs of the packetbuf's frame counter
 */
uint8_t anti_replay_get_counter_lsbs(void);

#if ANTI_REPLAY_WITH_SUPPRESSION
/**
 * \brief Writes my broadcast frame counter to the specified destination.
 */
void anti_replay_write_my_broadcast_counter(uint8_t *dst);

/**
 * \brief             Restores suppressed frame counter
 * \param sender_info Anti-replay information about the sender
 */
void anti_replay_restore_counter(const struct anti_replay_info *sender_info,
                                 uint8_t lsbs);
#else /* ANTI_REPLAY_WITH_SUPPRESSION */
/**
 * \brief Increments frame counter and stores it in counter
 */
void anti_replay_set_counter_to(frame802154_frame_counter_t *counter);
#endif /* ANTI_REPLAY_WITH_SUPPRESSION */

#endif /* ANTI_REPLAY_H */

/** @} */
