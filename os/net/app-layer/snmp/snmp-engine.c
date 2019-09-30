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

#include "snmp-engine.h"
#include "snmp-message.h"
#include "snmp-mib.h"
#include "snmp-oid.h"

#define LOG_MODULE "SNMP [engine]"
#define LOG_LEVEL LOG_LEVEL_SNMP

/*---------------------------------------------------------------------------*/
int
snmp_engine_get(snmp_header_t *header, snmp_varbind_t *varbinds, uint32_t varbinds_length)
{
  snmp_mib_resource_t *resource;
  uint32_t i;

  for(i = 0; i < varbinds_length; i++) {
    resource = snmp_mib_find(varbinds[i].oid);
    if(!resource) {
      switch(header->version) {
      case SNMP_VERSION_1:
        header->error_status_non_repeaters.error_status = SNMP_STATUS_NO_SUCH_NAME;
        /*
         * Varbinds are 1 indexed
         */
        header->error_index_max_repetitions.error_index = i + 1;
        break;
      case SNMP_VERSION_2C:
        (&varbinds[i])->value_type = SNMP_DATA_TYPE_NO_SUCH_INSTANCE;
        break;
      default:
        header->error_status_non_repeaters.error_status = SNMP_STATUS_NO_SUCH_NAME;
        header->error_index_max_repetitions.error_index = 0;
      }
    } else {
      resource->handler(&varbinds[i], resource->oid);
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int
snmp_engine_get_next(snmp_header_t *header, snmp_varbind_t *varbinds, uint32_t varbinds_length)
{
  snmp_mib_resource_t *resource;
  uint32_t i;

  for(i = 0; i < varbinds_length; i++) {
    resource = snmp_mib_find_next(varbinds[i].oid);
    if(!resource) {
      switch(header->version) {
      case SNMP_VERSION_1:
        header->error_status_non_repeaters.error_status = SNMP_STATUS_NO_SUCH_NAME;
        /*
         * Varbinds are 1 indexed
         */
        header->error_index_max_repetitions.error_index = i + 1;
        break;
      case SNMP_VERSION_2C:
        (&varbinds[i])->value_type = SNMP_DATA_TYPE_END_OF_MIB_VIEW;
        break;
      default:
        header->error_status_non_repeaters.error_status = SNMP_STATUS_NO_SUCH_NAME;
        header->error_index_max_repetitions.error_index = 0;
      }
    } else {
      resource->handler(&varbinds[i], resource->oid);
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
int
snmp_engine_get_bulk(snmp_header_t *header, snmp_varbind_t *varbinds, uint32_t *varbinds_length)
{
  snmp_mib_resource_t *resource;
  uint32_t i, j, original_varbinds_length;
  uint32_t oid[SNMP_MAX_NR_VALUES][SNMP_MSG_OID_MAX_LEN];
  uint8_t repeater;

  /*
   * A local copy of the requested oids must be kept since
   *  the varbinds are modified on the fly
   */
  original_varbinds_length = *varbinds_length;
  for(i = 0; i < original_varbinds_length; i++) {
    snmp_oid_copy(oid[i], varbinds[i].oid);
  }

  *varbinds_length = 0;
  for(i = 0; i < original_varbinds_length; i++) {
    if(i >= header->error_status_non_repeaters.non_repeaters) {
      break;
    }

    resource = snmp_mib_find_next(oid[i]);
    if(!resource) {
      switch(header->version) {
      case SNMP_VERSION_1:
        header->error_status_non_repeaters.error_status = SNMP_STATUS_NO_SUCH_NAME;
        /*
         * Varbinds are 1 indexed
         */
        header->error_index_max_repetitions.error_index = i + 1;
        break;
      case SNMP_VERSION_2C:
        (&varbinds[i])->value_type = SNMP_DATA_TYPE_END_OF_MIB_VIEW;
        break;
      default:
        header->error_status_non_repeaters.error_status = SNMP_STATUS_NO_SUCH_NAME;
        header->error_index_max_repetitions.error_index = 0;
      }
    } else {
      if(*varbinds_length < SNMP_MAX_NR_VALUES) {
        resource->handler(&varbinds[*varbinds_length], resource->oid);
        (*varbinds_length)++;
      }
    }
  }

  for(i = 0; i < header->error_index_max_repetitions.max_repetitions; i++) {
    repeater = 0;
    for(j = header->error_status_non_repeaters.non_repeaters; j < original_varbinds_length; j++) {
      resource = snmp_mib_find_next(oid[j]);
      if(!resource) {
        switch(header->version) {
        case SNMP_VERSION_1:
          header->error_status_non_repeaters.error_status = SNMP_STATUS_NO_SUCH_NAME;
          /*
           * Varbinds are 1 indexed
           */
          header->error_index_max_repetitions.error_index = *varbinds_length + 1;
          break;
        case SNMP_VERSION_2C:
          if(*varbinds_length < SNMP_MAX_NR_VALUES) {
            (&varbinds[*varbinds_length])->value_type = SNMP_DATA_TYPE_END_OF_MIB_VIEW;
            snmp_oid_copy((&varbinds[*varbinds_length])->oid, oid[j]);
            (*varbinds_length)++;
          }
          break;
        default:
          header->error_status_non_repeaters.error_status = SNMP_STATUS_NO_SUCH_NAME;
          header->error_index_max_repetitions.error_index = 0;
        }
      } else {
        if(*varbinds_length < SNMP_MAX_NR_VALUES) {
          resource->handler(&varbinds[*varbinds_length], resource->oid);
          (*varbinds_length)++;
          snmp_oid_copy(oid[j], resource->oid);
          repeater++;
        }
      }
    }
    if(repeater == 0) {
      break;
    }
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
unsigned char *
snmp_engine(unsigned char *buff, uint32_t buff_len, unsigned char *out, uint32_t *out_len)
{
  static snmp_header_t header;
  static snmp_varbind_t varbinds[SNMP_MAX_NR_VALUES];
  static uint32_t varbind_length;

  buff = snmp_message_decode(buff, buff_len, &header, varbinds, &varbind_length);
  if(buff == NULL) {
    return NULL;
  }

  if(header.version != SNMP_VERSION_1) {
    if(strncmp(header.community.community, SNMP_COMMUNITY, header.community.length)) {
      LOG_ERR("Request with invalid community\n");
      return NULL;
    }
  }

  /*
   * Now handle the SNMP requests depending on their type
   */
  switch(header.pdu_type) {
  case SNMP_DATA_TYPE_PDU_GET_REQUEST:
    if(snmp_engine_get(&header, varbinds, varbind_length) == -1) {
      return NULL;
    }
    break;

  case SNMP_DATA_TYPE_PDU_GET_NEXT_REQUEST:
    if(snmp_engine_get_next(&header, varbinds, varbind_length) == -1) {
      return NULL;
    }
    break;

  case SNMP_DATA_TYPE_PDU_GET_BULK:
    if(snmp_engine_get_bulk(&header, varbinds, &varbind_length) == -1) {
      return NULL;
    }
    break;

  default:
    LOG_ERR("Invalid request type");
    return NULL;
  }

  header.pdu_type = SNMP_DATA_TYPE_PDU_GET_RESPONSE;
  out = snmp_message_encode(out, out_len, &header, varbinds, varbind_length);

  return ++out;
}
