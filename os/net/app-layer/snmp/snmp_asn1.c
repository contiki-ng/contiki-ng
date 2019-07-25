/* SNMP protocol
 *
 * Copyright (C) 2008-2010  Robert Ernst <robert.ernst@linux-solutions.at>
 * Copyright (C) 2011       Javier Palacios <javiplx@gmail.com>
 * Copyright (C) 2019       Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
 *
 * This file may be distributed and/or modified under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation and appearing in the file LICENSE.GPL included in the
 * packaging of this file.
 *
 * This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
 * WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See COPYING for GPL licensing information.
 */

#include "snmp.h"
#include "snmp_utils.h"
#include "snmp_asn1.h"

#ifdef SNMP_DEBUG
#include "snmp_debug.h"
#endif /* SNMP_DEBUG */

#define LOG_MODULE "SNMP [asn1]"
#define LOG_LEVEL LOG_LEVEL_SNMP

static const snmp_data_t snmp_null = { { '\x05', '\x00' }, 2, 2 };

int
snmp_asn1_decode_len(const unsigned char *packet, size_t size, size_t *pos, int *type, size_t *len)
{
  size_t length_of_len;

  if(*pos >= size) {
    LOG_INFO("underflow for element type\n");
    return -1;
  }

  /* Fetch the ASN.1 element type (only subset of universal tags supported) */
  switch(packet[*pos]) {
  case BER_TYPE_BOOLEAN:
  case BER_TYPE_INTEGER:
  case BER_TYPE_BIT_STRING:
  case BER_TYPE_OCTET_STRING:
  case BER_TYPE_NULL:
  case BER_TYPE_OID:
  case BER_TYPE_SEQUENCE:
  case BER_TYPE_COUNTER:
  case BER_TYPE_GAUGE:
  case BER_TYPE_TIME_TICKS:
  case BER_TYPE_NO_SUCH_OBJECT:
  case BER_TYPE_NO_SUCH_INSTANCE:
  case BER_TYPE_END_OF_MIB_VIEW:
  case BER_TYPE_SNMP_GET:
  case BER_TYPE_SNMP_GETNEXT:
  case BER_TYPE_SNMP_RESPONSE:
  case BER_TYPE_SNMP_SET:
  case BER_TYPE_SNMP_GETBULK:
  case BER_TYPE_SNMP_INFORM:
  case BER_TYPE_SNMP_TRAP:
    *type = packet[*pos];
    *pos = *pos + 1;
    break;

  default:
    LOG_INFO("unsupported element type %02X\n", packet[*pos]);
    return -1;
  }

  if(*pos >= size) {
    LOG_INFO("underflow for element length\n");
    return -1;
  }

  /* Fetch the ASN.1 element length (only lengths up to 16 bit supported) */
  if(!(packet[*pos] & 0x80)) {
    *len = packet[*pos];
    *pos = *pos + 1;
  } else {
    length_of_len = packet[*pos] & 0x7F;
    if(length_of_len > 2) {
      LOG_INFO("overflow for element length\n");
      return -1;
    }

    *pos = *pos + 1;
    *len = 0;
    while(length_of_len--) {
      if(*pos >= size) {
        LOG_INFO("underflow for element length\n");
        return -1;
      }

      *len = (*len << 8) + packet[*pos];
      *pos = *pos + 1;
    }
  }

  return 0;
}
/* Fetch the value as unsigned integer (copy sign bit into all bytes first) */
int
snmp_asn1_decode_int(const unsigned char *packet, size_t size, size_t *pos, size_t len, int *value)
{
  unsigned int tmp;

  if(*pos >= (size - len + 1)) {
    LOG_INFO("underflow for integer\n");
    return -1;
  }

  memset(&tmp, (packet[*pos] & 0x80) ? 0xFF : 0x00, sizeof(tmp));
  while(len--) {
    tmp = (tmp << 8) | packet[*pos];
    *pos = *pos + 1;
  }
  *(int *)value = tmp;

  return 0;
}
/* Fetch the value as unsigned integer (copy sign bit into all bytes first) */
int
snmp_asn1_decode_cnt(const unsigned char *packet, size_t size, size_t *pos, size_t len, uint32_t *value)
{
  if(*pos >= (size - len + 1)) {
    LOG_INFO("underflow for unsigned\n");
    return -1;
  }

  *value = 0;
  while(len--) {
    *value = (*value << 8) | packet[*pos];
    *pos = *pos + 1;
  }

  return 0;
}
/* Fetch the value as C string (user must have made sure the length is ok) */
int
decode_str(const unsigned char *packet, size_t size, size_t *pos, size_t len, char *str, size_t str_len)
{
  if(*pos >= (size - len + 1)) {
    LOG_INFO("underflow for string\n");
    return -1;
  }

  snprintf(str, str_len, "%.*s", (int)len, &packet[*pos]);
  *pos = *pos + len;

  return 0;
}
/* Fetch the value as C string (user must have made sure the length is ok) */
int
snmp_asn1_decode_oid(const unsigned char *packet, size_t size, size_t *pos, size_t len, snmp_oid_t *value)
{
  if(*pos >= (size - len + 1)) {
    LOG_INFO("underflow for oid\n");
    return -1;
  }

  value->encoded_length = len;
  if(len > 0xFFFF) {
    LOG_ERR("could not decode: internal error\n");
    return -1;
  }

  if(len > 0xFF) {
    value->encoded_length += 4;
  } else if(len > 0x7F) {
    value->encoded_length += 3;
  } else {
    value->encoded_length += 2;
  }

  value->subid_list_length = 0;
  if(!len) {
    LOG_INFO("underflow for OID startbyte\n");
    return -1;
  }

  if(packet[*pos] & 0x80) {
    LOG_INFO("unsupported OID startbyte %02X\n", packet[*pos]);
    return -1;
  }

  value->subid_list[value->subid_list_length++] = packet[*pos] / 40;
  value->subid_list[value->subid_list_length++] = packet[*pos] % 40;
  *pos = *pos + 1;
  len--;

  while(len) {
    if(value->subid_list_length >= MAX_NR_SUBIDS) {
      LOG_INFO("overflow for OID byte\n");
      return -1;
    }

    value->subid_list[value->subid_list_length] = 0;
    while(len--) {
      value->subid_list[value->subid_list_length]
        = (value->subid_list[value->subid_list_length] << 7) + (packet[*pos] & 0x7F);
      if(packet[*pos] & 0x80) {
        if(!len) {
          LOG_INFO("underflow for OID byte\n");
          return -1;
        }
        *pos = *pos + 1;
      } else {
        *pos = *pos + 1;
        break;
      }
    }
    value->subid_list_length++;
  }

  return 0;
}
/* Fetch the value as pointer (user must make sure not to overwrite packet) */
int
decode_ptr(const unsigned char UNUSED(*packet), size_t size, size_t *pos, int len)
{
  if(*pos >= (size - len + 1)) {
    LOG_INFO("underflow for ptr\n");
    return -1;
  }

  *pos = *pos + len;

  return 0;
}
int
snmp_asn1_decode_request(request_t *request, snmp_client_t *client)
{
  int type;
  size_t pos = 0, len = 0;

  /* The SNMP message is enclosed in a sequence */
  if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
    LOG_INFO("Error while deconding the the SNMP header\n");
    return -1;
  }

  if(type != BER_TYPE_SEQUENCE || len != (client->size - pos)) {
    LOG_INFO("%s type %02X length %zu\n", "Unexpected SNMP header", type, len);
    return -1;
  }

  /* The first element of the sequence is the version */
  if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
    LOG_INFO("Error while deconding the the SNMP version\n");
    return -1;
  }

  if(type != BER_TYPE_INTEGER || len != 1) {
    LOG_INFO("Unexpected %s type %02X length %zu\n", "SNMP version", type, len);
    return -1;
  }

  if(snmp_asn1_decode_int(client->packet, client->size, &pos, len, &request->version) == -1) {
    LOG_INFO("Error while deconding the the SNMP version\n");
    return -1;
  }

  if(request->version != SNMP_VERSION_1 && request->version != SNMP_VERSION_2C) {
    LOG_INFO("Unsupported %s %d\n", "SNMP version", request->version);
    return -1;
  }

  /* The second element of the sequence is the community string */
  if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
    LOG_INFO("Error while deconding the the SNMP community\n");
    return -1;
  }

  if(type != BER_TYPE_OCTET_STRING || len >= sizeof(request->community)) {
    LOG_INFO("Unexpected %s type %02X length %zu\n", "SNMP community", type, len);
    return -1;
  }

  if(decode_str(client->packet, client->size, &pos, len, request->community, sizeof(request->community)) == -1) {
    LOG_INFO("Error while deconding the the SNMP community\n");
    return -1;
  }

  if(strlen(request->community) < 1) {
    LOG_INFO("unsupported %s '%s'\n", "SNMP community", request->community);
    return -1;
  }

  /* The third element of the sequence is the SNMP request */
  if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
    LOG_INFO("Error while deconding the the SNMP request\n");
    return -1;
  }

  if(len != (client->size - pos)) {
    LOG_INFO("%s type type %02X length %zu\n", "Unexpected SNMP request", type, len);
    return -1;
  }
  request->type = type;

  /* The first element of the SNMP request is the request ID */
  if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
    LOG_INFO("Error while deconding the the SNMP request ID\n");
    return -1;
  }

  if(type != BER_TYPE_INTEGER || len < 1) {
    LOG_INFO("%s id type %02X length %zu\n", "Unexpected SNMP request", type, len);
    return -1;
  }

  if(snmp_asn1_decode_int(client->packet, client->size, &pos, len, &request->id) == -1) {
    LOG_INFO("Error while deconding the the SNMP request ID\n");
    return -1;
  }

  /* The second element of the SNMP request is the error state / non repeaters (0..2147483647) */
  if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
    LOG_INFO("Error while deconding the the SNMP error\n");
    return -1;
  }

  if(type != BER_TYPE_INTEGER || len < 1) {
    LOG_INFO("%s state type %02X length %zu\n", "Unexpected SNMP error", type, len);
    return -1;
  }

  if(snmp_asn1_decode_cnt(client->packet, client->size, &pos, len, &request->non_repeaters) == -1) {
    LOG_INFO("Error while deconding the the SNMP error\n");
    return -1;
  }

  /* The third element of the SNMP request is the error index / max repetitions (0..2147483647) */
  if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
    LOG_INFO("Error while deconding the the SNMP error\n");
    return -1;
  }

  if(type != BER_TYPE_INTEGER || len < 1) {
    LOG_INFO("%s index type %02X length %zu\n", "Unexpected SNMP error", type, len);
    return -1;
  }

  if(snmp_asn1_decode_cnt(client->packet, client->size, &pos, len, &request->max_repetitions) == -1) {
    LOG_INFO("Error while deconding the the SNMP error\n");
    return -1;
  }

  /* The fourth element of the SNMP request are the variable bindings */
  if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
    LOG_INFO("Error while deconding the the SNMP varbindings\n");
    return -1;
  }

  if(type != BER_TYPE_SEQUENCE || len != (client->size - pos)) {
    LOG_INFO("%s type %02X length %zu\n", "Unexpected SNMP varbindings", type, len);
    return -1;
  }

  /* Loop through the variable bindings */
  request->oid_list_length = 0;
  while(pos < client->size) {
    /* If there is not enough room in the OID list, bail out now */
    if(request->oid_list_length >= MAX_NR_OIDS) {
      LOG_INFO("Overflow in OID list\n");
      return -1;
    }

    /* Each variable binding is a sequence describing the variable */
    if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
      LOG_INFO("Error while deconding the the SNMP varbindings\n");
      return -1;
    }

    if(type != BER_TYPE_SEQUENCE || len < 1) {
      LOG_INFO("%s type %02X length %zu\n", "Unexpected SNMP varbindings", type, len);
      return -1;
    }

    /* The first element of the variable binding is the OID */
    if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
      LOG_INFO("Error while deconding the the SNMP varbindings\n");
      return -1;
    }

    if(type != BER_TYPE_OID || len < 1) {
      LOG_INFO("%s OID type %02X length %zu\n", "Unexpected SNMP varbindings", type, len);
      return -1;
    }

    if(snmp_asn1_decode_oid(client->packet, client->size, &pos, len, &request->oid_list[request->oid_list_length]) == -1) {
      LOG_INFO("Error while deconding the the SNMP varbindings\n");
      return -1;
    }

    /* The second element of the variable binding is the new type and value */
    if(snmp_asn1_decode_len(client->packet, client->size, &pos, &type, &len) == -1) {
      LOG_INFO("Error while deconding the the SNMP varbindings\n");
      return -1;
    }

    if((type == BER_TYPE_NULL && len) || (type != BER_TYPE_NULL && !len)) {
      LOG_INFO("%s value type %02X length %zu\n", "Unexpected SNMP varbindings", type, len);
      return -1;
    }

    if(decode_ptr(client->packet, client->size, &pos, len) == -1) {
      LOG_INFO("Error while deconding the the SNMP varbindings\n");
      return -1;
    }

    /* Now the OID list has one more entry */
    request->oid_list_length++;
  }

  return 0;
}
size_t
get_intlen(int val)
{
  if(val < -8388608 || val > 8388607) {
    return 6;
  }
  if(val < -32768 || val > 32767) {
    return 5;
  }
  if(val < -128 || val > 127) {
    return 4;
  }

  return 3;
}
size_t
get_strlen(const char *str)
{
  size_t len = strlen(str);

  if(len > 0xFFFF) {
    return MAX_PACKET_SIZE;
  }
  if(len > 0xFF) {
    return len + 4;
  }
  if(len > 0x7F) {
    return len + 3;
  }

  return len + 2;
}
size_t
get_hdrlen(size_t len)
{
  if(len > 0xFFFF) {
    return MAX_PACKET_SIZE;
  }
  if(len > 0xFF) {
    return 4;
  }
  if(len > 0x7F) {
    return 3;
  }

  return 2;
}
int
encode_snmp_integer(unsigned char *buf, int val)
{
  size_t len;

  if(val < -8388608 || val > 8388607) {
    len = 4;
  } else if(val < -32768 || val > 32767) {
    len = 3;
  } else if(val < -128 || val > 127) {
    len = 2;
  } else {
    len = 1;
  }

  *buf++ = BER_TYPE_INTEGER;
  *buf++ = len;
  while(len--)
    *buf++ = ((unsigned int)val >> (8 * len)) & 0xFF;

  return 0;
}
int
encode_snmp_string(unsigned char *buf, const char *str)
{
  size_t len;

  len = strlen(str);
  if(len > 0xFFFF) {
    return -1;
  }

  *buf++ = BER_TYPE_OCTET_STRING;
  if(len > 0xFF) {
    *buf++ = 0x82;
    *buf++ = (len >> 8) & 0xFF;
    *buf++ = len & 0xFF;
  } else if(len > 0x7F) {
    *buf++ = 0x81;
    *buf++ = len & 0xFF;
  } else {
    *buf++ = len & 0x7F;
  }
  memcpy(buf, str, len);

  return 0;
}
int
encode_snmp_sequence_header(unsigned char *buf, size_t len, int type)
{
  if(len > 0xFFFF) {
    return -1;
  }

  *buf++ = type;
  if(len > 0xFF) {
    *buf++ = 0x82;
    *buf++ = (len >> 8) & 0xFF;
    *buf++ = len & 0xFF;
  } else if(len > 0x7F) {
    *buf++ = 0x81;
    *buf++ = len & 0xFF;
  } else {
    *buf++ = len & 0x7F;
  }

  return 0;
}
int
encode_snmp_oid(unsigned char *buf, const snmp_oid_t *oid)
{
  size_t i, len;

  len = 1;
  for(i = 2; i < oid->subid_list_length; i++) {
    if(oid->subid_list[i] >= (1 << 28)) {
      len += 5;
    } else if(oid->subid_list[i] >= (1 << 21)) {
      len += 4;
    } else if(oid->subid_list[i] >= (1 << 14)) {
      len += 3;
    } else if(oid->subid_list[i] >= (1 << 7)) {
      len += 2;
    } else {
      len += 1;
    }
  }

  *buf++ = BER_TYPE_OID;
  if(len > 0xFFFF) {
    LOG_ERR("could not encode '%s': OID overflow\n", snmp_oid_ntoa(oid));
    return -1;
  }

  if(len > 0xFF) {
    *buf++ = 0x82;
    *buf++ = (len >> 8) & 0xFF;
    *buf++ = len & 0xFF;
  } else if(len > 0x7F) {
    *buf++ = 0x81;
    *buf++ = len & 0xFF;
  } else {
    *buf++ = len & 0x7F;
  }

  *buf++ = oid->subid_list[0] * 40 + oid->subid_list[1];
  for(i = 2; i < oid->subid_list_length; i++) {
    if(oid->subid_list[i] >= (1 << 28)) {
      len = 5;
    } else if(oid->subid_list[i] >= (1 << 21)) {
      len = 4;
    } else if(oid->subid_list[i] >= (1 << 14)) {
      len = 3;
    } else if(oid->subid_list[i] >= (1 << 7)) {
      len = 2;
    } else {
      len = 1;
    }

    while(len--) {
      if(len) {
        *buf++ = ((oid->subid_list[i] >> (7 * len)) & 0x7F) | 0x80;
      } else {
        *buf++ = (oid->subid_list[i] >> (7 * len)) & 0x7F;
      }
    }
  }

  return 0;
}
int
snmp_log_encoding_error(const char *what, const char *why)
{
  LOG_ERR("Failed encoding %s: %s\n", what, why);
  return -1;
}
int
snmp_ans1_encode_varbind(unsigned char *buf, size_t *pos, const snmp_value_t *value)
{
  size_t len;

  /* The value of the variable binding (NULL for error responses) */
  len = value->data.encoded_length;
  if(*pos < len) {
    return snmp_log_encoding_error(snmp_oid_ntoa(&value->oid), "DATA overflow");
  }

  memcpy(&buf[*pos - len], value->data.buffer, len);
  *pos = *pos - len;

  /* The OID of the variable binding */
  len = value->oid.encoded_length;
  if(*pos < len) {
    return snmp_log_encoding_error(snmp_oid_ntoa(&value->oid), "OID overflow");
  }

  encode_snmp_oid(&buf[*pos - len], &value->oid);
  *pos = *pos - len;

  /* The sequence header (type and length) of the variable binding */
  len = get_hdrlen(value->oid.encoded_length + value->data.encoded_length);
  if(*pos < len) {
    return snmp_log_encoding_error(snmp_oid_ntoa(&value->oid), "VARBIND overflow");
  }

  encode_snmp_sequence_header(&buf[*pos - len], value->oid.encoded_length + value->data.encoded_length, BER_TYPE_SEQUENCE);
  *pos = *pos - len;

  return 0;
}
int
snmp_asn1_encode_response(request_t *request, response_t *response, snmp_client_t *client)
{
  size_t i, len, pos;

  /* If there was an error, we have to encode the original varbind list, but
   * omit any varbind values (replace them with NULL values)
   */
  if(response->error_status != SNMP_STATUS_OK) {
    if(request->oid_list_length > MAX_NR_VALUES) {
      return snmp_log_encoding_error("SNMP response", "value list overflow");
    }

    for(i = 0; i < request->oid_list_length; i++) {
      memcpy(&response->value_list[i].oid, &request->oid_list[i], sizeof(request->oid_list[i]));
      memcpy(&response->value_list[i].data, &snmp_null, sizeof(snmp_null));
    }
    response->value_list_length = request->oid_list_length;
  }

  /* Dump the response for debugging purposes */
#ifdef SNMP_DEBUG
  snmp_debug_dump_response(response);
#endif

  /* To make the code more compact and save processing time, we are encoding the
   * data beginning at the last byte of the buffer backwards. Thus, the encoded
   * packet will not be positioned at offset 0..(size-1) of the client's packet
   * buffer, but at offset (bufsize-size..bufsize-1)!
   */
  pos = MAX_PACKET_SIZE;
  for(i = response->value_list_length; i > 0; i--) {
    if(snmp_ans1_encode_varbind(client->packet, &pos, &response->value_list[i - 1]) == -1) {
      return -1;
    }
  }

  len = get_hdrlen(MAX_PACKET_SIZE - pos);
  if(pos < len) {
    return snmp_log_encoding_error("SNMP response", "VARBINDS overflow");
  }

  encode_snmp_sequence_header(&client->packet[pos - len], MAX_PACKET_SIZE - pos, BER_TYPE_SEQUENCE);
  pos = pos - len;

  len = get_intlen(response->error_index);
  if(pos < len) {
    return snmp_log_encoding_error("SNMP response", "ERROR INDEX overflow");
  }

  encode_snmp_integer(&client->packet[pos - len], response->error_index);
  pos = pos - len;

  len = get_intlen(response->error_status);
  if(pos < len) {
    return snmp_log_encoding_error("SNMP response", "ERROR STATUS overflow");
  }

  encode_snmp_integer(&client->packet[pos - len], response->error_status);
  pos = pos - len;

  len = get_intlen(request->id);
  if(pos < len) {
    return snmp_log_encoding_error("SNMP response", "ID overflow");
  }

  encode_snmp_integer(&client->packet[pos - len], request->id);
  pos = pos - len;

  len = get_hdrlen(MAX_PACKET_SIZE - pos);
  if(pos < len) {
    return snmp_log_encoding_error("SNMP response", "PDU overflow");
  }

  encode_snmp_sequence_header(&client->packet[pos - len], MAX_PACKET_SIZE - pos, BER_TYPE_SNMP_RESPONSE);
  pos = pos - len;

  len = get_strlen(request->community);
  if(pos < len) {
    return snmp_log_encoding_error("SNMP response", "COMMUNITY overflow");
  }

  encode_snmp_string(&client->packet[pos - len], request->community);
  pos = pos - len;

  len = get_intlen(request->version);
  if(pos < len) {
    return snmp_log_encoding_error("SNMP response", "VERSION overflow");
  }

  encode_snmp_integer(&client->packet[pos - len], request->version);
  pos = pos - len;

  len = get_hdrlen(MAX_PACKET_SIZE - pos);
  if(pos < len) {
    return snmp_log_encoding_error("SNMP response", "RESPONSE overflow");
  }

  encode_snmp_sequence_header(&client->packet[pos - len], MAX_PACKET_SIZE - pos, BER_TYPE_SEQUENCE);
  pos = pos - len;

  /*
   * Now move the packet to the start of the buffer so that the caller does not have
   * to deal with this messy detail (the CPU cycles needed are worth their money!)
   * and set up the packet size.
   */
  if(pos > 0) {
    memmove(&client->packet[0], &client->packet[pos], MAX_PACKET_SIZE - pos);
  }
  client->size = MAX_PACKET_SIZE - pos;

  return 0;
}
