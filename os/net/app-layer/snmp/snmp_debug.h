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
#ifndef SNMP_DEBUG_H_
#define SNMP_DEBUG_H_

#include "snmp.h"

#ifdef SNMP_DEBUG

void snmp_debug_dump_response(const response_t *response);

#endif /* SNMP_DEBUG */

#endif /* SNMP_DEBUG_H_ */

