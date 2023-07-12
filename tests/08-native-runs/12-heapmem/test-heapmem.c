/*
 * Copyright (c) 2022, RISE Research Institutes of Sweden.
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
 */

/*
 * \file
 *      A set of unit tests for the heap memory module.
 * \author
 *      Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "lib/heapmem.h"
#include "unit-test/unit-test.h"
/*****************************************************************************/
/* Configuration for the Many Allocations test. */

/* Total number of allocations. */
#ifdef TEST_CONF_LIMIT
#define TEST_LIMIT TEST_CONF_LIMIT
#else
#define TEST_LIMIT      100000
#endif

/* Maximum number of concurrent allocations. */
#ifdef TEST_CONF_CONCURRENT
#define TEST_CONCURRENT TEST_CONF_CONCURRENT
#else
#define TEST_CONCURRENT    1000
#endif

/* Maximum allocation size. All allocations sizes will be chosen
   randomly in the range [1, TEST_MAX_SIZE].*/
#ifdef TEST_CONF_MAX_SIZE
#define TEST_MAX_SIZE TEST_CONF_MAX_SIZE
#else
#define TEST_MAX_SIZE       200
#endif
/*****************************************************************************/
PROCESS(test_heapmem_process, "Heapmem test process");
AUTOSTART_PROCESSES(&test_heapmem_process);
/*****************************************************************************/
UNIT_TEST_REGISTER(do_many_allocations, "Many allocations");
UNIT_TEST(do_many_allocations)
{
  UNIT_TEST_BEGIN();

  char *ptrs[TEST_CONCURRENT] = { NULL };
  unsigned failed_allocations = 0;
  unsigned failed_deallocations = 0;
  unsigned misalignments = 0;
  unsigned min_alignment = heapmem_alignment();

  /* The minimum alignment value must be a power of two */
  UNIT_TEST_ASSERT(min_alignment != 0);
  UNIT_TEST_ASSERT(!(min_alignment & (min_alignment - 1)));

  printf("Using heapmem alignment of %u\n", min_alignment);

  /*
   * Do a number of allocations (TEST_LIMIT) of a random size in the range
   * [1, TEST_MAX_SIZE], and keep up to TEST_CONCURRENT objects allocated
   * at the same time.
   */
  for(unsigned alloc_index = 0, count = 0;
      count < TEST_LIMIT;
      count++, alloc_index++) {
    if(alloc_index >= TEST_CONCURRENT) {
      alloc_index = 0;
    }

    /* If this item in the allocation table is occupied, we free it first. */
    if(ptrs[alloc_index] != NULL) {
      if(heapmem_free(ptrs[alloc_index]) == false) {
        failed_deallocations++;
      }
      ptrs[alloc_index] = NULL;
    }

    size_t alloc_size = 1 + (rand() % TEST_MAX_SIZE);
    ptrs[alloc_index] = heapmem_alloc(alloc_size);
    if(ptrs[alloc_index] == NULL) {
      failed_allocations++;
    } else {
      if((uintptr_t)ptrs[alloc_index] & (min_alignment - 1)) {
        misalignments++;
      }
      memset(ptrs[alloc_index], '!', alloc_size);
    }
  }

  /* Clear all allocations before exiting to avoid leaking memory. */
  for(unsigned alloc_index = 0; alloc_index < TEST_CONCURRENT; alloc_index++) {
    if(ptrs[alloc_index] != NULL) {
      if(heapmem_free(ptrs[alloc_index]) == false) {
        failed_deallocations++;
      }
    }
  }

  printf("Failed allocations: %u\n", failed_allocations);
  printf("Failed deallocations: %u\n", failed_deallocations);
  printf("Misaligned addresses: %u\n", misalignments);
  UNIT_TEST_ASSERT(failed_allocations == 0);
  UNIT_TEST_ASSERT(failed_deallocations == 0);
  UNIT_TEST_ASSERT(misalignments == 0);

  UNIT_TEST_END();
}
/*****************************************************************************/
UNIT_TEST_REGISTER(invalid_freeing, "Invalid free operations");
UNIT_TEST(invalid_freeing)
{
  UNIT_TEST_BEGIN();

  /* A free operation on a NULL pointer should fail. */
  UNIT_TEST_ASSERT(heapmem_free(NULL) == false);

  char *ptr = heapmem_alloc(10);
  /* This small allocation should succeed. */
  UNIT_TEST_ASSERT(ptr != NULL);
  /* A single free operation on allocated memory should succeed. */
  UNIT_TEST_ASSERT(heapmem_free(ptr) == true);

  /* A double free operation should fail. */
  UNIT_TEST_ASSERT(heapmem_free(ptr) == false);

  /* A free operation on an address outside the heapmem arena should fail. */
  UNIT_TEST_ASSERT(heapmem_free(&test_heapmem_process) == false);

  UNIT_TEST_END();
}
/*****************************************************************************/
UNIT_TEST_REGISTER(max_alloc, "Maximum allocation");
UNIT_TEST(max_alloc)
{
  UNIT_TEST_BEGIN();

  heapmem_stats_t stats_before;
  heapmem_stats(&stats_before);

  /* The test uses a much smaller size than the theoretical maximum because
     the heapmem module is not yet supporting such large allocations. */
  printf("Allocate %zu bytes\n", stats_before.available / 2);
  char *ptr = heapmem_alloc(stats_before.available / 2);

  UNIT_TEST_ASSERT(ptr != NULL);
  UNIT_TEST_ASSERT(heapmem_free(ptr) == true);

  heapmem_stats_t stats_after;
  heapmem_stats(&stats_after);

  UNIT_TEST_ASSERT(memcmp(&stats_before, &stats_after,
                          sizeof(heapmem_stats_t)) == 0);

  UNIT_TEST_END();
}
/*****************************************************************************/
UNIT_TEST_REGISTER(reallocations, "Heapmem reallocations");
UNIT_TEST(reallocations)
{
#define INITIAL_SIZE 100

  UNIT_TEST_BEGIN();

  uint8_t *ptr1 = heapmem_realloc(NULL, INITIAL_SIZE);
  UNIT_TEST_ASSERT(ptr1 != NULL);

  for(size_t i = 0; i < INITIAL_SIZE; i++) {
    ptr1[i] = i + 128;
  }

  /* Extend the initial array. */
  uint8_t *ptr2 = heapmem_realloc(ptr1, INITIAL_SIZE * 2);
  UNIT_TEST_ASSERT(ptr2 != NULL);

  for(size_t i = 0; i < INITIAL_SIZE; i++) {
    /* Check that the bytes of the lower half have been preserved. */
    UNIT_TEST_ASSERT(ptr2[i] == (uint8_t)(i + 128));
    /* Initialize the upper half of the reallocated area. */
    ptr2[i + INITIAL_SIZE] = i * 2 + 128;
  }

  /* Reduce the extended array. */
  uint8_t *ptr3 = heapmem_realloc(ptr2, (2 * INITIAL_SIZE) / 3);
  UNIT_TEST_ASSERT(ptr3 != NULL);

  /* Check that the array is correctly preserved after
     the final reallocation. */
  for(size_t i = 0; i < (2 * INITIAL_SIZE) / 3; i++) {
    if(i < INITIAL_SIZE) {
      UNIT_TEST_ASSERT(ptr2[i] == (uint8_t)(i + 128));
    } else {
      UNIT_TEST_ASSERT(ptr2[i] == (uint8_t)(i * 2 + 128));
    }
  }

  UNIT_TEST_ASSERT(heapmem_realloc(ptr3, 0) == NULL);

  UNIT_TEST_END();
}
/*****************************************************************************/
UNIT_TEST_REGISTER(stats_check, "Heapmem statistics validation");
UNIT_TEST(stats_check)
{
  UNIT_TEST_BEGIN();

  heapmem_stats_t stats;
  heapmem_stats(&stats);

  printf("* allocated %zu\n* overhead %zu\n* available %zu\n"
	 "* footprint %zu\n* chunks %zu\n",
         stats.allocated, stats.overhead, stats.available,
         stats.footprint, stats.chunks);

  UNIT_TEST_ASSERT(stats.allocated == 0);
  UNIT_TEST_ASSERT(stats.overhead == 0);
  UNIT_TEST_ASSERT(stats.available == HEAPMEM_CONF_ARENA_SIZE);
  UNIT_TEST_ASSERT(stats.footprint == 0);
  UNIT_TEST_ASSERT(stats.chunks == 0);

  UNIT_TEST_END();
}
/*****************************************************************************/
UNIT_TEST_REGISTER(zones, "Zone allocations");
UNIT_TEST(zones)
{
  UNIT_TEST_BEGIN();

  UNIT_TEST_ASSERT(heapmem_zone_register(NULL, 10) == HEAPMEM_ZONE_INVALID);
  UNIT_TEST_ASSERT(heapmem_zone_register("A", 0) == HEAPMEM_ZONE_INVALID);

  heapmem_zone_t zone = heapmem_zone_register("Test", 1000);
  UNIT_TEST_ASSERT(zone != HEAPMEM_ZONE_INVALID);
  UNIT_TEST_ASSERT(zone != HEAPMEM_ZONE_GENERAL);
  UNIT_TEST_ASSERT(heapmem_zone_register("Test", 10) == HEAPMEM_ZONE_INVALID);

  void *ptr = heapmem_zone_alloc(zone, 100);
  UNIT_TEST_ASSERT(ptr != NULL);
  UNIT_TEST_ASSERT(heapmem_free(ptr) != false);

  UNIT_TEST_ASSERT(heapmem_zone_alloc(zone, 1001) == NULL);
  ptr = heapmem_alloc(1001);
  UNIT_TEST_ASSERT(ptr != NULL);
  UNIT_TEST_ASSERT(heapmem_free(ptr) != false);

  UNIT_TEST_END();
}
/*****************************************************************************/
PROCESS_THREAD(test_heapmem_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  /* Ensure repeatable allocation patterns because some allocation sizes
     are determined by using rand(). */
  srand(500);

  UNIT_TEST_RUN(do_many_allocations);
  UNIT_TEST_RUN(max_alloc);
  UNIT_TEST_RUN(invalid_freeing);
  UNIT_TEST_RUN(reallocations);
  UNIT_TEST_RUN(stats_check);
  UNIT_TEST_RUN(zones);

  if(!UNIT_TEST_PASSED(do_many_allocations) ||
     !UNIT_TEST_PASSED(max_alloc) ||
     !UNIT_TEST_PASSED(invalid_freeing) ||
     !UNIT_TEST_PASSED(stats_check) ||
     !UNIT_TEST_PASSED(zones)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
