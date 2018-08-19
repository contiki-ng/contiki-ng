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
#include "dev/ext-flash/ext-flash.h"
#include "lib/crc16.h"
#include "net/app-layer/ota/ota.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "bootloader"
#ifndef LOG_LEVEL_BOOTLOADER
#define LOG_LEVEL_BOOTLOADER LOG_LEVEL_NONE
#endif
#define LOG_LEVEL LOG_LEVEL_BOOTLOADER
/*---------------------------------------------------------------------------*/
bool
bootloader_validate_internal_image()
{
  unsigned short crc;
  ota_firmware_metadata_t *md;

  LOG_INFO("Read internal metadata from: 0x%08lX\n",
           (unsigned long)OTA_METADATA_BASE);

  md = (ota_firmware_metadata_t *)OTA_METADATA_BASE;

  if((md->length == OTA_IMAGE_INVALID_LEN) ||
     (md->length > OTA_MAIN_FW_MAX_LEN)) {
    LOG_ERR("Invalid Length: 0x%08lX (Max=0x%08lX)\n",
            (unsigned long)md->length, (unsigned long)OTA_MAIN_FW_MAX_LEN);
    return false;
  }

  LOG_INFO("Firmware Length: 0x%08lX\n", (unsigned long)md->length);
  LOG_INFO("Firmware Version: 0x%04X\n", md->version);
  LOG_INFO("Firmware UUID: 0x%08lX\n", (unsigned long)md->uuid);

  crc = crc16_data((unsigned char *)OTA_MAIN_FW_BASE, md->length, 0);

  LOG_INFO("CRC=0x%04X, MD CRC=0x%04X\n", crc, md->crc);

  return md->crc == crc;
}
/*---------------------------------------------------------------------------*/
