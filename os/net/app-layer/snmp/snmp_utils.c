/* Utility functions
 *
 * Copyright (C) 2008-2010  Robert Ernst <robert.ernst@linux-solutions.at>
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

#include "snmp_utils.h"

#include <stdio.h>

int
snmp_oid_cmp(const snmp_oid_t *oid1, const snmp_oid_t *oid2)
{
  int subid1, subid2;
  size_t i;

  for(i = 0; i < MAX_NR_OIDS; i++) {
    subid1 = (oid1->subid_list_length > i) ? (int)oid1->subid_list[i] : -1;
    subid2 = (oid2->subid_list_length > i) ? (int)oid2->subid_list[i] : -1;

    if(subid1 == -1 && subid2 == -1) {
      return 0;
    }
    if(subid1 > subid2) {
      return 1;
    }
    if(subid1 < subid2) {
      return -1;
    }
  }

  return 0;
}
char *
snmp_oid_ntoa(const snmp_oid_t *oid)
{
  size_t i, len = 0;
  static char snmp_oid_ntoa_buf[MAX_NR_SUBIDS * 10 + 2];

  snmp_oid_ntoa_buf[0] = '\0';
  for(i = 0; i < oid->subid_list_length; i++) {
    len += snprintf(snmp_oid_ntoa_buf + len, sizeof(snmp_oid_ntoa_buf) - len, ".%u", oid->subid_list[i]);
    if(len >= sizeof(snmp_oid_ntoa_buf)) {
      break;
    }
  }

  return snmp_oid_ntoa_buf;
}
