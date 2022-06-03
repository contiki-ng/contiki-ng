/*
 * Copyright (c) 2008, Swedish Institute of Computer Science.
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

/**
 * \file
 *         Unit tests for the Coffee file system.
 * \author
 *         Nicolas Tsiftes <nvt@sics.se>
 */

#include "contiki.h"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
#include "lib/crc16.h"
#include "lib/random.h"

#include "unit-test/unit-test.h"

#include <stdio.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
PROCESS(testcoffee_process, "Test CFS/Coffee process");
AUTOSTART_PROCESSES(&testcoffee_process);
/*---------------------------------------------------------------------------*/
#ifdef FILE_CONF_SIZE
#define FILE_SIZE FILE_CONF_SIZE
#else
#define FILE_SIZE	4096
#endif /* FILE_CONF_SIZE */
/*---------------------------------------------------------------------------*/
static int wfd, rfd, afd;
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(coffee_basic_io, "Basic Coffee I/O test");
UNIT_TEST(coffee_basic_io)
{
  UNIT_TEST_BEGIN();

  unsigned char buf[256];
  int r;

  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  for(r = 0; r < sizeof(buf); r++) {
    buf[r] = r;
  }

  /* Test 1: Open for writing. */
  wfd = cfs_open("T1", CFS_WRITE);
  UNIT_TEST_ASSERT(wfd >= 0);

  /* Test 2 and 3: Write buffer. */
  r = cfs_write(wfd, buf, sizeof(buf));
  UNIT_TEST_ASSERT(r >= 0);
  UNIT_TEST_ASSERT(r == sizeof(buf));

  /* Test 4: Deny reading. */
  r = cfs_read(wfd, buf, sizeof(buf));
  UNIT_TEST_ASSERT(r < 0);

  /* Test 5: Open for reading. */
  rfd = cfs_open("T1", CFS_READ);
  UNIT_TEST_ASSERT(rfd >= 0);

  /* Test 6: Write to read-only file. */
  r = cfs_write(rfd, buf, sizeof(buf));
  UNIT_TEST_ASSERT(r < 0);

  /* Test 7 and 8: Read the buffer written in Test 2. */
  memset(buf, 0, sizeof(buf));
  r = cfs_read(rfd, buf, sizeof(buf));
  UNIT_TEST_ASSERT(r >= 0);
  UNIT_TEST_ASSERT(r == sizeof(buf));

  /* Test 9: Verify that the buffer is correct. */
  for(r = 0; r < sizeof(buf); r++) {
    UNIT_TEST_ASSERT(buf[r] == r);
  }

  /* Test 10: Seek to beginning. */
  UNIT_TEST_ASSERT(cfs_seek(wfd, 0, CFS_SEEK_SET) == 0);

  /* Test 11 and 12: Write to the log. */
  r = cfs_write(wfd, buf, sizeof(buf));
  UNIT_TEST_ASSERT(r >= 0);
  UNIT_TEST_ASSERT(r == sizeof(buf));

  /* Test 13 and 14: Read the data from the log. */
  cfs_seek(rfd, 0, CFS_SEEK_SET);
  memset(buf, 0, sizeof(buf));
  r = cfs_read(rfd, buf, sizeof(buf));
  UNIT_TEST_ASSERT(r >= 0);
  UNIT_TEST_ASSERT(r == sizeof(buf));

  /* Test 16: Verify that the data is correct. */
  for(r = 0; r < sizeof(buf); r++) {
    UNIT_TEST_ASSERT(buf[r] == r);
  }

  /* Test 17 to 20: Write a reversed buffer to the file. */
  for(r = 0; r < sizeof(buf); r++) {
    buf[r] = sizeof(buf) - r - 1;
  }

  UNIT_TEST_ASSERT(cfs_seek(wfd, 0, CFS_SEEK_SET) == 0);

  r = cfs_write(wfd, buf, sizeof(buf));
  UNIT_TEST_ASSERT(r >= 0);
  UNIT_TEST_ASSERT(r == sizeof(buf));

  UNIT_TEST_ASSERT(cfs_seek(rfd, 0, CFS_SEEK_SET) == 0);

  /* Test 21 and 22: Read the reversed buffer. */
  cfs_seek(rfd, 0, CFS_SEEK_SET);
  memset(buf, 0, sizeof(buf));
  r = cfs_read(rfd, buf, sizeof(buf));
  UNIT_TEST_ASSERT(r >= 0);
  UNIT_TEST_ASSERT(r == sizeof(buf));

  /* Test 23: Verify that the data is correct. */
  for(r = 0; r < sizeof(buf); r++) {
    UNIT_TEST_ASSERT(buf[r] == sizeof(buf) - r - 1);
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(coffee_append, "Coffee append operations");
UNIT_TEST(coffee_append)
{
  UNIT_TEST_BEGIN();

  unsigned char buf[256], buf2[11];
  int r;
#define APPEND_BYTES 1000
#define BULK_SIZE 10

  /* Test 1 and 2: Append data to the same file many times. */
  for(int i = 0; i < APPEND_BYTES; i += BULK_SIZE) {
    afd = cfs_open("T2", CFS_WRITE | CFS_APPEND);
    UNIT_TEST_ASSERT(afd >= 0);

    for(int j = 0; j < BULK_SIZE; j++) {
      buf[j] = 1 + ((i + j) & 0x7f);
    }
    r = cfs_write(afd, buf, BULK_SIZE);
    UNIT_TEST_ASSERT(r == BULK_SIZE);
    cfs_close(afd);
  }

  /* Test 3-6: Read back the data written previously and verify that it
     is correct. */
  afd = cfs_open("T2", CFS_READ);
  UNIT_TEST_ASSERT(afd >= 0);

  int total_read = 0;
  while((r = cfs_read(afd, buf2, sizeof(buf2))) > 0) {
    for(int j = 0; j < r; j++) {
      UNIT_TEST_ASSERT(buf2[j] == 1 + ((total_read + j) & 0x7f));
    }
    total_read += r;
  }

  UNIT_TEST_ASSERT(r == 0);
  UNIT_TEST_ASSERT(total_read == APPEND_BYTES);

  cfs_close(afd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(coffee_modify, "Coffee modifications");
UNIT_TEST(coffee_modify)
{
  UNIT_TEST_BEGIN();

  unsigned char buf[256];

  cfs_close(wfd);

  UNIT_TEST_ASSERT(cfs_coffee_reserve("T3", FILE_SIZE) >= 0);
  UNIT_TEST_ASSERT(cfs_coffee_configure_log("T3", FILE_SIZE / 2, 11) >= 0);

  /* Test 16: Test multiple writes at random offset. */
  for(int r = 0; r < 100; r++) {
    wfd = cfs_open("T3", CFS_WRITE | CFS_READ);
    UNIT_TEST_ASSERT(wfd >= 0);

    unsigned offset = random_rand() % FILE_SIZE;

    for(int i = 0; i < sizeof(buf); i++) {
      buf[i] = i;
    }

    UNIT_TEST_ASSERT(cfs_seek(wfd, offset, CFS_SEEK_SET) == offset);
    UNIT_TEST_ASSERT(cfs_write(wfd, buf, sizeof(buf)) == sizeof(buf));
    UNIT_TEST_ASSERT(cfs_seek(wfd, offset, CFS_SEEK_SET) == offset);

    memset(buf, 0, sizeof(buf));
    UNIT_TEST_ASSERT(cfs_read(wfd, buf, sizeof(buf)) == sizeof(buf));

    for(int i = 0; i < sizeof(buf); i++) {
      UNIT_TEST_ASSERT(buf[i] == i);
    }

    cfs_close(wfd);
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(coffee_gc, "Coffee garbage collection");
UNIT_TEST(coffee_gc)
{
  UNIT_TEST_BEGIN();

  for (int i = 0; i < 100; i++) {
    if (i & 1) {
      UNIT_TEST_ASSERT(cfs_coffee_reserve("FileB", random_rand() & 0xffff) == 0);
      cfs_remove("FileA");
    } else {
      UNIT_TEST_ASSERT(cfs_coffee_reserve("FileA", 93171) == 0);
      cfs_remove("FileB");
    }
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(testcoffee_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(coffee_basic_io);
  UNIT_TEST_RUN(coffee_append);
  UNIT_TEST_RUN(coffee_modify);
  UNIT_TEST_RUN(coffee_gc);

  cfs_close(wfd);
  cfs_close(rfd);
  cfs_close(afd);
  cfs_remove("T1");
  cfs_remove("T2");
  cfs_remove("T3");

  if(!UNIT_TEST_PASSED(coffee_basic_io) ||
     !UNIT_TEST_PASSED(coffee_append) ||
     !UNIT_TEST_PASSED(coffee_modify) ||
     !UNIT_TEST_PASSED(coffee_gc)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
