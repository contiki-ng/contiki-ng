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

#ifdef SNMP_CONF_COMMUNITY
#define SNMP_COMMUNITY SNMP_CONF_COMMUNITY
#else
#define SNMP_COMMUNITY "public"
#endif

#ifdef SNMP_CONF_DEBUG
#define SNMP_DEBUG SNMP_CONF_DEBUG
#elif LOG_CONF_LEVEL_SNMP == LOG_LEVEL_DBG
#define SNMP_DEBUG (1)
#else
#define SNMP_DEBUG (1)
#endif
