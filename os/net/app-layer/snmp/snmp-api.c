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

#include "contiki.h"

#include "snmp-api.h"

#include "snmp-message.h"
#include "snmp-ber.h"

/*---------------------------------------------------------------------------*/
void
snmp_api_set_string(snmp_varbind_t *varbind, snmp_oid_t *oid, char *string)
{
  memcpy(&varbind->oid, oid, sizeof(snmp_oid_t));
  varbind->value_type = BER_DATA_TYPE_OCTET_STRING;
  varbind->value.string.string = string;
  varbind->value.string.length = strlen(string);
}
/*---------------------------------------------------------------------------*/
void
snmp_api_set_time_ticks(snmp_varbind_t *varbind, snmp_oid_t *oid, uint32_t integer)
{
  memcpy(&varbind->oid, oid, sizeof(snmp_oid_t));
  varbind->value_type = BER_DATA_TYPE_TIMETICKS;
  varbind->value.integer = integer;
}
/*---------------------------------------------------------------------------*/
void
snmp_api_set_oid(snmp_varbind_t *varbind, snmp_oid_t *oid, snmp_oid_t *ret_oid)
{
  memcpy(&varbind->oid, oid, sizeof(snmp_oid_t));
  varbind->value_type = BER_DATA_TYPE_OBJECT_IDENTIFIER;
  memcpy(&varbind->value.oid, ret_oid, sizeof(snmp_oid_t));
}
/*---------------------------------------------------------------------------*/
void
snmp_api_add_resource(snmp_mib_resource_t *new_resource)
{
  return snmp_mib_add(new_resource);
}
