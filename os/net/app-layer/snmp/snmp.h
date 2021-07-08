/*
 * Copyright (C) 2019-2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
 *      SNMP Implementation of the process
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

/**
 * \addtogroup apps
 * @{
 *
 * \addtogroup snmp An implementation of SNMP
 * @{
 *
 * This is an implementation of the Simple Network Management Protocol
 */

#ifndef SNMP_H_
#define SNMP_H_

/**
 * \addtogroup SNMPInternal SNMP Internal API
 * @{
 *
 * This group contains all the functions that can be used inside the OS level.
 */

#include "contiki.h"

#include "sys/log.h"

#include "snmp-conf.h"

#include <stdlib.h>
#include <stdint.h>

/**
 * \addtogroup SNMPCore SNMP Core
 * @{
 *
 * This group contains the SNMP MIB implementation
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
   * @brief The error status
   */
  uint32_t error_status;
  /**
   * @brief The non repeaters
   */
  uint32_t non_repeaters;
  /**
   * @brief The error index
   */
  uint32_t error_index;
  /**
   * @brief The max repetitions
   */
  uint32_t max_repetitions;
} snmp_header_t;

/**
 * @brief The OID struct
 */
typedef struct snmp_oid_s {
  /**
   * @brief The OID
   */
  uint32_t data[SNMP_MSG_OID_MAX_LEN];
  /**
   * @brief The OID length
   *
   */
  uint8_t length;
} snmp_oid_t;

/**
 * @brief The varbind struct
 */
typedef struct snmp_varbind_s {
  /**
   * @brief The OID
   */
  snmp_oid_t oid;
  /**
   * @brief The type in this varbind
   */
  uint8_t value_type;
  /**
   * @brief A union to represent the value in this varbind
   *
   * @remarks A union is used since the varbind can only have one value of one type
   */
  union {
    /**
     * @brief The integer value
     */
    uint32_t integer;
    /**
     * @brief A struct that contains the string
     */
    struct {
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
     * @brief The OID value
     */
    snmp_oid_t oid;
  } value;
} snmp_varbind_t;

/**
 * @brief The packet struct
 *
 */
typedef struct {
  /**
   * @brief The number of bytes used
   *
   */
  uint16_t used;
  /**
   * @brief The maximum number of bytes
   *
   */
  uint16_t max;
  /**
   * @brief The pointer used for the incoming packet
   *
   */
  uint8_t *in;
  /**
   * @brief The pointer used for the outgoing packet
   *
   */
  uint8_t *out;
} snmp_packet_t;

/**
 * @brief Initializes the SNMP engine
 */
void
snmp_init();

/** @}*/

/** @}*/

#endif /* SNMP_H_ */

/** @} */

/** @} */
