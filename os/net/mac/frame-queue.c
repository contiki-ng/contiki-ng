/*
 * Copyright (c) 2023, Uppsala universitet.
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

/**
 * \file
 *         Common functionality for scheduling retransmissions.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "net/mac/frame-queue.h"
#include "lib/assert.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "lib/random.h"
#include "net/mac/wake-up-counter.h"
#include "net/nbr-table.h"
#include "net/packetbuf.h"
#include "net/queuebuf.h"
#include "services/akes/akes-mac.h"
#include "sys/cc.h"

#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
#include "services/akes/akes-nbr.h"
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "frame-queue"
#define LOG_LEVEL LOG_LEVEL_MAC

/* macMaxFrameRetries */
#ifdef FRAME_QUEUE_CONF_MAX_RETRANSMISSIONS
#define MAX_RETRANSMISSIONS FRAME_QUEUE_CONF_MAX_RETRANSMISSIONS
#else /* FRAME_QUEUE_CONF_MAX_RETRANSMISSIONS */
#define MAX_RETRANSMISSIONS (3) /* default as per IEEE 802.15.4 */
#endif /* FRAME_QUEUE_CONF_MAX_RETRANSMISSIONS */

/* macMaxCsmaBackoffs */
#ifdef FRAME_QUEUE_CONF_MAX_CSMA_BACKOFF
#define MAX_CSMA_BACKOFF FRAME_QUEUE_CONF_MAX_CSMA_BACKOFF
#else /* FRAME_QUEUE_CONF_MAX_CSMA_BACKOFF */
#define MAX_CSMA_BACKOFF (4) /* default as per IEEE 802.15.4 */
#endif /* FRAME_QUEUE_CONF_MAX_CSMA_BACKOFF */

/* macMinBe */
#ifdef FRAME_QUEUE_CONF_MIN_BACKOFF_EXPONENT
#define MIN_BACKOFF_EXPONENT FRAME_QUEUE_CONF_MIN_BACKOFF_EXPONENT
#else /* FRAME_QUEUE_CONF_MIN_BACKOFF_EXPONENT */
#define MIN_BACKOFF_EXPONENT (3) /* default as per IEEE 802.15.4 */
#endif /* FRAME_QUEUE_CONF_MIN_BACKOFF_EXPONENT */

/* macMaxBe */
#ifdef FRAME_QUEUE_CONF_MAX_BACKOFF_EXPONENT
#define MAX_BACKOFF_EXPONENT FRAME_QUEUE_CONF_MAX_BACKOFF_EXPONENT
#else /* FRAME_QUEUE_CONF_MAX_BACKOFF_EXPONENT */
#define MAX_BACKOFF_EXPONENT (5) /* default as per IEEE 802.15.4 */
#endif /* FRAME_QUEUE_CONF_MAX_BACKOFF_EXPONENT */

/* aUnitBackoffPeriod */
#ifdef FRAME_QUEUE_CONF_BACKOFF_PERIOD
#define BACKOFF_PERIOD FRAME_QUEUE_CONF_BACKOFF_PERIOD
#else /* FRAME_QUEUE_CONF_BACKOFF_PERIOD */
#define BACKOFF_PERIOD (CLOCK_SECOND / WAKE_UP_COUNTER_RATE)
#endif /* FRAME_QUEUE_CONF_BACKOFF_PERIOD */

struct csmaca_status {
  clock_time_t next_attempt;
  bool is_active;
  uint8_t transmissions;
  uint8_t collisions;
};

LIST(frame_queue_list);
MEMB(frame_queue_memb, frame_queue_entry_t, QUEUEBUF_NUM);
static struct csmaca_status broadcast_csmaca_status;
NBR_TABLE(struct csmaca_status, unicast_csmaca_statuses);

/*---------------------------------------------------------------------------*/
void
frame_queue_init(void)
{
  list_init(frame_queue_list);
  memb_init(&frame_queue_memb);
  nbr_table_register(unicast_csmaca_statuses, NULL);
}
/*---------------------------------------------------------------------------*/
static struct csmaca_status *
get_csmaca_status(const linkaddr_t *addr)
{
  assert(!linkaddr_cmp(&linkaddr_node_addr, addr));
  struct csmaca_status *csmaca_status;
  if(linkaddr_cmp(addr, &linkaddr_null)) {
    csmaca_status = &broadcast_csmaca_status;
  } else {
    csmaca_status = nbr_table_get_from_lladdr(unicast_csmaca_statuses, addr);
    if(!csmaca_status) {
      csmaca_status = nbr_table_add_lladdr(unicast_csmaca_statuses,
                                           addr,
                                           NBR_TABLE_REASON_MAC,
                                           NULL);
      if(!csmaca_status) {
        LOG_ERR("nbr_table_add_lladdr failed\n");
        return NULL;
      }
      csmaca_status->is_active = false;
    }
    /* lock while having pending frames */
    nbr_table_lock(unicast_csmaca_statuses, csmaca_status);
  }
  if(!csmaca_status->is_active) {
    csmaca_status->is_active = true;
    csmaca_status->next_attempt = clock_time();
    csmaca_status->transmissions = 0;
    csmaca_status->collisions = 0;
  }
  return csmaca_status;
}
/*---------------------------------------------------------------------------*/
static void
release_csmaca_status(struct csmaca_status *csmaca_status)
{
  csmaca_status->is_active = false;
  if(csmaca_status != &broadcast_csmaca_status) {
    nbr_table_remove(unicast_csmaca_statuses, csmaca_status);
  }
}
/*---------------------------------------------------------------------------*/
bool
frame_queue_add(mac_callback_t sent, void *ptr)
{
  if(!packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS)) {
    packetbuf_set_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS,
                       MAX_RETRANSMISSIONS + 1);
  }
  frame_queue_entry_t *new_fqe = memb_alloc(&frame_queue_memb);
  if(!new_fqe) {
    LOG_ERR("buffer is full\n");
    mac_call_sent_callback(sent, ptr, MAC_TX_QUEUE_FULL, 0);
    return false;
  }
  new_fqe->qb = queuebuf_new_from_packetbuf();
  if(!new_fqe->qb) {
    LOG_ERR("queuebuf is full\n");
    memb_free(&frame_queue_memb, new_fqe);
    mac_call_sent_callback(sent, ptr, MAC_TX_QUEUE_FULL, 0);
    return false;
  }
  new_fqe->ptr = ptr;
  new_fqe->sent = sent;
#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
  new_fqe->neighbor_bitmap = 0;
  assert(AKES_NBR_MAX <= (sizeof(new_fqe->neighbor_bitmap) * 8));
  new_fqe->is_broadcast_as_unicast = packetbuf_holds_broadcast()
                                     && !akes_mac_is_hello();
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */
  list_add(frame_queue_list, new_fqe);
  return true;
}
/*---------------------------------------------------------------------------*/
static void
remove_entry(frame_queue_entry_t *fqe, int status, int num_tx)
{
  mac_callback_t sent = fqe->sent;
  void *ptr = fqe->ptr;
  queuebuf_free(fqe->qb);
  list_remove(frame_queue_list, fqe);
  memb_free(&frame_queue_memb, fqe);
  mac_call_sent_callback(sent, ptr, status, num_tx);
}
/*---------------------------------------------------------------------------*/
frame_queue_entry_t *
frame_queue_pick(void)
{
  clock_time_t now = clock_time();
  for(frame_queue_entry_t *fqe = frame_queue_head();
      fqe;
      fqe = frame_queue_next(fqe)) {
#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
    if(fqe->is_broadcast_as_unicast) {
      /* find a permanent neighbor that has not received this frame, yet */
      const struct csmaca_status *csmaca_status = NULL;
      for(akes_nbr_entry_t *entry = akes_nbr_head(AKES_NBR_PERMANENT);
          entry;
          entry = akes_nbr_next(entry, AKES_NBR_PERMANENT)) {
        size_t mask = 1ul << akes_nbr_index_of(entry->permanent);
        if(fqe->neighbor_bitmap & mask) {
          /* this neighbor received this frame already */
          continue;
        }
        csmaca_status = get_csmaca_status(akes_nbr_get_addr(entry));
        assert(csmaca_status);
        if(CLOCK_LT(now, csmaca_status->next_attempt)) {
          continue;
        }
        queuebuf_to_packetbuf(fqe->qb);
        packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, akes_nbr_get_addr(entry));
        return fqe;
      }
      if(!csmaca_status) {
        queuebuf_to_packetbuf(fqe->qb);
        remove_entry(fqe, MAC_TX_OK, 0);
        fqe = frame_queue_head();
      }
      continue;
    }
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */

    const struct csmaca_status *csmaca_status =
        get_csmaca_status(queuebuf_addr(fqe->qb, PACKETBUF_ADDR_RECEIVER));
    if(!csmaca_status) {
      LOG_ERR("could not get CSMA-CA status\n");
      queuebuf_to_packetbuf(fqe->qb);
      frame_queue_on_transmitted(MAC_TX_ERR_FATAL, fqe);
      fqe = frame_queue_head();
      continue;
    }

    if(CLOCK_LT(now, csmaca_status->next_attempt)) {
      continue;
    }

    queuebuf_to_packetbuf(fqe->qb);
    return fqe;
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
frame_queue_entry_t *
frame_queue_head(void)
{
  return list_head(frame_queue_list);
}
/*---------------------------------------------------------------------------*/
frame_queue_entry_t *
frame_queue_next(frame_queue_entry_t *fqe)
{
  return list_item_next(fqe);
}
/*---------------------------------------------------------------------------*/
frame_queue_entry_t *
frame_queue_burst(frame_queue_entry_t *fqe)
{
#if AKES_MAC_ENABLED
  if(packetbuf_holds_cmd_frame()
     && akes_mac_is_hello_helloack_or_ack(packetbuf_get_dispatch_byte())) {
    return NULL;
  }
#endif /* AKES_MAC_ENABLED */

#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
  akes_nbr_entry_t *entry = akes_nbr_get_receiver_entry();
  if(!entry || !entry->permanent) {
    LOG_ERR("neighbor disappeared\n");
    return NULL;
  }
  size_t mask = 1ul << akes_nbr_index_of(entry->permanent);
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */

  while((fqe = frame_queue_next(fqe))) {
#if AKES_MAC_ENABLED
    if(queuebuf_holds_cmd_frame(fqe->qb)
       && akes_mac_is_hello_helloack_or_ack(
           queuebuf_get_dispatch_byte(fqe->qb))) {
      continue;
    }
#endif /* AKES_MAC_ENABLED */

#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
    if(fqe->is_broadcast_as_unicast) {
      if(!(fqe->neighbor_bitmap & mask)) {
        return fqe;
      }
      continue;
    }
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */

    if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                    queuebuf_addr(fqe->qb, PACKETBUF_ADDR_RECEIVER))) {
      return fqe;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
frame_queue_postpone(clock_time_t duration)
{
  struct csmaca_status *csmaca_status =
      get_csmaca_status(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
  assert(csmaca_status);
  csmaca_status->next_attempt = clock_time() + duration;
}
/*---------------------------------------------------------------------------*/
static void
schedule_next_attempt(struct csmaca_status *csmaca_status)
{
  uint_fast8_t backoff_exponent = MIN(csmaca_status->collisions
                                      + csmaca_status->transmissions
                                      + MIN_BACKOFF_EXPONENT
                                      - 1,
                                      MAX_BACKOFF_EXPONENT);
  uint_fast8_t backoff_periods = ((1 << backoff_exponent) - 1) & random_rand();
  csmaca_status->next_attempt = clock_time()
                                + (BACKOFF_PERIOD * backoff_periods);
}
/*---------------------------------------------------------------------------*/
void
frame_queue_on_transmitted(int result, frame_queue_entry_t *fqe)
{
  assert(result != MAC_TX_DEFERRED);
  assert(result != MAC_TX_QUEUE_FULL);

  const linkaddr_t *addr = packetbuf_addr(PACKETBUF_ADDR_RECEIVER);
  struct csmaca_status *csmaca_status = get_csmaca_status(addr);
  if(!csmaca_status) {
    result = MAC_TX_ERR_FATAL;
  }

  switch(result) {
  case MAC_TX_ERR:
    LOG_WARN("Retrying to send in one backoff period\n");
    csmaca_status->next_attempt = clock_time() + BACKOFF_PERIOD;
    return;
  case MAC_TX_COLLISION:
    if(++csmaca_status->collisions <= MAX_CSMA_BACKOFF) {
      /* following Section 6.2.5.1 of IEEE 802.15.4-2020 */
      schedule_next_attempt(csmaca_status);
      return;
    }
    break;
  case MAC_TX_NOACK:
    if(++csmaca_status->transmissions
       < packetbuf_attr(PACKETBUF_ATTR_MAX_MAC_TRANSMISSIONS)) {
      /*
       * Deviating from IEEE 802.15.4, we not only back off after collisions
       * but also retransmissions. This is because CCAs do not work perfectly
       * (e.g., hidden terminal). Furthermore, in CSL, the receiver may be busy
       * with sending an unsynchronized transmission on a different channel.
       */
      schedule_next_attempt(csmaca_status);
      return;
    }
    break;
  case MAC_TX_OK:
    csmaca_status->transmissions++;
    break;
  default:
    break;
  }

  if(csmaca_status) {
    release_csmaca_status(csmaca_status);
  }

#if FRAME_QUEUE_BROADCASTS_AS_UNICASTS
  if(fqe->is_broadcast_as_unicast) {
    akes_mac_report_to_network_layer(result, transmissions);
    bool one_remaining = false;
    for(akes_nbr_entry_t *entry = akes_nbr_head(AKES_NBR_PERMANENT);
        entry;
        entry = akes_nbr_next(entry, AKES_NBR_PERMANENT)) {
      size_t mask = 1ul << akes_nbr_index_of(entry->permanent);
      if(linkaddr_cmp(packetbuf_addr(PACKETBUF_ADDR_RECEIVER),
                      akes_nbr_get_addr(entry))) {
        fqe->neighbor_bitmap |= mask;
      } else if(!(fqe->neighbor_bitmap & mask)) {
        one_remaining = true;
      }
    }
    if(!one_remaining) {
      /* finish broadcast */
      packetbuf_set_addr(PACKETBUF_ADDR_RECEIVER, &linkaddr_null);
      remove_entry(fqe, MAC_TX_OK, 0);
    }
    return;
  }
#endif /* FRAME_QUEUE_BROADCASTS_AS_UNICASTS */

  remove_entry(fqe, result, csmaca_status ? csmaca_status->transmissions : 0);
}
/*---------------------------------------------------------------------------*/
void
frame_queue_cancel_command(const linkaddr_t *receiver, uint_fast8_t cmd_id)
{
  for(frame_queue_entry_t *fqe = frame_queue_head();
      fqe;
      fqe = frame_queue_next(fqe)) {
    if(!queuebuf_holds_cmd_frame(fqe->qb)
       || (cmd_id != queuebuf_get_dispatch_byte(fqe->qb))
       || !linkaddr_cmp(receiver,
                        queuebuf_addr(fqe->qb, PACKETBUF_ADDR_RECEIVER))) {
      continue;
    }
    queuebuf_to_packetbuf(fqe->qb);
    frame_queue_on_transmitted(MAC_TX_OK, fqe);
    return;
  }
}
/*---------------------------------------------------------------------------*/
