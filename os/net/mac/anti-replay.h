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
 * \file
 *         Interface to anti-replay mechanisms.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

/**
 * \addtogroup llsec802154
 * @{
 */

#ifndef ANTI_REPLAY_H
#define ANTI_REPLAY_H

#include "net/mac/framer/frame802154.h"
#include "net/mac/llsec802154.h"

struct anti_replay_info {
  frame802154_frame_counter_t his_broadcast_counter;
  frame802154_frame_counter_t his_unicast_counter;
};

/**
 * \brief Parses the frame counter to packetbuf attributes
 */
void anti_replay_parse_counter(uint8_t *p);

/**
 * \brief Writes the frame counter of packetbuf to dst
 */
void anti_replay_write_counter(uint8_t *dst);

/**
 * \brief                Copies a new frame counter value to the packetbuf attributes
 * \retval 0             Frame counter is exhausted
 */
int anti_replay_set_counter(void);

/**
 * \brief                Copies a new frame counter value to the specified location
 * \retval 0             Frame counter is exhausted
 */
int anti_replay_set_counter_to(frame802154_frame_counter_t *counter);

/**
 * \brief Gets the frame counter from packetbuf
 */
uint32_t anti_replay_get_counter(void);

/**
 * \brief             Initializes the anti-replay information about the sender
 * \param sender_info Anti-replay information about the sender
 */
void anti_replay_init_info(struct anti_replay_info *sender_info);

/**
 * \brief              Checks if received frame was replayed
 * \param  sender_info Anti-replay information about the sender
 * \retval 0           <-> received frame was not replayed
 */
int anti_replay_was_replayed(struct anti_replay_info *sender_info);

#endif /* ANTI_REPLAY_H */

/** @} */
