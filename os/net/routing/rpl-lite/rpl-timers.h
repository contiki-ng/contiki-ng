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
 *	Header file for rpl-timers module
 * \author
 *	Joakim Eriksson <joakime@sics.se>, Nicolas Tsiftes <nvt@sics.se>,
 *  Simon DUquennoy <simon.duquennoy@inria.fr>
 *
 */

#ifndef RPL_TIMERS_H
#define RPL_TIMERS_H

/********** Includes **********/

#include "net/routing/rpl-lite/rpl.h"

/********** Public functions **********/

/**
 * Schedule periodic DIS with a random delay based on RPL_DIS_INTERVAL, until
 * we join a DAG.
*/
void rpl_timers_schedule_periodic_dis(void);

/**
 * Cancel scheduled leaving if any
*/
void rpl_timers_unschedule_leaving(void);

/**
 * Schedule leaving after RPL_DELAY_BEFORE_LEAVING
*/
void rpl_timers_schedule_leaving(void);

/**
 * Initialize rpl-timers module
*/
void rpl_timers_init(void);

/**
 * Stop all timers related to the DAG
*/
void rpl_timers_stop_dag_timers(void);

/**
 * Reset DIO Trickle timer
 *
 * \param str A textual description of caused the DIO timer reset
*/
void rpl_timers_dio_reset(const char *str);

/**
 * Schedule unicast DIO with no delay
*/
void rpl_timers_schedule_unicast_dio(rpl_nbr_t *target);

/**
 * Schedule a DAO with random delay based on RPL_DAO_DELAY
*/
void rpl_timers_schedule_dao(void);

/**
 * Schedule a DAO-ACK with no delay
*/
void rpl_timers_schedule_dao_ack(uip_ipaddr_t *target, uint16_t sequence);

/**
 * Let the rpl-timers module know that the last DAO was ACKed
*/
void rpl_timers_notify_dao_ack(void);

/**
 * Schedule probing with delay RPL_PROBING_DELAY_FUNC()
*/
void rpl_schedule_probing(void);

/**
 * Schedule probing within a few seconds
*/
void rpl_schedule_probing_now(void);

/**
 * Schedule a state update ASAP. Useful to force an update from a context
 * where updating directly would be unsafe.
*/
void rpl_timers_schedule_state_update(void);

/**
 * Cancelled any scheduled state update.
*/
void rpl_timers_unschedule_state_update(void);

 /** @} */

#endif /* RPL_TIMERS_H */
