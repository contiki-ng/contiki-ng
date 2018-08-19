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
 *    Header file for the Contiki-NG OTA engine external flash manipulation
 */
/*---------------------------------------------------------------------------*/
#ifndef OTA_EXT_FLASH_H_
#define OTA_EXT_FLASH_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/**
 * \name External flash map
 *
 * The external flash contains a number of image areas. "Areas" are logical
 * partitions used by the Contiki-NG OAP engine and are not to be confused
 * with sectors or pages of the actual flash part.
 *
 * Area 0 is reserved for a golden image.
 *
 * Each stored firmware will be stored at offset 0 from the start of an area
 * @{
 */

/**
 * \brief Area on flash reserved for a golden image. Not configurable
 */
#define OTA_EXT_FLASH_GOLDEN_AREA                    0

/**
 * \brief Number of areas on external flash.
 *
 * Must be provided by the platform
 */
#define OTA_EXT_FLASH_AREA_COUNT OTA_CONF_EXT_FLASH_AREA_COUNT
#if !OTA_EXT_FLASH_AREA_COUNT
#error "OTA_CONF_EXT_FLASH_AREA_COUNT must be defined by the platform"
#endif

/**
 * \brief Length of each area on external flash.
 *
 * Must be provided by the platform and must be an exact multiple of the
 * part's erasable block size.
 */
#define OTA_EXT_FLASH_AREA_LEN OTA_CONF_EXT_FLASH_AREA_LEN
#if !OTA_EXT_FLASH_AREA_LEN
#error "OTA_CONF_EXT_FLASH_AREA_LEN must be defined by the platform"
#endif
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \brief Erase an image area in external flash
 * \param area The area to be erased
 *
 * \e area must be less than OTA_EXT_FLASH_AREA_COUNT
 *
 * \note This function will attempt to erase starting from an offset of
 * area * OTA_EXT_FLASH_AREA_LEN. It is the platform developer's responsibility
 * to map the external flash such that this offset is aligned with a sector
 * erase boundary.
 */
void ext_flash_area_erase(uint8_t area);
/*---------------------------------------------------------------------------*/
#endif /* OTA_EXT_FLASH_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 */
