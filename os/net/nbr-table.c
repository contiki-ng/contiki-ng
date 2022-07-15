/*
 * Copyright (c) 2013, Swedish Institute of Computer Science
 * Copyright (c) 2010, Vrije Universiteit Brussel
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
 *
 * Authors: Simon Duquennoy <simonduq@sics.se>
 *          Joris Borms <joris.borms@vub.ac.be>
 */

#include "contiki.h"

#include <stddef.h>
#include <string.h>
#include "lib/memb.h"
#include "lib/list.h"
#include "net/nbr-table.h"

#define DEBUG DEBUG_NONE
#include "net/ipv6/uip-debug.h"

#if DEBUG
#include "sys/ctimer.h"
static void handle_periodic_timer(void *ptr);
static struct ctimer periodic_timer;
static uint8_t initialized = 0;
static void print_table();
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

/* For each neighbor, a map of the tables that use the neighbor.
 * As we are using uint8_t, we have a maximum of 8 tables in the system */
static uint8_t used_map[NBR_TABLE_MAX_NEIGHBORS];
/* For each neighbor, a map of the tables that lock the neighbor */
static uint8_t locked_map[NBR_TABLE_MAX_NEIGHBORS];
/* The maximum number of tables */
#define MAX_NUM_TABLES 8
/* A list of pointers to tables in use */
static struct nbr_table *all_tables[MAX_NUM_TABLES];
/* The current number of tables */
static unsigned num_tables;

/* The neighbor address table */
MEMB(neighbor_addr_mem, nbr_table_key_t, NBR_TABLE_MAX_NEIGHBORS);
LIST(nbr_table_keys);

/*---------------------------------------------------------------------------*/
static void remove_key(nbr_table_key_t *key, bool do_free);
/*---------------------------------------------------------------------------*/
/* Get a key from a neighbor index */
static nbr_table_key_t *
key_from_index(int index)
{
  return index != -1 ? &((nbr_table_key_t *)neighbor_addr_mem.mem)[index] : NULL;
}
/*---------------------------------------------------------------------------*/
/* Get an item from its neighbor index */
static nbr_table_item_t *
item_from_index(const nbr_table_t *table, int index)
{
  return table != NULL && index != -1 ? (char *)table->data + index * table->item_size : NULL;
}
/*---------------------------------------------------------------------------*/
/* Get the neighbor index of an item */
static int
index_from_key(const nbr_table_key_t *key)
{
  return key != NULL ? key - (nbr_table_key_t *)neighbor_addr_mem.mem : -1;
}
/*---------------------------------------------------------------------------*/
/* Get the neighbor index of an item */
static int
index_from_item(const nbr_table_t *table, const nbr_table_item_t *item)
{
  return table != NULL && item != NULL ? ((int)((char *)item - (char *)table->data)) / table->item_size : -1;
}
/*---------------------------------------------------------------------------*/
/* Get an item from its key */
static nbr_table_item_t *
item_from_key(const nbr_table_t *table, const nbr_table_key_t *key)
{
  return item_from_index(table, index_from_key(key));
}
/*---------------------------------------------------------------------------*/
/* Get the key af an item */
static nbr_table_key_t *
key_from_item(const nbr_table_t *table, const nbr_table_item_t *item)
{
  return key_from_index(index_from_item(table, item));
}
/*---------------------------------------------------------------------------*/
/* Get the index of a neighbor from its link-layer address */
static int
index_from_lladdr(const linkaddr_t *lladdr)
{
  nbr_table_key_t *key;
  /* Allow lladdr-free insertion, useful e.g. for IPv6 ND.
   * Only one such entry is possible at a time, indexed by linkaddr_null. */
  if(lladdr == NULL) {
    lladdr = &linkaddr_null;
  }
  key = list_head(nbr_table_keys);
  while(key != NULL) {
    if(lladdr && linkaddr_cmp(lladdr, &key->lladdr)) {
      return index_from_key(key);
    }
    key = list_item_next(key);
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
/* Get bit from "used" or "locked" bitmap */
static int
nbr_get_bit(const uint8_t *bitmap, const nbr_table_t *table,
            const nbr_table_item_t *item)
{
  int item_index = index_from_item(table, item);
  if(table != NULL && item_index != -1) {
    return (bitmap[item_index] & (1 << table->index)) != 0;
  } else {
    return 0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Set bit in "used" or "locked" bitmap */
static int
nbr_set_bit(uint8_t *bitmap, const nbr_table_t *table,
            const nbr_table_item_t *item, int value)
{
  int item_index = index_from_item(table, item);

  if(table != NULL && item_index != -1) {
    if(value) {
      bitmap[item_index] |= 1 << table->index;
    } else {
      bitmap[item_index] &= ~(1 << table->index);
    }
    return 1;
  } else {
    return 0;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
remove_key(nbr_table_key_t *key, bool do_free)
{
  int i;
  for(i = 0; i < MAX_NUM_TABLES; i++) {
    if(all_tables[i] != NULL && all_tables[i]->callback != NULL) {
      /* Call table callback for each table that uses this item */
      nbr_table_item_t *removed_item = item_from_key(all_tables[i], key);
      if(nbr_get_bit(used_map, all_tables[i], removed_item) == 1) {
        all_tables[i]->callback(removed_item);
      }
    }
  }
  /* Empty used and locked map */
  used_map[index_from_key(key)] = 0;
  locked_map[index_from_key(key)] = 0;
  /* Remove neighbor from list */
  list_remove(nbr_table_keys, key);
  if(do_free) {
    /* Release the memory */
    memb_free(&neighbor_addr_mem, key);
  }
}
/*---------------------------------------------------------------------------*/
int
nbr_table_count_entries(void)
{
  int i;
  int count = 0;
  for(i = 0; i < NBR_TABLE_MAX_NEIGHBORS; i++) {
    if(used_map[i] > 0) {
      count++;
    }
  }
  return count;
}
/*---------------------------------------------------------------------------*/
static int
used_count(const linkaddr_t *lladdr)
{
  int item_index = index_from_lladdr(lladdr);
  int used_count = 0;
  if(item_index != -1) {
    int used = used_map[item_index];
    /* Count how many tables are using this item */
    while(used != 0) {
      if((used & 1) == 1) {
        used_count++;
      }
      used >>= 1;
    }
  }
  return used_count;
}
/*---------------------------------------------------------------------------*/
const linkaddr_t *
nbr_table_gc_get_worst(const linkaddr_t *lladdr1, const linkaddr_t *lladdr2)
{
  return used_count(lladdr2) < used_count(lladdr1) ? lladdr2 : lladdr1;
}
/*---------------------------------------------------------------------------*/
bool
nbr_table_can_accept_new(const linkaddr_t *new,
                         const linkaddr_t *candidate_for_removal,
                         nbr_table_reason_t reason, const void *data)
{
  /* Default behavior: if full, always replace worst entry. */
  return true;
}
/*---------------------------------------------------------------------------*/
static const linkaddr_t *
select_for_removal(const linkaddr_t *new, nbr_table_reason_t reason,
                   const void *data)
{
  nbr_table_key_t *k;
  const linkaddr_t *worst_lladdr = NULL;

  /* No more space, try to free a neighbor */
  for(k = list_head(nbr_table_keys); k != NULL; k = list_item_next(k)) {
    int item_index = index_from_key(k);
    int locked = locked_map[item_index];
    /* Never delete a locked item */
    if(!locked) {
      if(worst_lladdr == NULL) {
        worst_lladdr = &k->lladdr;
      } else {
        worst_lladdr = NBR_TABLE_GC_GET_WORST(worst_lladdr, &k->lladdr);
      }
    }
  }

  /* Finally compare against current candidate for insertion */
  if(worst_lladdr != NULL && NBR_TABLE_CAN_ACCEPT_NEW(new, worst_lladdr, reason, data)) {
    return worst_lladdr;
  } else {
    return NULL;
  }
}
/*---------------------------------------------------------------------------*/
static bool
entry_is_allowed(const nbr_table_t *table, const linkaddr_t *lladdr,
                 nbr_table_reason_t reason, const void *data,
                 const linkaddr_t **to_be_removed_ptr)
{
  bool ret;
  const linkaddr_t *to_be_removed = NULL;

  if(nbr_table_get_from_lladdr(table, lladdr) != NULL) {
    /* Already present in the given table */
    ret = true;
  } else {
    if(index_from_lladdr(lladdr) != -1
       || memb_numfree(&neighbor_addr_mem) > 0) {
      /* lladdr already present globally, or there is space for a new lladdr,
       * check if entry can be added */
      ret = NBR_TABLE_CAN_ACCEPT_NEW(lladdr, NULL, reason, data);
    } else {
      ret = (to_be_removed = select_for_removal(lladdr, reason, data)) != NULL;
    }
  }
  if(to_be_removed_ptr != NULL) {
    *to_be_removed_ptr = to_be_removed;
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
bool
nbr_table_entry_is_allowed(const nbr_table_t *table, const linkaddr_t *lladdr,
                           nbr_table_reason_t reason, const void *data)
{
  return entry_is_allowed(table, lladdr, reason, data, NULL);
}
/*---------------------------------------------------------------------------*/
static nbr_table_key_t *
nbr_table_allocate(nbr_table_reason_t reason, const void *data,
                   const linkaddr_t *to_be_removed_lladdr)
{
  nbr_table_key_t *new = memb_alloc(&neighbor_addr_mem);
  if(new != NULL) {
    return new;
  } else {
    if(to_be_removed_lladdr == NULL) {
      /* No candidate for GC, allocation fails */
      return NULL;
    }
    nbr_table_key_t *to_be_removed = key_from_index(index_from_lladdr(to_be_removed_lladdr));
    if(to_be_removed == NULL) {
      return NULL;
    }
    /* Reuse to_be_removed item's spot */
    remove_key(to_be_removed, false);
    return to_be_removed;
  }
}
/*---------------------------------------------------------------------------*/
/* Register a new neighbor table. To be used at initialization by modules
 * using a neighbor table */
int
nbr_table_register(nbr_table_t *table, nbr_table_callback *callback)
{
#if DEBUG
  if(!initialized) {
    initialized = 1;
    /* schedule a debug printout per minute */
    ctimer_set(&periodic_timer, CLOCK_SECOND * 60, handle_periodic_timer, NULL);
  }
#endif

  if(nbr_table_is_registered(table)) {
    /* Table already registered, just update callback */
    table->callback = callback;
    return 1;
  }

  if(num_tables < MAX_NUM_TABLES) {
    table->index = num_tables++;
    table->callback = callback;
    all_tables[table->index] = table;
    return 1;
  } else {
    /* Maximum number of tables exceeded */
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
/* Test whether a specified table has been registered or not */
int
nbr_table_is_registered(const nbr_table_t *table)
{
  if(table != NULL && table->index >= 0 && table->index < MAX_NUM_TABLES
                   && all_tables[table->index] == table) {
    return 1;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
/* Returns the first item of the current table */
nbr_table_item_t *
nbr_table_head(const nbr_table_t *table)
{
  /* Get item from first key */
  nbr_table_item_t *item = item_from_key(table, list_head(nbr_table_keys));
  /* Item is the first neighbor, now check is it is in the current table */
  if(nbr_get_bit(used_map, table, item)) {
    return item;
  } else {
    return nbr_table_next(table, item);
  }
}
/*---------------------------------------------------------------------------*/
/* Iterates over the current table */
nbr_table_item_t *
nbr_table_next(const nbr_table_t *table, nbr_table_item_t *item)
{
  do {
    void *key = key_from_item(table, item);
    key = list_item_next(key);
    /* Loop until the next item is in the current table */
    item = item_from_key(table, key);
  } while(item && !nbr_get_bit(used_map, table, item));
  return item;
}
/*---------------------------------------------------------------------------*/
/* Add a neighbor indexed with its link-layer address */
nbr_table_item_t *
nbr_table_add_lladdr(const nbr_table_t *table, const linkaddr_t *lladdr,
                     nbr_table_reason_t reason, const void *data)
{
  int index;
  nbr_table_item_t *item;
  nbr_table_key_t *key;
  const linkaddr_t *to_be_removed;

  if(table == NULL) {
    return NULL;
  }

  /* Allow lladdr-free insertion, useful e.g. for IPv6 ND.
   * Only one such entry is possible at a time, indexed by linkaddr_null. */
  if(lladdr == NULL) {
    lladdr = &linkaddr_null;
  }

  if(!entry_is_allowed(table, lladdr, reason, data, &to_be_removed)) {
    return NULL;
  }

  if((index = index_from_lladdr(lladdr)) == -1) {
     /* Neighbor not yet in table, let's try to allocate one */
    key = nbr_table_allocate(reason, data, to_be_removed);

    /* No space available for new entry. Should never happen as entry_is_allowed
     * already checks we can add. */
    if(key == NULL) {
      return NULL;
    }

    /* Add neighbor to list */
    list_add(nbr_table_keys, key);

    /* Get index from newly allocated neighbor */
    index = index_from_key(key);

    /* Set link-layer address */
    linkaddr_copy(&key->lladdr, lladdr);
  }

  /* Get item in the current table */
  item = item_from_index(table, index);

  /* Initialize item data and set "used" bit */
  memset(item, 0, table->item_size);
  nbr_set_bit(used_map, table, item, 1);

#if DEBUG
  print_table();
#endif
  return item;
}
/*---------------------------------------------------------------------------*/
/* Get an item from its link-layer address */
void *
nbr_table_get_from_lladdr(const nbr_table_t *table, const linkaddr_t *lladdr)
{
  void *item = item_from_index(table, index_from_lladdr(lladdr));
  return nbr_get_bit(used_map, table, item) ? item : NULL;
}
/*---------------------------------------------------------------------------*/
/* Removes a neighbor from the current table (unset "used" bit) */
int
nbr_table_remove(const nbr_table_t *table, const void *item)
{
  int ret = nbr_set_bit(used_map, table, item, 0);
  nbr_set_bit(locked_map, table, item, 0);
  return ret;
}
/*---------------------------------------------------------------------------*/
/* Lock a neighbor for the current table (set "locked" bit) */
int
nbr_table_lock(const nbr_table_t *table, const void *item)
{
#if DEBUG
  int i = index_from_item(table, item);
  PRINTF("*** Lock %d\n", i);
#endif
  return nbr_set_bit(locked_map, table, item, 1);
}
/*---------------------------------------------------------------------------*/
/* Release the lock on a neighbor for the current table (unset "locked" bit) */
int
nbr_table_unlock(const nbr_table_t *table, const void *item)
{
#if DEBUG
  int i = index_from_item(table, item);
  PRINTF("*** Unlock %d\n", i);
#endif
  return nbr_set_bit(locked_map, table, item, 0);
}
/*---------------------------------------------------------------------------*/
/* Get link-layer address of an item */
linkaddr_t *
nbr_table_get_lladdr(const nbr_table_t *table, const void *item)
{
  nbr_table_key_t *key = key_from_item(table, item);
  return key != NULL ? &key->lladdr : NULL;
}
/*---------------------------------------------------------------------------*/
void
nbr_table_clear(void)
{
  nbr_table_key_t *k;
  /* Delete until nothing left */
  while((k = list_head(nbr_table_keys))) {
    remove_key(k, true);
  }
}
/*---------------------------------------------------------------------------*/
nbr_table_key_t *
nbr_table_key_head(void)
{
  return list_head(nbr_table_keys);
}
/*---------------------------------------------------------------------------*/
nbr_table_key_t *
nbr_table_key_next(const nbr_table_key_t *key)
{
  return list_item_next(key);
}
/*---------------------------------------------------------------------------*/
#if DEBUG
static void
print_table()
{
  int i, j;
  /* Printout all neighbors and which tables they are used in */
  PRINTF("NBR TABLE:\n");
  for(i = 0; i < NBR_TABLE_MAX_NEIGHBORS; i++) {
    if(used_map[i] > 0) {
      PRINTF(" %02d %02d",i , key_from_index(i)->lladdr.u8[LINKADDR_SIZE - 1]);
      for(j = 0; j < num_tables; j++) {
        PRINTF(" [%d:%d]", (used_map[i] & (1 << j)) != 0,
               (locked_map[i] & (1 << j)) != 0);
      }
      PRINTF("\n");
    }
  }
}
/*---------------------------------------------------------------------------*/
static void
handle_periodic_timer(void *ptr)
{
  print_table();
  ctimer_reset(&periodic_timer);
}
#endif
