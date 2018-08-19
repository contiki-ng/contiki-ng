/*
 * Copyright (c) 2016, Mark Solters <msolters@gmail.com>
 * Copyright (c) 2018, George Oikonomou - http://www.spd.gr
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
/*---------------------------------------------------------------------------*/
/**
 * \addtogroup ota
 * @{
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *    Implementation of the Contiki-NG OTA engine external flash manipulation
 */
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/ext-flash/ext-flash.h"
#include "net/app-layer/ota/ota.h"
#include "net/app-layer/ota/ota-ext-flash.h"

#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "ota-ext-flash"
#ifndef LOG_LEVEL_OTA
#define LOG_LEVEL_OTA LOG_LEVEL_NONE
#endif
#define LOG_LEVEL LOG_LEVEL_OTA
/*---------------------------------------------------------------------------*/
void
ext_flash_area_erase(uint8_t area)
{
  uint32_t erase_offset;

  if(area >= OTA_EXT_FLASH_AREA_COUNT) {
    return;
  }

  erase_offset = area * OTA_EXT_FLASH_AREA_LEN;

  if(!ext_flash_open(NULL)) {
    return;
  }

  ext_flash_erase(NULL, erase_offset, OTA_EXT_FLASH_AREA_LEN);

  ext_flash_close(NULL);
}
/*---------------------------------------------------------------------------*/
bool
ota_ext_flash_read_metadata(uint8_t area, ota_firmware_metadata_t *md)
{
  bool success = false;
  uint32_t read_addr;

  if(area >= OTA_EXT_FLASH_AREA_COUNT) {
    return false;
  }

  if(!ext_flash_open(NULL)) {
    return false;
  }

  read_addr = area * OTA_EXT_FLASH_AREA_LEN + OTA_METADATA_OFFSET;

  LOG_INFO("Read from 0x%08lX\n", (unsigned long)read_addr);

  success = ext_flash_read(NULL, read_addr, sizeof(ota_firmware_metadata_t),
                           (uint8_t *)md);

  ext_flash_close(NULL);

  return success;
}
/*---------------------------------------------------------------------------*/
#if 0
bool
bootloader_validate_image()
{
  int i, j, k;
  unsigned short crc;
  ota_firmware_metadata_t metadata;

  bool success = ext_flash_open(NULL);

  if(!success) {
    return false;
  }

  for(i = 0; i < BOOTLOADER_EXT_FLASH_AREA_COUNT; i++) {
    uint32_t metadata_loc = i * BOOTLOADER_EXT_FLASH_AREA_LEN +
                            BOOTLOADER_EXT_FLASH_OTA_METADATA_OFFSET;
    crc = 0;
    memset(&metadata, 0, 0);
    success = ext_flash_read(NULL, metadata_loc, sizeof(metadata),
                             (uint8_t *)&metadata);
    if(!success) {
      ext_flash_close(NULL);
      return false;
    }

    for(j = 1; j < BOOTLOADER_EXT_FLASH_AREA_LEN / BUF_LEN; j += 1) {
      memset(&buf, 0, BUF_LEN);
      success = ext_flash_read(NULL, metadata_loc + j * BUF_LEN, BUF_LEN, buf);
      if(!success) {
        ext_flash_close(NULL);
        return false;
      }


      for(k = 0; k < BUF_LEN; k++) {
        crc = crc16_add(buf[k], crc);
      }
    }

//    printf("Len=0x%08lx, CRC=0x%04x, Calculated CRC=0x%04x\n",
//           (unsigned long)metadata.length,
//           metadata.crc, crc);
  }

  return true;
}
#endif

/*---------------------------------------------------------------------------*/
/**
 * @}
 */

