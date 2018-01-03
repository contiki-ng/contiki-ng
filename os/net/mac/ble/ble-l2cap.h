/*
 * Copyright (c) 2017, Graz University of Technology
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
 *    MAC layer that implements BLE L2CAP credit-based flow control
 *    channels to support IPv6 over BLE (RFC 7668)
 *
 * \author
 *    Michael Spoerk <michael.spoerk@tugraz.at>
 */
/*---------------------------------------------------------------------------*/
#ifndef BLE_L2CAP_H_
#define BLE_L2CAP_H_

#include "contiki.h"
#include "net/mac/mac.h"
#include "dev/radio.h"
/*---------------------------------------------------------------------------*/
/* device name used for BLE advertisement */
#ifdef BLE_CONF_DEVICE_NAME
#define BLE_DEVICE_NAME   BLE_CONF_DEVICE_NAME
#else
#define BLE_DEVICE_NAME "BLE device name"
#endif

/* BLE advertisement in milliseconds */
#ifdef BLE_CONF_ADV_INTERVAL
#define BLE_ADV_INTERVAL  BLE_CONF_ADV_INTERVAL
#else
#define BLE_ADV_INTERVAL              50
#endif

#define BLE_SLAVE_CONN_INTERVAL_MIN  0x0150
#define BLE_SLAVE_CONN_INTERVAL_MAX  0x01F0
#define L2CAP_SIGNAL_CHANNEL         0x0005
#define L2CAP_FLOW_CHANNEL           0x0041
#define L2CAP_CODE_CONN_UPDATE_REQ   0x12
#define L2CAP_CODE_CONN_UPDATE_RSP   0x13
#define L2CAP_CODE_CONN_REQ          0x14
#define L2CAP_CODE_CONN_RSP          0x15
#define L2CAP_CODE_CREDIT            0x16
#define L2CAP_IPSP_PSM               0x0023

/* the maximum MTU size of the L2CAP channel */
#ifdef BLE_L2CAP_CONF_NODE_MTU
#define BLE_L2CAP_NODE_MTU      BLE_L2CAP_CONF_NODE_MTU
#else
#define BLE_L2CAP_NODE_MTU              1280
#endif

/* the max. supported L2CAP fragment length */
#ifdef BLE_L2CAP_CONF_NODE_FRAG_LEN
#define BLE_L2CAP_NODE_FRAG_LEN   BLE_L2CAP_CONF_NODE_FRAG_LEN
#else
#ifdef BLE_MODE_CONF_CONN_MAX_PACKET_SIZE
#define BLE_L2CAP_NODE_FRAG_LEN   BLE_MODE_CONF_CONN_MAX_PACKET_SIZE
#else
#define BLE_L2CAP_NODE_FRAG_LEN         256
#endif
#endif

#define L2CAP_CREDIT_NEW       (BLE_L2CAP_NODE_MTU / BLE_L2CAP_NODE_FRAG_LEN)
#define L2CAP_CREDIT_THRESHOLD    2

#define L2CAP_INIT_INTERVAL     (2 * CLOCK_SECOND)

/* BLE connection interval in milliseconds */
#ifdef BLE_CONF_CONNECTION_INTERVAL
#define CONNECTION_INTERVAL_MS    BLE_CONF_CONNECTION_INTERVAL
#else
#define CONNECTION_INTERVAL_MS    125
#endif

/* BLE slave latency */
#ifdef BLE_CONF_CONNECTION_SLAVE_LATENCY
#define CONNECTION_SLAVE_LATENCY  BLE_CONF_CONNECTION_SLAVE_LATENCY
#else
#define CONNECTION_SLAVE_LATENCY    0
#endif

/* BLE supervision timeout */
#define CONNECTION_TIMEOUT         42

#define L2CAP_FIRST_HEADER_SIZE         6
#define L2CAP_SUBSEQ_HEADER_SIZE        4

#if UIP_CONF_ROUTER
#ifdef BLE_MODE_CONF_MAX_CONNECTIONS
#define L2CAP_CHANNELS          BLE_MODE_CONF_MAX_CONNECTIONS
#else
#define L2CAP_CHANNELS          1
#endif
#else
#define L2CAP_CHANNELS          1
#endif

extern const struct mac_driver ble_l2cap_driver;

#endif /* BLE_L2CAP_H_ */
