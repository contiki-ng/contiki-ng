/*
 * Copyright (c) 2018, Hasso-Plattner-Institut.
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
 * \addtogroup csl
 * @{
 * \file
 *         Stores CSL-specific metadata of L2-neighbors.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef CSL_NBR_H_
#define CSL_NBR_H_

#include "net/mac/csl/csl.h"
#include "net/mac/csl/csl-synchronizer-splo.h"
#include "net/mac/csl/csl-synchronizer-compliant.h"
#include "services/akes/akes-nbr.h"

typedef struct {
#if !CSL_COMPLIANT
  uint8_t q[AKES_NBR_CHALLENGE_LEN];
  rtimer_clock_t helloack_sfd_timestamp;
  wake_up_counter_t predicted_wake_up_counter;
#endif /* !CSL_COMPLIANT */
} csl_nbr_tentative_t;

typedef struct {
#if CSL_COMPLIANT
  struct csl_synchronizer_compliant_data sync_data;
#else /* CSL_COMPLIANT */
  struct csl_synchronizer_splo_data sync_data;
  int32_t drift;
  struct csl_synchronizer_splo_data historical_sync_data;
#endif /* CSL_COMPLIANT */
} csl_nbr_t;

csl_nbr_tentative_t *csl_nbr_get_tentative(struct akes_nbr_tentative *tentative);
csl_nbr_t *csl_nbr_get(struct akes_nbr *nbr);

#endif /* CSL_NBR_H_ */

/** @} */
