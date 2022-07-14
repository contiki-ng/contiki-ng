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

#ifndef NBR_TABLE_H_
#define NBR_TABLE_H_

#include "contiki.h"
#include "net/linkaddr.h"
#include "net/netstack.h"

typedef enum {
  NBR_TABLE_REASON_UNDEFINED,
  NBR_TABLE_REASON_RPL_DIO,
  NBR_TABLE_REASON_RPL_DAO,
  NBR_TABLE_REASON_RPL_DIS,
  NBR_TABLE_REASON_ROUTE,
  NBR_TABLE_REASON_IPV6_ND,
  NBR_TABLE_REASON_MAC,
  NBR_TABLE_REASON_LLSEC,
  NBR_TABLE_REASON_LINK_STATS,
  NBR_TABLE_REASON_IPV6_ND_AUTOFILL,
  NBR_TABLE_REASON_SIXTOP,
} nbr_table_reason_t;

#define NBR_TABLE_MAX_NEIGHBORS NBR_TABLE_CONF_MAX_NEIGHBORS

#ifdef NBR_TABLE_CONF_GC_GET_WORST
#define NBR_TABLE_GC_GET_WORST NBR_TABLE_CONF_GC_GET_WORST
#else /* NBR_TABLE_CONF_GC_GET_WORST */
#define NBR_TABLE_GC_GET_WORST nbr_table_gc_get_worst
#endif /* NBR_TABLE_CONF_GC_GET_WORST */

#ifdef NBR_TABLE_CONF_CAN_ACCEPT_NEW
#define NBR_TABLE_CAN_ACCEPT_NEW NBR_TABLE_CONF_CAN_ACCEPT_NEW
#else /* NBR_TABLE_CONF_CAN_ACCEPT_NEW */
#define NBR_TABLE_CAN_ACCEPT_NEW nbr_table_can_accept_new
#endif /* NBR_TABLE_CONF_CAN_ACCEPT_NEW */

const linkaddr_t *NBR_TABLE_GC_GET_WORST(const linkaddr_t *lladdr1,
                                         const linkaddr_t *lladdr2);
bool NBR_TABLE_CAN_ACCEPT_NEW(const linkaddr_t *new,
                              const linkaddr_t *candidate_for_removal,
                              nbr_table_reason_t reason, const void *data);

/* An item in a neighbor table */
typedef void nbr_table_item_t;

/* Callback function, called when removing an item from a table */
typedef void(nbr_table_callback)(nbr_table_item_t *item);

/* A neighbor table */
typedef struct nbr_table {
  int index;
  int item_size;
  nbr_table_callback *callback;
  nbr_table_item_t *data;
} nbr_table_t;

/* List of link-layer addresses of the neighbors, used as key in the tables */
typedef struct nbr_table_key {
  struct nbr_table_key *next;
  linkaddr_t lladdr;
} nbr_table_key_t;

/** \brief A static neighbor table. To be initialized through nbr_table_register(name) */
#define NBR_TABLE(type, name) \
  static type _##name##_mem[NBR_TABLE_MAX_NEIGHBORS]; \
  static nbr_table_t name##_struct = { 0, sizeof(type), NULL, (nbr_table_item_t *)_##name##_mem }; \
  static nbr_table_t *name = &name##_struct \

/** \brief A non-static neighbor table. To be initialized through nbr_table_register(name) */
#define NBR_TABLE_GLOBAL(type, name) \
  static type _##name##_mem[NBR_TABLE_MAX_NEIGHBORS]; \
  static nbr_table_t name##_struct = { 0, sizeof(type), NULL, (nbr_table_item_t *)_##name##_mem }; \
  nbr_table_t *name = &name##_struct \

/** \brief Declaration of non-static neighbor tables */
#define NBR_TABLE_DECLARE(name) extern nbr_table_t *name

/** \name Neighbor tables: register and loop through table elements */
/** @{ */
int nbr_table_register(nbr_table_t *table, nbr_table_callback *callback);
int nbr_table_is_registered(const nbr_table_t *table);
nbr_table_item_t *nbr_table_head(const nbr_table_t *table);
nbr_table_item_t *nbr_table_next(const nbr_table_t *table,
                                 nbr_table_item_t *item);
/** @} */

/** \name Neighbor tables: add and get data */
/** @{ */
nbr_table_item_t *nbr_table_add_lladdr(const nbr_table_t *table,
                                       const linkaddr_t *lladdr,
                                       nbr_table_reason_t reason,
                                       const void *data);
nbr_table_item_t *nbr_table_get_from_lladdr(const nbr_table_t *table,
                                            const linkaddr_t *lladdr);
/** @} */

/** \name Neighbor tables: set flags (unused, locked, unlocked) */
/** @{ */
int nbr_table_remove(const nbr_table_t *table, const nbr_table_item_t *item);
int nbr_table_lock(const nbr_table_t *table, const nbr_table_item_t *item);
int nbr_table_unlock(const nbr_table_t *table, const nbr_table_item_t *item);
/** @} */

/** \name Neighbor tables: address manipulation */
/** @{ */
linkaddr_t *nbr_table_get_lladdr(const nbr_table_t *table,
                                 const nbr_table_item_t *item);
/** @} */

/** \name Neighbor tables: other */
/** @{ */
void nbr_table_clear(void);
bool nbr_table_entry_is_allowed(const nbr_table_t *table,
                                const linkaddr_t *lladdr,
                                nbr_table_reason_t reason, const void *data);
nbr_table_key_t *nbr_table_key_head(void);
nbr_table_key_t *nbr_table_key_next(const nbr_table_key_t *key);
int nbr_table_count_entries(void);

/** @} */
#endif /* NBR_TABLE_H_ */
