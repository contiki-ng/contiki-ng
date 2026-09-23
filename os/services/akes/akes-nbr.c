/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
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
 * \addtogroup akes
 * @{
 *
 * \file
 *         Neighbor management.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "contiki.h"
#include "akes/akes-nbr.h"
#include "akes/akes.h"
#include "lib/assert.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "net/link-stats.h"
#include "net/mac/framer/frame802154.h"
#include "net/mac/llsec802154.h"
#include "net/packetbuf.h"
#include "sys/clock.h"
#include "sys/mutex.h"
#include <inttypes.h>
#include <stdbool.h>
#include <string.h>

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "AKES-nbr"
#define LOG_LEVEL LOG_LEVEL_MAC

#ifndef AKES_NBR_CONF_WITH_LOCKING
#define AKES_NBR_CONF_WITH_LOCKING (0)
#endif /* AKES_NBR_CONF_WITH_LOCKING */

NBR_TABLE(akes_nbr_entry_t, entries_table);
MEMB(nbrs_memb, akes_nbr_t, AKES_NBR_MAX);
MEMB(tentatives_memb, akes_nbr_tentative_t, AKES_NBR_MAX_TENTATIVES);
#if AKES_NBR_CONF_WITH_LOCKING
static mutex_t lock;
#endif /* AKES_NBR_CONF_WITH_LOCKING */

/*---------------------------------------------------------------------------*/
static bool
get_lock(void)
{
#if AKES_NBR_CONF_WITH_LOCKING
  return mutex_try_lock(&lock);
#else /* AKES_NBR_CONF_WITH_LOCKING */
  return true;
#endif /* AKES_NBR_CONF_WITH_LOCKING */
}
/*---------------------------------------------------------------------------*/
static void
release_lock(void)
{
#if AKES_NBR_CONF_WITH_LOCKING
  mutex_unlock(&lock);
#endif /* AKES_NBR_CONF_WITH_LOCKING */
}
/*---------------------------------------------------------------------------*/
bool
akes_nbr_can_query_asynchronously(void)
{
  if(!get_lock()) {
    return false;
  }
  release_lock();
  return true;
}
/*---------------------------------------------------------------------------*/
size_t
akes_nbr_index_of_tentative(const akes_nbr_tentative_t *tentative)
{
  return tentative - tentatives_memb_memb_mem;
}
/*---------------------------------------------------------------------------*/
size_t
akes_nbr_index_of(const akes_nbr_t *nbr)
{
  return nbr - (akes_nbr_t *)nbrs_memb.mem;
}
/*---------------------------------------------------------------------------*/
akes_nbr_t *
akes_nbr_get_nbr(size_t index)
{
  if(index >= AKES_NBR_MAX) {
    return NULL;
  }
  return &((akes_nbr_t *)nbrs_memb.mem)[index];
}
/*---------------------------------------------------------------------------*/
akes_nbr_entry_t *
akes_nbr_get_entry_of(akes_nbr_t *nbr)
{
  for(akes_nbr_entry_t *entry = nbr_table_head(entries_table);
      entry;
      entry = nbr_table_next(entries_table, entry)) {
    if((entry->tentative == nbr) || (entry->permanent == nbr)) {
      return entry;
    }
  }
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
akes_nbr_free_tentative_metadata(akes_nbr_tentative_t *meta)
{
  if(memb_free(&tentatives_memb, meta) == -1) {
    assert(false);
  }
}
/*---------------------------------------------------------------------------*/
void
akes_nbr_copy_challenge(uint8_t dest[static AKES_NBR_CHALLENGE_LEN],
                        const uint8_t source[static AKES_NBR_CHALLENGE_LEN])
{
  memcpy(dest, source, AKES_NBR_CHALLENGE_LEN);
}
/*---------------------------------------------------------------------------*/
void
akes_nbr_copy_key(uint8_t dest[static AES_128_KEY_LENGTH],
                  const uint8_t source[static AES_128_KEY_LENGTH])
{
  memcpy(dest, source, AES_128_KEY_LENGTH);
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
akes_nbr_get_addr(const akes_nbr_entry_t *entry)
{
  return nbr_table_get_lladdr(entries_table, entry);
}
/*---------------------------------------------------------------------------*/
static void
on_entry_change(akes_nbr_entry_t *entry)
{
  if(!entry->permanent && !entry->tentative) {
    LOG_DBG("removing neighbor\n");
    nbr_table_remove(entries_table, entry);
  }
}
/*---------------------------------------------------------------------------*/
akes_nbr_entry_t *
akes_nbr_head(akes_nbr_status_t status)
{
  akes_nbr_entry_t *entry;

  entry = nbr_table_head(entries_table);
  return !entry || entry->refs[status]
         ? entry
         : akes_nbr_next(entry, status);
}
/*---------------------------------------------------------------------------*/
akes_nbr_entry_t *
akes_nbr_next(akes_nbr_entry_t *entry, akes_nbr_status_t status)
{
  do {
    entry = nbr_table_next(entries_table, entry);
  } while(entry && !entry->refs[status]);
  return entry;
}
/*---------------------------------------------------------------------------*/
size_t
akes_nbr_count(akes_nbr_status_t status)
{
  size_t count = 0;
  for(akes_nbr_entry_t *entry = akes_nbr_head(status);
      entry;
      entry = akes_nbr_next(entry, status)) {
    count++;
  }
  return count;
}
/*---------------------------------------------------------------------------*/
size_t
akes_nbr_free_slots(void)
{
  return memb_numfree(&nbrs_memb);
}
/*---------------------------------------------------------------------------*/
static void
delete_nbr_with_lock(akes_nbr_entry_t *entry, akes_nbr_status_t status)
{
  assert(entry);
  if(status) {
    akes_nbr_free_tentative_metadata(entry->refs[status]->meta);
  }
  if(memb_free(&nbrs_memb, entry->refs[status]) == -1) {
    LOG_ERR("nbr was not in memb\n");
    assert(false);
  }
  entry->refs[status] = NULL;
  on_entry_change(entry);
}
/*---------------------------------------------------------------------------*/
akes_nbr_entry_t *
akes_nbr_new(akes_nbr_status_t status)
{
  if(status
     && (akes_nbr_count(AKES_NBR_TENTATIVE) >= AKES_NBR_MAX_TENTATIVES)) {
    LOG_WARN("too many tentative neighbors\n");
    return NULL;
  }

  akes_nbr_entry_t *entry = akes_nbr_get_sender_entry();
  if(!entry) {
    entry = nbr_table_add_lladdr(entries_table,
                                 packetbuf_addr(PACKETBUF_ADDR_SENDER),
                                 NBR_TABLE_REASON_LLSEC,
                                 NULL);
    if(!entry) {
      LOG_WARN("nbr-table is full\n");
      return NULL;
    }
  }

  while(!get_lock()) {
    assert(false);
  }
  assert(!entry->refs[status]);
  entry->refs[status] = memb_alloc(&nbrs_memb);
  if(!entry->refs[status]) {
    LOG_WARN("RAM is running low\n");
    on_entry_change(entry);
    release_lock();
    return NULL;
  }
#if LLSEC802154_USES_FRAME_COUNTER
  anti_replay_init_info(&entry->refs[status]->anti_replay_info);
#endif /* LLSEC802154_USES_FRAME_COUNTER */
  nbr_table_lock(entries_table, entry);
  if(status) {
    entry->refs[status]->meta = memb_alloc(&tentatives_memb);
    if(!entry->refs[status]->meta) {
      LOG_WARN("tentatives_memb full\n");
      delete_nbr_with_lock(entry, status);
      release_lock();
      return NULL;
    }
  }
  release_lock();
  return entry;
}
/*---------------------------------------------------------------------------*/
akes_nbr_t *
akes_nbr_clone(const akes_nbr_t *nbr)
{
  akes_nbr_t *clone = memb_alloc(&nbrs_memb);
  if(!clone) {
    return NULL;
  }
  memcpy(clone, nbr, sizeof(*clone));
  return clone;
}
/*---------------------------------------------------------------------------*/
akes_nbr_entry_t *
akes_nbr_get_entry(const linkaddr_t *addr)
{
  return nbr_table_get_from_lladdr(entries_table, addr);
}
/*---------------------------------------------------------------------------*/
akes_nbr_entry_t *
akes_nbr_get_sender_entry(void)
{
  return akes_nbr_get_entry(packetbuf_addr(PACKETBUF_ADDR_SENDER));
}
/*---------------------------------------------------------------------------*/
akes_nbr_entry_t *
akes_nbr_get_receiver_entry(void)
{
  return akes_nbr_get_entry(packetbuf_addr(PACKETBUF_ADDR_RECEIVER));
}
/*---------------------------------------------------------------------------*/
void
akes_nbr_delete(akes_nbr_entry_t *entry, akes_nbr_status_t status)
{
  assert(!status || ctimer_expired(&entry->tentative->meta->wait_timer));
  LOG_INFO("deleting %s neighbor ", status ? "tentative" : "permanent");
  LOG_INFO_LLADDR(akes_nbr_get_addr(entry));
  LOG_INFO_("\n");
  while(!get_lock()) {
    assert(false);
  }
  delete_nbr_with_lock(entry, status);
  release_lock();
}
/*---------------------------------------------------------------------------*/
void
akes_nbr_init(void)
{
  memb_init(&nbrs_memb);
  nbr_table_register(entries_table, NULL);
  memb_init(&tentatives_memb);
}
/*---------------------------------------------------------------------------*/

/** @} */
