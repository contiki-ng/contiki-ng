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

#include "snmp-api.h"

/*---------------------------------------------------------------------------*/
static void
sysDescr_handler(snmp_varbind_t *varbind, snmp_oid_t *oid);

MIB_RESOURCE(sysDescr, sysDescr_handler, 1, 3, 6, 1, 2, 1, 1, 1, 0);

static void
sysDescr_handler(snmp_varbind_t *varbind, snmp_oid_t *oid)
{
  snmp_api_set_string(varbind, oid, CONTIKI_VERSION_STRING);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
sysObjectID_handler(snmp_varbind_t *varbind, snmp_oid_t *oid);

MIB_RESOURCE(sysObjectID, sysObjectID_handler, 1, 3, 6, 1, 2, 1, 1, 2, 0);

static void
sysObjectID_handler(snmp_varbind_t *varbind, snmp_oid_t *oid)
{
  OID(sysObjectID_oid, 1, 3, 6, 1, 4, 1, 54352);
  snmp_api_set_oid(varbind, oid, &sysObjectID_oid);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
sysUpTime_handler(snmp_varbind_t *varbind, snmp_oid_t *oid);

MIB_RESOURCE(sysUpTime, sysUpTime_handler, 1, 3, 6, 1, 2, 1, 1, 3, 0);

static void
sysUpTime_handler(snmp_varbind_t *varbind, snmp_oid_t *oid)
{
  snmp_api_set_time_ticks(varbind, oid, clock_seconds() * 100);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
sysContact_handler(snmp_varbind_t *varbind, snmp_oid_t *oid);

MIB_RESOURCE(sysContact, sysContact_handler, 1, 3, 6, 1, 2, 1, 1, 4, 0);

static void
sysContact_handler(snmp_varbind_t *varbind, snmp_oid_t *oid)
{
  snmp_api_set_string(varbind, oid, "Contiki-NG, https://github.com/contiki-ng/contiki-ng");
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
sysName_handler(snmp_varbind_t *varbind, snmp_oid_t *oid);

MIB_RESOURCE(sysName, sysName_handler, 1, 3, 6, 1, 2, 1, 1, 5, 0);

static void
sysName_handler(snmp_varbind_t *varbind, snmp_oid_t *oid)
{
  snmp_api_set_string(varbind, oid, "Contiki-NG - "CONTIKI_TARGET_STRING);
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
sysLocation_handler(snmp_varbind_t *varbind, snmp_oid_t *oid);

MIB_RESOURCE(sysLocation, sysLocation_handler, 1, 3, 6, 1, 2, 1, 1, 6, 0);

static void
sysLocation_handler(snmp_varbind_t *varbind, snmp_oid_t *oid)
{
  snmp_api_set_string(varbind, oid, "");
}
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
static void
sysServices_handler(snmp_varbind_t *varbind, snmp_oid_t *oid);

MIB_RESOURCE(sysServices, sysServices_handler, 1, 3, 6, 1, 2, 1, 1, 7, 0);

static void
sysServices_handler(snmp_varbind_t *varbind, snmp_oid_t *oid)
{
  snmp_api_set_time_ticks(varbind, oid, clock_seconds() * 100);
}
/*---------------------------------------------------------------------------*/
