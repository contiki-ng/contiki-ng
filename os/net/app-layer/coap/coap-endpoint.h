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
 *         API to address CoAP endpoints
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

/**
 * \addtogroup coap-transport
 * @{
 */

#ifndef COAP_ENDPOINT_H_
#define COAP_ENDPOINT_H_

#include "contiki.h"
#include <stdlib.h>

#ifndef COAP_ENDPOINT_CUSTOM
#include "net/ipv6/uip.h"

typedef struct {
  uip_ipaddr_t ipaddr;
  uint16_t port;
  uint8_t secure;
} coap_endpoint_t;
#endif /* COAP_ENDPOINT_CUSTOM */

/**
 * \brief      Copy a CoAP endpoint from one memory area to another.
 *
 * \param dest A pointer to a CoAP endpoint to copy to.
 * \param src  A pointer to a CoAP endpoint to copy from.
 */
void coap_endpoint_copy(coap_endpoint_t *dest, const coap_endpoint_t *src);

/**
 * \brief      Compare two CoAP endpoints.
 *
 * \param e1   A pointer to the first CoAP endpoint.
 * \param e2   A pointer to the second CoAP endpoint.
 * \return     Non-zero if the endpoints are identical and zero otherwise.
 */
int coap_endpoint_cmp(const coap_endpoint_t *e1, const coap_endpoint_t *e2);

/**
 * \brief      Print a CoAP endpoint via the logging module.
 *
 * \param ep   A pointer to the CoAP endpoint to log.
 */
void coap_endpoint_log(const coap_endpoint_t *ep);

/**
 * \brief      Print a CoAP endpoint.
 *
 * \param ep   A pointer to the CoAP endpoint to print.
 */
void coap_endpoint_print(const coap_endpoint_t *ep);

/**
 * \brief      Print a CoAP endpoint to a string. The output is always
 *             null-terminated unless size is zero.
 *
 * \param str  The string to write to.
 * \param size The max number of characters to write.
 * \param ep   A pointer to the CoAP endpoint to print.
 * \return     Returns the number of characters needed for the output
 *             excluding the ending null-terminator or negative if an
 *             error occurred.
 */
int  coap_endpoint_snprint(char *str, size_t size,
                           const coap_endpoint_t *ep);

/**
 * \brief      Parse a CoAP endpoint.
 *
 * \param text The string to parse.
 * \param size The max number of characters in the string.
 * \param ep   A pointer to the CoAP endpoint to write to.
 * \return     Returns non-zero if the endpoint was successfully parsed and
 *             zero otherwise.
 */
int coap_endpoint_parse(const char *text, size_t size, coap_endpoint_t *ep);

/**
 * \brief      Check if a CoAP endpoint is secure (encrypted).
 *
 * \param ep   A pointer to a CoAP endpoint.
 * \return     Returns non-zero if the endpoint is secure and zero otherwise.
 */
int coap_endpoint_is_secure(const coap_endpoint_t *ep);

/**
 * \brief      Check if a CoAP endpoint is connected.
 *
 * \param ep   A pointer to a CoAP endpoint.
 * \return     Returns non-zero if the endpoint is connected and zero otherwise.
 */
int coap_endpoint_is_connected(const coap_endpoint_t *ep);

/**
 * \brief      Request a connection to a CoAP endpoint.
 *
 * \param ep   A pointer to a CoAP endpoint.
 * \return     Returns zero if an error occured and non-zero otherwise.
 */
int coap_endpoint_connect(coap_endpoint_t *ep);

/**
 * \brief      Request that any connection to a CoAP endpoint is discontinued.
 *
 * \param ep   A pointer to a CoAP endpoint.
 */
void coap_endpoint_disconnect(coap_endpoint_t *ep);

#endif /* COAP_ENDPOINT_H_ */
/** @} */
