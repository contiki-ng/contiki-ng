/* SNMP protocol
 *
 * Copyright (C) 2008 Robert Ernst <robert.ernst@linux-solutions.at>
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
 * \addtogroup apps
 * @{
 *
 * \defgroup snmp SNMP (Simple Network Management Protocol)
 * @{
 *
 * This is an implementation of the Simple Network Management Protocol
 */

#ifndef SNMP_H_
#define SNMP_H_

#include "contiki.h"
#include "contiki-net.h"

#include "sys/log.h"
#include "sys/energest.h"

#include "snmp_conf.h"

#include <stddef.h> /* for size_t */

/**
 * \defgroup SNMPDefines SNMP dependent defines
 * @{
 */
/**
 * \brief SNMP Version 1
 */
#define SNMP_VERSION_1                                  0
/**
 * \brief SNMP Version 2C
 */
#define SNMP_VERSION_2C                                 1
/**
 * \brief SNMP Version 3
 */
#define SNMP_VERSION_3                                  3
/**
 * \brief The agent reports that no errors occurred during transmission.
 */
#define SNMP_STATUS_OK                                  0
/**
 * \brief The agent could not place the results of the requested SNMP operation in a single SNMP message.
 */
#define SNMP_STATUS_TOO_BIG                             1
/**
 * \brief The requested SNMP operation identified an unknown variable.
 */
#define SNMP_STATUS_NO_SUCH_NAME                        2
/**
 * \brief The requested SNMP operation tried to change a variable but it specified either a syntax or value error.
 */
#define SNMP_STATUS_BAD_VALUE                           3
/**
 * \brief The requested SNMP operation tried to change a variable that was not allowed to change, according to the community profile of the variable.
 */
#define SNMP_STATUS_READ_ONLY                           4
/**
 * \brief An error other than one of those listed here occurred during the requested SNMP operation.
 */
#define SNMP_STATUS_GEN_ERR                             5
/**
 * \brief The specified SNMP variable is not accessible.
 */
#define SNMP_STATUS_NO_ACCESS                           6
/**
 * \brief The value specifies a type that is inconsistent with the type required for the variable.
 */
#define SNMP_STATUS_WRONG_TYPE                          7
/**
 * \brief The value specifies a length that is inconsistent with the length required for the variable.
 */
#define SNMP_STATUS_WRONG_LENGTH                        8
/**
 * \brief The value contains an Abstract Syntax Notation One (ASN.1) encoding that is inconsistent with the ASN.1 tag of the field.
 */
#define SNMP_STATUS_WRONG_ENCODING                      9
/**
 * \brief The value cannot be assigned to the variable.
 */
#define SNMP_STATUS_WRONG_VALUE                         10
/**
 * \brief The variable does not exist, and the agent cannot create it.
 */
#define SNMP_STATUS_NO_CREATION                         11
/**
 * \brief The value is inconsistent with values of other managed objects.
 */
#define SNMP_STATUS_INCONSISTENT_VALUE                  12
/**
 * \brief Assigning the value to the variable requires allocation of resources that are currently unavailable.
 */
#define SNMP_STATUS_RESOURCE_UNAVAILABLE                13
/**
 * \brief No validation errors occurred, but no variables were updated.
 */
#define SNMP_STATUS_COMMIT_FAILED                       14
/**
 * \brief No validation errors occurred. Some variables were updated because it was not possible to undo their assignment.
 */
#define SNMP_STATUS_UNDO_FAILED                         15
/**
 * \brief An authorization error occurred.
 */
#define SNMP_STATUS_AUTHORIZATION_ERROR                 16
/**
 * \brief The variable exists but the agent cannot modify it.
 */
#define SNMP_STATUS_NOT_WRITABLE                        17
/**
 * \brief The variable does not exist; the agent cannot create it because the named object instance is inconsistent with the values of other managed objects.
 */
#define SNMP_STATUS_INCONSISTENT_NAME                   18
/*@}*/

#ifndef UNUSED
#define UNUSED(x) x __attribute__((unused))
#endif

/**
 * \defgroup SNMPDataTypes SNMP Data types
 * @{
 */
/**
 * @brief Client struct
 */
typedef struct client_s {
  /**
   * @brief Packet buffer
   */
  unsigned char packet[SNMP_MAX_PACKET_SIZE];
  /**
   * @brief Used size packet
   */
  size_t size;
} snmp_client_t;

/**
 * @brief Oid struct
 */
typedef struct oid_s {
  /**
   * @brief List of sub ids
   */
  unsigned int subid_list[SNMP_MAX_NR_SUBIDS];
  /**
   * @brief Number of sud ids
   */
  size_t subid_list_length;
  /**
   * @brief Encoded length of the oids
   */
  short encoded_length;
} snmp_oid_t;

/**
 * @brief Data struct
 */
typedef struct data_s {
  /**
   * @brief Buffer for the data
   */
  unsigned char buffer[(SNMP_MAX_NR_SUBIDS * 5) + 4];
  /**
   * @brief Max length of the data
   */
  size_t max_length;
  /**
   * @brief Encoded length of the data
   */
  short encoded_length;
} snmp_data_t;

/**
 * @brief Value struct
 */
typedef struct value_s {
  /**
   * @brief The OID for this data
   */
  snmp_oid_t oid;
  /**
   * @brief The data for this OID
   */
  snmp_data_t data;
} snmp_value_t;

/**
 * @brief Request struct
 */
typedef struct request_s {
  /**
   * @brief Community in request
   */
  char community[SNMP_MAX_COMMUNITY_SIZE];
  /**
   * @brief Type of request
   */
  int type;
  /**
   * @brief Protocol version
   */
  int version;
  /**
   * @brief Request ID
   */
  int id;
  /**
   * @brief How many Oid's in the request should be treated as Get request variables
   */
  uint32_t non_repeaters;
  /**
   * @brief How many GetNext operations to perform on each request variable
   */
  uint32_t max_repetitions;
  /**
   * @brief Oid list
   */
  snmp_oid_t oid_list[SNMP_MAX_NR_OIDS];
  /**
   * @brief Length of the oids list
   */
  size_t oid_list_length;
} request_t;

/**
 * @brief
 */
typedef struct response_s {
  /**
   * @brief
   */
  int error_status;
  /**
   * @brief
   */
  int error_index;
  /**
   * @brief List of values
   */
  snmp_value_t value_list[SNMP_MAX_NR_VALUES];
  /**
   * @brief Length of values list
   */
  size_t value_list_length;
} response_t;
/*@}*/

/**
 * \defgroup SNMPFunctions SNMP Functions
 * @{
 */

/**
 * @brief Initilize the SNMP processes and variables
 */
void snmp_init();

/*@}*/

#endif /* SNMP_H_ */
/** @} */
/** @} */
