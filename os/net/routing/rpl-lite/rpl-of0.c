/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
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
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *         An implementation of RPL's objective function 0, RFC6552
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 * Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#include "net/routing/rpl-lite/rpl.h"
#include "net/nbr-table.h"
#include "net/link-stats.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

/* Constants from RFC6552. We use the default values. */
#define RANK_STRETCH       0 /* Must be in the range [0;5] */
#define RANK_FACTOR        1 /* Must be in the range [1;4] */

#define MIN_STEP_OF_RANK   1
#define MAX_STEP_OF_RANK   9

/* OF0 computes rank increase as follows:
 * rank_increase = (RANK_FACTOR * STEP_OF_RANK + RANK_STRETCH) * min_hop_rank_increase
 * STEP_OF_RANK is an implementation-specific scalar value in the range [1;9].
 * RFC6552 provides a default value of 3 but recommends to use a dynamic link metric
 * such as ETX.
 * */

#define RPL_OF0_FIXED_SR      0
#define RPL_OF0_ETX_BASED_SR  1
/* Select RPL_OF0_FIXED_SR or RPL_OF0_ETX_BASED_SR */
#ifdef RPL_OF0_CONF_SR
#define RPL_OF0_SR            RPL_OF0_CONF_SR
#else /* RPL_OF0_CONF_SR */
#define RPL_OF0_SR            RPL_OF0_ETX_BASED_SR
#endif /* RPL_OF0_CONF_SR */

#if RPL_OF0_FIXED_SR
#define STEP_OF_RANK(nbr)       (3)
#endif /* RPL_OF0_FIXED_SR */

#if RPL_OF0_ETX_BASED_SR
/* Numbers suggested by P. Thubert for in the 6TiSCH WG. Anything that maps ETX to
 * a step between 1 and 9 works. */
#define STEP_OF_RANK(nbr)       (((3 * nbr_link_metric(nbr)) / LINK_STATS_ETX_DIVISOR) - 2)
#endif /* RPL_OF0_ETX_BASED_SR */

/*---------------------------------------------------------------------------*/
static void
reset(void)
{
  LOG_INFO("reset OF0\n");
}
/*---------------------------------------------------------------------------*/
static uint16_t
nbr_link_metric(rpl_nbr_t *nbr)
{
  /* OF0 operates without metric container; the only metric we have is ETX */
  const struct link_stats *stats = rpl_neighbor_get_link_stats(nbr);
  return stats != NULL ? stats->etx : 0xffff;
}
/*---------------------------------------------------------------------------*/
static uint16_t
nbr_rank_increase(rpl_nbr_t *nbr)
{
  uint16_t min_hoprankinc;
  if(nbr == NULL) {
    return RPL_INFINITE_RANK;
  }
  min_hoprankinc = curr_instance.min_hoprankinc;
  return (RANK_FACTOR * STEP_OF_RANK(nbr) + RANK_STRETCH) * min_hoprankinc;
}
/*---------------------------------------------------------------------------*/
static uint16_t
nbr_path_cost(rpl_nbr_t *nbr)
{
  if(nbr == NULL) {
    return 0xffff;
  }
  /* path cost upper bound: 0xffff */
  return MIN((uint32_t)nbr->rank + nbr_link_metric(nbr), 0xffff);
}
/*---------------------------------------------------------------------------*/
static rpl_rank_t
rank_via_nbr(rpl_nbr_t *nbr)
{
  if(nbr == NULL) {
    return RPL_INFINITE_RANK;
  } else {
    return MIN((uint32_t)nbr->rank + nbr_rank_increase(nbr), RPL_INFINITE_RANK);
  }
}
/*---------------------------------------------------------------------------*/
static int
nbr_has_usable_link(rpl_nbr_t *nbr)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
static int
nbr_is_acceptable_parent(rpl_nbr_t *nbr)
{
  return STEP_OF_RANK(nbr) >= MIN_STEP_OF_RANK
      && STEP_OF_RANK(nbr) <= MAX_STEP_OF_RANK;
}
/*---------------------------------------------------------------------------*/
static rpl_nbr_t *
best_parent(rpl_nbr_t *nbr1, rpl_nbr_t *nbr2)
{
  uint16_t nbr1_cost;
  uint16_t nbr2_cost;
  int nbr1_is_acceptable;
  int nbr2_is_acceptable;

  nbr1_is_acceptable = nbr1 != NULL && nbr_is_acceptable_parent(nbr1);
  nbr2_is_acceptable = nbr2 != NULL && nbr_is_acceptable_parent(nbr2);

  if(!nbr1_is_acceptable) {
    return nbr2_is_acceptable ? nbr2 : NULL;
  }
  if(!nbr2_is_acceptable) {
    return nbr1_is_acceptable ? nbr1 : NULL;
  }

  nbr1_cost = nbr_path_cost(nbr1);
  nbr2_cost = nbr_path_cost(nbr2);

  /* Paths costs coarse-grained (multiple of min_hoprankinc), we operate without hysteresis */
  if(nbr1_cost != nbr2_cost) {
    /* Pick nbr with lowest path cost */
    return nbr1_cost < nbr2_cost ? nbr1 : nbr2;
  } else {
    /* We have a tie! */
    /* Stik to current preferred parent if possible */
    if(nbr1 == curr_instance.dag.preferred_parent || nbr2 == curr_instance.dag.preferred_parent) {
      return curr_instance.dag.preferred_parent;
    }
    /* None of the nodes is the current preferred parent,
     * choose nbr with best link metric */
    return nbr_link_metric(nbr1) < nbr_link_metric(nbr2) ? nbr1 : nbr2;
  }
}
/*---------------------------------------------------------------------------*/
static void
update_metric_container(void)
{
  curr_instance.mc.type = RPL_DAG_MC_NONE;
}
/*---------------------------------------------------------------------------*/
rpl_of_t rpl_of0 = {
  reset,
  nbr_link_metric,
  nbr_has_usable_link,
  nbr_is_acceptable_parent,
  nbr_path_cost,
  rank_via_nbr,
  best_parent,
  update_metric_container,
  RPL_OCP_OF0
};

/** @}*/
