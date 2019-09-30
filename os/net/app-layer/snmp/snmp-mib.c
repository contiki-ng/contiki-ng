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

#include "snmp-mib.h"
#include "snmp-oid.h"
#include "lib/list.h"

#define LOG_MODULE "SNMP [mib]"
#define LOG_LEVEL LOG_LEVEL_SNMP

LIST(snmp_mib);

snmp_mib_resource_t *
snmp_mib_find(uint32_t *oid)
{
  snmp_mib_resource_t *resource;

  resource = NULL;
  for(resource = list_head(snmp_mib);
      resource; resource = resource->next) {

    if(!snmp_oid_cmp_oid(oid, resource->oid)) {
      return resource;
    }
  }

  return NULL;
}
snmp_mib_resource_t *
snmp_mib_find_next(uint32_t *oid)
{
  snmp_mib_resource_t *resource;

  resource = NULL;
  for(resource = list_head(snmp_mib);
      resource; resource = resource->next) {

    if(snmp_oid_cmp_oid(resource->oid, oid) > 0) {
      return resource;
    }
  }

  return NULL;
}
void
snmp_mib_add(snmp_mib_resource_t *new_resource)
{
  snmp_mib_resource_t *resource;

  for(resource = list_head(snmp_mib);
      resource; resource = resource->next) {

    if(snmp_oid_cmp_oid(resource->oid, new_resource->oid) > 0) {
      break;
    }
  }
  if(resource == NULL) {
    list_add(snmp_mib, new_resource);
  } else {
    list_insert(snmp_mib, new_resource, resource);
  }

#if LOG_LEVEL == LOG_LEVEL_DBG
  /*
   * We print the entire resource table
   */
  LOG_DBG("Table after insert.\n");
  for(resource = list_head(snmp_mib);
      resource; resource = resource->next) {

    snmp_oid_print(resource->oid);
  }
#endif /* LOG_LEVEL == LOG_LEVEL_DBG  */
}
void
snmp_mib_init(void)
{
  list_init(snmp_mib);
}
