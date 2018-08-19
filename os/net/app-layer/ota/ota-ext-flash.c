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
#include "lib/crc16.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
/*---------------------------------------------------------------------------*/
#include "sys/log.h"
#define LOG_MODULE "ota-ext-flash"
#ifndef LOG_LEVEL_OTA
#define LOG_LEVEL_OTA LOG_LEVEL_NONE
#endif
#define LOG_LEVEL LOG_LEVEL_OTA
/*---------------------------------------------------------------------------*/
/* Chunk size to use when write an entire image to external flash */
#define WRITE_CHUNK_SIZE 128
/*---------------------------------------------------------------------------*/
/* Temporary buffer for flash reads when required */
#define BUF_LEN 128
static uint8_t tmp_buf[BUF_LEN];
/*---------------------------------------------------------------------------*/
void
ota_ext_flash_area_erase(uint8_t area)
{
  uint32_t erase_offset;

  if(area >= OTA_EXT_FLASH_AREA_COUNT) {
    return;
  }

  erase_offset = area * OTA_EXT_FLASH_AREA_LEN;

  LOG_INFO("Erase area %u, addr=0x%08lX, 0x%08lX bytes\n", area,
           (unsigned long)erase_offset, (unsigned long)OTA_EXT_FLASH_AREA_LEN);


  if(!ext_flash_open(NULL)) {
    LOG_ERR("Failed to open external flash\n");
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
    LOG_ERR("Failed to open external flash\n");
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
bool
ota_ext_flash_area_write_chunk(uint8_t area, uint32_t offset, uint8_t *chunk,
                               uint8_t chunk_len)
{
  uint32_t write_addr;
  bool success;

  if(area >= OTA_EXT_FLASH_AREA_COUNT) {
    return false;
  }

  if(!ext_flash_open(NULL)) {
    LOG_ERR("Failed to open external flash\n");
    return false;
  }

  write_addr = area * OTA_EXT_FLASH_AREA_LEN + offset;

  success = ext_flash_write(NULL, write_addr, chunk_len, chunk);

  if(!success) {
    LOG_ERR("Failed to write to external flash at 0x%08lX\n",
            (unsigned long)write_addr);
  }

  ext_flash_close(NULL);

  return success;
}
/*---------------------------------------------------------------------------*/
bool
ota_ext_flash_area_write_image(uint8_t area, uint8_t *img, uint32_t img_len)
{
  uint32_t write_addr;
  uint32_t bytes_written = 0;;
  uint8_t *read_addr;
  uint8_t write_size;
  bool success;

  if(area >= OTA_EXT_FLASH_AREA_COUNT) {
    return false;
  }

  /* Must leave enough space for metadata */
  if(img_len > OTA_EXT_FLASH_AREA_LEN - OTA_METADATA_LEN) {
    return false;
  }

  if(!ext_flash_open(NULL)) {
    LOG_ERR("Failed to open external flash\n");
    return false;
  }

  write_addr = area * OTA_EXT_FLASH_AREA_LEN;
  read_addr = img;

  LOG_INFO("Write image to area %u\n", area);
  LOG_INFO("  From 0x%08lX to 0x%08lX, 0x%08lX bytes\n",
           (unsigned long)read_addr, (unsigned long)write_addr,
           (unsigned long)img_len);

  while(bytes_written < img_len) {
    write_size = bytes_written + WRITE_CHUNK_SIZE <= img_len ?
                 WRITE_CHUNK_SIZE : img_len - bytes_written;

    success = ext_flash_write(NULL, write_addr, write_size, read_addr);

    if(!success) {
      LOG_ERR("Failed to write to external flash at 0x%08lX\n",
              (unsigned long)write_addr);
      break;
    }

    read_addr += write_size;
    bytes_written += write_size;
    write_addr += write_size;
  }

  LOG_INFO("  Wrote 0x%08lX bytes\n", (unsigned long)bytes_written);

  /* If image write succeeded, proceed with metadata */
  if(success) {
    write_addr = area * OTA_EXT_FLASH_AREA_LEN + OTA_METADATA_OFFSET;

    LOG_INFO("  Metadata from 0x%08lX to 0x%08lX, 0x%08lX bytes\n",
        (unsigned long)OTA_METADATA_BASE, (unsigned long)write_addr,
        (unsigned long)OTA_METADATA_LEN);
    success = ext_flash_write(NULL, write_addr, OTA_METADATA_LEN,
                              (const uint8_t *)OTA_METADATA_BASE);
  }

  ext_flash_close(NULL);

  return success;
}
/*---------------------------------------------------------------------------*/
bool
ota_ext_flash_area_validate(uint8_t area)
{
  int i;
  unsigned short crc;
  ota_firmware_metadata_t metadata;
  uint32_t read_start_addr;
  uint32_t bytes_read;
  uint8_t read_len;
  bool success;

  if(area >= OTA_EXT_FLASH_AREA_COUNT) {
    return false;
  }

  memset(&metadata, 0, sizeof(metadata));
  success = ota_ext_flash_read_metadata(area, &metadata);

  if(!success) {
    LOG_ERR("Failed to read metadata from external flash\n");
    return false;
  }

  if((metadata.length == OTA_IMAGE_INVALID_LEN) ||
     (metadata.length > OTA_MAIN_FW_MAX_LEN)) {
    LOG_ERR("Invalid image length 0x%08lX\n", (unsigned long)metadata.length);
    return false;
  }

  /* Metadata retrieved successfully and image length is valid */
  if(!ext_flash_open(NULL)) {
    LOG_ERR("Failed to open external flash\n");
    return false;
  }

  read_start_addr = area * OTA_EXT_FLASH_AREA_LEN;
  bytes_read = 0;
  crc = 0;

  while(bytes_read < metadata.length) {
    memset(&tmp_buf, 0, BUF_LEN);

    read_len = bytes_read + BUF_LEN <= metadata.length ?
               BUF_LEN: metadata.length - bytes_read;

    success = ext_flash_read(NULL, read_start_addr, read_len, tmp_buf);
    if(!success) {
      LOG_ERR("Failed to read from external flash at 0x%08lX\n",
              (unsigned long)read_start_addr);
      ext_flash_close(NULL);
      return false;
    }

    for(i = 0; i < read_len; i++) {
      crc = crc16_add(tmp_buf[i], crc);
    }

    read_start_addr += read_len;
    bytes_read += read_len;
  }

  LOG_INFO("Area %u: Read=0x%08lx, Len=0x%08lx, CRC=0x%04x, Calc CRC=0x%04x\n",
           area, (unsigned long)bytes_read, (unsigned long)metadata.length,
           metadata.crc, crc);

  ext_flash_close(NULL);

  return crc == metadata.crc;
}
/*---------------------------------------------------------------------------*/
/**
 * @}
 */

