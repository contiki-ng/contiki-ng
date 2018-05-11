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
 *         The Minimum Rank with Hysteresis Objective Function (MRHOF), RFC6719
 *
 *         This implementation uses the estimated number of
 *         transmissions (ETX) as the additive routing metric,
 *         and also provides stubs for the energy metric.
 *
 * \author Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>
 *  Simon Duquennoy <simon.duquennoy@inria.fr>
 */

#include "net/routing/rpl-lite/rpl.h"
#include "net/nbr-table.h"
#include "net/link-stats.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "RPL"
#define LOG_LEVEL LOG_LEVEL_RPL

/* RFC6551 and RFC6719 do not mandate the use of a specific formula to
 * compute the ETX value. This MRHOF implementation relies on the value
 * computed by the link-stats module.It has an optional feature,
 * RPL_MRHOF_CONF_SQUARED_ETX, that consists in squaring this value.
 *
 * Squaring basically penalizes bad links while preserving the semantics of ETX
 * (1 = perfect link, more = worse link). As a result, MRHOF will favor
 * good links over short paths. Without this feature, a hop with 50% PRR (ETX=2)
 * is equivalent to two perfect hops with 100% PRR (ETX=1+1=2). With this
 * feature, the former path obtains ETX=2*2=4 and the former ETX=1*1+1*1=2.
 *
 * While this feature helps achieve extra relaibility, it also results in
 * added churn. In networks with high congestion or poor links, this can lead
 * to poor connectivity due to more parent switches, loops, Trickle resets, etc.
  */
#ifdef RPL_MRHOF_CONF_SQUARED_ETX
#define RPL_MRHOF_SQUARED_ETX RPL_MRHOF_CONF_SQUARED_ETX
#else /* RPL_MRHOF_CONF_SQUARED_ETX */
#define RPL_MRHOF_SQUARED_ETX 0
#endif /* RPL_MRHOF_CONF_SQUARED_ETX */

/* Configuration parameters of RFC6719. Reject parents that have a higher
 * link metric than the following. The default value is 512. */
#ifdef RPL_MRHOF_CONF_MAX_LINK_METRIC
#define MAX_LINK_METRIC     RPL_MRHOF_CONF_MAX_LINK_METRIC
#else /* RPL_MRHOF_CONF_MAX_LINK_METRIC */
#define MAX_LINK_METRIC     512 /* Eq ETX of 4 */
#endif /* RPL_MRHOF_CONF_MAX_LINK_METRIC */

/* Reject parents that have a higher path cost than the following. */
#ifdef RPL_MRHOF_CONF_MAX_PATH_COST
#define MAX_PATH_COST      RPL_MRHOF_CONF_MAX_PATH_COST
#else /*  RPL_MRHOF_CONF_MAX_PATH_COST */
#define MAX_PATH_COST      32768   /* Eq path ETX of 256 */
#endif /* RPL_MRHOF_CONF_MAX_PATH_COST */

#if !RPL_MRHOF_SQUARED_ETX
/* Hysteresis of MRHOF: the rank must differ more than PARENT_SWITCH_THRESHOLD_DIV
 * in order to switch preferred parent. Default in RFC6719: 192, eq ETX of 1.5. */
#define RANK_THRESHOLD 192 /* Eq ETX of 1.5 */
#else /* !RPL_MRHOF_SQUARED_ETX */
#define RANK_THRESHOLD 384 /* Eq ETX of sqrt(3) */
#endif /* !RPL_MRHOF_SQUARED_ETX */

/* Additional, custom hysteresis based on time. If a neighbor was consistently
 * better than our preferred parent for at least TIME_THRESHOLD, switch to
 * this neighbor regardless of RANK_THRESHOLD. */
#define TIME_THRESHOLD (10 * 60 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
static void
reset(void)
{
  LOG_INFO("reset MRHOF\n");
}
/*---------------------------------------------------------------------------*/
static uint16_t
nbr_link_metric(rpl_nbr_t *nbr)
{
  const struct link_stats *stats = rpl_neighbor_get_link_stats(nbr);
  return stats != NULL ? stats->etx : 0xffff;
}
/*---------------------------------------------------------------------------*/
static uint16_t
link_metric_to_rank(uint16_t etx)
{
#if RPL_MRHOF_SQUARED_ETX
  uint32_t squared_etx = ((uint32_t)etx * etx) / LINK_STATS_ETX_DIVISOR;
  return (uint16_t)MIN(squared_etx, 0xffff);
#else /* RPL_MRHOF_SQUARED_ETX */
  return etx;
#endif /* RPL_MRHOF_SQUARED_ETX */
}
/*---------------------------------------------------------------------------*/
static uint16_t
nbr_path_cost(rpl_nbr_t *nbr)
{
  uint16_t base;

  if(nbr == NULL) {
    return 0xffff;
  }

#if RPL_WITH_MC
  /* Handle the different MC types */
  switch(curr_instance.mc.type) {
    case RPL_DAG_MC_ETX:
      base = nbr->mc.obj.etx;
      break;
    case RPL_DAG_MC_ENERGY:
      base = nbr->mc.obj.energy.energy_est << 8;
      break;
    default:
      base = nbr->rank;
      break;
  }
#else /* RPL_WITH_MC */
  base = nbr->rank;
#endif /* RPL_WITH_MC */

  /* path cost upper bound: 0xffff */
  return MIN((uint32_t)base + link_metric_to_rank(nbr_link_metric(nbr)), 0xffff);
}
/*---------------------------------------------------------------------------*/
static rpl_rank_t
rank_via_nbr(rpl_nbr_t *nbr)
{
  uint16_t min_hoprankinc;
  uint16_t path_cost;

  if(nbr == NULL) {
    return RPL_INFINITE_RANK;
  }

  min_hoprankinc = curr_instance.min_hoprankinc;
  path_cost = nbr_path_cost(nbr);

  /* Rank lower-bound: nbr rank + min_hoprankinc */
  return MAX(MIN((uint32_t)nbr->rank + min_hoprankinc, RPL_INFINITE_RANK), path_cost);
}
/*---------------------------------------------------------------------------*/
static int
nbr_has_usable_link(rpl_nbr_t *nbr)
{
  uint16_t link_metric = nbr_link_metric(nbr);
  /* Exclude links with too high link metrics  */
  return link_metric <= MAX_LINK_METRIC;
}
/*---------------------------------------------------------------------------*/
static int
nbr_is_acceptable_parent(rpl_nbr_t *nbr)
{
  uint16_t path_cost = nbr_path_cost(nbr);
  /* Exclude links with too high link metrics or path cost (RFC6719, 3.2.2) */
  return nbr_has_usable_link(nbr) && path_cost <= MAX_PATH_COST;
}
/*---------------------------------------------------------------------------*/
static int
within_hysteresis(rpl_nbr_t *nbr)
{
  uint16_t path_cost = nbr_path_cost(nbr);
  uint16_t parent_path_cost = nbr_path_cost(curr_instance.dag.preferred_parent);

  int within_rank_hysteresis = path_cost + RANK_THRESHOLD > parent_path_cost;
  int within_time_hysteresis = nbr->better_parent_since == 0
    || (clock_time() - nbr->better_parent_since) <= TIME_THRESHOLD;

  /* As we want to consider neighbors that are either beyond the rank or time
  hystereses, return 1 here iff the neighbor is within both hystereses. */
  return within_rank_hysteresis && within_time_hysteresis;
}
/*---------------------------------------------------------------------------*/
static rpl_nbr_t *
best_parent(rpl_nbr_t *nbr1, rpl_nbr_t *nbr2)
{
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

  /* Maintain stability of the preferred parent. Switch only if the gain
  is greater than RANK_THRESHOLD, or if the neighbor has been better than the
  current parent for at more than TIME_THRESHOLD. */
  if(nbr1 == curr_instance.dag.preferred_parent && within_hysteresis(nbr2)) {
    return nbr1;
  }
  if(nbr2 == curr_instance.dag.preferred_parent && within_hysteresis(nbr1)) {
    return nbr2;
  }

  return nbr_path_cost(nbr1) < nbr_path_cost(nbr2) ? nbr1 : nbr2;
}
/*---------------------------------------------------------------------------*/
#if !RPL_WITH_MC
static void
update_metric_container(void)
{
  curr_instance.mc.type = RPL_DAG_MC_NONE;
}
#else /* RPL_WITH_MC */
static void
update_metric_container(void)
{
  uint16_t path_cost;
  uint8_t type;

  if(!curr_instance.used) {
    LOG_WARN("cannot update the metric container when not joined\n");
    return;
  }

  if(curr_instance.dag.rank == ROOT_RANK) {
    /* Configure MC at root only, other nodes are auto-configured when joining */
    curr_instance.mc.type = RPL_DAG_MC;
    curr_instance.mc.flags = 0;
    curr_instance.mc.aggr = RPL_DAG_MC_AGGR_ADDITIVE;
    curr_instance.mc.prec = 0;
    path_cost = curr_instance.dag.rank;
  } else {
    path_cost = nbr_path_cost(curr_instance.dag.preferred_parent);
  }

  /* Handle the different MC types */
  switch(curr_instance.mc.type) {
    case RPL_DAG_MC_NONE:
      break;
    case RPL_DAG_MC_ETX:
      curr_instance.mc.length = sizeof(curr_instance.mc.obj.etx);
      curr_instance.mc.obj.etx = path_cost;
      break;
    case RPL_DAG_MC_ENERGY:
      curr_instance.mc.length = sizeof(curr_instance.mc.obj.energy);
      if(curr_instance.dag.rank == ROOT_RANK) {
        type = RPL_DAG_MC_ENERGY_TYPE_MAINS;
      } else {
        type = RPL_DAG_MC_ENERGY_TYPE_BATTERY;
      }
      curr_instance.mc.obj.energy.flags = type << RPL_DAG_MC_ENERGY_TYPE;
      /* Energy_est is only one byte, use the least significant byte of the path metric. */
      curr_instance.mc.obj.energy.energy_est = path_cost >> 8;
      break;
    default:
      LOG_WARN("MRHOF, non-supported MC %u\n", curr_instance.mc.type);
      break;
  }
}
#endif /* RPL_WITH_MC */
/*---------------------------------------------------------------------------*/
rpl_of_t rpl_mrhof = {
  reset,
  nbr_link_metric,
  nbr_has_usable_link,
  nbr_is_acceptable_parent,
  nbr_path_cost,
  rank_via_nbr,
  best_parent,
  update_metric_container,
  RPL_OCP_MRHOF
};

/** @}*/
