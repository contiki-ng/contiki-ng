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
#ifndef BOOTLOADER_H_
#define BOOTLOADER_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
#include <stdbool.h>
/*---------------------------------------------------------------------------*/
/* Set to 1 to backup the golden image to external flash */
#ifdef BOOTLOADER_CONF_BACKUP_GOLDEN_IMAGE
#define BOOTLOADER_BACKUP_GOLDEN_IMAGE BOOTLOADER_CONF_BACKUP_GOLDEN_IMAGE
#else
#define BOOTLOADER_BACKUP_GOLDEN_IMAGE 0
#endif

/* Set to 1 to erase the entire external flash */
#ifdef BOOTLOADER_CONF_ERASE_EXT_FLASH
#define BOOTLOADER_ERASE_EXT_FLASH BOOTLOADER_CONF_ERASE_EXT_FLASH
#else
#define BOOTLOADER_ERASE_EXT_FLASH 0
#endif
/*---------------------------------------------------------------------------*/
void bootloader_arch_jump_to_app(void);
void bootloader_arch_init(void);
void bootloader_arch_install_image_from_area(uint8_t area);
bool bootloader_validate_internal_image(void);
/*---------------------------------------------------------------------------*/
#endif /* BOOTLOADER_H_ */
/*---------------------------------------------------------------------------*/
