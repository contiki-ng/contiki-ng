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
 *	Header file for rpl-dag module
 * \author
 *	Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 *  Simon DUquennoy <simon.duquennoy@inria.fr>
 *
 */

#ifndef RPL_DAG_H
#define RPL_DAG_H

/********** Includes **********/

#include "uip.h"

/********** Public functions **********/

/**
 * Returns a textual description of the current DAG state
 *
 * \param state The DAG state
 * \return The description string
*/
const char *rpl_dag_state_to_str(enum rpl_dag_state state);
/**
 * Returns the IPv6 address of the RPL DAG root, if any
 *
 * \param ipaddr A pointer where to copy the IP address of the DAG root
 * \return 1 if the root address was copied, 0 otherwise
*/
int rpl_dag_get_root_ipaddr(uip_ipaddr_t *ipaddr);
/**
 * Start poisoning and leave the DAG after a delay
 *
*/
void rpl_dag_poison_and_leave(void);
/**
 * Leaves the current DAG
 *
*/
void rpl_dag_leave(void);
/**
 * A function called periodically. Used to age the DAG (decrease lifetime
 * and expire DAG accordingly)
 *
 * \param seconds The number of seconds elapsted since last call
*/
void rpl_dag_periodic(unsigned seconds);

/**
 * Triggers a RPL global repair
 *
 * \param str A textual description of the cause for triggering a repair
*/
void rpl_global_repair(const char *str);

/**
 * Triggers a RPL local repair
 *
 * \param str A textual description of the cause for triggering a repair
*/
void rpl_local_repair(const char *str);

/**
 * Tells whether a given global IPv6 address is in our current DAG
 *
 * \param addr The global IPv6 address to be tested
 * \return 1 if addr is in our current DAG, 0 otherwise
*/
int rpl_is_addr_in_our_dag(const uip_ipaddr_t *addr);

/**
 * Initializes DAG internal structure for a root node
 *
 * \param instance_id The instance ID
 * \param dag_id The DAG ID
 * \param prefix The prefix
 * \param prefix_len The prefix length
 * \param flags The prefix flags (from DIO)
*/
void rpl_dag_init_root(uint8_t instance_id, uip_ipaddr_t *dag_id,
  uip_ipaddr_t *prefix, unsigned prefix_len, uint8_t flags);

/**
 * Returns pointer to the default instance (for compatibility with legagy RPL code)
 *
 * \return A pointer to the only supported instance
*/
rpl_instance_t *rpl_get_default_instance(void);

/**
 * Returns pointer to any DAG (for compatibility with legagy RPL code)
 *
 * \return A pointer to the only supported DAG
*/
rpl_dag_t *rpl_get_any_dag(void);

/**
 * Processes Hop-by-Hop (HBH) Extension Header of a packet currently being forwrded.
 *
 * \param sender The IPv6 address of the originator
 * \param sender_rank The rank advertised by the sender in the HBH header
 * \param loop_detected 1 if we could detect a loop while forwarding, 0 otherwise
 * \param rank_error_signaled 1 if the HBH header advertises a rank error, 0 otherwise
 * \return 1 if the packet is to be forwarded, 0 if it is to be dropped
*/
int rpl_process_hbh(rpl_nbr_t *sender, uint16_t sender_rank, int loop_detected, int rank_error_signaled);

/**
 * Processes incoming DIS
 *
 * \param from The IPv6 address of the originator
 * \param is_multicast Set to 1 for multicast DIS, 0 for unicast DIS
*/
void rpl_process_dis(uip_ipaddr_t *from, int is_multicast);

/**
 * Processes incoming DIO
 *
 * \param from The IPv6 address of the originator
 * \param dio A pointer to a parsed DIO
*/
void rpl_process_dio(uip_ipaddr_t *from, rpl_dio_t *dio);

/**
 * Processes incoming DAO
 *
 * \param from The IPv6 address of the originator
 * \param dao A pointer to a parsed DAO
*/
void rpl_process_dao(uip_ipaddr_t *from, rpl_dao_t *dao);

/**
 * Processes incoming DAO-ACK
 *
 * \param sequence The DAO-ACK sequence number
 * \param status The DAO-ACK status (see RPL_DAO_ACK_* defines)
*/
void rpl_process_dao_ack(uint8_t sequence, uint8_t status);

/**
 * Tells whether RPL is ready to advertise the DAG
 *
 * \return 1 is ready, 0 otherwise
*/
int rpl_dag_ready_to_advertise(void);

/**
 * Updates RPL internal state: selects preferred parent, updates rank & metreic
 * container, triggers control traffic accordingly and updates uIP6 internal state.
*/
void rpl_dag_update_state(void);

/**
 * Initializes rpl-dag module
*/
void rpl_dag_init(void);

 /** @} */

#endif /* RPL_DAG_H */
