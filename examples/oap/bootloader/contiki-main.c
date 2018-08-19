/*
 * Copyright (c) 2018, George Oikonomou - http://www.spd.gr
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
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "sys/platform.h"
#include "dev/watchdog.h"
#include "dev/ext-flash/ext-flash.h"
#include "dev/leds.h"
#include "net/app-layer/ota/ota.h"
#include "bootloader.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
int
main(void)
{
  bootloader_arch_init();

  clock_init();
  watchdog_init();

//  ext_flash_init(NULL);

  watchdog_start();

  printf("OTA_MAIN_FW_BASE=0x%08lX\n", (unsigned long)OTA_MAIN_FW_BASE);
  printf("OTA_MAIN_FW_MAX_LEN=0x%08lX\n", (unsigned long)OTA_MAIN_FW_MAX_LEN);
  printf("OTA_METADATA_OFFSET=0x%08lX\n", (unsigned long)OTA_METADATA_OFFSET);
  printf("OTA_METADATA_BASE=0x%08lX\n", (unsigned long)OTA_METADATA_BASE);

  /*
   * Collect firmware versions from ext flash
   *
   * Compare with version in internal flash
   *
   * start with highest version > current
   *   calculate CRC
   *     if pass, copy, verify then break
   *     else fallback to next highest version
   *
   * calculate internal CRC
   *   if pass jump
   *   else copy golden image, verify, jump
   */

  //    printf("Len=0x%08lx, CRC=0x%04x, Calculated CRC=0x%04x\n",
  //           (unsigned long)metadata.length,
  //           metadata.crc, crc);

  if(bootloader_validate_internal_image()) {
    bootloader_arch_jump_to_app();
  }
  leds_on(LEDS_RED);

//  if(bootloader_validate_image()) {
//    bootloader_arch_jump_to_app();
//  }

  return 0;
}
/*---------------------------------------------------------------------------*/
