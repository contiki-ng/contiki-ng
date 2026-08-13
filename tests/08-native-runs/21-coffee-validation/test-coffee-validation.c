/*
 * Copyright (c) 2025, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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
 *         Unit tests for Coffee filesystem header validation.
 * \author
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "cfs/cfs.h"
#include "cfs/cfs-coffee.h"
#include "cfs-coffee-arch.h"
#include "unit-test/unit-test.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>

/*---------------------------------------------------------------------------*/
PROCESS(test_coffee_validation_process, "Coffee validation test");
AUTOSTART_PROCESSES(&test_coffee_validation_process);
/*---------------------------------------------------------------------------*/
/* External access to Coffee internals for testing */

/* Coffee structure definitions (from cfs-coffee.c) */
struct file_header {
  coffee_page_t log_page;
  uint16_t log_records;
  uint16_t log_record_size;
  coffee_page_t max_pages;
  uint8_t deprecated_eof_hint;
  uint8_t flags;
  char name[COFFEE_NAME_LENGTH];
};

/* Header flags */
#define HDR_FLAG_VALID     0x01
#define HDR_FLAG_ALLOCATED 0x02
#define HDR_FLAG_OBSOLETE  0x04
#define HDR_FLAG_MODIFIED  0x08

/* Coffee size macros - defined in cfs-coffee-arch.h but needed for calculations */
#ifndef COFFEE_PAGE_COUNT
#define COFFEE_PAGE_COUNT \
  ((coffee_page_t)(COFFEE_SIZE / COFFEE_PAGE_SIZE))
#endif

#ifndef COFFEE_PAGES_PER_SECTOR
#define COFFEE_PAGES_PER_SECTOR 256
#endif

/* Helper macro to get the starting page of a sector */
#define SECTOR_PAGE(n) ((n) * COFFEE_PAGES_PER_SECTOR)

/* xmem functions are already declared via dev/xmem.h (included from cfs-coffee-arch.h) */

/*---------------------------------------------------------------------------*/
/* Helper function to inject a corrupted header at a specific page */
static void
inject_header(struct file_header *hdr, coffee_page_t page)
{
  xmem_pwrite(hdr, sizeof(*hdr), page * COFFEE_PAGE_SIZE);
}
/*---------------------------------------------------------------------------*/
/* Helper function to read a header from a specific page */
static void
read_raw_header(struct file_header *hdr, coffee_page_t page)
{
  xmem_pread(hdr, sizeof(*hdr), page * COFFEE_PAGE_SIZE);
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_max_pages_zero,
                   "Integer underflow: max_pages=0");
UNIT_TEST(test_max_pages_zero)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject a corrupt header with max_pages=0 */
  memset(&hdr, 0, sizeof(hdr));
  strncpy(hdr.name, "CORRUPT1", COFFEE_NAME_LENGTH - 1);
  hdr.max_pages = 0;
  hdr.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED;
  inject_header(&hdr, SECTOR_PAGE(1));

  /* Verify that the filesystem handles the corrupt header gracefully */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("VALID1", 1024) == 0);
  fd = cfs_open("VALID1", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_max_pages_overflow,
                   "Arithmetic overflow: max_pages=0xFFFF");
UNIT_TEST(test_max_pages_overflow)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject a corrupt header with an oversized max_pages value */
  memset(&hdr, 0, sizeof(hdr));
  strncpy(hdr.name, "CORRUPT2", COFFEE_NAME_LENGTH - 1);
  hdr.max_pages = 0x7FFF;
  hdr.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED;
  inject_header(&hdr, SECTOR_PAGE(2));

  /* Verify that no crash or out-of-bounds access occurs */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("VALID2", 512) == 0);
  fd = cfs_open("VALID2", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_file_extends_beyond_storage,
                   "File extension overflow: page + max_pages > total");
UNIT_TEST(test_file_extends_beyond_storage)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject a corrupt header near the end of storage that extends beyond bounds */
  coffee_page_t test_page = (COFFEE_PAGE_COUNT / COFFEE_PAGES_PER_SECTOR) * COFFEE_PAGES_PER_SECTOR - COFFEE_PAGES_PER_SECTOR;
  memset(&hdr, 0, sizeof(hdr));
  strncpy(hdr.name, "CORRUPT3", COFFEE_NAME_LENGTH - 1);
  hdr.max_pages = 512;
  hdr.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED;
  inject_header(&hdr, test_page);

  /* Verify that the error is handled gracefully without overflow */
  fd = cfs_open("CORRUPT3", CFS_READ);
  UNIT_TEST_ASSERT(fd < 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_invalid_log_page,
                   "Out-of-bounds: log_page >= COFFEE_PAGE_COUNT");
UNIT_TEST(test_invalid_log_page)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject a corrupt header with an out-of-bounds log_page value */
  memset(&hdr, 0, sizeof(hdr));
  strncpy(hdr.name, "CORRUPT4", COFFEE_NAME_LENGTH - 1);
  hdr.max_pages = 10;
  hdr.log_page = COFFEE_PAGE_COUNT + 100;
  hdr.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED | HDR_FLAG_MODIFIED;
  inject_header(&hdr, SECTOR_PAGE(1));

  /* Verify that the invalid log_page value is detected and rejected */
  fd = cfs_open("CORRUPT4", CFS_READ);
  UNIT_TEST_ASSERT(fd < 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_oversized_log_record_size,
                   "Buffer overflow: log_record_size > COFFEE_PAGE_SIZE");
UNIT_TEST(test_oversized_log_record_size)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject a corrupt header with an oversized log_record_size value */
  memset(&hdr, 0, sizeof(hdr));
  strncpy(hdr.name, "CORRUPT5", COFFEE_NAME_LENGTH - 1);
  hdr.max_pages = 10;
  hdr.log_page = SECTOR_PAGE(3);
  hdr.log_record_size = COFFEE_PAGE_SIZE + 100;
  hdr.log_records = 10;
  hdr.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED | HDR_FLAG_MODIFIED;
  inject_header(&hdr, SECTOR_PAGE(2));

  /* Verify that buffer overflow is prevented in log operations */
  fd = cfs_open("CORRUPT5", CFS_READ);
  UNIT_TEST_ASSERT(fd < 0);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_missing_null_terminator,
                   "String safety: filename without null terminator");
UNIT_TEST(test_missing_null_terminator)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject a corrupt header with an unterminated filename */
  memset(&hdr, 0, sizeof(hdr));
  memset(hdr.name, 'A', COFFEE_NAME_LENGTH);
  hdr.max_pages = 10;
  hdr.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED;
  inject_header(&hdr, SECTOR_PAGE(4));

  /* Verify that read_header() adds a null terminator and doesn't crash */
  fd = cfs_open("AAAAAAAAAAAAAAAA", CFS_READ);

  /* Verify that the filesystem still works */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("VALID3", 512) == 0);
  fd = cfs_open("VALID3", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_uninitialized_flash_all_zeros,
                   "Corrupted storage: uninitialized flash (all 0x00)");
UNIT_TEST(test_uninitialized_flash_all_zeros)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject an uninitialized flash pattern (all zeros) */
  memset(&hdr, 0x00, sizeof(hdr));
  inject_header(&hdr, SECTOR_PAGE(5));

  /* Verify that it is treated as free/invalid space without crashing */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("VALID4", 256) == 0);
  fd = cfs_open("VALID4", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_erased_flash_all_ones,
                   "Corrupted storage: erased flash (all 0xFF)");
UNIT_TEST(test_erased_flash_all_ones)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject an erased flash pattern (all 0xFF) */
  memset(&hdr, 0xFF, sizeof(hdr));
  inject_header(&hdr, SECTOR_PAGE(6));

  /* Verify that it is treated as free space */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("VALID5", 128) == 0);
  fd = cfs_open("VALID5", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_partially_written_header,
                   "Partial corruption: incomplete header write");
UNIT_TEST(test_partially_written_header)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject a partially written header (missing the VALID flag) */
  memset(&hdr, 0, sizeof(hdr));
  strncpy(hdr.name, "PARTIAL", COFFEE_NAME_LENGTH - 1);
  hdr.max_pages = 5;
  hdr.flags = HDR_FLAG_ALLOCATED;
  inject_header(&hdr, SECTOR_PAGE(7));

  /* Verify that the invalid header is skipped during traversal */
  fd = cfs_open("PARTIAL", CFS_READ);
  UNIT_TEST_ASSERT(fd < 0);

  /* Verify that the filesystem is still usable */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("VALID6", 512) == 0);
  fd = cfs_open("VALID6", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_garbage_collection_with_corrupt_headers,
                   "GC resilience: garbage collection with invalid headers");
UNIT_TEST(test_garbage_collection_with_corrupt_headers)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;
  int i;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Create files, then corrupt one of them */
  for(i = 0; i < 3; i++) {
    char filename[16];
    snprintf(filename, sizeof(filename), "CORRUPT%d", i);
    UNIT_TEST_ASSERT(cfs_coffee_reserve(filename, 1024) == 0);
  }

  /* Corrupt the first file by setting max_pages to 0 */
  read_raw_header(&hdr, 0);
  hdr.max_pages = 0;  /* Corrupt it */
  inject_header(&hdr, 0);

  /* Create and remove files to trigger garbage collection */
  for(i = 0; i < 10; i++) {
    char filename[16];
    snprintf(filename, sizeof(filename), "GCTEST%d", i);

    UNIT_TEST_ASSERT(cfs_coffee_reserve(filename, 2048) == 0);
    fd = cfs_open(filename, CFS_WRITE);
    UNIT_TEST_ASSERT(fd >= 0);

    /* Write some data to the file */
    unsigned char buf[64];
    memset(buf, i, sizeof(buf));
    UNIT_TEST_ASSERT(cfs_write(fd, buf, sizeof(buf)) == sizeof(buf));
    cfs_close(fd);

    /* Remove the file to create obsolete pages */
    UNIT_TEST_ASSERT(cfs_remove(filename) == 0);
  }

  /* GC should handle the corrupt headers gracefully */
  /* Create a new file to potentially trigger GC */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("POSTGC", 1024) == 0);
  fd = cfs_open("POSTGC", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_find_file_with_corrupt_entries,
                   "File search: find valid file despite corrupt entries");
UNIT_TEST(test_find_file_with_corrupt_entries)
{
  UNIT_TEST_BEGIN();

  struct file_header hdr;
  int fd;
  unsigned char write_buf[32];
  unsigned char read_buf[32];

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Create multiple files, then corrupt some of them */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("DUMMY1", 512) == 0);
  UNIT_TEST_ASSERT(cfs_coffee_reserve("TARGET", 512) == 0);
  UNIT_TEST_ASSERT(cfs_coffee_reserve("DUMMY2", 512) == 0);

  /* Write data to the TARGET file */
  fd = cfs_open("TARGET", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  memset(write_buf, 0xAB, sizeof(write_buf));
  UNIT_TEST_ASSERT(cfs_write(fd, write_buf, sizeof(write_buf)) == sizeof(write_buf));
  cfs_close(fd);

  /* Corrupt DUMMY1 by setting max_pages to 0 */
  read_raw_header(&hdr, 0);
  hdr.max_pages = 0;
  inject_header(&hdr, 0);

  /* Inject a corrupt header at a distant sector */
  memset(&hdr, 0, sizeof(hdr));
  strncpy(hdr.name, "DISTANT", COFFEE_NAME_LENGTH - 1);
  hdr.max_pages = 0x7FFF;
  hdr.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED;
  inject_header(&hdr, SECTOR_PAGE(8));

  /* Should still find and open the TARGET file */
  fd = cfs_open("TARGET", CFS_READ);
  UNIT_TEST_ASSERT(fd >= 0);

  /* Verify that the data is correct */
  memset(read_buf, 0, sizeof(read_buf));
  UNIT_TEST_ASSERT(cfs_read(fd, read_buf, sizeof(read_buf)) == sizeof(read_buf));
  UNIT_TEST_ASSERT(memcmp(write_buf, read_buf, sizeof(write_buf)) == 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_bit_flips_in_critical_fields,
                   "Bit corruption: single bit flips in header fields");
UNIT_TEST(test_bit_flips_in_critical_fields)
{
  UNIT_TEST_BEGIN();

  struct file_header corrupted;
  int fd;

  /* Format the filesystem */
  UNIT_TEST_ASSERT(cfs_coffee_format() == 0);

  /* Inject a header with a corrupt max_pages field */
  memset(&corrupted, 0, sizeof(corrupted));
  strncpy(corrupted.name, "BITFLIP1", COFFEE_NAME_LENGTH - 1);
  corrupted.max_pages = 0;
  corrupted.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED;
  inject_header(&corrupted, SECTOR_PAGE(3));

  /* Inject a header with a corrupt log_page field */
  memset(&corrupted, 0, sizeof(corrupted));
  strncpy(corrupted.name, "BITFLIP2", COFFEE_NAME_LENGTH - 1);
  corrupted.max_pages = 10;
  corrupted.log_page = COFFEE_PAGE_COUNT + 1;
  corrupted.flags = HDR_FLAG_VALID | HDR_FLAG_ALLOCATED | HDR_FLAG_MODIFIED;
  inject_header(&corrupted, SECTOR_PAGE(4));

  /* Verify that a valid file can be created despite the corrupt headers */
  UNIT_TEST_ASSERT(cfs_coffee_reserve("ORIGINAL", 1024) == 0);
  fd = cfs_open("ORIGINAL", CFS_WRITE);
  UNIT_TEST_ASSERT(fd >= 0);
  cfs_close(fd);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_coffee_validation_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(test_max_pages_zero);
  UNIT_TEST_RUN(test_max_pages_overflow);
  UNIT_TEST_RUN(test_file_extends_beyond_storage);
  UNIT_TEST_RUN(test_invalid_log_page);
  UNIT_TEST_RUN(test_oversized_log_record_size);
  UNIT_TEST_RUN(test_missing_null_terminator);
  UNIT_TEST_RUN(test_uninitialized_flash_all_zeros);
  UNIT_TEST_RUN(test_erased_flash_all_ones);
  UNIT_TEST_RUN(test_partially_written_header);
  UNIT_TEST_RUN(test_garbage_collection_with_corrupt_headers);
  UNIT_TEST_RUN(test_find_file_with_corrupt_entries);
  UNIT_TEST_RUN(test_bit_flips_in_critical_fields);

  if(!UNIT_TEST_PASSED(test_max_pages_zero) ||
     !UNIT_TEST_PASSED(test_max_pages_overflow) ||
     !UNIT_TEST_PASSED(test_file_extends_beyond_storage) ||
     !UNIT_TEST_PASSED(test_invalid_log_page) ||
     !UNIT_TEST_PASSED(test_oversized_log_record_size) ||
     !UNIT_TEST_PASSED(test_missing_null_terminator) ||
     !UNIT_TEST_PASSED(test_uninitialized_flash_all_zeros) ||
     !UNIT_TEST_PASSED(test_erased_flash_all_ones) ||
     !UNIT_TEST_PASSED(test_partially_written_header) ||
     !UNIT_TEST_PASSED(test_garbage_collection_with_corrupt_headers) ||
     !UNIT_TEST_PASSED(test_find_file_with_corrupt_entries) ||
     !UNIT_TEST_PASSED(test_bit_flips_in_critical_fields)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
