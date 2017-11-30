/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 *         Configuration for Contiki library
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

#ifndef CONTIKI_H_
#define CONTIKI_H_

#include <stdint.h>

#include "posix-main.h"

#define COAP_LOG_CONF_PATH "coap-log-conf.h"

#define COAP_TIMER_CONF_DRIVER coap_timer_native_driver

#define LWM2M_ENGINE_CLIENT_ENDPOINT_NAME "lwm2m-ex"
#define LWM2M_DEVICE_MANUFACTURER "RISE SICS"
#define LWM2M_DEVICE_TYPE "lwm2m-example"
#define LWM2M_DEVICE_MODEL_NUMBER "000"
#define LWM2M_DEVICE_SERIAL_NO "1"
#define LWM2M_DEVICE_FIRMWARE_VERSION "0.1"

/* Use LWM2M as DTLS keystore */
#define COAP_DTLS_KEYSTORE_CONF_WITH_LWM2M 1

#ifdef COAP_TRANSPORT_CONF_H
#include COAP_TRANSPORT_CONF_H
#endif

#ifndef COAP_MAX_CHUNK_SIZE
#define COAP_MAX_CHUNK_SIZE           256
#endif /* COAP_MAX_CHUNK_SIZE */

#endif /* CONTIKI_H_ */
