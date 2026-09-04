/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden
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
 * \addtogroup nrf
 * @{
 *
 * \file
 *         External-memory (xmem) API over a reserved region of the
 *         nRF54L15's internal RRAM.
 *
 *         The region is carved from the top of the code memory by the
 *         linker script when MAKE_WITH_XMEM=1 (XMEM_CONF_SIZE bytes,
 *         default 64 kB); the linker exports its bounds. RRAM is
 *         byte-addressable and needs no erase; "erase" fills with 0xff
 *         to match what callers written for flash expect.
 * \author
 *         Niclas Finne <niclas.finne@ri.se>
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/xmem.h"
#include "dev/watchdog.h"

#include <nrfx_rramc.h>
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "XMEM"
#define LOG_LEVEL LOG_LEVEL_NONE
/*---------------------------------------------------------------------------*/
#ifndef XMEM_CONF_SIZE
#error "xmem-rram.c needs XMEM_CONF_SIZE (set by Makefile.nrf54l15 with MAKE_WITH_XMEM=1)"
#endif
#if (XMEM_CONF_SIZE % 4096) != 0
#error "XMEM_CONF_SIZE must be a multiple of 4096 (the xmem erase unit)"
#endif

/* Bounds of the reserved region, from the linker script. */
extern uint32_t __xmem_start;
extern uint32_t __xmem_size;
#define XMEM_START ((uintptr_t)&__xmem_start)
#define XMEM_SIZE ((uint32_t)(uintptr_t)&__xmem_size)

#ifndef XMEM_ERASE_UNIT_SIZE
#define XMEM_ERASE_UNIT_SIZE 4096
#endif

/* RRAMC write buffer, in 128-bit words. */
#define WRITE_BUFFER_SIZE 16

/* One full RRAMC write buffer: WRITE_BUFFER_SIZE 128-bit words. */
#define ERASE_CHUNK_SIZE (WRITE_BUFFER_SIZE * 16)

#if (XMEM_ERASE_UNIT_SIZE % ERASE_CHUNK_SIZE) != 0
#error "XMEM_ERASE_UNIT_SIZE must be a multiple of the RRAMC write buffer size"
#endif

static bool initialized;
/*---------------------------------------------------------------------------*/
static bool
in_region(unsigned long offset, unsigned long nbytes)
{
  return offset <= XMEM_SIZE && nbytes <= XMEM_SIZE - offset;
}
/*---------------------------------------------------------------------------*/
/*
 * Writes go through the RRAMC write buffer and are committed explicitly by
 * the end of the window opened here; write mode is enabled only for the
 * duration of a write, so that a stray pointer elsewhere cannot alter
 * non-volatile memory.
 */
static void
rram_write_begin(void)
{
  nrfx_rramc_write_enable_set(true, WRITE_BUFFER_SIZE);
}
/*---------------------------------------------------------------------------*/
static void
rram_write_end(void)
{
  nrf_rramc_task_trigger(NRF_RRAMC, NRF_RRAMC_TASK_COMMIT_WRITEBUF);
  while(!nrf_rramc_ready_check(NRF_RRAMC)) {
  }
  nrfx_rramc_write_enable_set(false, 0);
}
/*---------------------------------------------------------------------------*/
static void
rram_write(uintptr_t address, const void *src, uint32_t nbytes)
{
  rram_write_begin();
  nrfx_rramc_bytes_write(address, src, nbytes);
  rram_write_end();
}
/*---------------------------------------------------------------------------*/
/*
 * RRAM has no erase; a range is "erased" by writing 0xff over it. The source
 * is a single 128-bit RRAM line, written repeatedly inside one write window,
 * so that a whole write buffer still reaches RRAM per commit.
 */
static void
rram_fill(uintptr_t address, uint32_t nbytes)
{
  static const uint8_t blank[16] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
  };

  rram_write_begin();
  for(uint32_t i = 0; i < nbytes; i += sizeof(blank)) {
    nrfx_rramc_bytes_write(address + i, blank, sizeof(blank));
  }
  rram_write_end();
}
/*---------------------------------------------------------------------------*/
void
xmem_init(void)
{
  if(initialized) {
    return;
  }
  if(XMEM_SIZE == 0) {
    LOG_WARN("No RRAM reserved for xmem\n");
    return;
  }

  nrfx_rramc_config_t config = NRFX_RRAMC_DEFAULT_CONFIG(WRITE_BUFFER_SIZE);
  nrfx_err_t err = nrfx_rramc_init(&config, NULL);
  if(err != NRFX_SUCCESS && err != NRFX_ERROR_ALREADY_INITIALIZED) {
    LOG_ERR("RRAMC init failed: %d\n", (int)err);
    return;
  }
  initialized = true;
  LOG_INFO("RRAM region for xmem: %lu bytes at 0x%08lx\n",
           (unsigned long)XMEM_SIZE, (unsigned long)XMEM_START);
}
/*---------------------------------------------------------------------------*/
int
xmem_pread(void *buf, int nbytes, unsigned long offset)
{
  if(!initialized || nbytes < 0 || !in_region(offset, nbytes)) {
    return -1;
  }
  memcpy(buf, (const void *)(XMEM_START + offset), nbytes);
  return nbytes;
}
/*---------------------------------------------------------------------------*/
int
xmem_pwrite(const void *buf, int nbytes, unsigned long offset)
{
  if(!initialized || nbytes < 0 || !in_region(offset, nbytes)) {
    return -1;
  }
  rram_write(XMEM_START + offset, buf, nbytes);
  return nbytes;
}
/*---------------------------------------------------------------------------*/
int
xmem_erase(long nbytes, unsigned long offset)
{
  if(!initialized || nbytes < 0 || !in_region(offset, nbytes)) {
    return -1;
  }
  if(offset % XMEM_ERASE_UNIT_SIZE != 0 || nbytes % XMEM_ERASE_UNIT_SIZE != 0) {
    LOG_ERR("xmem_erase: offset and size must be multiples of %u\n",
            (unsigned)XMEM_ERASE_UNIT_SIZE);
    return -1;
  }
  for(unsigned long done = 0; done < (unsigned long)nbytes;
      done += ERASE_CHUNK_SIZE) {
    rram_fill(XMEM_START + offset + done, ERASE_CHUNK_SIZE);
    watchdog_periodic();
  }
  return nbytes;
}
/** @} */
