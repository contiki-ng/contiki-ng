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
#include "dev/ext-flash/ext-flash.h"
#include "dev/leds.h"
#include "net/app-layer/ota/ota.h"
#include "net/app-layer/ota/ota-ext-flash.h"
#include "bootloader.h"

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "main"
#ifndef LOG_LEVEL_BOOTLOADER
#define LOG_LEVEL_BOOTLOADER LOG_LEVEL_NONE
#endif
#define LOG_LEVEL LOG_LEVEL_BOOTLOADER
/*---------------------------------------------------------------------------*/
static ota_firmware_metadata_t external_metadata[OTA_EXT_FLASH_AREA_COUNT];
/*---------------------------------------------------------------------------*/
int
main(void)
{
  int i;
  ota_firmware_metadata_t md;
  bool success;

  bootloader_arch_init();

  clock_init();
  process_init();
  process_start(&etimer_process, NULL);

  memset(external_metadata, 0, sizeof(external_metadata));

  ext_flash_init(NULL);

  LOG_INFO("OTA_MAIN_FW_BASE=0x%08lX\n", (unsigned long)OTA_MAIN_FW_BASE);
  LOG_INFO("OTA_MAIN_FW_MAX_LEN=0x%08lX\n", (unsigned long)OTA_MAIN_FW_MAX_LEN);
  LOG_INFO("OTA_METADATA_OFFSET=0x%08lX\n", (unsigned long)OTA_METADATA_OFFSET);
  LOG_INFO("OTA_METADATA_BASE=0x%08lX\n", (unsigned long)OTA_METADATA_BASE);

#if BOOTLOADER_ERASE_EXT_FLASH
  for(i = 0; i < OTA_EXT_FLASH_AREA_COUNT; i++) {
    ota_ext_flash_area_erase(i);
  }
#endif

#if BOOTLOADER_BACKUP_GOLDEN_IMAGE
  ota_ext_flash_area_erase(OTA_EXT_FLASH_GOLDEN_AREA);
  if(!ota_ext_flash_area_write_image(
       ((ota_firmware_metadata_t *)OTA_METADATA_BASE)->length)) {
       OTA_EXT_FLASH_GOLDEN_AREA, (const uint8_t *)OTA_MAIN_FW_BASE,
    LOG_ERR("Write image to external flash failed\n");
  }
#endif

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

  for(i = 0; i < OTA_EXT_FLASH_AREA_COUNT; i++) {
    memset(&md, 0, sizeof(md));
    success = ota_ext_flash_read_metadata(i, &md);
    if(success) {
      LOG_INFO("Read metadata area %d:\n", i);
      LOG_INFO("   Len=0x%08lX:\n", (unsigned long)md.length);
      LOG_INFO("  UUID=0x%08lX:\n", (unsigned long)md.uuid);
      LOG_INFO("   Ver=0x%04X:\n", md.version);
      LOG_INFO("   CRC=0x%04X:\n", md.crc);
    } else {
      LOG_ERR("Read metadata from area %d failed\n", i);
    }
  }

  for(i = 0; i < OTA_EXT_FLASH_AREA_COUNT; i++) {
    success = ota_ext_flash_area_validate(i, &external_metadata[i]);
    if(success) {
      LOG_INFO("Area %d: valid\n", i);
    } else {
      LOG_INFO("Area %d: invalid or error\n", i);
    }
  }

  if(bootloader_validate_internal_image()) {
    bootloader_arch_jump_to_app();
  }
  leds_on(LEDS_RED);

  return 0;
}
/*---------------------------------------------------------------------------*/
