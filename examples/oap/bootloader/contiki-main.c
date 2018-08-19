/*
 * Copyright (c) 2016, Mark Solters <msolters@gmail.com>
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
int
main(void)
{
  int i;
  uint16_t latest;
  ota_firmware_metadata_t *internal_metadata;
  ota_firmware_metadata_t this;
  uint8_t area_with_latest_valid_ver;
  bool success;
  bool newer_in_ext_flash;

  bootloader_arch_init();

  clock_init();
  process_init();
  process_start(&etimer_process, NULL);

  memset(&this, 0, sizeof(this));

  ext_flash_init(NULL);

  internal_metadata = (ota_firmware_metadata_t *)OTA_METADATA_BASE;

#if BOOTLOADER_ERASE_EXT_FLASH
  for(i = 0; i < OTA_EXT_FLASH_AREA_COUNT; i++) {
    ota_ext_flash_area_erase(i);
  }
#endif

#if BOOTLOADER_BACKUP_GOLDEN_IMAGE
  ota_ext_flash_area_erase(OTA_EXT_FLASH_GOLDEN_AREA);
  if(!ota_ext_flash_area_write_image(
       OTA_EXT_FLASH_GOLDEN_AREA, (const uint8_t *)OTA_MAIN_FW_BASE,
       internal_metadata->length)) {
    LOG_ERR("Write image to external flash failed\n");
  }
#endif

  latest = internal_metadata->version;
  newer_in_ext_flash = false;

  LOG_INFO("Ext flash validation\n");
  for(i = 0; i < OTA_EXT_FLASH_AREA_COUNT; i++) {
    success = ota_ext_flash_area_validate(i, &this);
    if(success) {
      LOG_INFO("Area %d: valid, Ver=0x%04X\n", i, this.version);
      if(this.version > latest) {
        newer_in_ext_flash = true;
        latest = this.version;
        area_with_latest_valid_ver = i;
      }
    } else {
      LOG_INFO("Area %d: invalid or error\n", i);
    }
  }

  LOG_INFO("Internal Ver=0x%04X, Latest=0x%04X\n",
           internal_metadata->version, latest);

  /* Found newer image on flash, copy over */
  if(newer_in_ext_flash) {
    LOG_INFO("Copy image Ver=0x%04X from area %u\n",
             latest, area_with_latest_valid_ver);
    bootloader_arch_install_image_from_area(area_with_latest_valid_ver);
  }

  /* Validate internal image: This could well be the one just copied over */
  if(bootloader_validate_internal_image()) {
    bootloader_arch_jump_to_app();
  }

  /* Copy golden image */
  LOG_INFO("Fall back to golden image\n");
  bootloader_arch_install_image_from_area(OTA_EXT_FLASH_GOLDEN_AREA);

  /* Validate the golden image. This should work, really... */
  if(bootloader_validate_internal_image()) {
    bootloader_arch_jump_to_app();
  }

  /*
   * Fail... Toggle a LED and let the dog reset us. If this was a transient
   * golden image fallback failure, next time it could work. Or we will
   * end up restarting forever...
   */
  leds_on(LEDS_RED);

  return 0;
}
/*---------------------------------------------------------------------------*/
