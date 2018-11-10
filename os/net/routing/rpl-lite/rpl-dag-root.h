/*
 * Copyright (c) 2012-2014, Thingsquare, http://www.thingsquare.com/.
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
 *
 */

#ifndef RPL_DAG_ROOT_H_
#define RPL_DAG_ROOT_H_

/**
* \addtogroup rpl-lite
* @{
*
* \file
*         DAG root utility functions for RPL.
*/

/********** Public functions **********/

/**
 * Set a prefix in case the node is later set as dag root.
 *
 * \param prefix The prefix. If NULL, uip_ds6_default_prefix() is used instead
 * \param iid The IID. If NULL, it will be built from uip_ds6_set_addr_iid.
*/
void rpl_dag_root_set_prefix(uip_ipaddr_t *prefix, uip_ipaddr_t *iid);

/**
 * Set the node as root and start a DAG
 *
 * \return 0 in case of success, -1 otherwise
*/
int rpl_dag_root_start(void);

/**
 * Tells whether we are DAG root or not
 *
 * \return 1 if we are dag root, 0 otherwise
*/
int rpl_dag_root_is_root(void);
/**
 * Prints a summary of all routing links
 *
 * \param str A descriptive text on the caller
*/
void rpl_dag_root_print_links(const char *str);

 /** @} */

#endif /* RPL_DAG_ROOT_H_ */
