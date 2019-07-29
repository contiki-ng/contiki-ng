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
#include "snmp_asn1.h"
#include "snmp_mib.h"
#include "snmp_utils.h"

#define LOG_MODULE "SNMP [mib]"
#define LOG_LEVEL LOG_LEVEL_SNMP

snmp_value_t g_mib[SNMP_MAX_MIB_SIZE];
size_t g_mib_length = 0;

static const snmp_oid_t snmp_mib_system_oid = { { 1, 3, 6, 1, 2, 1, 1 }, 7, 8 };
static const snmp_oid_t snmp_mib_host_oid = { { 1, 3, 6, 1, 2, 1, 25, 1 }, 8, 9 };

#define CONTIKI_NG_IANA 10
static const snmp_oid_t snmp_mib_contiki_ng_oid = { { 1, 3, 6, 1, 4, 1, CONTIKI_NG_IANA }, 7, 8 };
static const snmp_oid_t snmp_mib_contiki_ng_modules_oid = { { 1, 3, 6, 1, 4, 1, CONTIKI_NG_IANA, 1 }, 8, 9 };
static const snmp_oid_t snmp_mib_energest_module_oid = { { 1, 3, 6, 1, 4, 1, CONTIKI_NG_IANA, 1, 1 }, 9, 10 };

static int
snmp_mib_data_alloc(snmp_data_t *data, int type)
{
  switch(type) {
  case BER_TYPE_INTEGER:
    data->max_length = sizeof(int) + 2;
    data->encoded_length = 0;
    break;

  case BER_TYPE_OCTET_STRING:
    data->max_length = 4;
    data->encoded_length = 0;
    break;

  case BER_TYPE_OID:
    data->max_length = SNMP_MAX_NR_SUBIDS * 5 + 4;
    data->encoded_length = 0;
    break;

  case BER_TYPE_COUNTER:
  case BER_TYPE_GAUGE:
  case BER_TYPE_TIME_TICKS:
    data->max_length = sizeof(unsigned int) + 3;
    data->encoded_length = 0;
    break;

  default:
    return -1;
  }

  data->buffer[0] = type;
  data->buffer[1] = 0;
  data->buffer[2] = 0;
  data->encoded_length = 3;

  return 0;
}
/*
 * Set data buffer to its new value, depending on the type.
 *
 * Note: we assume the buffer was allocated to hold the maximum possible
 *       value when the MIB was built.
 */
static int
snmp_mib_data_set(snmp_data_t *data, int type, const void *arg)
{
  /* Make sure to always initialize the buffer, in case of error below. */
  memset(data->buffer, 0, data->max_length);

  switch(type) {
  case BER_TYPE_INTEGER:
    return snmp_asn1_encode_integer(data, (intptr_t)arg);

  case BER_TYPE_OCTET_STRING:
    return snmp_asn1_encode_string(data, (const char *)arg);

  case BER_TYPE_OID:
    return snmp_asn1_encode_oid(data, snmp_oid_aton((const char *)arg));

  case BER_TYPE_COUNTER:
  case BER_TYPE_GAUGE:
  case BER_TYPE_TIME_TICKS:
    return snmp_asn1_encode_unsigned(data, type, (uintptr_t)arg);

  default:
    break;    /* Fall through */
  }

  return 1;
}
static int
snmp_mib_mib_data_set(const snmp_oid_t *oid, snmp_data_t *data, int column, int row, int type, const void *arg)
{
  int ret;

  ret = snmp_mib_data_set(data, type, arg);
  if(ret) {
    if(ret == 1) {
      LOG_ERR("%s '%s.%d.%d': unsupported type %d\n", "Failed assigning value to OID", snmp_oid_ntoa(oid), column, row, type);
    } else if(ret == 2) {
      LOG_ERR("%s '%s.%d.%d': invalid default value\n", "Failed assigning value to OID", snmp_oid_ntoa(oid), column, row);
    }

    return -1;
  }

  return 0;
}
/* Create OID from the given prefix, column, and row */
static int
snmp_mib_oid_build(snmp_oid_t *oid, const snmp_oid_t *prefix, int column, int row)
{
  memcpy(oid, prefix, sizeof(*oid));

  if(oid->subid_list_length >= SNMP_MAX_NR_SUBIDS) {
    return -1;
  }

  oid->subid_list[oid->subid_list_length++] = column;

  if(oid->subid_list_length >= SNMP_MAX_NR_SUBIDS) {
    return -1;
  }

  oid->subid_list[oid->subid_list_length++] = row;

  return 0;
}
static snmp_value_t *
snmp_mib_alloc_entry(const snmp_oid_t *prefix, int column, int row, int type)
{
  int ret;
  snmp_value_t *value;
  const char *msg = "Failed creating MIB entry";

  /* Create a new entry in the MIB table */
  if(g_mib_length >= SNMP_MAX_MIB_SIZE) {
    LOG_ERR("%s '%s.%d.%d': table overflow\n", msg, snmp_oid_ntoa(prefix), column, row);
    return NULL;
  }

  value = &g_mib[g_mib_length++];
  memcpy(&value->oid, prefix, sizeof(value->oid));

  /* Create the OID from the prefix, the column and the row */
  if(snmp_mib_oid_build(&value->oid, prefix, column, row)) {
    LOG_ERR("%s '%s.%d.%d': oid overflow\n", msg, snmp_oid_ntoa(prefix), column, row);
    return NULL;
  }

  ret = snmp_asn1_encode_oid_len(&value->oid);
  ret += snmp_mib_data_alloc(&value->data, type);
  if(ret) {
    LOG_ERR("%s '%s.%d.%d': unsupported type %d\n", msg,
            snmp_oid_ntoa(&value->oid), column, row, type);
    return NULL;
  }

  return value;
}
static snmp_value_t *
snmp_mib_value_find(const snmp_oid_t *prefix, int column, int row, size_t *pos)
{
  snmp_oid_t oid;
  snmp_value_t *value;
  const char *msg = "Failed updating OID";

  memcpy(&oid, prefix, sizeof(oid));

  /* Create the OID from the prefix, the column and the row */
  if(snmp_mib_oid_build(&oid, prefix, column, row)) {
    LOG_ERR("%s '%s.%d.%d': OID overflow\n", msg, snmp_oid_ntoa(prefix), column, row);
    return NULL;
  }

  /* Search the MIB for the given OID beginning at the given position */
  value = snmp_mib_find(&oid, pos);
  if(!value) {
    LOG_ERR("%s '%s.%d.%d': OID not found\n", msg, snmp_oid_ntoa(prefix), column, row);
    return NULL;
  }

  return value;
}
static int
snmp_mib_build_entry(const snmp_oid_t *prefix, int column, int row, int type, const void *arg)
{
  snmp_value_t *value;

  value = snmp_mib_alloc_entry(prefix, column, row, type);
  if(!value) {
    return -1;
  }

  return snmp_mib_mib_data_set(&value->oid, &value->data, column, row, type, arg);
}
static int
snmp_mib_update_entry(const snmp_oid_t *prefix, int column, int row, size_t *pos, int type, const void *arg)
{
  snmp_value_t *value;

  value = snmp_mib_value_find(prefix, column, row, pos);
  if(!value) {
    return -1;
  }

  return snmp_mib_mib_data_set(prefix, &value->data, column, row, type, arg);
}
int
snmp_mib_build()
{

  /*
   * The system MIB: basic info about the host (SNMPv2-MIB.txt)
   */
  if(snmp_mib_build_entry(&snmp_mib_system_oid, 1, 0, BER_TYPE_OCTET_STRING, (char *)CONTIKI_VERSION_STRING) == -1) {
    return -1;
  }

  /*
   * The host MIB: additional host info (HOST-RESOURCES-MIB.txt)
   */
  if(snmp_mib_build_entry(&snmp_mib_host_oid, 1, 0, BER_TYPE_TIME_TICKS, (const void *)(unsigned long)(clock_seconds() * 100)) == -1) {
    return -1;
  }

  /*
   * The host MIB: additional host info (CONTIKI-NG-ENERGEST-MIB.mib)
   */
#if ENERGEST_CONF_ON
  energest_flush();
  if((snmp_mib_build_entry(&snmp_mib_energest_module_oid, 1, 0, BER_TYPE_INTEGER,
                           (const void *)(int)(1)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 2, 0, BER_TYPE_COUNTER,
                              (const void *)(unsigned long)((unsigned long)(ENERGEST_GET_TOTAL_TIME() / ENERGEST_SECOND))) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 3, 0, BER_TYPE_COUNTER,
                              (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_CPU) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 4, 0, BER_TYPE_COUNTER,
                              (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_LPM) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 5, 0, BER_TYPE_COUNTER,
                              (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_LPM) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 6, 0, BER_TYPE_COUNTER,
                              (const void *)(unsigned long)((unsigned long)((ENERGEST_GET_TOTAL_TIME()
                                                                             - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                                                                             - energest_type_time(ENERGEST_TYPE_LISTEN)) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 7, 0, BER_TYPE_COUNTER,
                              (const void *)(unsigned long)((unsigned long)((energest_type_time(ENERGEST_TYPE_LISTEN)
                                                                             + energest_type_time(ENERGEST_TYPE_TRANSMIT)) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 8, 0, BER_TYPE_COUNTER,
                              (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_LISTEN) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 9, 0, BER_TYPE_COUNTER,
                              (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_TRANSMIT) / ENERGEST_SECOND))) == -1)) {
    return -1;
  }
#else /* ENERGEST_CONF_ON */
  if((snmp_mib_build_entry(&snmp_mib_energest_module_oid, 1, 0, BER_TYPE_INTEGER, (const void *)(int)(0)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 2, 0, BER_TYPE_COUNTER, (const void *)(unsigned long)(0)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 3, 0, BER_TYPE_COUNTER, (const void *)(unsigned long)(0)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 4, 0, BER_TYPE_COUNTER, (const void *)(unsigned long)(0)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 5, 0, BER_TYPE_COUNTER, (const void *)(unsigned long)(0)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 6, 0, BER_TYPE_COUNTER, (const void *)(unsigned long)(0)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 7, 0, BER_TYPE_COUNTER, (const void *)(unsigned long)(0)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 8, 0, BER_TYPE_COUNTER, (const void *)(unsigned long)(0)) == -1)
     || (snmp_mib_build_entry(&snmp_mib_energest_module_oid, 9, 0, BER_TYPE_COUNTER, (const void *)(unsigned long)(0)) == -1)) {
    return -1;
  }

#endif /* ENERGEST_CONF_ON */

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
  if(snmp_mib_update_entry(&snmp_mib_host_oid, 1, 0, &pos, BER_TYPE_TIME_TICKS, (const void *)(unsigned long)(clock_seconds() * 100)) == -1) {
    return -1;
  }

  /*
   * The host MIB: additional host info (CONTIKI-NG-ENERGEST-MIB.mib)
   */
#if ENERGEST_CONF_ON
  energest_flush();
  if((snmp_mib_update_entry(&snmp_mib_energest_module_oid, 1, 0, &pos, BER_TYPE_INTEGER,
                            (const void *)(int)(1)) == -1)
     || (snmp_mib_update_entry(&snmp_mib_energest_module_oid, 2, 0, &pos, BER_TYPE_COUNTER,
                               (const void *)(unsigned long)((unsigned long)(ENERGEST_GET_TOTAL_TIME() / ENERGEST_SECOND))) == -1)
     || (snmp_mib_update_entry(&snmp_mib_energest_module_oid, 3, 0, &pos, BER_TYPE_COUNTER,
                               (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_CPU) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_update_entry(&snmp_mib_energest_module_oid, 4, 0, &pos, BER_TYPE_COUNTER,
                               (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_LPM) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_update_entry(&snmp_mib_energest_module_oid, 5, 0, &pos, BER_TYPE_COUNTER,
                               (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_LPM) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_update_entry(&snmp_mib_energest_module_oid, 6, 0, &pos, BER_TYPE_COUNTER,
                               (const void *)(unsigned long)((unsigned long)((ENERGEST_GET_TOTAL_TIME()
                                                                              - energest_type_time(ENERGEST_TYPE_TRANSMIT)
                                                                              - energest_type_time(ENERGEST_TYPE_LISTEN)) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_update_entry(&snmp_mib_energest_module_oid, 7, 0, &pos, BER_TYPE_COUNTER,
                               (const void *)(unsigned long)((unsigned long)((energest_type_time(ENERGEST_TYPE_LISTEN)
                                                                              + energest_type_time(ENERGEST_TYPE_TRANSMIT)) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_update_entry(&snmp_mib_energest_module_oid, 8, 0, &pos, BER_TYPE_COUNTER,
                               (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_LISTEN) / ENERGEST_SECOND))) == -1)
     || (snmp_mib_update_entry(&snmp_mib_energest_module_oid, 9, 0, &pos, BER_TYPE_COUNTER,
                               (const void *)(unsigned long)((unsigned long)(energest_type_time(ENERGEST_TYPE_TRANSMIT) / ENERGEST_SECOND))) == -1)) {
    return -1;
  }
#endif /* ENERGEST_CONF_ON */

  return 0;
}
/* Find the OID in the MIB that is exactly the given one or a subid */
snmp_value_t *
snmp_mib_find(const snmp_oid_t *oid, size_t *pos)
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

  *pos = -1;
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
