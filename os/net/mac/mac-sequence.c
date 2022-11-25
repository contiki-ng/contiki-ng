/*
 * Copyright (c) 2010, Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Copyright (c) 2013, ADVANSEE - http://www.advansee.com/
 * Benoît Thébaudeau <benoit.thebaudeau@advansee.com>
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         MAC sequence numbers management
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Benoît Thébaudeau <benoit.thebaudeau@advansee.com>
 */

#include <string.h>

#include "contiki-net.h"
#include "lib/random.h"
#include "net/mac/mac-sequence.h"
#include "net/packetbuf.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "mac-sequence"
#define LOG_LEVEL LOG_LEVEL_MAC

#ifdef NETSTACK_CONF_MAC_SEQNO_MAX_AGE
#define SEQNO_MAX_AGE NETSTACK_CONF_MAC_SEQNO_MAX_AGE
#else /* NETSTACK_CONF_MAC_SEQNO_MAX_AGE */
#define SEQNO_MAX_AGE (20 /* seconds */)
#endif /* NETSTACK_CONF_MAC_SEQNO_MAX_AGE */

#ifdef MAC_SEQUENCE_CONF_WITH_BROADCAST_DEDUPLICATION
#define WITH_BROADCAST_DEDUPLICATION MAC_SEQUENCE_CONF_WITH_BROADCAST_DEDUPLICATION
#else /* MAC_SEQUENCE_CONF_WITH_BROADCAST_DEDUPLICATION */
#define WITH_BROADCAST_DEDUPLICATION 0
#endif /* MAC_SEQUENCE_CONF_WITH_BROADCAST_DEDUPLICATION */

#define BEFORE(a, b) ((int16_t)((a) - (b)) < 0)

struct deduplication_record {
#if WITH_BROADCAST_DEDUPLICATION
  uint16_t last_broadcast_seqno_timeout;
#endif /* WITH_BROADCAST_DEDUPLICATION */
  uint16_t last_unicast_seqno_timeout;
#if WITH_BROADCAST_DEDUPLICATION
  uint8_t last_broadcast_seqno;
#endif /* WITH_BROADCAST_DEDUPLICATION */
  uint8_t last_unicast_seqno;
};

static uint8_t mac_dsn;
NBR_TABLE(struct deduplication_record, deduplication_table);

/*---------------------------------------------------------------------------*/
void
mac_sequence_init(void)
{
  mac_dsn = random_rand();
  nbr_table_register(deduplication_table, NULL);
}
/*---------------------------------------------------------------------------*/
void
mac_sequence_set_dsn(void)
{
  packetbuf_set_attr(PACKETBUF_ATTR_MAC_SEQNO, mac_dsn++);
}
/*---------------------------------------------------------------------------*/
enum mac_sequence_result
mac_sequence_is_duplicate(void)
{
  uint16_t now;
  uint8_t seqno;
  const linkaddr_t *source_address;
  struct deduplication_record *record;

#if !WITH_BROADCAST_DEDUPLICATION
  if(packetbuf_holds_broadcast()) {
    return MAC_SEQUENCE_NO_DUPLICATE;
  }
#endif /* !WITH_BROADCAST_DEDUPLICATION */

  now = clock_seconds() & 0xFFFF;
  seqno = packetbuf_attr(PACKETBUF_ATTR_MAC_SEQNO);
  source_address = packetbuf_addr(PACKETBUF_ADDR_SENDER);
  record = nbr_table_get_from_lladdr(deduplication_table, source_address);

  if(!record) {
    /* allocate new deduplication record */
    record = nbr_table_add_lladdr(deduplication_table,
        source_address,
        NBR_TABLE_REASON_MAC,
        NULL);
    if(!record) {
      LOG_WARN("Could not allocate deduplication record\n");
      return MAC_SEQUENCE_ERROR;
    }
    record->last_unicast_seqno
#if WITH_BROADCAST_DEDUPLICATION
        = record->last_broadcast_seqno
#endif /* WITH_BROADCAST_DEDUPLICATION */
        = seqno;
    record->last_unicast_seqno_timeout
#if WITH_BROADCAST_DEDUPLICATION
        = record->last_broadcast_seqno_timeout
#endif /* WITH_BROADCAST_DEDUPLICATION */
        = now + SEQNO_MAX_AGE;
#if WITH_BROADCAST_DEDUPLICATION
  } else if(packetbuf_holds_broadcast()) {
    /* check for duplicate broadcast frame */
    if((record->last_broadcast_seqno == seqno)
        && BEFORE(record->last_broadcast_seqno_timeout, now)) {
      return MAC_SEQUENCE_DUPLICATE;
    }
    record->last_broadcast_seqno = seqno;
    record->last_broadcast_seqno_timeout = now + SEQNO_MAX_AGE;
#endif /* WITH_BROADCAST_DEDUPLICATION */
  } else {
    /* check for duplicate unicast frame */
    if((record->last_unicast_seqno == seqno)
        && BEFORE(record->last_unicast_seqno_timeout, now)) {
      return MAC_SEQUENCE_DUPLICATE;
    }
    record->last_unicast_seqno = seqno;
    record->last_unicast_seqno_timeout = now + SEQNO_MAX_AGE;
  }
  return MAC_SEQUENCE_NO_DUPLICATE;
}
/*---------------------------------------------------------------------------*/
