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
 *         Implements the Trickle algorithm as per RFC 6206.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef TRICKLE_H_
#define TRICKLE_H_

#include "contiki.h"
#include "sys/ctimer.h"
#include "sys/pt.h"
#include <stdint.h>

typedef void (* trickle_callback_t)(void);

struct trickle {
  struct ctimer timer;
  struct pt protothread;
  trickle_callback_t on_broadcast;
  trickle_callback_t on_new_interval;
  clock_time_t interval_size;
  clock_time_t imin;
  uint16_t counter;
  uint8_t max_doublings;
  uint8_t redundancy_constant;
};

/**
 * \brief                     Starts Trickle
 * \param trickle             Trickle structure in heap
 * \param imin                Corresponds to I_min
 * \param max_doublings       Encodes I_max as I_max = I_min * 2^max_doublings
 * \param redundancy_constant Corresponds to k
 * \param on_broadcast        Called when Trickle schedules a broadcast
 * \param on_new_interval     Called when a new interval starts (nullable)
 */
void trickle_start(struct trickle *trickle,
                   clock_time_t imin,
                   uint8_t max_doublings,
                   uint8_t redundancy_constant,
                   trickle_callback_t on_broadcast,
                   trickle_callback_t on_new_interval);

/**
 * \brief To be called when receiving a consistent broadcast
 */
void trickle_increment_counter(struct trickle *trickle);

/**
 * \brief Resets Trickle when receiving an inconsistent broadcast
 */
void trickle_reset(struct trickle *trickle);

/**
 * \brief Stops Trickle
 */
void trickle_stop(struct trickle *trickle);

#endif /* TRICKLE_H_ */
