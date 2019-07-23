/*
 * Copyright (C) 2008 Robert Ernst <robert.ernst@linux-solutions.at>
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

#ifndef MINI_SNMPD_H_
#define MINI_SNMPD_H_

#include "contiki.h"
#include "contiki-net.h"

/* #define SNMP_DEBUG (1) */

#include <stddef.h> /* for size_t */

/*
 * Project dependent defines
 */
#define MAX_NR_CLIENTS                                  16
#define MAX_NR_OIDS                                     16
#define MAX_NR_SUBIDS                                   16
#define MAX_NR_DISKS                                    4
#define MAX_NR_INTERFACES                               8
#define MAX_NR_VALUES                                   12

#define MAX_PACKET_SIZE                                 2048
#define MAX_STRING_SIZE                                 64

/*
 * SNMP dependent defines
 */
#define SNMP_VERSION_1                                  0
#define SNMP_VERSION_2C                                 1
#define SNMP_VERSION_3                                  3

#define SNMP_STATUS_OK                                  0
#define SNMP_STATUS_TOO_BIG                             1
#define SNMP_STATUS_NO_SUCH_NAME                        2
#define SNMP_STATUS_BAD_VALUE                           3
#define SNMP_STATUS_READ_ONLY                           4
#define SNMP_STATUS_GEN_ERR                             5
#define SNMP_STATUS_NO_ACCESS                           6
#define SNMP_STATUS_WRONG_TYPE                          7
#define SNMP_STATUS_WRONG_LENGTH                        8
#define SNMP_STATUS_WRONG_ENCODING                      9
#define SNMP_STATUS_WRONG_VALUE                         10
#define SNMP_STATUS_NO_CREATION                         11
#define SNMP_STATUS_INCONSISTENT_VALUE                  12
#define SNMP_STATUS_RESOURCE_UNAVAILABLE                13
#define SNMP_STATUS_COMMIT_FAILED                       14
#define SNMP_STATUS_UNDO_FAILED                         15
#define SNMP_STATUS_AUTHORIZATION_ERROR                 16
#define SNMP_STATUS_NOT_WRITABLE                        17
#define SNMP_STATUS_INCONSISTENT_NAME                   18

#ifndef UNUSED
#define UNUSED(x) x __attribute__((unused))
#endif

/*
 * Data types
 */

typedef struct client_s {
  unsigned char packet[MAX_PACKET_SIZE];
  size_t size;
} snmp_client_t;

typedef struct oid_s {
  unsigned int subid_list[MAX_NR_SUBIDS];
  size_t subid_list_length;
  short encoded_length;
} snmp_oid_t;

typedef struct data_s {
  unsigned char buffer[(MAX_NR_SUBIDS * 5) + 4];
  /* unsigned char  *buffer; */
  size_t max_length;
  short encoded_length;
} snmp_data_t;

typedef struct value_s {
  snmp_oid_t oid;
  snmp_data_t data;
} snmp_value_t;

typedef struct field_s {
  char *prefix;

  size_t len;
  unsigned int *value[12];
} field_t;

typedef struct request_s {
  char community[MAX_STRING_SIZE];
  int type;
  int version;
  int id;
  uint32_t non_repeaters;
  uint32_t max_repetitions;
  snmp_oid_t oid_list[MAX_NR_OIDS];
  size_t oid_list_length;
} request_t;

typedef struct response_s {
  int error_status;
  int error_index;
  snmp_value_t value_list[MAX_NR_VALUES];
  size_t value_list_length;
} response_t;

/*
 * Functions
 */
void snmp_init();

/*
 * Globals
 */

extern snmp_value_t g_mib[MAX_NR_VALUES];
extern size_t g_mib_length;

#endif /* MINI_SNMPD_H_ */
