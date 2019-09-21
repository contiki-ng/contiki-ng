/*
 * Copyright (c) 2019, Inria.
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

#include <stdio.h>
#include <inttypes.h>
#include <string.h>

#include <lib/memb.h>

#define NUM_MEMB_BLOCKS 8
#define DATA_LEN 128
#define ONE_BYTE_OFF_ADDR(p) ((uint8_t *)p + 1)

typedef struct test_struct {
  uint8_t index;
  uint16_t length;
  char data[DATA_LEN];
} test_struct_t;

MEMB(memb_pool, test_struct_t, NUM_MEMB_BLOCKS);

int
main(void)
{
  int ret;
  test_struct_t *memb_block_p;
  test_struct_t *memb_block_list[NUM_MEMB_BLOCKS];

  /* initialize the memory blocks */
  memb_init(&memb_pool);

  /*
   * all the blocks should be "unused"; memb_numfree() should return
   * NUM_MEMB_BLOCKS
   */
  if((ret = memb_numfree(&memb_pool)) != NUM_MEMB_BLOCKS) {
    printf("test failed: memb_numfree() returns %d, which should be %d\n",
           ret, NUM_MEMB_BLOCKS);
    return -1;
  } else {
    printf("- memb_init (and memb_numfree) is OK\n");
  }

  /* allocate memory blocks */
  memset(memb_block_list, 0, sizeof(memb_block_list));
  for(int i = 0; i < NUM_MEMB_BLOCKS; i++) {
    memb_block_p = (test_struct_t *)memb_alloc(&memb_pool);
    if(memb_block_p == NULL) {
      printf("test failed: memb_alloc() returns NULL with i==%d\n", i);
      return -1;
    } else if((ret = memb_inmemb(&memb_pool, memb_block_p)) != 1) {
      printf("test failed: %p returned memb_alloc() is invalid\n",
             memb_block_p);
      return -1;
    } else if((ret = memb_numfree(&memb_pool)) != NUM_MEMB_BLOCKS - i - 1) {
      printf("test failed: memb_numfree() returns an invalid value %d, "
             "which should be %d\n", ret, NUM_MEMB_BLOCKS - i - 1);
      return -1;
    } else {
      printf("- memb_alloc is OK: memory block %p is allocated\n",
             memb_block_p);
    }
    memb_block_list[i] = memb_block_p;
  }

  /* try to allocate another memory block, which should fail */
  if((memb_block_p = (test_struct_t *)memb_alloc(&memb_pool)) != NULL) {
    printf("test failed: memb_alloc() allocates more memory than defined\n");
    return -1;
  } else {
    printf("- memb_alloc is OK: we cannot get any more memory block\n");
  }

  /* free the allocated memory blocks */
  for(int i = 0; i < NUM_MEMB_BLOCKS; i++) {
    memb_block_p = memb_block_list[i];
    if((ret = memb_free(&memb_pool, memb_block_p)) != 0) {
      printf("test failed: cannot memb_free() to %p, return value is %d\n",
             memb_block_p, ret);
      return -1;
    } else if((ret = memb_numfree(&memb_pool)) != i + 1) {
      printf("test failed: memb_numfree() returns an invalid value %d, "
             "which should be %d\n", ret, i + 1);
      return -1;
    } else {
      printf("- memb_free is OK: memory block %p is freed\n", memb_block_p);
    }
  }

  /*
   * call memb_free() again with the addresses of previously allocated
   * memory blocks (test for double-free)
   */
  for(int i = 0; i < NUM_MEMB_BLOCKS; i++) {
    memb_block_p = memb_block_list[i];
    if((ret = memb_free(&memb_pool, memb_block_p)) != -1) {
      /* double free shouldn't succeed (we should have -1 returned) */
      printf("test failed: cannot double free to %p, return value is %d\n",
             memb_block_p, ret);
      return -1;
    } else if((ret = memb_numfree(&memb_pool)) != NUM_MEMB_BLOCKS) {
      /* memb_numfree() should return NUM_MEMB_BLOCKS as no memory is used */
      printf("test failed: memb_numfree() returns an invalid value %d, "
             "which should be %d\n", ret, NUM_MEMB_BLOCKS);
      return -1;
    } else {
      printf("- memb_free is OK: memory block %p is double-freed\n",
             memb_block_p);
    }
  }

  /* free with a invalid address, which are not the beginning of a block */
  if((memb_block_p = memb_alloc(&memb_pool)) == NULL) {
    printf("test failed: memb_alloc() returns NULL while no memory is used\n");
    return -1;
  } else if(memb_free(&memb_pool, ONE_BYTE_OFF_ADDR(memb_block_p)) != -1) {
    printf("test failed: memb_free accepts an invalid address %p, "
           "which is one byte off from memory block starting at %p\n",
           ONE_BYTE_OFF_ADDR(memb_block_p), memb_block_p);
    return -1;
  } else {
    printf("- memb_free is OK: reject an invalid address %p\n",
           ONE_BYTE_OFF_ADDR(memb_block_p));
    (void)memb_free(&memb_pool, memb_block_p);
  }

  return 0;
}
