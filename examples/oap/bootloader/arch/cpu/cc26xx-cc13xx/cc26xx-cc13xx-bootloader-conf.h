/*
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
#ifndef CC26XX_CC13XX_BOOTLOADER_CONF_H_
#define CC26XX_CC13XX_BOOTLOADER_CONF_H_
/*---------------------------------------------------------------------------*/
/* Limit the bootloader image to the early pages on flash. */
#define FLASH_FW_ORIGIN  0x00000000
#if BUILD_WITH_BOOTLOADER_DEBUG
#define FLASH_FW_LENGTH  0x00003000
#else
#define FLASH_FW_LENGTH  0x00002000
#endif

/* Total internal flash length */
#define INTERNAL_FLASH_LENGTH 0x00020000

/*
 * Length of the CC13xx/CC26xx CCFG area, high 88 bytes on flash
 * Even though incoming images will not overwrite CCFG, the current CCFG must
 * be protected and therefore the area is reserved
 */
#define CCFG_LENGTH           0x00000058

/*
 * Max area available for the main firmware on internal flash, including
 * metadata
 */
#define OTA_CONF_MAIN_FW_MAX_LEN (INTERNAL_FLASH_LENGTH - FLASH_FW_LENGTH - \
                                  CCFG_LENGTH)

/*
 * Start address of main firmware on internal flash
 * This is where we will start the validation of the internal image. This is
 * where we will start writing a new image
 */
#define OTA_CONF_MAIN_FW_BASE (FLASH_FW_ORIGIN + FLASH_FW_LENGTH)

/*
 * 12 bytes for metadata
 */
#define OTA_METADATA_LEN    0x0C

/*
 * Offset of OTA metadata from the start of any image (including one in
 * external flash)
 */
#define OTA_CONF_METADATA_OFFSET (OTA_CONF_MAIN_FW_MAX_LEN - OTA_METADATA_LEN)
/*---------------------------------------------------------------------------*/
#endif /* CC26XX_CC13XX_BOOTLOADER_CONF_H_ */
/*---------------------------------------------------------------------------*/
