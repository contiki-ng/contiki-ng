/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden
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

/**
 * \file
 *         CBOR-over-SLIP protocol for talking to the serialradio firmware
 *         (examples/serialradio) from the native border router.
 *
 *         Frames carried over SLIP have the layout:
 *
 *             [ CBOR map ] [ CRC16-LE (2 bytes) ]
 *
 *         The opcode constants below MUST be kept in sync with
 *         examples/serialradio/serial-radio.h.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#ifndef BORDER_ROUTER_CBOR_H_
#define BORDER_ROUTER_CBOR_H_

#include "contiki.h"
#include <stdbool.h>
#include <stdint.h>

/* Command opcodes (host -> radio) */
#define SRADIO_CMD_PING            0
#define SRADIO_CMD_GET_PARAM       1
#define SRADIO_CMD_SET_PARAM       2
#define SRADIO_CMD_RX_ON           5
#define SRADIO_CMD_RX_OFF          6
#define SRADIO_CMD_GET_ADDR64      20
#define SRADIO_CMD_ROUTER_MODE     21
#define SRADIO_CMD_TX_RAW_FRAME    50

/* Event opcodes (radio -> host) */
#define SRADIO_EVT_PARAM_RESPONSE  51
#define SRADIO_EVT_RX_FRAME        52
#define SRADIO_EVT_TX_RESPONSE     54
#define SRADIO_EVT_HEARTBEAT       55
#define SRADIO_EVT_ADDR64_RESPONSE 57
#define SRADIO_EVT_PONG            100
#define SRADIO_EVT_ERROR           255

/**
 * \brief Send a raw 802.15.4 frame to the radio for transmission.
 * \param msg_id Session id echoed back in the TX_RESPONSE event.
 * \param frame  The fully-framed 802.15.4 packet to transmit.
 * \param len    Length of \p frame in bytes.
 */
void br_cbor_send_tx_frame(uint8_t msg_id, const uint8_t *frame, uint16_t len);

/**
 * \brief Set a radio parameter on the serial radio (SET_PARAM).
 */
void br_cbor_send_set_param(uint8_t msg_id, uint16_t param, int32_t value);

/**
 * \brief Request the radio's EUI-64 link-layer address (GET_ADDR64).
 */
void br_cbor_send_get_addr64(uint8_t msg_id);

/**
 * \brief Enable or disable border-router radio mode (ROUTER_MODE).
 */
void br_cbor_send_router_mode(uint8_t msg_id, bool enable);

/**
 * \brief Parse and dispatch a complete CBOR frame (including trailing CRC16)
 *        received from the serial radio.
 */
void border_router_cbor_input(const uint8_t *data, int len);

#endif /* BORDER_ROUTER_CBOR_H_ */
