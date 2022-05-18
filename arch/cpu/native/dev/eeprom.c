/*
 * Copyright (c) 2013, Robert Quattlebaum.
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
 * This file is part of the Contiki operating system.
 *
 * Author: Robert Quattlebaum <darco@deepdarc.com>
 *
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "contiki.h"
#include "dev/eeprom.h"

/* Log configuration */
#include "sys/log.h"
#define LOG_MODULE "EEPROM"
#define LOG_LEVEL LOG_LEVEL_MAIN

static FILE *eeprom_file;

/*---------------------------------------------------------------------------*/
static bool
eeprom_fill(eeprom_addr_t addr, unsigned char value, size_t size)
{
  if(!eeprom_file) {
    return false;
  }

  if(addr > EEPROM_END_ADDR || addr + size > EEPROM_END_ADDR + 1) {
    LOG_ERR("Bad address and/or size (addr = %04x, size = %zu)\n", addr, size);
    return false;
  }

  if(fseek(eeprom_file, addr, SEEK_SET) < 0) {
    LOG_ERR("fseek() failed: %s\n", strerror(errno));
    return false;
  }

  while(size--) {
    if(fputc(value, eeprom_file) != value) {
      LOG_ERR("fputc() failed\n");
      return false;
    }
  }
  return true;
}

/*---------------------------------------------------------------------------*/
bool
eeprom_init(void)
{
  if(eeprom_file) {
    LOG_WARN("Re-initializing EEPROM\n");
    fclose(eeprom_file);
    eeprom_file = NULL;
  }

  if(EEPROM_SIZE == 0) {
    LOG_ERR("Cannot initialize EEPROM when EEPROM_SIZE is 0\n");
    return false;
  }

  char *eeprom_filename = getenv("CONTIKI_EEPROM");
  if(eeprom_filename != NULL) {
    LOG_INFO("Using EEPROM file \"%s\"\n", eeprom_filename);
  } else {
    LOG_INFO("CONTIKI_EEPROM env is not set; using a temp file instead\n");
  }

  eeprom_file = eeprom_filename ? fopen(eeprom_filename, "w+") : tmpfile();

  if(eeprom_file == NULL) {
    LOG_ERR("Unable to open the EEPROM file\n");
    goto error;
  }


  /* Make sure that the file is the correct size by seeking
   * to the end and checking the file position. If it is
   * less than what we expect, we pad out the rest of the file
   * with 0xFF, just like a real EEPROM. */

  if(fseek(eeprom_file, 0, SEEK_END) == -1) {
    LOG_ERR("fseek() failed: %s\n", strerror(errno));
    goto error;
  }

  off_t length = ftell(eeprom_file);
  if(length < 0) {
    LOG_ERR("ftell() failed\n");
    goto error;
  }

  if(length < EEPROM_END_ADDR) {
    /* Fill with 0xFF, just like a real EEPROM. */
    if(eeprom_fill(length, 0xFF, EEPROM_SIZE - length) == false) {
      goto error;
    }
  }

  return true;

error:
  if(eeprom_file != NULL) {
    fclose(eeprom_file);
    eeprom_file = NULL;
  }
  return false;
}

/*---------------------------------------------------------------------------*/
bool
eeprom_write(eeprom_addr_t addr, const unsigned char *buf, size_t size)
{
  if(!eeprom_file) {
    return false;
  }

  if(addr > EEPROM_END_ADDR || addr + size > EEPROM_END_ADDR + 1) {
    LOG_ERR("Bad address and/or size (addr = %04x, size = %zu)\n", addr, size);
    return false;
  }

  if(fseek(eeprom_file, addr, SEEK_SET) < 0) {
    LOG_ERR("fseek() failed: %s\n", strerror(errno));
    return false;
  }

  if(fwrite(buf, 1, size, eeprom_file) != size) {
    LOG_ERR("fwrite() failed\n");
    return false;
  }

  return true;
}

/*---------------------------------------------------------------------------*/
bool
eeprom_read(eeprom_addr_t addr, unsigned char *buf, size_t size)
{
  if(!eeprom_file) {
    return false;
  }

  if(addr > EEPROM_END_ADDR || addr + size > EEPROM_END_ADDR + 1) {
    LOG_ERR("Bad address and/or size (addr = %04x, size = %zu)\n", addr, size);
    return false;
  }

  if(fseek(eeprom_file, addr, SEEK_SET) < 0) {
    LOG_ERR("fseek() failed: %s\n", strerror(errno));
    return false;
  }

  if(fread(buf, 1, size, eeprom_file) != size) {
    LOG_ERR("fread() failed\n");
    return false;
  }

  return true;
}
