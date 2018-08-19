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
 * \addtogroup apps
 * @{
 *
 * \defgroup ota Contiki-NG Over the Air firmware update engine
 * @{
 *
 * The Contiki-NG OTA engine allows users to update firmware on running
 * devices using a number of different application layers, including HTTP and
 * CoAP.
 *
 * Largely based on the excellent work of Mark Solters <msolters@gmail.com>
 *
 * http://marksolters.com/programming/2016/06/07/contiki-ota.html
 *
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *    Header file for the Contiki-NG OTA engine
 */
/*---------------------------------------------------------------------------*/
#ifndef OTA_H_
#define OTA_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdint.h>
/*---------------------------------------------------------------------------*/
/**
 * \name Properties of firmware on internal flash
 * @{
 */

/**
 * \brief Start address of main firmware on internal flash
 *
 * Must be provided by the platform
 */
#define OTA_MAIN_FW_BASE OTA_CONF_MAIN_FW_BASE
#if !OTA_MAIN_FW_BASE
#error "OTA_CONF_MAIN_FW_BASE must be defined by the platform"
#endif

/**
 * \brief Maximum space on internal flash available for firmware
 *
 * Must be provided by the platform.
 */
#define OTA_MAIN_FW_MAX_LEN OTA_CONF_MAIN_FW_MAX_LEN
#if !OTA_MAIN_FW_MAX_LEN
#error "OTA_MAIN_FW_MAX_LEN must be defined by the platform"
#endif
/** @} */
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
 * \name Metadata properties
 * @{
 */
/**
 * \brief Offset of OTA metadata from the start of any image
 *
 * Must be provided by the platform. It is assumed that the metadata will be
 * at the same offset on external flash as it will be in the current image in
 * internal flash
 */
#define OTA_METADATA_OFFSET OTA_CONF_METADATA_OFFSET
#if !OTA_METADATA_OFFSET
#error "OTA_CONF_METADATA_OFFSET must be defined by the platform"
#endif

/**
 * \brief Absolute metadata location on internal flash
 */
#define OTA_METADATA_BASE (OTA_MAIN_FW_BASE + OTA_METADATA_OFFSET)
/** @} */
/*---------------------------------------------------------------------------*/
/**
 * \brief A data type representing firmware image metadata
 */
typedef struct ota_firmware_metadata_s {
  /**< Image length, not including metadata */
  uint32_t length;

  /**< Image unique identifier. Generation is implementation specific */
  uint32_t uuid;

  /**< Image verification code. */
  uint16_t crc;

  /**< Image version. Comparison uses signed arithmetic */
  uint16_t version;
} ota_firmware_metadata_t;
/*---------------------------------------------------------------------------*/
/**
 * \brief Used to test an image for validity
 */
#define OTA_IMAGE_INVALID_LEN  0xFFFFFFFF
/*---------------------------------------------------------------------------*/
#endif /* OTA_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
