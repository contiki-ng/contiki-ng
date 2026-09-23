/*
 * Copyright (c) 2017, Hasso-Plattner-Institut.
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
 *         Leaky bucket implementation.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef LEAKY_BUCKET_H_
#define LEAKY_BUCKET_H_

#include "contiki.h"
#include <stdbool.h>

typedef struct leaky_bucket_t {
  uint16_t last_update_timestamp;
  uint16_t capacity;
  uint16_t leakage_duration;
  uint16_t filling_level;
} leaky_bucket_t;

/**
 * \param lb pointer to the bucket in question
 * \param capacity number of drops that fit into the bucket
 * \param leakage_duration how long it takes until one drop leaks in seconds
 */
void leaky_bucket_init(leaky_bucket_t *lb,
                       uint16_t capacity,
                       uint16_t leakage_duration);

/**
 * \brief pours a drop in the bucket
 */
void leaky_bucket_pour(leaky_bucket_t *lb);

/**
 * \brief removes a drop from the bucket
 */
void leaky_bucket_effuse(leaky_bucket_t *lb);

/**
 * \return whether the bucket is full
 */
bool leaky_bucket_is_full(leaky_bucket_t *lb);

#endif /* LEAKY_BUCKET_H_ */
