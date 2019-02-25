/*
 * Copyright (c) 2018, Yasuyuki Tanaka
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

#include <contiki.h>
#include <sys/log.h>
#include <lib/random.h>
#include <net/nbr-table.h>
#include <net/ipv6/uip-ds6-nbr.h>
#include <unit-test/unit-test.h>

#include <stdlib.h>

#define LOG_MODULE "test"
#define LOG_LEVEL LOG_LEVEL_DBG

/* report function defined in unit-test.c */
void unit_test_print_report(const unit_test_t *utp);

static const uint8_t is_router = 1;
static const uint8_t state = NBR_INCOMPLETE;
static const nbr_table_reason_t reason = NBR_TABLE_REASON_UNDEFINED;

static void remove_all_entries_in_neighbor_cache(void);

PROCESS(node_process, "Node");
AUTOSTART_PROCESSES(&node_process);

void
my_test_print(const unit_test_t *utp)
{
  unit_test_print_report(utp);
  if(utp->result == unit_test_failure) {
    printf("\nTEST FAILED\n");
    exit(1); /* exit by failure */
  }
}

/* my_always_return_null() is set to NBR_TABLE_FIND_REMOVABLE */
const linkaddr_t *
my_always_return_null(nbr_table_reason_t reason, void *data)
{
  return NULL;
}

void
remove_all_entries_in_neighbor_cache(void)
{
  uip_ds6_nbr_t *nbr, *next_nbr;
  for(nbr = uip_ds6_nbr_head(); nbr != NULL; nbr = next_nbr) {
    next_nbr = uip_ds6_nbr_next(nbr);
    uip_ds6_nbr_rm(nbr);
  }
  /*
   * uip_ds6_nbr_rm() cannot free the memory for an entry in nbr-table. There is
   * no API to free or deallocate unused nbr-table entry. Because of that,
   * nbr-table has some link-layer addresses even though this function removes
   * all the neighbor cache entries.
   */
}

UNIT_TEST_REGISTER(add_v6addrs_to_neighbor,
                   "add IPv6 addresses to a single neighbor");
UNIT_TEST(add_v6addrs_to_neighbor)
{
  uip_ipaddr_t ipaddr;
  uip_lladdr_t lladdr;
  uip_ds6_nbr_t *nbr;
  const uip_lladdr_t *ret_lladdr;

  memset(&ipaddr, 0, sizeof(ipaddr));
  memset(&lladdr, 0, sizeof(lladdr));

  UNIT_TEST_BEGIN();

  /* make sure the neighbor cache table is empty */
  remove_all_entries_in_neighbor_cache();
  UNIT_TEST_ASSERT(uip_ds6_nbr_head() == NULL);
  UNIT_TEST_ASSERT(uip_ds6_nbr_num() == 0);

  /* prepare a link-layer address */
  LOG_DBG("link-layer addr: ");
  LOG_DBG_LLADDR((const linkaddr_t *)&lladdr);
  LOG_DBG_("\n");

  for(int i = 0; i <= UIP_DS6_NBR_MAX_6ADDRS_PER_NBR; i++) {
    ipaddr.u8[0] = i;
    LOG_DBG("adding ipv6 addr (");
    LOG_DBG_6ADDR(&ipaddr);
    LOG_DBG_("[i=%u])\n", i);

    /* add a binding of the IPv6 address and the MAC address */
    nbr = uip_ds6_nbr_add(&ipaddr, &lladdr, is_router, state, reason, NULL);
    if(i < UIP_DS6_NBR_MAX_6ADDRS_PER_NBR) {
      UNIT_TEST_ASSERT(nbr != NULL);
      UNIT_TEST_ASSERT(memcmp(&nbr->ipaddr, &ipaddr, sizeof(ipaddr)) == 0);
      UNIT_TEST_ASSERT(nbr->state == state);
      UNIT_TEST_ASSERT(uip_ds6_nbr_num() == (i + 1));
      /*
       * for some reason, nbr->isrouter is not set if both UIP_ND6_SEND_RA and
       * !UIP_CONF_ROUTER is 0 (see uip-ds6-nbr.c)
       */
      // UNIT_TEST_ASSERT(nbr->isrouter == is_router);
      ret_lladdr = uip_ds6_nbr_lladdr_from_ipaddr((const uip_ipaddr_t *)&ipaddr);
      UNIT_TEST_ASSERT(ret_lladdr != NULL);
      UNIT_TEST_ASSERT(memcmp(ret_lladdr, &lladdr, sizeof(lladdr)) == 0);

    } else {
      /* i == UIP_DS6_NBR_MAX_6ADDRS_PER_NBR; the address shouldn't be added */
      UNIT_TEST_ASSERT(nbr == NULL);
    }
  }

  UNIT_TEST_END();
}

UNIT_TEST_REGISTER(remove_v6addrs_of_neighbor,
                   "remove IPv6 addresses associated with a single neighbor");
UNIT_TEST(remove_v6addrs_of_neighbor)
{
  uip_ipaddr_t ipaddr;
  uip_lladdr_t lladdr;
  uip_ds6_nbr_t *nbr, *next_nbr;

  memset(&ipaddr, 0, sizeof(ipaddr));
  memset(&lladdr, 0, sizeof(lladdr));

  UNIT_TEST_BEGIN();

  /* make sure the neighbor cache table is empty */
  remove_all_entries_in_neighbor_cache();
  UNIT_TEST_ASSERT(uip_ds6_nbr_head() == NULL);
  UNIT_TEST_ASSERT(uip_ds6_nbr_num() == 0);

  /* prepare a link-layer address */
  LOG_DBG("link-layer addr: ");
  LOG_DBG_LLADDR((const linkaddr_t *)&lladdr);
  LOG_DBG_("\n");

  /* fill the neighbor entry associated with the link-layer address */
  for(int i = 0; i < UIP_DS6_NBR_MAX_6ADDRS_PER_NBR; i++) {
    ipaddr.u8[0] = i;
    nbr = uip_ds6_nbr_add(&ipaddr, &lladdr, is_router, state, reason, NULL);
    UNIT_TEST_ASSERT(nbr != NULL);
    UNIT_TEST_ASSERT(uip_ds6_nbr_num() == (i + 1));
  }

  /* remove IPv6 addresses for the link-layer address one by one */
  for(nbr = uip_ds6_nbr_head(); nbr != NULL; nbr = next_nbr) {
    LOG_DBG("removing nbr:%p\n", nbr);
    next_nbr = uip_ds6_nbr_next(nbr);
    UNIT_TEST_ASSERT(uip_ds6_nbr_rm(nbr) == 1);
  }
  UNIT_TEST_ASSERT(uip_ds6_nbr_num() == 0);
  UNIT_TEST_ASSERT(uip_ds6_nbr_head() == NULL);

  UNIT_TEST_END();

}

UNIT_TEST_REGISTER(fill_neighbor_cache_table,
                   "fill the neighbor cache table");
UNIT_TEST(fill_neighbor_cache_table)
{
  /*
   * We should be able to add the same number of link-layer addresses as
   * NBR_TABLE_MAX_NEIGHBORS. In addition, we should be add the same number of
   * IPv6 addresses per link-layer address as
   * UIP_DS6_NBR_CONF_MAX_6ADDRS_PER_NBR.
   */
  uip_ipaddr_t ipaddr;
  uip_lladdr_t lladdr;
  uip_ds6_nbr_t *nbr;

  memset(&ipaddr, 0, sizeof(ipaddr));
  memset(&lladdr, 0, sizeof(lladdr));

  UNIT_TEST_BEGIN();

  /* make sure the neighbor cache table is empty */
  remove_all_entries_in_neighbor_cache();
  UNIT_TEST_ASSERT(uip_ds6_nbr_head() == NULL);
  UNIT_TEST_ASSERT(uip_ds6_nbr_num() == 0);

  for(int i = 0; i <= NBR_TABLE_MAX_NEIGHBORS; i++) {
    lladdr.addr[0] = i & 0xFF;
    lladdr.addr[1] = i >> 8;
    for(int j = 0; j <= UIP_DS6_NBR_MAX_6ADDRS_PER_NBR; j++) {
      ipaddr.u8[0] = i & 0xFF;
      ipaddr.u8[1] = i >> 8;
      ipaddr.u8[2] = j;
      LOG_DBG("adding ipv6 addr (");
      LOG_DBG_6ADDR(&ipaddr);
      LOG_DBG_(") to link-layer addr (");
      LOG_DBG_LLADDR((const linkaddr_t *)&lladdr);
      LOG_DBG_(") [i=%u,j=%u]\n", i, j);

      nbr = uip_ds6_nbr_add(&ipaddr, &lladdr, is_router, state, reason, NULL);
      if((i < NBR_TABLE_MAX_NEIGHBORS) &&
         (j < UIP_DS6_NBR_MAX_6ADDRS_PER_NBR)) {
        UNIT_TEST_ASSERT(nbr != NULL);
      } else if(i == NBR_TABLE_MAX_NEIGHBORS) {
        /* we should not be able to add a link-layer address any more */
        UNIT_TEST_ASSERT(j == 0);
        UNIT_TEST_ASSERT(nbr == NULL);
        break;
      } else if(j == UIP_DS6_NBR_MAX_6ADDRS_PER_NBR) {
        /* we should not be able to add an IPv6 address any more */
        UNIT_TEST_ASSERT(i < NBR_TABLE_MAX_NEIGHBORS);
        UNIT_TEST_ASSERT(nbr == NULL);
        break;
      } else {
        /* shouldn't come here */
        UNIT_TEST_ASSERT(false);
      }
    }
  }

  UNIT_TEST_END();
}

PROCESS_THREAD(node_process, ev, data)
{
  PROCESS_BEGIN();

  UNIT_TEST_RUN(add_v6addrs_to_neighbor);
  UNIT_TEST_RUN(remove_v6addrs_of_neighbor);
  UNIT_TEST_RUN(fill_neighbor_cache_table);

  printf("\nTEST SUCCEEDED\n");
  exit(0); /* success: all the test passed */

  PROCESS_END();
}
