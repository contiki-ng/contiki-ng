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
 * \addtogroup tsch
 * @{
 * \file
 *	TSCH-RPL interaction
*/

#ifndef __TSCH_RPL_H__
#define __TSCH_RPL_H__

/********** Includes **********/

#include "net/routing/routing.h"
#if ROUTING_CONF_RPL_LITE
#include "net/routing/rpl-lite/rpl.h"
#elif ROUTING_CONF_RPL_CLASSIC
#include "net/routing/rpl-classic/rpl.h"
#endif

/********** Functions *********/

/**
* \brief Report statiscs from KA packet sent in RPL.
* To use, set TSCH_CALLBACK_KA_SENT to tsch_rpl_callback_ka_sent
* \param status The packet sent status
* \param transmissions The total number of transmissions
*/
void tsch_rpl_callback_ka_sent(int status, int transmissions);
/**
 * \brief Let RPL know that TSCH joined a new network.
 * To use, set TSCH_CALLBACK_JOINING_NETWORK to tsch_rpl_callback_joining_network
 */
void tsch_rpl_callback_joining_network(void);
/**
 * \brief Let RPL know that TSCH joined a new network. Triggers a local repair.
 * To use, set TSCH_CALLBACK_LEAVING_NETWORK to tsch_rpl_callback_leaving_network
 */
void tsch_rpl_callback_leaving_network(void);
/**
 * \brief Set TSCH EB period based on current RPL DIO period.
 * To use, set RPL_CALLBACK_NEW_DIO_INTERVAL to tsch_rpl_callback_new_dio_interval
 * \param dio_interval The new DIO interval in clock ticks
 */
void tsch_rpl_callback_new_dio_interval(clock_time_t dio_interval);
/**
 * \brief Set TSCH time source based on current RPL preferred parent.
 * To use, set RPL_CALLBACK_PARENT_SWITCH to tsch_rpl_callback_parent_switch
 * \param old The old RPL parent
 * \param new The new RPL parent
 */
void tsch_rpl_callback_parent_switch(rpl_parent_t *old, rpl_parent_t *new);
/**
 * \brief Check RPL has joined DODAG.
 * To use, set TSCH_RPL_CHECK_DODAG_JOINED tsch_rpl_check_dodag_joined
 * \return 1 if joined, 0 otherwise
 */
int tsch_rpl_check_dodag_joined(void);

#endif /* __TSCH_RPL_H__ */
/** @} */
