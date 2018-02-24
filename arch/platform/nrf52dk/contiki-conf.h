/*
 * Copyright (c) 2015, Nordic Semiconductor
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
 *
 */
/**
 * \addtogroup nrf52dk
 * @{
 *
 * \addtogroup nrf52dk-contikic-conf Contiki configuration
 * @{
 *
 * \file
 *  Contiki configuration for the nRF52 DK
 */
#ifndef CONTIKI_CONF_H
#define CONTIKI_CONF_H

#include <stdint.h>
#include <inttypes.h>
/*---------------------------------------------------------------------------*/
/* Include Project Specific conf */
#ifdef PROJECT_CONF_PATH
#include PROJECT_CONF_PATH
#endif /* PROJECT_CONF_PATH */
/*---------------------------------------------------------------------------*/
/* Include platform peripherals configuration */
#include "nrf52dk-def.h"
#include "nrf52832-def.h"
/*---------------------------------------------------------------------------*/
/**
 * \name Network Stack Configuration
 *
 * @{
 */

/* Select the BLE mac driver */
#if MAC_CONF_WITH_OTHER
#define NETSTACK_CONF_MAC     ble_ipsp_mac_driver
#endif

/* 6LoWPAN */
#define SICSLOWPAN_CONF_MAC_MAX_PAYLOAD         1280

#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG                    0     /**< We don't use 6LoWPAN fragmentation as IPSP takes care of that for us.*/
#endif

#define SICSLOWPAN_FRAMER_HDRLEN                0     /**< Use fixed header len rather than framer.length() function */

/* Packet buffer */
#define PACKETBUF_CONF_SIZE                     1280  /**< Required IPv6 MTU size */
/** @} */

/**
 * \name BLE configuration
 * @{
 */
#ifndef DEVICE_NAME
#define DEVICE_NAME "Contiki nRF52dk"  /**< Device name used in BLE undirected advertisement. */
#endif
/**
 * @}
 */

/**
 * \name IPv6 network buffer configuration
 *
 * @{
 */

#if NETSTACK_CONF_WITH_IPV6
/*---------------------------------------------------------------------------*/

/* ND and Routing */
#define UIP_CONF_ROUTER                      0 /**< BLE master role, which allows for routing, isn't supported. */
#define UIP_CONF_ND6_SEND_NS                 1

#endif /* NETSTACK_CONF_WITH_IPV6 */

/** @} */
#endif /* CONTIKI_CONF_H */
/**
 * @}
 * @}
 */
