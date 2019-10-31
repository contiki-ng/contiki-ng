/*
 * Copyright (C) 2019 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 *
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
/*---------------------------------------------------------------------------*/

/**
 * \file
 *      SNMP Configurable Macros
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

/**
 * \addtogroup snmp
 * @{
 */

#ifndef SNMP_CONF_H_
#define SNMP_CONF_H_

/**
 * \defgroup SNMPConfs SNMP Configurable Defines
 * @{
 */

#ifdef SNMP_CONF_COMMUNITY
/**
 * \brief Configurable SNMP Community
 */
#define SNMP_COMMUNITY SNMP_CONF_COMMUNITY
#else
/**
 * \brief Default SNMP Community
 */
#define SNMP_COMMUNITY "public"
#endif

#ifdef SNMP_CONF_MSG_OID_MAX_LEN
/**
 * \brief Configurable maximum number of IDs in one OID
 */
#define SNMP_MSG_OID_MAX_LEN SNMP_CONF_MSG_OID_MAX_LEN
#else
/**
 * \brief Default maximum number of IDs in one OID
 */
#define SNMP_MSG_OID_MAX_LEN 16
#endif

#ifdef SNMP_CONF_MAX_NR_VALUES
/**
 * \brief Configurable maximum number of OIDs in one response
 */
#define SNMP_MAX_NR_VALUES SNMP_CONF_MAX_NR_VALUES
#else
/**
 * \brief Default maximum number of OIDs in one response
 */
#define SNMP_MAX_NR_VALUES 2
#endif

#ifdef SNMP_CONF_MAX_PACKET_SIZE
/**
 * \brief Configurable maximum size of the packet in bytes
 */
#define SNMP_MAX_PACKET_SIZE SNMP_CONF_MAX_PACKET_SIZE
#else
/**
 * \brief Default maximum size of the packet in bytes
 */
#define SNMP_MAX_PACKET_SIZE 512
#endif

#ifdef SNMP_CONF_PORT
/**
 * \brief Configurable SNMP port
 */
#define SNMP_PORT SNMP_CONF_PORT
#else
/**
 * \brief Default SNMP port
 */
#define SNMP_PORT 161
#endif

/*@}*/

#endif /* SNMP_CONF_H_ */
/** @} */
