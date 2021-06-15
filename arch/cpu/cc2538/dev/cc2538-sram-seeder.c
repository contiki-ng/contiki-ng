/*
 * Copyright (c) 2015, Hasso-Plattner-Institut.
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
 */

/**
 * \addtogroup csprng
 * @{
 *
 * \file
 *         SRAM-based CSPRNG seeder.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "dev/cc2538-sram-seeder.h"
#include "sys/rtimer.h"
#include "lpm.h"
#include <string.h>

#define SRAM_BYTES 1024

/* in the middle of the second half of the SRAM */
static const uint8_t *sram_pointer = (uint8_t *)0x20001FFF;

/*---------------------------------------------------------------------------*/
static void
reset_sram(void)
{
  /*
   * Although it should not, the system gets stuck when sleeping less than
   * two seconds at this point. Sleeping for two seconds seems to be safe.
   */
  rtimer_arch_schedule(RTIMER_NOW() + RTIMER_ARCH_SECOND * 2);
  lpm_enter();
}
/*---------------------------------------------------------------------------*/
void
cc2538_sram_seeder_seed(void)
{
  struct csprng_seed seed;
  uint16_t byte_pos;
  uint8_t bit_pos;
  uint8_t sram_snapshot[SRAM_BYTES];
  uint16_t i;
  uint8_t j;
  int bit1;
  int bit2;

  byte_pos = 0;
  bit_pos = 0;

  while(1) {
    reset_sram();
    memcpy(sram_snapshot, sram_pointer, SRAM_BYTES);
    reset_sram();

    /* von Neumann extractor */
    for(i = 0; i < SRAM_BYTES; i++) {
      for(j = 0; j < 8; j++) {
        bit1 = (1 << j) & sram_snapshot[i];
        bit2 = (1 << j) & sram_pointer[i];
        if(bit1 < bit2) {
          seed.u8[byte_pos] |= 1 << bit_pos++;
        } else if(bit1 > bit2) {
          seed.u8[byte_pos] &= ~(1 << bit_pos++);
        }
        if(bit_pos == 8) {
          bit_pos = 0;
          byte_pos++;
        }
        if(byte_pos == CSPRNG_SEED_LEN) {
          csprng_feed(&seed);
          return;
        }
      }
    }
  }
}
/*---------------------------------------------------------------------------*/

/** @} */
