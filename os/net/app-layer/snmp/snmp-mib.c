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
 *      SNMP Implementation of the MIB
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#include "contiki.h"

#include "snmp-mib.h"
#include "lib/list.h"

#define LOG_MODULE "SNMP [mib]"
#define LOG_LEVEL LOG_LEVEL_SNMP

LIST(snmp_mib);

/*---------------------------------------------------------------------------*/
/**
 * @brief Compares to oids
 *
 * @param oid1 First Oid
 * @param oid2 Second Oid
 *
 * @return < 0 if oid1 < oid2, > 0 if oid1 > oid2 and 0 if they are equal
 */
static inline int
snmp_mib_cmp_oid(snmp_oid_t *oid1, snmp_oid_t *oid2)
{
  uint8_t i;

  i = 0;
  while(i < oid1->length && i < oid2->length) {
    if(oid1->data[i] != oid2->data[i]) {
      if(oid1->data[i] < oid2->data[i]) {
        return -1;
      }
      return 1;
    }
    i++;
  }

  if(i == oid1->length &&
     i < oid2->length) {
    return -1;
  }

  if(i < oid1->length &&
     i == oid2->length) {
    return 1;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
snmp_mib_resource_t *
snmp_mib_find(snmp_oid_t *oid)
{
  snmp_mib_resource_t *resource;

  resource = NULL;
  for(resource = list_head(snmp_mib);
      resource; resource = resource->next) {

    if(!snmp_mib_cmp_oid(oid, &resource->oid)) {
      return resource;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
snmp_mib_resource_t *
snmp_mib_find_next(snmp_oid_t *oid)
{
  snmp_mib_resource_t *resource;

  resource = NULL;
  for(resource = list_head(snmp_mib);
      resource; resource = resource->next) {

    if(snmp_mib_cmp_oid(&resource->oid, oid) > 0) {
      return resource;
    }
  }

  return NULL;
}
/*---------------------------------------------------------------------------*/
void
snmp_mib_add(snmp_mib_resource_t *new_resource)
{
  snmp_mib_resource_t *resource;
  uint8_t i;

  for(resource = list_head(snmp_mib);
      resource; resource = resource->next) {

    if(snmp_mib_cmp_oid(&resource->oid, &new_resource->oid) > 0) {
      break;
    }
  }
  if(resource == NULL) {
    list_add(snmp_mib, new_resource);
  } else {
    list_insert(snmp_mib, new_resource, resource);
  }

  if(LOG_DBG_ENABLED) {
    /*
     * We print the entire resource table
     */
    LOG_DBG("Table after insert.\n");
    for(resource = list_head(snmp_mib);
        resource; resource = resource->next) {

      i = 0;
      LOG_DBG("{");
      while(i < resource->oid.length) {
        LOG_DBG_("%lu", (unsigned long)resource->oid.data[i]);
        i++;
        if(i < resource->oid.length) {
          LOG_DBG_(".");
        }
      }
      LOG_DBG_("}\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
void
snmp_mib_init(void)
{
  list_init(snmp_mib);
}
