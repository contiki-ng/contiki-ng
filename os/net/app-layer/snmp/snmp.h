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

#include "snmp-conf.h"

#include <stdlib.h>
#include <stdint.h>

/**
 * \defgroup SNMPDefine SNMP Defines
 * @{
 */

/**
 * @brief SNMP Version 1 code
 */
#define SNMP_VERSION_1  0
/**
 * @brief SNMP Version 2c code
 */
#define SNMP_VERSION_2C 1

/**
 * @brief SNMP No Such Name error code
 */
#define SNMP_STATUS_NO_SUCH_NAME 2

/** @} */

/**
 * \defgroup SNMPStructs SNMP Structs
 * @{
 */

/**
 * @brief The SNMP header struct
 */
typedef struct snmp_header_s {
  /**
   * @brief SNMP Version
   */
  uint32_t version;
  /**
   * @brief Struct to wrap the community
   */
  struct snmp_msg_community {
    /**
     * @brief A pointer to the community
     *
     * @remarks This pointer refers to the beginning of the string in the packet
     */
    const char *community;
    /**
     * @brief The string length
     *
     * @remarks Do not use strlen on the community pointer since it is not null terminated
     */
    uint32_t length;
  } community;
  /**
   * @brief The PDU type
   */
  uint8_t pdu_type;
  /**
   * @brief The request ID
   */
  uint32_t request_id;
  /**
   * @brief Union to hold the error status or the non repeaters
   *
   * @remarks A union was used since these values cannot co-exist
   */
  union error_status_non_repeaters_u {
    /**
     * @brief The error status
     */
    uint32_t error_status;
    /**
     * @brief The non repeaters
     */
    uint32_t non_repeaters;
  } error_status_non_repeaters;
  /**
   * @brief Union to hold the error index or the max repetitions
   *
   * @remarks A union was used since these values cannot co-exist
   */
  union error_index_max_repetitions_u {
    /**
     * @brief The error index
     */
    uint32_t error_index;
    /**
     * @brief The max repetitions
     */
    uint32_t max_repetitions;
  } error_index_max_repetitions;
} snmp_header_t;

/**
 * @brief The varbind struct
 */
typedef struct snmp_varbind_s {
  /**
   * @brief The OID
   *
   * @remarks The length is configurable
   */
  uint32_t oid[SNMP_MSG_OID_MAX_LEN];
  /**
   * @brief The type in this varbind
   */
  uint8_t value_type;
  /**
   * @brief A union to represent the value in this varbind
   *
   * @remarks A union is used since the varbind can only have one value of one type
   */
  union snmp_varbind_val_u {
    /**
     * @brief The integer value
     */
    uint32_t integer;
    /**
     * @brief A struct that contains the string
     */
    struct snmp_varbind_string_s {
      /**
       * @brief A pointer to the string value from this varbind
       *
       * @remarks This pointer points to a string that cannot be changed
       */
      const char *string;
      /**
       * @brief The string length
       *
       * @remarks Do not use strlen on the string since it might not be null terminated
       */
      uint32_t length;
    } string;
    /**
     * @brief A pointer to the beggining of a oid array
     */
    uint32_t *oid;
  } value;
} snmp_varbind_t;

/** @}*/

/**
 * \defgroup SNMPFunctions SNMP Functions
 * @{
 */

/**
 * @brief Initializes the SNMP engine
 */
void
snmp_init();

/** @}*/

#endif /* SNMP_H_ */
/** @} */
/** @} */
