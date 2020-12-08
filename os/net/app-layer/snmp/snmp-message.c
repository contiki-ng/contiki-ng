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
 *      SNMP Implementation of the messages
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#include "contiki.h"

#include "snmp-message.h"
#include "snmp-ber.h"

#define LOG_MODULE "SNMP [message]"
#define LOG_LEVEL LOG_LEVEL_SNMP

int
snmp_message_encode(snmp_packet_t *snmp_packet, snmp_header_t *header, snmp_varbind_t *varbinds)
{
  uint32_t last_out_len;
  int8_t i;

  for(i = SNMP_MAX_NR_VALUES - 1; i >= 0; i--) {
    if(varbinds[i].value_type == BER_DATA_TYPE_EOC) {
      continue;
    }

    last_out_len = snmp_packet->used;

    switch(varbinds[i].value_type) {
    case BER_DATA_TYPE_INTEGER:
      if(!snmp_ber_encode_integer(snmp_packet, varbinds[i].value.integer)) {
        LOG_DBG("Could not encode integer type\n");
        return 0;
      }
      break;
    case BER_DATA_TYPE_TIMETICKS:
      if(!snmp_ber_encode_timeticks(snmp_packet, varbinds[i].value.integer)) {
        LOG_DBG("Could not encode timeticks type\n");
        return 0;
      }
      break;
    case BER_DATA_TYPE_OCTET_STRING:
      if(!snmp_ber_encode_string_len(snmp_packet, varbinds[i].value.string.string, varbinds[i].value.string.length)) {
        LOG_DBG("Could not encode octet string type\n");
        return 0;
      }
      break;
    case BER_DATA_TYPE_OBJECT_IDENTIFIER:
      if(!snmp_ber_encode_oid(snmp_packet, &varbinds[i].value.oid)) {
        LOG_DBG("Could not encode oid type\n");
        return 0;
      }
      break;
    case BER_DATA_TYPE_NULL:
    case BER_DATA_TYPE_NO_SUCH_INSTANCE:
    case BER_DATA_TYPE_END_OF_MIB_VIEW:
      if(!snmp_ber_encode_null(snmp_packet, varbinds[i].value_type)) {
        LOG_DBG("Could not encode null type\n");
        return 0;
      }
      break;
    default:
      LOG_DBG("Could not encode invlid type\n");
      return 0;
    }

    if(!snmp_ber_encode_oid(snmp_packet, &varbinds[i].oid)) {
      LOG_DBG("Could not encode oid\n");
      return 0;
    }

    if(!snmp_ber_encode_length(snmp_packet, (snmp_packet->used - last_out_len))) {
      LOG_DBG("Could not encode length\n");
      return 0;
    }

    if(!snmp_ber_encode_type(snmp_packet, BER_DATA_TYPE_SEQUENCE)) {
      LOG_DBG("Could not encode type\n");
      return 0;
    }
  }

  if(!snmp_ber_encode_length(snmp_packet, snmp_packet->used)) {
    LOG_DBG("Could not encode length\n");
    return 0;
  }
  if(!snmp_ber_encode_type(snmp_packet, BER_DATA_TYPE_SEQUENCE)) {
    LOG_DBG("Could not encode type\n");
    return 0;
  }

  switch(header->pdu_type) {
  case BER_DATA_TYPE_PDU_GET_BULK:
    if(!snmp_ber_encode_integer(snmp_packet, header->max_repetitions)) {
      LOG_DBG("Could not encode max repetition\n");
      return 0;
    }

    if(!snmp_ber_encode_integer(snmp_packet, header->non_repeaters)) {
      LOG_DBG("Could not encode non repeaters\n");
      return 0;
    }
    break;
  default:
    if(!snmp_ber_encode_integer(snmp_packet, header->error_index)) {
      LOG_DBG("Could not encode error index\n");
      return 0;
    }

    if(!snmp_ber_encode_integer(snmp_packet, header->error_status)) {
      LOG_DBG("Could not encode error status\n");
      return 0;
    }
    break;
  }

  if(!snmp_ber_encode_integer(snmp_packet, header->request_id)) {
    LOG_DBG("Could not encode request id\n");
    return 0;
  }

  if(!snmp_ber_encode_length(snmp_packet, snmp_packet->used)) {
    LOG_DBG("Could not encode length\n");
    return 0;
  }

  if(!snmp_ber_encode_type(snmp_packet, header->pdu_type)) {
    LOG_DBG("Could not encode pdu type\n");
    return 0;
  }

  if(!snmp_ber_encode_string_len(snmp_packet, header->community.community, header->community.length)) {
    LOG_DBG("Could not encode community\n");
    return 0;
  }

  if(!snmp_ber_encode_integer(snmp_packet, header->version)) {
    LOG_DBG("Could not encode version\n");
    return 0;
  }

  if(!snmp_ber_encode_length(snmp_packet, snmp_packet->used)) {
    LOG_DBG("Could not encode length\n");
    return 0;
  }

  if(!snmp_ber_encode_type(snmp_packet, BER_DATA_TYPE_SEQUENCE)) {
    LOG_DBG("Could not encode type\n");
    return 0;
  }

  /* Move the pointer to the last position */
  snmp_packet->out++;
  return 1;
}
int
snmp_message_decode(snmp_packet_t *snmp_packet, snmp_header_t *header, snmp_varbind_t *varbinds)
{
  uint8_t type, len, i;

  if(!snmp_ber_decode_type(snmp_packet, &type)) {
    LOG_DBG("Could not decode type\n");
    return 0;
  }

  if(type != BER_DATA_TYPE_SEQUENCE) {
    LOG_DBG("Invalid type\n");
    return 0;
  }

  if(!snmp_ber_decode_length(snmp_packet, &len)) {
    LOG_DBG("Could not decode length\n");
    return 0;
  }

  if(!snmp_ber_decode_integer(snmp_packet, &header->version)) {
    LOG_DBG("Could not decode version\n");
    return 0;
  }

  switch(header->version) {
  case SNMP_VERSION_1:
  case SNMP_VERSION_2C:
    break;
  default:
    LOG_DBG("Invalid version\n");
    return 0;
  }

  if(!snmp_ber_decode_string_len_buffer(snmp_packet, &header->community.community, &header->community.length)) {
    LOG_DBG("Could not decode community\n");
    return 0;
  }

  if(!snmp_ber_decode_type(snmp_packet, &header->pdu_type)) {
    LOG_DBG("Could not decode pdu type\n");
    return 0;
  }

  switch(header->pdu_type) {
  case BER_DATA_TYPE_PDU_GET_REQUEST:
  case BER_DATA_TYPE_PDU_GET_NEXT_REQUEST:
  case BER_DATA_TYPE_PDU_GET_RESPONSE:
  case BER_DATA_TYPE_PDU_SET_REQUEST:
  case BER_DATA_TYPE_PDU_GET_BULK:
    break;
  default:
    LOG_DBG("Invalid version\n");
    return 0;
  }

  if(!snmp_ber_decode_length(snmp_packet, &len)) {
    LOG_DBG("Could not decode length\n");
    return 0;
  }

  if(!snmp_ber_decode_integer(snmp_packet, &header->request_id)) {
    LOG_DBG("Could not decode request id\n");
    return 0;
  }

  switch(header->pdu_type) {
  case BER_DATA_TYPE_PDU_GET_BULK:
    if(!snmp_ber_decode_integer(snmp_packet, &header->non_repeaters)) {
      LOG_DBG("Could not decode non repeaters\n");
      return 0;
    }

    if(!snmp_ber_decode_integer(snmp_packet, &header->max_repetitions)) {
      LOG_DBG("Could not decode max repetition\n");
      return 0;
    }
    break;
  default:
    if(!snmp_ber_decode_integer(snmp_packet, &header->error_status)) {
      LOG_DBG("Could not decode error status\n");
      return 0;
    }

    if(!snmp_ber_decode_integer(snmp_packet, &header->error_index)) {
      LOG_DBG("Could not decode error index\n");
      return 0;
    }
    break;
  }

  if(!snmp_ber_decode_type(snmp_packet, &type)) {
    LOG_DBG("Could not decode type\n");
    return 0;
  }

  if(type != BER_DATA_TYPE_SEQUENCE) {
    LOG_DBG("Invalid type\n");
    return 0;
  }

  if(!snmp_ber_decode_length(snmp_packet, &len)) {
    LOG_DBG("Could not decode length\n");
    return 0;
  }

  for(i = 0; snmp_packet->used > 0; ++i) {
    if(i >= SNMP_MAX_NR_VALUES) {
      LOG_DBG("OID's overflow\n");
      return 0;
    }

    if(!snmp_ber_decode_type(snmp_packet, &type)) {
      LOG_DBG("Could not decode type\n");
      return 0;
    }

    if(type != BER_DATA_TYPE_SEQUENCE) {
      LOG_DBG("Invalid (%X) type\n", type);
      return 0;
    }

    if(!snmp_ber_decode_length(snmp_packet, &len)) {
      LOG_DBG("Could not decode length\n");
      return 0;
    }

    if(!snmp_ber_decode_oid(snmp_packet, &varbinds[i].oid)) {
      LOG_DBG("Could not decode oid\n");
      return 0;
    }

    varbinds[i].value_type = *snmp_packet->in;

    switch(varbinds[i].value_type) {
    case BER_DATA_TYPE_INTEGER:
      if(!snmp_ber_decode_integer(snmp_packet, &varbinds[i].value.integer)) {
        LOG_DBG("Could not decode integer type\n");
        return 0;
      }
      break;
    case BER_DATA_TYPE_TIMETICKS:
      if(!snmp_ber_decode_timeticks(snmp_packet, &varbinds[i].value.integer)) {
        LOG_DBG("Could not decode timeticks type\n");
        return 0;
      }
      break;
    case BER_DATA_TYPE_OCTET_STRING:
      if(!snmp_ber_decode_string_len_buffer(snmp_packet, &varbinds[i].value.string.string, &varbinds[i].value.string.length)) {
        LOG_DBG("Could not decode octed string type\n");
        return 0;
      }
      break;
    case BER_DATA_TYPE_NULL:
      if(!snmp_ber_decode_null(snmp_packet)) {
        LOG_DBG("Could not decode null type\n");
        return 0;
      }
      break;
    default:
      LOG_DBG("Invalid varbind type\n");
      return 0;
    }
  }

  return 1;
}
