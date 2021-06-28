/*
 * Copyright (c) 2014, SICS Swedish ICT.
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
 *         Log functions for TSCH, meant for logging from interrupt
 *         during a timeslot operation. Saves ASN, slot and link information
 *         and adds the log to a ringbuf for later printout.
 * \author
 *         Simon Duquennoy <simonduq@sics.se>
 *
 */

/**
 * \addtogroup tsch
 * @{
*/

#include "contiki.h"
#include <stdio.h>
#include <inttypes.h>
#include "net/mac/tsch/tsch.h"
#include "lib/ringbufindex.h"
#include "sys/log.h"

#if TSCH_LOG_PER_SLOT

PROCESS_NAME(tsch_pending_events_process);

/* Check if TSCH_LOG_QUEUE_LEN is a power of two */
#if (TSCH_LOG_QUEUE_LEN & (TSCH_LOG_QUEUE_LEN - 1)) != 0
#error TSCH_LOG_QUEUE_LEN must be power of two
#endif
static struct ringbufindex log_ringbuf;
static struct tsch_log_t log_array[TSCH_LOG_QUEUE_LEN];
static int log_dropped = 0;
static int log_active = 0;

/*---------------------------------------------------------------------------*/
/* Process pending log messages */
void
tsch_log_process_pending(void)
{
  static int last_log_dropped = 0;
  int16_t log_index;
  /* Loop on accessing (without removing) a pending input packet */
  if(log_dropped != last_log_dropped) {
    printf("[WARN: TSCH-LOG  ] logs dropped %u\n", log_dropped);
    last_log_dropped = log_dropped;
  }
  while((log_index = ringbufindex_peek_get(&log_ringbuf)) != -1) {
    struct tsch_log_t *log = &log_array[log_index];
    if(log->link == NULL) {
      printf("[INFO: TSCH-LOG  ] {asn %02x.%08"PRIx32" link-NULL} ", log->asn.ms1b, log->asn.ls4b);
    } else {
      struct tsch_slotframe *sf = tsch_schedule_get_slotframe_by_handle(log->link->slotframe_handle);
      printf("[INFO: TSCH-LOG  ] {asn %02x.%08"PRIx32" link %2u %3u %3u %2u %2u ch %2u} ",
             log->asn.ms1b, log->asn.ls4b,
             log->link->slotframe_handle, sf ? sf->size.val : 0,
             log->burst_count, log->link->timeslot + log->burst_count, log->channel_offset,
             log->channel);
    }
    switch(log->type) {
      case tsch_log_tx:
        printf("%s-%u-%u tx ",
                linkaddr_cmp(&log->tx.dest, &linkaddr_null) ? "bc" : "uc", log->tx.is_data, log->tx.sec_level);
        log_lladdr_compact(&linkaddr_node_addr);
        printf("->");
        log_lladdr_compact(&log->tx.dest);
        printf(", len %3u, seq %3u, st %d %2d",
                log->tx.datalen, log->tx.seqno, log->tx.mac_tx_status, log->tx.num_tx);
        if(log->tx.drift_used) {
          printf(", dr %3d", log->tx.drift);
        }
        printf("\n");
        break;
      case tsch_log_rx:
        printf("%s-%u-%u rx ",
                log->rx.is_unicast == 0 ? "bc" : "uc", log->rx.is_data, log->rx.sec_level);
        log_lladdr_compact(&log->rx.src);
        printf("->");
        log_lladdr_compact(log->rx.is_unicast ? &linkaddr_node_addr : NULL);
        printf(", len %3u, seq %3u",
                log->rx.datalen, log->rx.seqno);
        printf(", edr %3d", (int)log->rx.estimated_drift);
        if(log->rx.drift_used) {
          printf(", dr %3d\n", log->rx.drift);
        } else {
          printf("\n");
        }
        break;
      case tsch_log_message:
        printf("%s\n", log->message);
        break;
    }
    /* Remove input from ringbuf */
    ringbufindex_get(&log_ringbuf);
  }
}
/*---------------------------------------------------------------------------*/
/* Prepare addition of a new log.
 * Returns pointer to log structure if success, NULL otherwise */
struct tsch_log_t *
tsch_log_prepare_add(void)
{
  int log_index = ringbufindex_peek_put(&log_ringbuf);
  if(log_index != -1) {
    struct tsch_log_t *log = &log_array[log_index];
    log->asn = tsch_current_asn;
    log->link = current_link;
    log->burst_count = tsch_current_burst_count;
    log->channel = tsch_current_channel;
    log->channel_offset = tsch_current_channel_offset;
    return log;
  } else {
    log_dropped++;
    return NULL;
  }
}
/*---------------------------------------------------------------------------*/
/* Actually add the previously prepared log */
void
tsch_log_commit(void)
{
  if(log_active == 1) {
    ringbufindex_put(&log_ringbuf);
    process_poll(&tsch_pending_events_process);
  }
}
/*---------------------------------------------------------------------------*/
/* Initialize log module */
void
tsch_log_init(void)
{
  if(log_active == 0) {
    ringbufindex_init(&log_ringbuf, TSCH_LOG_QUEUE_LEN);
    log_active = 1;
  }
}
/*---------------------------------------------------------------------------*/
/* Stop log module */
void
tsch_log_stop(void)
{
  if(log_active == 1) {
    tsch_log_process_pending();
    log_active = 0;
  }
}

#endif /* TSCH_LOG_PER_SLOT */
/** @} */
