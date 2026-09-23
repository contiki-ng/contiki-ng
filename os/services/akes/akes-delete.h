/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
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
 * \addtogroup akes
 * @{
 *
 * \file
 *         Deletes expired neighbors.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef AKES_DELETE_H_
#define AKES_DELETE_H_

#include "contiki.h"
#include "akes/akes-nbr.h"
#include <stdbool.h>

#ifdef AKES_DELETE_CONF_STRATEGY
#define AKES_DELETE_STRATEGY AKES_DELETE_CONF_STRATEGY
#else /* AKES_DELETE_CONF_STRATEGY */
#define AKES_DELETE_STRATEGY akes_delete_strategy_default
#endif /* AKES_DELETE_CONF_STRATEGY */

/**
 * Structure of a strategy regarding compromise resilience
 */
struct akes_delete_strategy {

  /**
   * \brief        Tells whether a permanent neighbor is expired
   * \param nbr    The permanent neighbor
   * \retval false The permanent neighbor is not expired
   */
  bool (* is_permanent_neighbor_expired)(akes_nbr_t *nbr);

  /**
   * \brief     Prolongs the expiration time of a permanent neighbor
   * \param nbr The permanent neighbor
   */
  void (* prolong_permanent_neighbor)(akes_nbr_t *nbr);
};

extern const struct akes_delete_strategy AKES_DELETE_STRATEGY;
extern const struct akes_delete_strategy akes_delete_strategy_default;

/**
 * \brief Called when an UPDATE was sent
 */
void akes_delete_on_update_sent(void *ptr, int status, int transmissions);

/**
 * \brief Initializes
 */
void akes_delete_init(void);

#endif /* AKES_DELETE_H_ */

/** @} */
