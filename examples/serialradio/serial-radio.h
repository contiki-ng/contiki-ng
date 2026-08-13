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
 *         Serial Radio Control Interface - Header
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */

#ifndef SERIAL_RADIO_H_
#define SERIAL_RADIO_H_

#include "contiki.h"
#include <stdint.h>
#include <stdbool.h>

/*---------------------------------------------------------------------------*/
/* Configuration */
/*---------------------------------------------------------------------------*/

#ifndef SERIAL_RADIO_CONF_BUF_SIZE
#define SERIAL_RADIO_CONF_BUF_SIZE 256
#endif

#ifndef SERIAL_RADIO_CONF_MAX_FRAME_SIZE
#define SERIAL_RADIO_CONF_MAX_FRAME_SIZE 127
#endif

#ifndef SERIAL_RADIO_CONF_HEARTBEAT_INTERVAL
#define SERIAL_RADIO_CONF_HEARTBEAT_INTERVAL (10 * CLOCK_SECOND)
#endif

/*---------------------------------------------------------------------------*/
/* Command opcodes (PC -> Node) */
/*---------------------------------------------------------------------------*/

#define SRADIO_CMD_PING              0
#define SRADIO_CMD_GET_PARAM         1
#define SRADIO_CMD_SET_PARAM         2
#define SRADIO_CMD_RSSI_SCAN_START   3
#define SRADIO_CMD_RSSI_SCAN_STOP    4
#define SRADIO_CMD_TX_RAW_FRAME      50
#define SRADIO_CMD_RX_ON             5
#define SRADIO_CMD_RX_OFF            6
#define SRADIO_CMD_FAST_SCAN_START   7
#define SRADIO_CMD_FAST_SCAN_STOP    8
#define SRADIO_CMD_JAM_START         9
#define SRADIO_CMD_JAM_STOP          10
#define SRADIO_CMD_GET_ADDR64        20  /* Report node link-layer (EUI-64) address */
#define SRADIO_CMD_ROUTER_MODE       21  /* Act as a border-router radio (filter+ACK) */

/*---------------------------------------------------------------------------*/
/* Event opcodes (Node -> PC) */
/*---------------------------------------------------------------------------*/

#define SRADIO_EVT_PONG              100
#define SRADIO_EVT_PARAM_RESPONSE    51
#define SRADIO_EVT_RX_FRAME          52
#define SRADIO_EVT_RSSI_SCAN_RESULT  53
#define SRADIO_EVT_TX_RESPONSE       54
#define SRADIO_EVT_HEARTBEAT         55
#define SRADIO_EVT_FAST_SCAN_RESULT  56
#define SRADIO_EVT_ADDR64_RESPONSE   57  /* Reply to GET_ADDR64: 'f' = 8-byte EUI-64 */
#define SRADIO_EVT_ERROR             255

/*---------------------------------------------------------------------------*/
/* CBOR Map Keys (single character for compactness) */
/*---------------------------------------------------------------------------*/

#define KEY_TYPE       't'   /* Message type/opcode */
#define KEY_ID         'i'   /* Message ID for request/response matching */
#define KEY_PARAM      'p'   /* Radio parameter code */
#define KEY_VALUE      'v'   /* Parameter value */
#define KEY_FRAME      'f'   /* Raw radio frame data */
#define KEY_RSSI       'r'   /* RSSI value */
#define KEY_LQI        'l'   /* Link quality indicator */
#define KEY_CHANNEL    'c'   /* Channel number */
#define KEY_START_CH   's'   /* Scan start channel */
#define KEY_END_CH     'e'   /* Scan end channel */
#define KEY_DWELL      'd'   /* Scan dwell time (ms) */
#define KEY_ERROR      'x'   /* Error code */
#define KEY_VERSION    'V'   /* Version string */
#define KEY_RSSI_ARRAY 'R'   /* Array of RSSI values for fast scan */
#define KEY_SEQ        'n'   /* Sequence number */

/*---------------------------------------------------------------------------*/
/* Error codes */
/*---------------------------------------------------------------------------*/

#define ERR_NONE              0
#define ERR_INVALID_CMD       1
#define ERR_INVALID_PARAM     2
#define ERR_CRC_FAIL          3
#define ERR_CBOR_DECODE       4
#define ERR_RADIO_ERROR       5
#define ERR_BUFFER_OVERFLOW   6
#define ERR_SCAN_ACTIVE       7

/*---------------------------------------------------------------------------*/
/* Process declaration */
/*---------------------------------------------------------------------------*/

PROCESS_NAME(serial_radio_process);

/*---------------------------------------------------------------------------*/
/* Public API */
/*---------------------------------------------------------------------------*/

/**
 * \brief Initialize the serial radio interface
 */
void serial_radio_init(void);

/**
 * \brief Send a raw frame over the radio
 * \param data Pointer to frame data
 * \param len Length of frame data
 * \param channel Channel to send on (-1 for current channel)
 * \return 0 on success, error code on failure
 */
int serial_radio_send_frame(const uint8_t *data, uint16_t len, int channel);

/**
 * \brief Start RSSI scanning
 * \param start_ch Start channel
 * \param end_ch End channel
 * \param dwell_ms Dwell time per channel in milliseconds
 * \return 0 on success, error code on failure
 */
int serial_radio_start_scan(uint8_t start_ch, uint8_t end_ch, uint16_t dwell_ms);

/**
 * \brief Stop RSSI scanning
 */
void serial_radio_stop_scan(void);

/**
 * \brief Check if scanning is active
 * \return true if scanning, false otherwise
 */
bool serial_radio_is_scanning(void);

#endif /* SERIAL_RADIO_H_ */
