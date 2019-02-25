/*
 * Copyright (c) 2006, Swedish Institute of Computer Science
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
 */

/**
 * \file
 *         Device driver for the MX25R8035F 1Mbyte external memory.
 * \author
 *         Dongda Lee <dongdongbhbh@gmail.com>
 *
 *         Data is written bit inverted (~-operator) to flash so that
 *         unwritten data will read as zeros (UNIX style).
 */

#include "contiki.h"
#include "ext-flash.h"
#include "dev/xmem.h"
#include "dev/watchdog.h"
#include "cfs-coffee-arch.h"
#include <stdio.h> /* For PRINTF() */

#define EXT_ERASE_UNIT_SIZE     4096UL

#define XMEM_BUFF_LENGHT        128

#if 0
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...) do {} while(0)
#endif

void
xmem_init(void)
{
  ext_flash_open(NULL);
}
int
xmem_pread(void *_p, int size, unsigned long addr)
{
  int rv;
  uint8_t x;
  int i;

  rv = ext_flash_open(NULL);

  if(!rv) {
    PRINTF("Could not open flash to save config\n");
    ext_flash_close(NULL);
    return -1;
  }

  rv = ext_flash_read(NULL, addr, size, _p);
  for(i = 0; i < size; i++) {
    x = ~*((uint8_t *)_p + i);
    *((uint8_t *)_p + i) = x;
  }

  ext_flash_close(NULL);

  if(rv) {
    return size;
  }

  PRINTF("Could not read flash memory!\n");
  return -1;
}
int
xmem_pwrite(const void *_buf, int size, unsigned long addr)
{
  int rv;
  int i, j;
  int remain;

  uint8_t tmp_buf[XMEM_BUFF_LENGHT];

  rv = ext_flash_open(NULL);

  if(!rv) {
    PRINTF("Could not open flash to save config!\n");
    ext_flash_close(NULL);
    return -1;
  }

  for(remain = size, j = 0; remain > 0; remain -= XMEM_BUFF_LENGHT, j += XMEM_BUFF_LENGHT) {
    int to_write = MIN(XMEM_BUFF_LENGHT, remain);
    for(i = 0; i < to_write; i++) {
      tmp_buf[i] = ~*((uint8_t *)_buf + j + i);
    }
    rv = ext_flash_write(NULL, addr + j, to_write, tmp_buf);
    if(!rv) {
      PRINTF("Could not write flash memory!\n");
      return size - remain;
    }
  }

  ext_flash_close(NULL);

  return size;
}
int
xmem_erase(long size, unsigned long addr)
{
  int rv;

  rv = ext_flash_open(NULL);

  if(!rv) {
    PRINTF("Could not open flash to save config\n");
    ext_flash_close(NULL);
    return -1;
  }

  if(size % EXT_ERASE_UNIT_SIZE != 0) {
    PRINTF("xmem_erase: bad size\n");
    return -1;
  }

  if(addr % EXT_ERASE_UNIT_SIZE != 0) {
    PRINTF("xmem_erase: bad offset\n");
    return -1;
  }

  rv = ext_flash_erase(NULL, addr, size);

  ext_flash_close(NULL);

  watchdog_periodic();

  if(rv) {
    return size;
  }

  PRINTF("Could not erase flash memory\n");
  return -1;
}
