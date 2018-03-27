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
 *	RPL types and macros
 * \author
 *	Joakim Eriksson <joakime@sics.se> & Nicolas Tsiftes <nvt@sics.se>
 *  Simon Duquennoy <simon.duquennoy@inria.fr>
 *
 */

#ifndef RPL_TYPES_H
#define RPL_TYPES_H

 /********** Macros **********/

/* Multicast address: create and compare */

/** \brief Set IP address addr to the link-local, all-rpl-nodes
   multicast address. */
#define uip_create_linklocal_rplnodes_mcast(addr)	\
  uip_ip6addr((addr), 0xff02, 0, 0, 0, 0, 0, 0, 0x001a)

/** \brief Is IPv6 address addr the link-local, all-RPL-nodes
   multicast address? */
#define uip_is_addr_linklocal_rplnodes_mcast(addr)	    \
  ((addr)->u8[0] == 0xff) &&				    \
  ((addr)->u8[1] == 0x02) &&				    \
  ((addr)->u16[1] == 0) &&				    \
  ((addr)->u16[2] == 0) &&				    \
  ((addr)->u16[3] == 0) &&				    \
  ((addr)->u16[4] == 0) &&				    \
  ((addr)->u16[5] == 0) &&				    \
  ((addr)->u16[6] == 0) &&				    \
  ((addr)->u8[14] == 0) &&				    \
  ((addr)->u8[15] == 0x1a))

/** \brief Compute lifetime, accounting for the lifetime unit */
#define RPL_LIFETIME(lifetime) \
         (((lifetime) == RPL_INFINITE_LIFETIME) ? \
         RPL_ROUTE_INFINITE_LIFETIME : \
         (unsigned long)curr_instance.lifetime_unit * (lifetime))

/** \brief Rank of a root node. */
#define ROOT_RANK             curr_instance.min_hoprankinc

/** \brief Return DAG RANK as per RFC 6550 (rank divided by min_hoprankinc) */
#define DAG_RANK(fixpt_rank) ((fixpt_rank) / curr_instance.min_hoprankinc)

#define RPL_LOLLIPOP_MAX_VALUE            255
#define RPL_LOLLIPOP_CIRCULAR_REGION     127
#define RPL_LOLLIPOP_SEQUENCE_WINDOWS    16
#define RPL_LOLLIPOP_INIT                (RPL_LOLLIPOP_MAX_VALUE - RPL_LOLLIPOP_SEQUENCE_WINDOWS + 1)
#define RPL_LOLLIPOP_INCREMENT(counter)                                 \
  do {                                                                  \
   if((counter) > RPL_LOLLIPOP_CIRCULAR_REGION) {                      \
     (counter) = ((counter) + 1) & RPL_LOLLIPOP_MAX_VALUE;             \
   } else {                                                            \
     (counter) = ((counter) + 1) & RPL_LOLLIPOP_CIRCULAR_REGION;       \
   }                                                                   \
  } while(0)

#define RPL_LOLLIPOP_IS_INIT(counter)		\
   ((counter) > RPL_LOLLIPOP_CIRCULAR_REGION)


/********** Data structures and types **********/

typedef uint16_t rpl_rank_t;
typedef uint16_t rpl_ocp_t;

/*---------------------------------------------------------------------------*/
/** \brief Structure for RPL energy metric. */
struct rpl_metric_object_energy {
  uint8_t flags;
  uint8_t energy_est;
};

/** \brief Logical representation of a DAG Metric Container. */
struct rpl_metric_container {
  uint8_t type;
  uint8_t flags;
  uint8_t aggr;
  uint8_t prec;
  uint8_t length;
  union metric_object {
   struct rpl_metric_object_energy energy;
   uint16_t etx;
  } obj;
};
typedef struct rpl_metric_container rpl_metric_container_t;

/** \brief RPL prefix information */
struct rpl_prefix {
  uip_ipaddr_t prefix;
  uint32_t lifetime;
  uint8_t length;
  uint8_t flags;
  };
typedef struct rpl_prefix rpl_prefix_t;

/** \brief All information related to a RPL neighbor */
struct rpl_nbr {
  clock_time_t better_parent_since;  /* The neighbor has been a possible
  replacement for our preferred parent consistently since 'parent_since'.
  Currently used by MRHOF only. */
#if RPL_WITH_MC
  rpl_metric_container_t mc;
#endif /* RPL_WITH_MC */
  rpl_rank_t rank;
  uint8_t dtsn;
};
typedef struct rpl_nbr rpl_nbr_t;

/*---------------------------------------------------------------------------*/
 /**
  * \brief API for RPL objective functions (OF)
  *
  * - reset(dag) Resets the objective function state for a specific DAG. This function is
  *            called when doing a global repair on the DAG.
  * - nbr_link_metric(n)  Returns the link metric of a neighbor
  * - nbr_has_usable_link(n) Returns 1 iff the neighbor has a usable link as defined by the OF
  * - nbr_is_acceptable_parent(n) Returns 1 iff the neighbor has a usable rank/link as defined by the OF
  * - nbr_path_cost(n) Returns the path cost of a neighbor
  * - rank_via_nbr(n) Returns our rank if we select a given neighbor as preferred parent
  * - best_parent(n1, n2) Compares two neighbors and returns the best one, according to the OF.
  * - update_metric_container() Updated the DAG metric container from the current OF state
  */
 struct rpl_of {
   void (*reset)(void);
   uint16_t (*nbr_link_metric)(rpl_nbr_t *);
   int (*nbr_has_usable_link)(rpl_nbr_t *);
   int (*nbr_is_acceptable_parent)(rpl_nbr_t *);
   uint16_t (*nbr_path_cost)(rpl_nbr_t *);
   rpl_rank_t (*rank_via_nbr)(rpl_nbr_t *);
   rpl_nbr_t *(*best_parent)(rpl_nbr_t *, rpl_nbr_t *);
   void (*update_metric_container)(void);
   rpl_ocp_t ocp;
 };
 typedef struct rpl_of rpl_of_t;

/*---------------------------------------------------------------------------*/
/** \brief RPL DAG states*/
enum rpl_dag_state {
  DAG_INITIALIZED,
  DAG_JOINED,
  DAG_REACHABLE,
  DAG_POISONING
};

/** \brief RPL DAG structure */
struct rpl_dag {
  uip_ipaddr_t dag_id;
  rpl_prefix_t prefix_info;
  rpl_nbr_t *preferred_parent;
  rpl_rank_t lowest_rank; /* The lowest rank seen in the current version */
  rpl_rank_t rank; /* The current rank */
  rpl_rank_t last_advertised_rank; /* The last rank advertised in a multicast-DIO */
  uint32_t lifetime;
  uint8_t version;
  uint8_t grounded;
  uint8_t preference;
  uint8_t dio_intcurrent; /* Current DIO interval */
  uint8_t dio_send; /* internal trickle timer state: do we need to send a DIO at the next wakeup? */
  uint8_t dio_counter; /* internal trickle timer state: redundancy counter */
  uint8_t dao_last_seqno; /* the node's last sent DAO seqno */
  uint8_t dao_last_acked_seqno; /* the last seqno we got an ACK for */
  uint8_t dao_curr_seqno; /* the node's current DAO seqno (sent or to be sent) */
  uint8_t dao_transmissions; /* the number of transmissions for the current DAO */
  enum rpl_dag_state state;

  /* Timers */
  clock_time_t dio_next_delay; /* delay for completion of dio interval */
  struct ctimer state_update;
  struct ctimer leave;
  struct ctimer dio_timer;
  struct ctimer unicast_dio_timer;
  struct ctimer dao_timer;
  rpl_nbr_t *unicast_dio_target;
#if RPL_WITH_PROBING
  struct ctimer probing_timer;
  rpl_nbr_t *urgent_probing_target;
#endif /* RPL_WITH_PROBING */
#if RPL_WITH_DAO_ACK
  uip_ipaddr_t dao_ack_target;
  uint16_t dao_ack_sequence;
  struct ctimer dao_ack_timer;
#endif /* RPL_WITH_DAO_ACK */
};
typedef struct rpl_dag rpl_dag_t;

/*---------------------------------------------------------------------------*/
/** \brief RPL instance structure */
struct rpl_instance {
  rpl_metric_container_t mc; /* Metric container. Set to MC_NONE when no mc is used */
  rpl_of_t *of; /* The objective function */
  uint8_t used;
  uint8_t instance_id;
  uint8_t mop; /* Mode of operation */
  uint8_t dtsn_out;
  uint8_t dio_intdoubl;
  uint8_t dio_intmin;
  uint8_t dio_redundancy;
  rpl_rank_t max_rankinc;
  rpl_rank_t min_hoprankinc;
  uint8_t default_lifetime;
  uint16_t lifetime_unit; /* lifetime in seconds = lifetime_unit * default_lifetime */
  rpl_dag_t dag; /* We support only one dag */
};
typedef struct rpl_instance rpl_instance_t;

 /** @} */

#endif /* RPL_TYPES_H */
