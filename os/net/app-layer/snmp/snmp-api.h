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
 *      SNMP Implementation of the public API
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

/**
 * \addtogroup snmp
 * @{
 */

#ifndef SNMP_API_H_
#define SNMP_API_H_

#include "snmp.h"
#include "snmp-mib.h"

/**
 * \addtogroup SNMPAPI SNMP Public API
 * @{
 *
 * This group contains all the functions that can be used outside the OS level.
 */

/**
 * @brief Initializes statically an oid with the "null" terminator
 *
 * @remarks This should be used inside handlers when declaring an oid
 *
 * @param name A name for the oid
 * @param ... The Oid (comma-separeted)
 */
#define OID(name, ...) \
  static snmp_oid_t name = { \
    .data = { __VA_ARGS__ }, \
    .length = (sizeof((uint32_t[]){ __VA_ARGS__ }) / sizeof(uint32_t)) \
  };

/**
 * @brief Declare a MIB resource
 *
 * @param name A name for the MIB resource
 * @param handler The handler function for this resource
 * @param ... The OID (comma-separated)
 */
#define MIB_RESOURCE(name, handler, ...) \
  snmp_mib_resource_t name = { \
    NULL, \
    { \
      .data = { __VA_ARGS__ }, \
      .length = (sizeof((uint32_t[]){ __VA_ARGS__ }) / sizeof(uint32_t)) \
    }, \
    handler \
  };

/**
 * @brief Function to set a varbind with a string
 *
 * This function should be used inside a handler to set the varbind correctly
 *
 * @param varbind The varbind from the handler
 * @param oid The oid from the handler
 * @param string The string
 */
void
snmp_api_set_string(snmp_varbind_t *varbind, snmp_oid_t *oid, char *string);

/**
 * @brief Function to set a varbind with a time tick
 *
 * This function should be used inside a handler to set the varbind correctly
 *
 * @param varbind The varbind from the handler
 * @param oid The oid from the handler
 * @param integer The time tick value
 */
void
snmp_api_set_time_ticks(snmp_varbind_t *varbind, snmp_oid_t *oid, uint32_t integer);

/**
 * @brief Function to set a varbind with a oid
 *
 * This function should be used inside a handler to set the varbind correctly
 *
 * @param varbind The varbind from the handler
 * @param oid The oid from the handler
 * @param ret_oid The oid value
 */
void
snmp_api_set_oid(snmp_varbind_t *varbind, snmp_oid_t *oid, snmp_oid_t *ret_oid);

/**
 * @brief Function to add a new resource
 *
 * @param new_resource The resource
 */
void
snmp_api_add_resource(snmp_mib_resource_t *new_resource);

/** @}*/

#endif /* SNMP_API_H_ */
/** @} */
