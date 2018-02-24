/*
 * Copyright (c) 2017, <copyright holders>
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
 * \addtogroup radio
 * @{
 *
 * \defgroup radio-ble API for BLE Radios
 *
 * The radio API module defines a set of functions that a BLE radio device
 * driver must implement.
 *
 * @{
 *
 * \file
 *         Header file for the API for BLE Radios
 */
/*---------------------------------------------------------------------------*/
#ifndef RADIO_BLE_H_
#define RADIO_BLE_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio/radio.h"

#include <stddef.h>
/*---------------------------------------------------------------------------*/
/**
 * The structure of a device driver for BLE radio.
 */
typedef struct radio_ble_driver {
  /**
   * Docs needed
   */
  int (* init)(void);

  /**
   * Docs needed
   */
  int (* send)(const void *payload, unsigned short payload_len);

  /**
   * Docs needed
   */
  int (* on)(void);

  /**
   * Docs needed
   */
  int (* off)(void);

  /**
   * Docs needed
   */
  radio_result_t (* get_value)(radio_param_t param, radio_value_t *value);

  /**
   * Docs needed
   */
  radio_result_t (* set_value)(radio_param_t param, radio_value_t value);

  /**
   * Docs needed
   */
  radio_result_t (* get_object)(radio_param_t param, void *dest, size_t size);

  /**
   * Docs needed
   */
  radio_result_t (* set_object)(radio_param_t param, const void *src,
                                size_t size);
} radio_ble_driver_t;
/*---------------------------------------------------------------------------*/
#endif /* RADIO_BLE_H_ */
/*---------------------------------------------------------------------------*/
/** @} */
/** @} */
