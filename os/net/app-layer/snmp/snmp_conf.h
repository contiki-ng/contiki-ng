/* SNMP protocol
 *
 * Copyright (C) 2019 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See COPYING for GPL licensing information.
 */

/**
 * \file
 *      An implementation of the Simple Network Management Protocol (RFC 3411-3418)
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

/**
 * \addtogroup snmp
 * @{
 *
 * This is the SNMP configurable defines
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

#ifdef SNMP_CONF_DEBUG
/**
 * \brief Configurable SNMP Debug Mode
 */
#define SNMP_DEBUG SNMP_CONF_DEBUG
#else
/**
 * \brief Default SNMP Debug Mode
 */
#define SNMP_DEBUG 1
#endif

#ifdef SNMP_CONF_MAX_NR_OIDS
/**
 * \brief Configurable maximum number of OIDs in one request
 */
#define SNMP_MAX_NR_OIDS SNMP_CONF_MAX_NR_OIDS
#else
/**
 * \brief Default maximum number of OIDs in one request
 */
#define SNMP_MAX_NR_OIDS 16
#endif

#ifdef SNMP_CONF_MAX_NR_SUBIDS
/**
 * \brief Configurable maximum number of IDs in one OID
 */
#define SNMP_MAX_NR_SUBIDS SNMP_CONF_MAX_NR_SUBIDS
#else
/**
 * \brief Default maximum number of IDs in one OID
 */
#define SNMP_MAX_NR_SUBIDS 16
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
#define SNMP_MAX_NR_VALUES 6
#endif

#ifdef SNMP_CONF_MAX_MIB_SIZE
/**
 * \brief Configurable maximum MIB size
 */
#define SNMP_MAX_MIB_SIZE SNMP_CONF_MAX_MIB_SIZE
#else
/**
 * \brief Default maximum MIB size
 */
#define SNMP_MAX_MIB_SIZE 12
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
#define SNMP_MAX_PACKET_SIZE 2048
#endif

#ifdef SNMP_CONF_MAX_COMMUNITY_SIZE
/**
 * \brief Configurable maximum size of the community string
 */
#define SNMP_MAX_COMMUNITY_SIZE SNMP_CONF_MAX_COMMUNITY_SIZE
#else
/**
 * \brief Default maximum size of the community string
 */
#define SNMP_MAX_COMMUNITY_SIZE 64
#endif

#ifdef SNMP_CONF_MIB_UPDATER_INTERVAL
/**
 * \brief Configurable interval between MIB updates (in seconds)
 */
#define SNMP_MIB_UPDATER_INTERVAL SNMP_CONF_MIB_UPDATER_INTERVAL
#else
/**
 * \brief Default interval between MIB updates (in seconds)
 */
#define SNMP_MIB_UPDATER_INTERVAL 60
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
