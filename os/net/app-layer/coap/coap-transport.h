/*
 * Copyright (c) 2016-2018, SICS, Swedish ICT AB.
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

/**
 * \file
 *         API for CoAP transport
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

/**
 * \addtogroup coap
 * @{
 *
 * \defgroup coap-transport CoAP transport API
 * @{
 *
 * The CoAP transport API defines a common interface for sending/receiving
 * CoAP messages.
 */

#ifndef COAP_TRANSPORT_H_
#define COAP_TRANSPORT_H_

#include "coap-endpoint.h"

/**
 * \brief      Returns a common data buffer that can be used when
 *             generating CoAP messages for transmission. The buffer
 *             size is at least COAP_MAX_PACKET_SIZE bytes.
 *
 *             In Contiki-NG, this corresponds to the uIP buffer.
 *
 * \return     A pointer to a data buffer where a CoAP message can be stored.
 */
uint8_t *coap_databuf(void);

/**
 * \brief      Send a message to the specified CoAP endpoint
 * \param ep   A pointer to a CoAP endpoint
 * \param data A pointer to data to send
 * \param len  The size of the data to send
 * \return     The number of bytes sent or negative if an error occurred.
 */
int coap_sendto(const coap_endpoint_t *ep, const uint8_t *data, uint16_t len);

/**
 * \brief      Initialize the CoAP transport.
 *
 *             This function initializes the CoAP transport implementation and
 *             should only be called by the CoAP engine.
 */
void coap_transport_init(void);

#endif /* COAP_TRANSPORT_H_ */
/** @} */
/** @} */
