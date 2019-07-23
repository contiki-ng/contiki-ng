/*
 * Copyright (C) 2019 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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

#include "snmp_debug.h"

#ifdef SNMP_DEBUG

#include "snmp_asn1.h"
#include "snmp_utils.h"

#include "sys/log.h"
#define LOG_MODULE "SNMP [debug]"
#define LOG_LEVEL LOG_LEVEL_SNMP

int
snmp_debug_element_as_string(const snmp_data_t *data, char *buf, size_t size)
{
  size_t i, len, pos = 0;
  int type, val;
  snmp_oid_t oid;
  unsigned int cnt;

  /* Decode the element type and length */
  if(snmp_asn1_decode_len(data->buffer, data->encoded_length, &pos, &type, &len) == -1) {
    return -1;
  }

  /* Depending on type and length, decode the data */
  switch(type) {
  case BER_TYPE_INTEGER:
    if(snmp_asn1_decode_int(data->buffer, data->encoded_length, &pos, len, &val) == -1) {
      return -1;
    }
    snprintf(buf, size, "%d", val);
    break;

  case BER_TYPE_OCTET_STRING:
    snprintf(buf, size, "%.*s", (int)len, &data->buffer[pos]);
    break;

  case BER_TYPE_OID:
    if(snmp_asn1_decode_oid(data->buffer, data->encoded_length, &pos, len, &oid) == -1) {
      return -1;
    }
    snprintf(buf, size, "%s", snmp_oid_ntoa(&oid));
    break;

  case BER_TYPE_COUNTER:
  case BER_TYPE_GAUGE:
  case BER_TYPE_TIME_TICKS:
    if(snmp_asn1_decode_cnt(data->buffer, data->encoded_length, &pos, len, &cnt) == -1) {
      return -1;
    }
    snprintf(buf, size, "%u", cnt);
    break;

  case BER_TYPE_NO_SUCH_OBJECT:
    snprintf(buf, size, "noSuchObject");
    break;

  case BER_TYPE_NO_SUCH_INSTANCE:
    snprintf(buf, size, "noSuchInstance");
    break;

  case BER_TYPE_END_OF_MIB_VIEW:
    snprintf(buf, size, "endOfMibView");
    break;

  default:
    for(i = 0; i < len && i < ((size - 1) / 3); i++) {
      snprintf(buf + 3 * i, 4, "%02X ", data->buffer[pos + i]);
    }

    if(len > 0) {
      buf[len * 3 - 1] = '\0';
    } else {
      buf[0] = '\0';
    }
    break;
  }

  return 0;
}
void
snmp_debug_dump_response(const response_t *response)
{
  size_t i;
  static char snmp_buf[MAX_PACKET_SIZE];

  LOG_INFO("response: status=%d, index=%d, nr_entries=%zu\n",
           response->error_status, response->error_index, response->value_list_length);
  for(i = 0; i < response->value_list_length; i++) {
    if(snmp_debug_element_as_string(&response->value_list[i].data, snmp_buf, MAX_PACKET_SIZE) == -1) {
      strncpy(snmp_buf, "?", MAX_PACKET_SIZE);
    }

    LOG_INFO("response: entry[%zu]='%s','%s'\n", i, snmp_oid_ntoa(&response->value_list[i].oid), snmp_buf);
  }
}
#endif /* SNMP_DEBUG */

