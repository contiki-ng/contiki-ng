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

#include "snmp-message.h"
#include "snmp-ber.h"
#include "snmp-oid.h"

#define LOG_MODULE "SNMP [message]"
#define LOG_LEVEL LOG_LEVEL_SNMP

unsigned char *
snmp_message_encode(unsigned char *out, uint32_t *out_len, snmp_header_t *header,
                    snmp_varbind_t *varbinds, uint32_t varbind_num)
{
  snmp_varbind_t *varbind;
  uint32_t original_out_len, last_out_len;
  int8_t i;

  original_out_len = *out_len;
  for(i = varbind_num - 1; i >= 0; i--) {
    varbind = &varbinds[i];

    last_out_len = *out_len;

    switch(varbind->value_type) {
    case BER_DATA_TYPE_INTEGER:
      out = snmp_ber_encode_integer(out, out_len, varbind->value.integer);
      break;
    case SNMP_DATA_TYPE_TIME_TICKS:
      out = snmp_ber_encode_unsigned_integer(out, out_len, varbind->value_type, varbind->value.integer);
      break;
    case BER_DATA_TYPE_OCTET_STRING:
      out = snmp_ber_encode_string_len(out, out_len, varbind->value.string.string, varbind->value.string.length);
      break;
    case BER_DATA_TYPE_OID:
      out = snmp_oid_encode_oid(out, out_len, varbind->value.oid);
      break;
    case BER_DATA_TYPE_NULL:
    case SNMP_DATA_TYPE_NO_SUCH_INSTANCE:
    case SNMP_DATA_TYPE_END_OF_MIB_VIEW:
      out = snmp_ber_encode_null(out, out_len, varbind->value_type);
      break;
    default:
      return NULL;
    }

    out = snmp_oid_encode_oid(out, out_len, varbind->oid);
    out = snmp_ber_encode_length(out, out_len, ((*out_len - last_out_len) & 0xFF));
    out = snmp_ber_encode_type(out, out_len, BER_DATA_TYPE_SEQUENCE);
  }

  out = snmp_ber_encode_length(out, out_len, ((*out_len - original_out_len) & 0xFF));
  out = snmp_ber_encode_type(out, out_len, BER_DATA_TYPE_SEQUENCE);

  if(header->pdu_type == SNMP_DATA_TYPE_PDU_GET_BULK) {
    out = snmp_ber_encode_integer(out, out_len, header->error_index_max_repetitions.max_repetitions);
    out = snmp_ber_encode_integer(out, out_len, header->error_status_non_repeaters.non_repeaters);
  } else {
    out = snmp_ber_encode_integer(out, out_len, header->error_index_max_repetitions.error_index);
    out = snmp_ber_encode_integer(out, out_len, header->error_status_non_repeaters.error_status);
  }
  out = snmp_ber_encode_integer(out, out_len, header->request_id);

  out = snmp_ber_encode_length(out, out_len, ((*out_len - original_out_len) & 0xFF));
  out = snmp_ber_encode_type(out, out_len, header->pdu_type);

  out = snmp_ber_encode_string_len(out, out_len, header->community.community, header->community.length);
  out = snmp_ber_encode_integer(out, out_len, header->version);

  out = snmp_ber_encode_length(out, out_len, ((*out_len - original_out_len) & 0xFF));
  out = snmp_ber_encode_type(out, out_len, BER_DATA_TYPE_SEQUENCE);

  return out;
}
uint8_t *
snmp_message_decode(uint8_t *buf, uint32_t buf_len, snmp_header_t *header,
                    snmp_varbind_t *varbinds, uint32_t *varbind_num)
{
  uint8_t type, len;
  uint32_t i, oid_len;

  buf = snmp_ber_decode_type(buf, &buf_len, &type);
  if(buf == NULL) {
    LOG_DBG("Could not decode type\n");
    return NULL;
  }

  if(type != BER_DATA_TYPE_SEQUENCE) {
    LOG_DBG("Invalid type\n");
    return NULL;
  }

  buf = snmp_ber_decode_length(buf, &buf_len, &len);
  if(buf == NULL) {
    LOG_DBG("Could not decode length\n");
    return NULL;
  }

  buf = snmp_ber_decode_integer(buf, &buf_len, &header->version);
  if(buf == NULL) {
    LOG_DBG("Could not decode version\n");
    return NULL;
  }

  buf = snmp_ber_decode_string_len_buffer(buf, &buf_len, &header->community.community, &header->community.length);
  if(buf == NULL) {
    LOG_DBG("Could not decode community\n");
    return NULL;
  }

  if(header->version != SNMP_VERSION_1 &&
     header->version != SNMP_VERSION_2C) {
    LOG_DBG("Invalid version\n");
    return NULL;
  }

  buf = snmp_ber_decode_type(buf, &buf_len, &type);
  if(buf == NULL) {
    LOG_DBG("Could not decode type\n");
    return NULL;
  }

  header->pdu_type = type;
  if(header->pdu_type != SNMP_DATA_TYPE_PDU_GET_REQUEST &&
     header->pdu_type != SNMP_DATA_TYPE_PDU_GET_NEXT_REQUEST &&
     header->pdu_type != SNMP_DATA_TYPE_PDU_GET_RESPONSE &&
     header->pdu_type != SNMP_DATA_TYPE_PDU_SET_REQUEST &&
     header->pdu_type != SNMP_DATA_TYPE_PDU_GET_BULK) {
    LOG_DBG("Invalid pdu type\n");
    return NULL;
  }

  buf = snmp_ber_decode_length(buf, &buf_len, &len);
  if(buf == NULL) {
    LOG_DBG("Could not decode length\n");
    return NULL;
  }

  buf = snmp_ber_decode_integer(buf, &buf_len, &header->request_id);
  if(buf == NULL) {
    LOG_DBG("Could not decode request id\n");
    return NULL;
  }

  if(header->pdu_type == SNMP_DATA_TYPE_PDU_GET_BULK) {
    buf = snmp_ber_decode_integer(buf, &buf_len, &header->error_status_non_repeaters.non_repeaters);
    if(buf == NULL) {
      LOG_DBG("Could not decode error status\n");
      return NULL;
    }

    buf = snmp_ber_decode_integer(buf, &buf_len, &header->error_index_max_repetitions.max_repetitions);
    if(buf == NULL) {
      LOG_DBG("Could not decode error index\n");
      return NULL;
    }
  } else {
    buf = snmp_ber_decode_integer(buf, &buf_len, &header->error_status_non_repeaters.error_status);
    if(buf == NULL) {
      LOG_DBG("Could not decode error status\n");
      return NULL;
    }

    buf = snmp_ber_decode_integer(buf, &buf_len, &header->error_index_max_repetitions.error_index);
    if(buf == NULL) {
      LOG_DBG("Could not decode error index\n");
      return NULL;
    }
  }

  buf = snmp_ber_decode_type(buf, &buf_len, &type);
  if(buf == NULL) {
    LOG_DBG("Could not decode type\n");
    return NULL;
  }

  if(type != BER_DATA_TYPE_SEQUENCE) {
    LOG_DBG("Invalid type\n");
    return NULL;
  }

  buf = snmp_ber_decode_length(buf, &buf_len, &len);
  if(buf == NULL) {
    LOG_DBG("Could not decode length\n");
    return NULL;
  }

  for(i = 0; buf_len > 0; ++i) {

    buf = snmp_ber_decode_type(buf, &buf_len, &type);
    if(buf == NULL) {
      LOG_DBG("Could not decode type\n");
      return NULL;
    }

    if(type != BER_DATA_TYPE_SEQUENCE) {
      LOG_DBG("Invalid (%X) type\n", type);
      return NULL;
    }

    buf = snmp_ber_decode_length(buf, &buf_len, &len);
    if(buf == NULL) {
      LOG_DBG("Could not decode length\n");
      return NULL;
    }

    buf = snmp_oid_decode_oid(buf, &buf_len, varbinds[i].oid, &oid_len);
    if(buf == NULL) {
      LOG_DBG("Could not decode oid\n");
      return NULL;
    }

    varbinds[i].value_type = *buf;

    switch(varbinds[i].value_type) {
    case BER_DATA_TYPE_INTEGER:
      buf = snmp_ber_decode_integer(buf, &buf_len, &varbinds[i].value.integer);
      break;
    case SNMP_DATA_TYPE_TIME_TICKS:
      buf = snmp_ber_decode_unsigned_integer(buf, &buf_len, varbinds[i].value_type, &varbinds[i].value.integer);
      break;
    case BER_DATA_TYPE_OCTET_STRING:
      buf = snmp_ber_decode_string_len_buffer(buf, &buf_len, &varbinds[i].value.string.string, &varbinds[i].value.string.length);
      break;
    case BER_DATA_TYPE_NULL:
      buf = snmp_ber_decode_null(buf, &buf_len);
      break;
    default:
      LOG_DBG("Invalid varbind type\n");
      return NULL;
    }

    if(buf == NULL) {
      LOG_DBG("Could varbind type\n");
      return NULL;
    }
  }

  *varbind_num = i;

  return buf;
}
