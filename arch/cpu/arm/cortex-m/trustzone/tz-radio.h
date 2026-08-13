/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

/*
 * \file
 *   TrustZone radio API for secure radio access from the normal world.
 * \author
 *   Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef TZ_RADIO_H_
#define TZ_RADIO_H_

#include "dev/radio.h"
#include "trustzone/tz-api.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * \brief      Initialize the radio via the secure world.
 * \return     1 on success, 0 on failure.
 */
int tz_radio_init(void);

/**
 * \brief      Prepare a frame for transmission.
 * \param payload     Pointer to the frame data.
 * \param payload_len Length of the frame data.
 * \return     0 on success, non-zero on failure.
 */
int tz_radio_prepare(const void *payload, unsigned short payload_len);

/**
 * \brief      Transmit the previously prepared frame.
 * \param transmit_len Length of the frame to transmit.
 * \return     RADIO_TX_OK on success, or a RADIO_TX_* error code.
 */
int tz_radio_transmit(unsigned short transmit_len);

/**
 * \brief      Prepare and transmit a frame in one operation.
 * \param payload     Pointer to the frame data.
 * \param payload_len Length of the frame data.
 * \return     RADIO_TX_OK on success, or a RADIO_TX_* error code.
 */
int tz_radio_send(const void *payload, unsigned short payload_len);

/**
 * \brief      Read a received frame from the secure world.
 * \param buf     Buffer to copy the frame into.
 * \param buf_len Maximum number of bytes to read.
 * \return     Number of bytes read, or 0 if no frame available.
 */
int tz_radio_read(void *buf, unsigned short buf_len);

/**
 * \brief      Perform Clear Channel Assessment.
 * \return     1 if channel is clear, 0 otherwise.
 */
int tz_radio_channel_clear(void);

/**
 * \brief      Check whether the radio is currently receiving a frame.
 * \return     1 if receiving, 0 otherwise.
 */
int tz_radio_receiving_packet(void);

/**
 * \brief      Check whether a received frame is pending.
 * \return     1 if a frame is pending, 0 otherwise.
 */
int tz_radio_pending_packet(void);

/**
 * \brief      Turn the radio on.
 * \return     1 on success, 0 on failure.
 */
int tz_radio_on(void);

/**
 * \brief      Turn the radio off.
 * \return     1 on success, 0 on failure.
 */
int tz_radio_off(void);

/**
 * \brief      Get a radio parameter value.
 * \param param The parameter to get.
 * \param value Pointer to store the value.
 * \return     RADIO_RESULT_OK on success, or a radio_result_t error.
 */
radio_result_t tz_radio_get_value(radio_param_t param, radio_value_t *value);

/**
 * \brief      Set a radio parameter value.
 * \param param The parameter to set.
 * \param value The value to set.
 * \return     RADIO_RESULT_OK on success, or a radio_result_t error.
 */
radio_result_t tz_radio_set_value(radio_param_t param, radio_value_t value);

/**
 * \brief      Get a radio parameter object.
 * \param param The parameter to get.
 * \param dest  Buffer to store the object.
 * \param size  Size of the buffer.
 * \return     RADIO_RESULT_OK on success, or a radio_result_t error.
 */
radio_result_t tz_radio_get_object(radio_param_t param,
                                   void *dest, size_t size);

/**
 * \brief      Set a radio parameter object.
 * \param param The parameter to set.
 * \param src   Pointer to the object data.
 * \param size  Size of the object data.
 * \return     RADIO_RESULT_OK on success, or a radio_result_t error.
 */
radio_result_t tz_radio_set_object(radio_param_t param,
                                   const void *src, size_t size);

/**
 * \brief      Get RSSI and LQI for the last received frame.
 * \param rssi Pointer to store the RSSI value.
 * \param lqi  Pointer to store the LQI value.
 * \return     true on success, false on failure.
 */
bool tz_radio_get_rx_attributes(int8_t *rssi, uint8_t *lqi);

/**
 * Callback type for requesting a poll from the normal world
 * when the secure world has received a radio frame.
 */
typedef bool (*tz_radio_ns_rx_callback_t)(void)
#ifdef TRUSTZONE_SECURE
  CC_TRUSTZONE_NONSECURE_CALL
#endif
;

/**
 * \brief      Register a normal-world callback for RX notification.
 * \param callback Function pointer in the normal world to call
 *                 when a frame is received.
 * \return     true on success, false on failure.
 */
bool tz_radio_register_rx_callback(tz_radio_ns_rx_callback_t callback);

/**
 * \brief      Read the FICR device ID from the secure world.
 *
 *             The nRF5340 FICR is only accessible from the secure
 *             world. The normal world calls this NSC function to
 *             obtain the unique device address for the link layer.
 *
 * \param id0  Pointer to store FICR DEVICEID[0].
 * \param id1  Pointer to store FICR DEVICEID[1].
 * \return     true on success, false on failure.
 */
bool tz_radio_get_device_id(uint32_t *id0, uint32_t *id1);

#ifdef TRUSTZONE_SECURE

/**
 * \brief      Notify the normal world that a frame has been received.
 *
 *             Called from the secure world's ipc_radio_process when
 *             a frame is pulled from shared memory.
 *
 * \param rssi RSSI of the received frame.
 * \param lqi  LQI of the received frame.
 */
void tz_radio_notify_rx(int8_t rssi, uint8_t lqi);

#endif /* TRUSTZONE_SECURE */

/**
 * The normal-world radio driver proxy.
 */
extern const struct radio_driver tz_radio_driver;

#endif /* TZ_RADIO_H_ */
