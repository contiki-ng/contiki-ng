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

#include "contiki.h"

#include "snmp-api.h"

#include "snmp-message.h"
#include "snmp-ber.h"
#include "snmp-oid.h"

static void
snmp_api_replace_oid(snmp_varbind_t *varbind, uint32_t *oid)
{
  uint8_t i;

  i = 0;
  while(oid[i] != ((uint32_t)-1)) {
    varbind->oid[i] = oid[i];
    i++;
  }
  varbind->oid[i] = ((uint32_t)-1);
}
/*---------------------------------------------------------------------------*/
void
snmp_api_set_string(snmp_varbind_t *varbind, uint32_t *oid, char *string)
{

  snmp_api_replace_oid(varbind, oid);
  varbind->value_type = BER_DATA_TYPE_OCTET_STRING;
  varbind->value.string.string = string;
  varbind->value.string.length = strlen(string);
}
/*---------------------------------------------------------------------------*/
void
snmp_api_set_time_ticks(snmp_varbind_t *varbind, uint32_t *oid, uint32_t integer)
{

  snmp_api_replace_oid(varbind, oid);
  varbind->value_type = SNMP_DATA_TYPE_TIME_TICKS;
  varbind->value.integer = integer;
}
/*---------------------------------------------------------------------------*/
void
snmp_api_set_oid(snmp_varbind_t *varbind, uint32_t *oid, uint32_t *ret_oid)
{

  snmp_api_replace_oid(varbind, oid);
  varbind->value_type = BER_DATA_TYPE_OID;
  varbind->value.oid = ret_oid;
}
/*---------------------------------------------------------------------------*/
void
snmp_api_add_resource(snmp_mib_resource_t *new_resource)
{
  return snmp_mib_add(new_resource);
}
