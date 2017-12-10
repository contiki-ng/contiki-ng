/*
 * Copyright (c) 2017, Inria.
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
 */

/**
 * \addtogroup rpl-lite
 * @{
 *
 * \file
 *         Header file for rpl-ext-header
 *
 * \author Simon Duquennoy <simon.duquennoy@inria.fr>
 */

 #ifndef RPL_EXT_HEADER_H_
 #define RPL_EXT_HEADER_H_

/********** Public functions **********/

/**
* Look for next hop from SRH of current uIP packet.
*
* \param ipaddr A pointer to the address where to store the next hop.
* \return 1 if a next hop was found, 0 otherwise
*/
int rpl_ext_header_srh_get_next_hop(uip_ipaddr_t *ipaddr);

/**
* Process and update SRH in-place,
* i.e. internal address swapping as per RFC6554
* \return 1 if SRH found, 0 otherwise
*/
int rpl_ext_header_srh_update(void);

/**
* Process and update the RPL hop-by-hop extension headers of
* the current uIP packet.
*
* \param uip_ext_opt_offset The offset within the uIP packet where
* extension headers start
* \return 1 in case the packet is valid and to be processed further,
* 0 in case the packet must be dropped.
*/
int rpl_ext_header_hbh_update(int uip_ext_opt_offset);

/**
 * Adds/updates all RPL extension headers to current uIP packet.
 *
 * \return 1 in case of success, 0 otherwise
*/
int rpl_ext_header_update(void);

/**
 * Removes all RPL extension headers.
*/
void rpl_ext_header_remove(void);

 /** @} */

#endif /* RPL_EXT_HEADER_H_ */
