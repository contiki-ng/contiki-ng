/*
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
#include "snmp_asn1.h"
#include "snmp_mib.h"
#include "snmp_utils.h"

#include "sys/log.h"
#define LOG_MODULE "SNMP [mib]"
#define LOG_LEVEL LOG_LEVEL_SNMP

static const snmp_oid_t snmp_mib_system_oid = { { 1, 3, 6, 1, 2, 1, 1 }, 7, 8 };
static const snmp_oid_t snmp_mib_host_oid = { { 1, 3, 6, 1, 2, 1, 25, 1 }, 8, 9 };

/* Create a data buffer for the value depending on the type:
 *
 * - strings and oids are assumed to be static or have the maximum allowed length
 * - integers are assumed to be dynamic and don't have more than 32 bits
 */
static int
data_alloc(snmp_data_t *data, int type)
{
  switch(type) {
  case BER_TYPE_INTEGER:
    data->max_length = sizeof(int) + 2;
    data->encoded_length = 0;
    /* data->buffer = allocate(data->max_length); */
    break;

  case BER_TYPE_OCTET_STRING:
    data->max_length = 4;
    data->encoded_length = 0;
    /* data->buffer = allocate(data->max_length); */
    break;

  case BER_TYPE_OID:
    data->max_length = MAX_NR_SUBIDS * 5 + 4;
    data->encoded_length = 0;
    /* data->buffer = allocate(data->max_length); */
    break;

  case BER_TYPE_COUNTER:
  case BER_TYPE_GAUGE:
  case BER_TYPE_TIME_TICKS:
    data->max_length = sizeof(unsigned int) + 3;
    data->encoded_length = 0;
    /* data->buffer = allocate(data->max_length); */
    break;

  default:
    return -1;
  }

  if(!data->buffer) {
    return -1;
  }

  data->buffer[0] = type;
  data->buffer[1] = 0;
  data->buffer[2] = 0;
  data->encoded_length = 3;

  return 0;
}
static int
encode_unsigned(snmp_data_t *data, int type, unsigned int ticks_value)
{
  unsigned char *buffer;
  int length;

  buffer = data->buffer;
  if(ticks_value & 0xFF000000) {
    length = 4;
  } else if(ticks_value & 0x00FF0000) {
    length = 3;
  } else if(ticks_value & 0x0000FF00) {
    length = 2;
  } else {
    length = 1;
  }

  /* check if the integer could be interpreted negative during a signed decode and prepend a zero-byte if necessary */
  if((ticks_value >> (8 * (length - 1))) & 0x80) {
    length++;
  }

  *buffer++ = type;
  *buffer++ = length;
  while(length--)
    *buffer++ = (ticks_value >> (8 * length)) & 0xFF;

  data->encoded_length = buffer - data->buffer;

  return 0;
}
static int
encode_byte_array(snmp_data_t *data, const char *string, size_t len)
{
  unsigned char *buffer;

  if(!string) {
    return 2;
  }

  /* if ((len + 4) > data->max_length) { */
  /*	data->max_length = len + 4; */
  /*	data->buffer = realloc(data->buffer, data->max_length); */
  /*	if (!data->buffer) */
  /*		return 2; */
  /* } */

  if(len > 0xFFFF) {
    LOG_INFO("Failed encoding: OCTET STRING overflow\n");
    return -1;
  }

  buffer = data->buffer;
  *buffer++ = BER_TYPE_OCTET_STRING;
  if(len > 255) {
    *buffer++ = 0x82;
    *buffer++ = (len >> 8) & 0xFF;
    *buffer++ = len & 0xFF;
  } else if(len > 127) {
    *buffer++ = 0x81;
    *buffer++ = len & 0xFF;
  } else {
    *buffer++ = len & 0x7F;
  }

  while(len--)
    *buffer++ = *string++;

  data->encoded_length = buffer - data->buffer;

  return 0;
}
static int
encode_string(snmp_data_t *data, const char *string)
{
  if(!string) {
    return 2;
  }

  return encode_byte_array(data, string, strlen(string));
}
/*
 * Set data buffer to its new value, depending on the type.
 *
 * Note: we assume the buffer was allocated to hold the maximum possible
 *       value when the MIB was built.
 */
static int
data_set(snmp_data_t *data, int type, const void *arg)
{
  /* Make sure to always initialize the buffer, in case of error below. */
  memset(data->buffer, 0, data->max_length);

  switch(type) {
  case BER_TYPE_INTEGER:
  /* return encode_integer(data, (intptr_t)arg); */

  case BER_TYPE_OCTET_STRING:
    return encode_string(data, (const char *)arg);

  case BER_TYPE_OID:
  /* return encode_oid(data, oid_aton((const char *)arg)); */

  case BER_TYPE_COUNTER:
  case BER_TYPE_GAUGE:
  case BER_TYPE_TIME_TICKS:
    return encode_unsigned(data, type, (uintptr_t)arg);

  default:
    break;    /* Fall through */
  }

  return 1;
}
static int
mib_data_set(const snmp_oid_t *oid, snmp_data_t *data, int column, int row, int type, const void *arg)
{
  int ret;

  ret = data_set(data, type, arg);
  if(ret) {
    if(ret == 1) {
      LOG_INFO("%s '%s.%d.%d': unsupported type %d\n", "Failed assigning value to OID", snmp_oid_ntoa(oid), column, row, type);
    } else if(ret == 2) {
      LOG_INFO("%s '%s.%d.%d': invalid default value\n", "Failed assigning value to OID", snmp_oid_ntoa(oid), column, row);
    }

    return -1;
  }

  return 0;
}
/*
 * Calculate the encoded length of the created OID (note: first the length
 * of the subid list, then the length of the length/type header!)
 */
static int
encode_oid_len(snmp_oid_t *oid)
{
  uint32_t len = 1;
  size_t i;

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

  if(len > 0xFFFF) {
    LOG_INFO("Failed encoding '%s': OID overflow\n", snmp_oid_ntoa(oid));
    oid->encoded_length = -1;
    return -1;
  }

  if(len > 0xFF) {
    len += 4;
  } else if(len > 0x7F) {
    len += 3;
  } else {
    len += 2;
  }

  oid->encoded_length = (short)len;

  return 0;
}
/* static int encode_oid(snmp_data_t *data, const snmp_oid_t *oid) */
/* { */
/*	size_t i, len = 1; */
/*	unsigned char *buffer = data->buffer; */
/*  */
/*	if (!oid) */
/*		return 2; */
/*  */
/*	for (i = 2; i < oid->subid_list_length; i++) { */
/*		if (oid->subid_list[i] >= (1 << 28)) */
/*			len += 5; */
/*		else if (oid->subid_list[i] >= (1 << 21)) */
/*			len += 4; */
/*		else if (oid->subid_list[i] >= (1 << 14)) */
/*			len += 3; */
/*		else if (oid->subid_list[i] >= (1 << 7)) */
/*			len += 2; */
/*		else */
/*			len += 1; */
/*	} */
/*  */
/*	if (len > 0xFFFF) { */
/*		LOG_INFO("Failed encoding '%s': OID overflow\n", snmp_oid_ntoa(oid)); */
/*		return -1; */
/*	} */
/*  */
/*	*buffer++ = BER_TYPE_OID; */
/*	if (len > 0xFF) { */
/*		*buffer++ = 0x82; */
/*		*buffer++ = (len >> 8) & 0xFF; */
/*		*buffer++ = len & 0xFF; */
/*	} else if (len > 0x7F) { */
/*		*buffer++ = 0x81; */
/*		*buffer++ = len & 0xFF; */
/*	} else { */
/*		*buffer++ = len & 0x7F; */
/*	} */
/*  */
/*	*buffer++ = oid->subid_list[0] * 40 + oid->subid_list[1]; */
/*	for (i = 2; i < oid->subid_list_length; i++) { */
/*		if (oid->subid_list[i] >= (1 << 28)) */
/*			len = 5; */
/*		else if (oid->subid_list[i] >= (1 << 21)) */
/*			len = 4; */
/*		else if (oid->subid_list[i] >= (1 << 14)) */
/*			len = 3; */
/*		else if (oid->subid_list[i] >= (1 << 7)) */
/*			len = 2; */
/*		else */
/*			len = 1; */
/*  */
/*		while (len--) { */
/*			if (len) */
/*				*buffer++ = ((oid->subid_list[i] >> (7 * len)) & 0x7F) | 0x80; */
/*			else */
/*				*buffer++ = (oid->subid_list[i] >> (7 * len)) & 0x7F; */
/*		} */
/*	} */
/*  */
/*	data->encoded_length = buffer - data->buffer; */
/*  */
/*	return 0; */
/* } */

/* Create OID from the given prefix, column, and row */
static int
oid_build(snmp_oid_t *oid, const snmp_oid_t *prefix, int column, int row)
{
  memcpy(oid, prefix, sizeof(*oid));

  if(oid->subid_list_length >= MAX_NR_SUBIDS) {
    return -1;
  }

  oid->subid_list[oid->subid_list_length++] = column;

  if(oid->subid_list_length >= MAX_NR_SUBIDS) {
    return -1;
  }

  oid->subid_list[oid->subid_list_length++] = row;

  return 0;
}
static snmp_value_t *
mib_alloc_entry(const snmp_oid_t *prefix, int column, int row, int type)
{
  int ret;
  snmp_value_t *value;
  const char *msg = "Failed creating MIB entry";

  /* Create a new entry in the MIB table */
  if(g_mib_length >= MAX_NR_VALUES) {
    LOG_INFO("%s '%s.%d.%d': table overflow\n", msg, snmp_oid_ntoa(prefix), column, row);
    return NULL;
  }

  value = &g_mib[g_mib_length++];
  memcpy(&value->oid, prefix, sizeof(value->oid));

  /* Create the OID from the prefix, the column and the row */
  if(oid_build(&value->oid, prefix, column, row)) {
    LOG_INFO("%s '%s.%d.%d': oid overflow\n", msg, snmp_oid_ntoa(prefix), column, row);
    return NULL;
  }

  ret = encode_oid_len(&value->oid);
  ret += data_alloc(&value->data, type);
  if(ret) {
    LOG_INFO("%s '%s.%d.%d': unsupported type %d\n", msg,
             snmp_oid_ntoa(&value->oid), column, row, type);
    return NULL;
  }

  return value;
}
static snmp_value_t *
mib_value_find(const snmp_oid_t *prefix, int column, int row, size_t *pos)
{
  snmp_oid_t oid;
  snmp_value_t *value;
  const char *msg = "Failed updating OID";

  memcpy(&oid, prefix, sizeof(oid));

  /* Create the OID from the prefix, the column and the row */
  if(oid_build(&oid, prefix, column, row)) {
    LOG_INFO("%s '%s.%d.%d': OID overflow\n", msg, snmp_oid_ntoa(prefix), column, row);
    return NULL;
  }

  /* Search the MIB for the given OID beginning at the given position */
  value = mib_find(&oid, pos);
  if(!value) {
    LOG_INFO("%s '%s.%d.%d': OID not found\n", msg, snmp_oid_ntoa(prefix), column, row);
  }

  return value;
}
static int
mib_build_entry(const snmp_oid_t *prefix, int column, int row, int type, const void *arg)
{
  snmp_value_t *value;

  value = mib_alloc_entry(prefix, column, row, type);
  if(!value) {
    return -1;
  }

  return mib_data_set(&value->oid, &value->data, column, row, type, arg);
}
static int
mib_update_entry(const snmp_oid_t *prefix, int column, int row, size_t *pos, int type, const void *arg)
{
  snmp_value_t *value;

  value = mib_value_find(prefix, column, row, pos);
  if(!value) {
    return -1;
  }

  return mib_data_set(prefix, &value->data, column, row, type, arg);
}
int
snmp_mib_build()
{

  /*
   * The system MIB: basic info about the host (SNMPv2-MIB.txt)
   */
  if(mib_build_entry(&snmp_mib_system_oid, 1, 0, BER_TYPE_OCTET_STRING, (char *)CONTIKI_VERSION_STRING) == -1) {
    return -1;
  }

  /*
   * The host MIB: additional host info (HOST-RESOURCES-MIB.txt)
   */
  if(mib_build_entry(&snmp_mib_host_oid, 1, 0, BER_TYPE_TIME_TICKS, (const void *)(unsigned long)(clock_seconds() * 100)) == -1) {
    return -1;
  }

  return 0;
}
int
snmp_mib_update()
{
  size_t pos;

  /* Begin searching at the first MIB entry */
  pos = 0;

  /*
   * The host MIB: additional host info (HOST-RESOURCES-MIB.txt)
   */
  if(mib_update_entry(&snmp_mib_host_oid, 1, 0, &pos, BER_TYPE_TIME_TICKS, (const void *)(unsigned long)(clock_seconds() * 100)) == -1) {
    return -1;
  }

  return 0;
}
/* Find the OID in the MIB that is exactly the given one or a subid */
snmp_value_t *
mib_find(const snmp_oid_t *oid, size_t *pos)
{
  while(*pos < g_mib_length) {
    snmp_value_t *curr = &g_mib[*pos];
    size_t len = oid->subid_list_length * sizeof(oid->subid_list[0]);

    if(curr->oid.subid_list_length >= oid->subid_list_length &&
       !memcmp(curr->oid.subid_list, oid->subid_list, len)) {
      return curr;
    }
    *pos = *pos + 1;
  }

  return NULL;
}
/* Find the OID in the MIB that is the one after the given one */
snmp_value_t *
snmp_mib_findnext(const snmp_oid_t *oid)
{
  size_t pos;

  for(pos = 0; pos < g_mib_length; pos++) {
    if(snmp_oid_cmp(&g_mib[pos].oid, oid) > 0) {
      return &g_mib[pos];
    }
  }

  return NULL;
}
