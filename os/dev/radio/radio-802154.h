/*
 * Copyright (c) 2005, Swedish Institute of Computer Science.
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
/*---------------------------------------------------------------------------*/
/**
 * \file
 *         Header file for the API for IEEE 802.15.4 Radios
 * \author
 *         Adam Dunkels <adam@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 *         Niclas Finne <nfi@sics.se>
 *         Nicolas Tsiftes <nvt@sics.se>
 */

/**
 * \addtogroup radio
 * @{
 */

/**
 * \defgroup radio-802154 API for IEEE 802.15.4 Radios
 *
 * The radio API module defines a set of functions that an IEEE 802.15.4 radio
 * device driver must implement.
 *
 * @{
 */
/*---------------------------------------------------------------------------*/
#ifndef RADIO_802154_H_
#define RADIO_802154_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"
#include "dev/radio/radio.h"

#include <stddef.h>
/*---------------------------------------------------------------------------*/
/**
 * The structure of a device driver for an IEEE 802.15.4 radio.
 */
typedef struct radio_802154_driver_s {

  int (* init)(void);

  /** Prepare the radio with a packet to be sent. */
  int (* prepare)(const void *payload, unsigned short payload_len);

  /** Send the packet that has previously been prepared. */
  int (* transmit)(unsigned short transmit_len);

  /** Prepare & transmit a packet. */
  int (* send)(const void *payload, unsigned short payload_len);

  /** Read a received packet into a buffer. */
  int (* read)(void *buf, unsigned short buf_len);

  /** Perform a Clear-Channel Assessment (CCA) to find out if there is
      a packet in the air or not. */
  int (* channel_clear)(void);

  /** Check if the radio driver is currently receiving a packet */
  int (* receiving_packet)(void);

  /** Check if the radio driver has just received a packet */
  int (* pending_packet)(void);

  /** Turn the radio on. */
  int (* on)(void);

  /** Turn the radio off. */
  int (* off)(void);

  /** Get a radio parameter value. */
  radio_result_t (* get_value)(radio_param_t param, radio_value_t *value);

  /** Set a radio parameter value. */
  radio_result_t (* set_value)(radio_param_t param, radio_value_t value);

  /**
   * Get a radio parameter object. The argument 'dest' must point to a
   * memory area of at least 'size' bytes, and this memory area will
   * contain the parameter object if the function succeeds.
   */
  radio_result_t (* get_object)(radio_param_t param, void *dest, size_t size);

  /**
   * Set a radio parameter object. The memory area referred to by the
   * argument 'src' will not be accessed after the function returns.
   */
  radio_result_t (* set_object)(radio_param_t param, const void *src,
                                size_t size);

} radio_802154_driver_t;

#endif /* RADIO_802154_H_ */

/** @} */
/** @} */
