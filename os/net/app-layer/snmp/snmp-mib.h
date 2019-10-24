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
 * \addtogroup snmp
 * @{
 */

#ifndef SNMP_MIB_H_
#define SNMP_MIB_H_

#include "snmp.h"

/**
 * @brief The MIB resource handler typedef
 *
 * @param varbind The varbind that is being changed
 * @param oid The oid from the resource
 */
typedef void (*snmp_mib_resource_handler_t)(snmp_varbind_t *varbind, uint32_t *oid);

/**
 * @brief The MIB Resource struct
 */
typedef struct snmp_mib_resource_s {
  /**
   * @brief A pointer to the next element in the linked list
   *
   * @remarks This MUST be the first element in the struct
   */
  struct snmp_mib_resource_s *next;
  /**
   * @brief A array that represents the OID
   *
   * @remarks This array is "null" terminated. In this case the -1 is used.
   */
  uint32_t *oid;
  /**
   * @brief The function handler that is called for this resource
   */
  snmp_mib_resource_handler_t handler;
} snmp_mib_resource_t;

/**
 * @brief Finds the MIB Resource for this OID
 *
 * @param oid The OID
 *
 * @return In case of success a pointer to the resouce or NULL in case of fail
 */
snmp_mib_resource_t *
snmp_mib_find(uint32_t *oid);

/**
 * @brief Finds the next MIB Resource after this OID
 *
 * @param oid The OID
 *
 * @return In case of success a pointer to the resouce or NULL in case of fail
 */
snmp_mib_resource_t *
snmp_mib_find_next(uint32_t *oid);

/**
 * @brief Adds a resource into the linked list
 *
 * @param resource The resource
 */
void
snmp_mib_add(snmp_mib_resource_t *resource);

/**
 * @brief Initialize the MIB resources list
 */
void
snmp_mib_init(void);

#endif /* SNMP_MIB_H_ */
/** @} */
