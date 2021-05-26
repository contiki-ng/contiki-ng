/*
 * Copyright (c) 2018, Amber Agriculture
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
 */

/**
 * \file
 *         Keeps track of which neighbors advertise themselves as roots.
 *         This information is used by the Orchestra root rule.
 *
 * \author
 *         Atis Elsts <atis.elsts@gmail.com>
 */

#include "contiki.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "net/mac/tsch/tsch.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "TSCH"
#define LOG_LEVEL LOG_LEVEL_MAC

/*---------------------------------------------------------------------------*/
#if BUILD_WITH_ORCHESTRA
/*---------------------------------------------------------------------------*/
#define TSCH_MAX_ROOT_NODES 5
#define ROOT_ALIVE_TIME_SECONDS     (2 * 60 * 60) /* 2h timeout */
#define PERIODIC_PROCESSING_TICKS   (60 * CLOCK_SECOND)

/*---------------------------------------------------------------------------*/
/* TSCH roots data structure */
struct tsch_root_info {
  struct tsch_root_info *next;
  linkaddr_t address;
  clock_time_t last_seen_seconds; /* the time when this was last seen */
};
/*---------------------------------------------------------------------------*/
MEMB(tsch_root_memb, struct tsch_root_info, TSCH_MAX_ROOT_NODES);
LIST(tsch_roots);
static struct ctimer periodic_timer;
/*---------------------------------------------------------------------------*/
void
tsch_roots_add_address(const linkaddr_t *new_root_address)
{
  struct tsch_root_info *root;

  LOG_INFO("add root address ");
  LOG_INFO_LLADDR(new_root_address);
  LOG_INFO_("\n");

  /* search for an existing entry */
  root = list_head(tsch_roots);
  while(root != NULL) {
    if(linkaddr_cmp(new_root_address, &root->address)) {
      break;
    }
    root = root->next;
  }

  if(root == NULL) {
    /* add a new entry */
    if((root = memb_alloc(&tsch_root_memb)) == NULL) {
      LOG_ERR("failed to add root ");
      LOG_ERR_LLADDR(new_root_address);
      LOG_ERR_("\n");
      return;
    }
    linkaddr_copy(&root->address, new_root_address);
    list_add(tsch_roots, root);

    /* make sure there is a link in the schedule */
    TSCH_CALLBACK_ROOT_NODE_UPDATED(&root->address, 1);
  }

  /* update the entry */
  root->last_seen_seconds = clock_seconds();
}
/*---------------------------------------------------------------------------*/
void
tsch_roots_set_self_to_root(uint8_t is_root)
{
  TSCH_CALLBACK_ROOT_NODE_UPDATED(&linkaddr_node_addr, is_root);
}
/*---------------------------------------------------------------------------*/
int
tsch_roots_is_root(const linkaddr_t *address)
{
  struct tsch_root_info *root;
  if(address == NULL) {
    return 0;
  }

  root = list_head(tsch_roots);
  while(root != NULL) {
    if(linkaddr_cmp(address, &root->address)) {
      return 1;
    }
    root = root->next;
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static void
periodic(void *ptr)
{
  struct tsch_root_info *root;
  struct tsch_root_info *next;
  clock_time_t now;

  now = clock_seconds();
  root = list_head(tsch_roots);
  while(root != NULL) {
    next = root->next;
    if((int32_t)(root->last_seen_seconds + ROOT_ALIVE_TIME_SECONDS - now) < 0) {
      /* the root info has become obsolete; remove its scheduled link */
      LOG_INFO("remove root address ");
      LOG_INFO_LLADDR(&root->address);
      LOG_INFO_("\n");
      TSCH_CALLBACK_ROOT_NODE_UPDATED(&root->address, 0);
      /* remove itself from the table */
      list_remove(tsch_roots, root);
      memb_free(&tsch_root_memb, root);
    }
    root = next;
  }

  /* schedule the next time */
  ctimer_set(&periodic_timer, PERIODIC_PROCESSING_TICKS, periodic, NULL);
}
/*---------------------------------------------------------------------------*/
void
tsch_roots_init(void)
{
  list_init(tsch_roots);
  memb_init(&tsch_root_memb);
  ctimer_set(&periodic_timer, PERIODIC_PROCESSING_TICKS, periodic, NULL);
}
/*---------------------------------------------------------------------------*/
#else /* BUILD_WITH_ORCHESTRA */
/*---------------------------------------------------------------------------*/
void
tsch_roots_add_address(const linkaddr_t *root_address)
{
}
void
tsch_roots_set_self_to_root(uint8_t is_root)
{
}
int
tsch_roots_is_root(const linkaddr_t *address)
{
  return 0;
}
void
tsch_roots_init(void)
{
}
/*---------------------------------------------------------------------------*/
#endif /* BUILD_WITH_ORCHESTRA */
/*---------------------------------------------------------------------------*/
