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
 *         Trickles HELLOs.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "akes/akes-trickle.h"
#include "akes/akes.h"
#include "lib/trickle.h"
#include <stddef.h>

#ifdef AKES_TRICKLE_CONF_IMIN
#define IMIN (AKES_TRICKLE_CONF_IMIN * CLOCK_SECOND)
#else /* AKES_TRICKLE_CONF_IMIN */
#define IMIN MAX(30 * CLOCK_SECOND, 2 * AKES_MAX_WAITING_PERIOD)
#endif /* AKES_TRICKLE_CONF_IMIN */

#ifdef AKES_TRICKLE_CONF_IMAX
#define IMAX AKES_TRICKLE_CONF_IMAX
#else /* AKES_TRICKLE_CONF_IMAX */
#define IMAX (8)
#endif /* AKES_TRICKLE_CONF_IMAX */

#ifdef AKES_TRICKLE_CONF_REDUNDANCY_CONSTANT
#define REDUNDANCY_CONSTANT AKES_TRICKLE_CONF_REDUNDANCY_CONSTANT
#else /* AKES_TRICKLE_CONF_REDUNDANCY_CONSTANT */
#define REDUNDANCY_CONSTANT (2)
#endif /* AKES_TRICKLE_CONF_REDUNDANCY_CONSTANT */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-Trickle"
#define LOG_LEVEL LOG_LEVEL_MAC

static size_t new_nbrs_count;
static struct trickle trickle;

/*---------------------------------------------------------------------------*/
void
akes_trickle_on_fresh_authentic_hello(akes_nbr_t *sender)
{
  if(!sender->sent_authentic_hello) {
    sender->sent_authentic_hello = true;
    trickle_increment_counter(&trickle);
  }
}
/*---------------------------------------------------------------------------*/
static void
on_new_interval(void)
{
  new_nbrs_count = 0;
}
/*---------------------------------------------------------------------------*/
void
akes_trickle_on_new_nbr(void)
{
  LOG_INFO("New neighbor\n");

  size_t k = akes_nbr_count(AKES_NBR_PERMANENT) / 4;
  k = MAX(k, 1);
  if(++new_nbrs_count < k) {
    return;
  }
  trickle_reset(&trickle);
}
/*---------------------------------------------------------------------------*/
void
akes_trickle_stop(void)
{
  trickle_stop(&trickle);
}
/*---------------------------------------------------------------------------*/
static void
on_broadcast(void)
{
  LOG_INFO("Broadcasting HELLO\n");
  akes_broadcast_hello();

  for(akes_nbr_entry_t *entry = akes_nbr_head(AKES_NBR_PERMANENT);
      entry;
      entry = akes_nbr_next(entry, AKES_NBR_PERMANENT)) {
    entry->permanent->sent_authentic_hello = false;
  }
}
/*---------------------------------------------------------------------------*/
void
akes_trickle_start(void)
{
  trickle_start(&trickle,
                IMIN,
                IMAX,
                REDUNDANCY_CONSTANT,
                on_broadcast,
                on_new_interval);
}
/*---------------------------------------------------------------------------*/

/** @} */
