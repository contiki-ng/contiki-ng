/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB.
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
/*---------------------------------------------------------------------------*/
/**
 * \file
 *         Project configuration for the lwm2m-nat64 example.
 * \author
 *         Joakim Eriksson <joakim.eriksson@ri.se>
 */
/*---------------------------------------------------------------------------*/
#ifndef PROJECT_CONF_H_
#define PROJECT_CONF_H_
/*---------------------------------------------------------------------------*/
/*
 * Register with the public Eclipse Leshan demo server through the IP64
 * border router's NAT64 translator.
 *
 * The address embeds Leshan's IPv4 address in the well-known NAT64 prefix
 * 64:ff9b::/96 (RFC 6052). It is a literal because the node has no DNS64
 * resolver configured, so re-derive it if Leshan moves:
 *
 *     dig +short leshan.eclipseprojects.io A
 *     printf '64:ff9b::%02x%02x:%02x%02x\n' <the four octets>
 *
 * 23.97.187.154 -> 64:ff9b::1761:bb9a   (checked 2026-08-30)
 */
#ifndef LWM2M_SERVER_ADDRESS
#define LWM2M_SERVER_ADDRESS "coap://[64:ff9b::1761:bb9a]"
#endif
/*---------------------------------------------------------------------------*/
/* Everything else follows the stock non-DTLS configuration. */
#include "project-conf-basic.h"
/*---------------------------------------------------------------------------*/
/*
 * Override the stock endpoint name, which is "Contiki-NG-Test". On the public
 * Leshan demo server that is near-certain to collide with someone else's
 * device, and two clients registering under one name fight over the
 * registration. Defined unconditionally upstream, hence the undef.
 */
#undef LWM2M_ENGINE_CLIENT_ENDPOINT_NAME
#define LWM2M_ENGINE_CLIENT_ENDPOINT_NAME "contiki-ng-nrf54l15-xiao"
/*---------------------------------------------------------------------------*/
#endif /* PROJECT_CONF_H_ */
