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
 *      SNMP Implementation of the protocol engine
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#include "contiki.h"

#include "snmp-engine.h"
#include "snmp-message.h"
#include "snmp-mib.h"
#include "snmp-ber.h"

#define LOG_MODULE "SNMP [engine]"
#define LOG_LEVEL LOG_LEVEL_SNMP

/*---------------------------------------------------------------------------*/
static inline int
snmp_engine_get(snmp_header_t *header, snmp_varbind_t *varbinds)
{
  snmp_mib_resource_t *resource;
  uint8_t i;

  i = 0;
  while(i < SNMP_MAX_NR_VALUES && varbinds[i].value_type != BER_DATA_TYPE_EOC) {
    resource = snmp_mib_find(&varbinds[i].oid);
    if(!resource) {
      switch(header->version) {
      case SNMP_VERSION_1:
        header->error_status = SNMP_STATUS_NO_SUCH_NAME;
        /*
         * Varbinds are 1 indexed
         */
        header->error_index = i + 1;
        break;
      case SNMP_VERSION_2C:
        (&varbinds[i])->value_type = BER_DATA_TYPE_NO_SUCH_INSTANCE;
        break;
      default:
        header->error_status = SNMP_STATUS_NO_SUCH_NAME;
        header->error_index = 0;
      }
    } else {
      resource->handler(&varbinds[i], &resource->oid);
    }

    i++;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static inline int
snmp_engine_get_next(snmp_header_t *header, snmp_varbind_t *varbinds)
{
  snmp_mib_resource_t *resource;
  uint8_t i;

  i = 0;
  while(i < SNMP_MAX_NR_VALUES && varbinds[i].value_type != BER_DATA_TYPE_EOC) {
    resource = snmp_mib_find_next(&varbinds[i].oid);
    if(!resource) {
      switch(header->version) {
      case SNMP_VERSION_1:
        header->error_status = SNMP_STATUS_NO_SUCH_NAME;
        /*
         * Varbinds are 1 indexed
         */
        header->error_index = i + 1;
        break;
      case SNMP_VERSION_2C:
        (&varbinds[i])->value_type = BER_DATA_TYPE_END_OF_MIB_VIEW;
        break;
      default:
        header->error_status = SNMP_STATUS_NO_SUCH_NAME;
        header->error_index = 0;
      }
    } else {
      resource->handler(&varbinds[i], &resource->oid);
    }

    i++;
  }

  return 0;
}
/*---------------------------------------------------------------------------*/
static inline int
snmp_engine_get_bulk(snmp_header_t *header, snmp_varbind_t *varbinds)
{
  snmp_mib_resource_t *resource;
  snmp_oid_t oids[SNMP_MAX_NR_VALUES];
  uint32_t j, original_varbinds_length;
  uint8_t repeater;
  uint8_t i, varbinds_length;

  /*
   * A local copy of the requested oids must be kept since
   *  the varbinds are modified on the fly
   */
  original_varbinds_length = 0;
  while(original_varbinds_length < SNMP_MAX_NR_VALUES &&
        varbinds[original_varbinds_length].value_type != BER_DATA_TYPE_EOC) {
    memcpy(&oids[original_varbinds_length], &varbinds[original_varbinds_length].oid, sizeof(snmp_oid_t));
    original_varbinds_length++;
  }

  varbinds_length = 0;
  for(i = 0; i < original_varbinds_length; i++) {
    if(i >= header->non_repeaters) {
      break;
    }

    resource = snmp_mib_find_next(&oids[i]);
    if(!resource) {
      switch(header->version) {
      case SNMP_VERSION_1:
        header->error_status = SNMP_STATUS_NO_SUCH_NAME;
        /*
         * Varbinds are 1 indexed
         */
        header->error_index = i + 1;
        break;
      case SNMP_VERSION_2C:
        (&varbinds[i])->value_type = BER_DATA_TYPE_END_OF_MIB_VIEW;
        break;
      default:
        header->error_status = SNMP_STATUS_NO_SUCH_NAME;
        header->error_index = 0;
      }
    } else {
      if(varbinds_length < SNMP_MAX_NR_VALUES) {
        resource->handler(&varbinds[varbinds_length], &resource->oid);
        (varbinds_length)++;
      } else {
        return -1;
      }
    }
  }

  for(i = 0; i < header->max_repetitions; i++) {
    repeater = 0;
    for(j = header->non_repeaters; j < original_varbinds_length; j++) {
      resource = snmp_mib_find_next(&oids[j]);
      if(!resource) {
        switch(header->version) {
        case SNMP_VERSION_1:
          header->error_status = SNMP_STATUS_NO_SUCH_NAME;
          /*
           * Varbinds are 1 indexed
           */
          header->error_index = varbinds_length + 1;
          break;
        case SNMP_VERSION_2C:
          if(varbinds_length < SNMP_MAX_NR_VALUES) {
            (&varbinds[varbinds_length])->value_type = BER_DATA_TYPE_END_OF_MIB_VIEW;
            memcpy(&varbinds[varbinds_length].oid, &oids[j], sizeof(snmp_oid_t));
            (varbinds_length)++;
          } else {
            return -1;
          }
          break;
        default:
          header->error_status = SNMP_STATUS_NO_SUCH_NAME;
          header->error_index = 0;
        }
      } else {
        if(varbinds_length < SNMP_MAX_NR_VALUES) {
          resource->handler(&varbinds[varbinds_length], &resource->oid);
          (varbinds_length)++;
          memcpy(&oids[j], &resource->oid, sizeof(snmp_oid_t));
          repeater++;
        } else {
          return -1;
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
int
snmp_engine(snmp_packet_t *snmp_packet)
{
  snmp_header_t header;
  snmp_varbind_t varbinds[SNMP_MAX_NR_VALUES];

  memset(&header, 0, sizeof(header));
  memset(varbinds, 0, sizeof(varbinds));

  if(!snmp_message_decode(snmp_packet, &header, varbinds)) {
    return 0;
  }

  if(header.version != SNMP_VERSION_1) {
    if(strncmp(header.community.community, SNMP_COMMUNITY, header.community.length)) {
      LOG_ERR("Request with invalid community\n");
      return 0;
    }
  }

  /*
   * Now handle the SNMP requests depending on their type
   */
  switch(header.pdu_type) {
  case BER_DATA_TYPE_PDU_GET_REQUEST:
    if(snmp_engine_get(&header, varbinds) == -1) {
      return 0;
    }
    break;

  case BER_DATA_TYPE_PDU_GET_NEXT_REQUEST:
    if(snmp_engine_get_next(&header, varbinds) == -1) {
      return 0;
    }
    break;

  case BER_DATA_TYPE_PDU_GET_BULK:
    if(snmp_engine_get_bulk(&header, varbinds) == -1) {
      return 0;
    }
    break;

  default:
    LOG_ERR("Invalid request type");
    return 0;
  }

  header.pdu_type = BER_DATA_TYPE_PDU_GET_RESPONSE;

  return snmp_message_encode(snmp_packet, &header, varbinds);
}
