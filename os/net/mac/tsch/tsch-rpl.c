/*
 * Copyright (c) 2014, SICS Swedish ICT.
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
 */

/**
 * \file
 *         Interaction between TSCH and RPL
 *
 * \author Simon Duquennoy <simonduq@sics.se>
 */

/**
 * \addtogroup tsch
 * @{
*/

#include "contiki.h"

#if UIP_CONF_IPV6_RPL

#include "net/routing/routing.h"
#include "net/mac/tsch/tsch.h"
#include "net/link-stats.h"

#if ROUTING_CONF_RPL_LITE
#include "net/routing/rpl-lite/rpl.h"
#elif ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#include "net/routing/rpl-classic/rpl-private.h"
#endif

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TSCH RPL"
#define LOG_LEVEL LOG_LEVEL_MAC

/*---------------------------------------------------------------------------*/
/* To use, set #define TSCH_CALLBACK_KA_SENT tsch_rpl_callback_ka_sent */
void
tsch_rpl_callback_ka_sent(int status, int transmissions)
{
  NETSTACK_ROUTING.link_callback(packetbuf_addr(PACKETBUF_ADDR_RECEIVER), status, transmissions);
}
/*---------------------------------------------------------------------------*/
/* To use, set #define TSCH_CALLBACK_JOINING_NETWORK tsch_rpl_callback_joining_network */
void
tsch_rpl_callback_joining_network(void)
{
}
/*---------------------------------------------------------------------------*/
/* Upon leaving a TSCH network, perform a local repair
 * (cleanup neighbor state, reset Trickle timer etc)
 * To use, set #define TSCH_CALLBACK_LEAVING_NETWORK tsch_rpl_callback_leaving_network */
void
tsch_rpl_callback_leaving_network(void)
{
  /* Forget past link statistics. If we are leaving a TSCH
  network, there are changes we've been out of sync in the recent past, and
  as a result have irrelevant link statistices. */
  link_stats_reset();
  /* RPL local repair */
  NETSTACK_ROUTING.local_repair("TSCH leaving");
}
/*---------------------------------------------------------------------------*/
/* Set TSCH EB period based on current RPL DIO period.
 * To use, set #define RPL_CALLBACK_NEW_DIO_INTERVAL tsch_rpl_callback_new_dio_interval */
void
tsch_rpl_callback_new_dio_interval(clock_time_t dio_interval)
{
  /* Transmit EBs only if we have a valid rank as per 6TiSCH minimal */
  rpl_dag_t *dag;
  rpl_rank_t root_rank;
  rpl_rank_t dag_rank;
#if ROUTING_CONF_RPL_LITE
  dag = &curr_instance.dag;
  root_rank = ROOT_RANK;
  dag_rank = DAG_RANK(dag->rank);
#else
  rpl_instance_t *instance;
  dag = rpl_get_any_dag();
  instance = dag != NULL ? dag->instance : NULL;
  root_rank = ROOT_RANK(instance);
  dag_rank = DAG_RANK(dag->rank, instance);
#endif

  if(dag != NULL && dag->rank != RPL_INFINITE_RANK) {
    /* If we are root set TSCH as coordinator */
    if(dag->rank == root_rank) {
      tsch_set_coordinator(1);
    }
    /* Set EB period */
    tsch_set_eb_period(dio_interval);
    /* Set join priority based on RPL rank */
    tsch_set_join_priority(dag_rank - 1);
  } else {
    tsch_set_eb_period(TSCH_EB_PERIOD);
  }
}
/*---------------------------------------------------------------------------*/
/* Set TSCH time source based on current RPL preferred parent.
 * To use, set #define RPL_CALLBACK_PARENT_SWITCH tsch_rpl_callback_parent_switch */
void
tsch_rpl_callback_parent_switch(rpl_parent_t *old, rpl_parent_t *new)
{
  /* Map the TSCH time source on the RPL preferred parent (but stick to the
   * current time source if there is no preferred aarent) */
  if(tsch_is_associated == 1 && new != NULL) {
    tsch_queue_update_time_source(
      (const linkaddr_t *)uip_ds6_nbr_lladdr_from_ipaddr(
        rpl_parent_get_ipaddr(new)));
  }
}
#endif /* UIP_CONF_IPV6_RPL */
/** @} */
