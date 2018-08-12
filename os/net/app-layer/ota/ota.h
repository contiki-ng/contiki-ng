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
 * \brief A data type representing firmware image metadata
 */
typedef struct ota_firmware_metadata_s {
  /**< Image length, not including metadata */
  uint32_t length;

  /**< Image unique identifier. Generation is implementation specific */
  uint32_t uuid;

  /**< Image verification code. */
  uint16_t crc;

  /**< Not quite sure why we need this */
  uint16_t crc_shadow;

  /**< Image version. Comparison uses signed arithmetic */
  uint16_t version;
} ota_firmware_metadata_t;
/*---------------------------------------------------------------------------*/
#endif /* OTA_H_ */
/*---------------------------------------------------------------------------*/
/**
 * @}
 * @}
 */
