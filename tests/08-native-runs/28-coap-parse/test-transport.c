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
 * This file is part of the Contiki-NG operating system.
 */

/*
 * A CoAP transport for the unit tests. It replaces coap-uip.c, sends
 * nothing, and keeps the last message that the CoAP engine sent so that
 * a test can parse it.
 */

#include "coap-engine.h"
#include "coap-transport.h"
#include "coap-endpoint.h"
#include "test-transport.h"
#include <string.h>

uint8_t test_transport_sent[COAP_MAX_PACKET_SIZE];
uint16_t test_transport_sent_len;

static uint8_t databuf[COAP_MAX_PACKET_SIZE];
/*---------------------------------------------------------------------------*/
void
coap_endpoint_copy(coap_endpoint_t *destination, const coap_endpoint_t *from)
{
  memcpy(destination, from, sizeof(coap_endpoint_t));
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_cmp(const coap_endpoint_t *e1, const coap_endpoint_t *e2)
{
  return memcmp(e1, e2, sizeof(coap_endpoint_t)) == 0;
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_log(const coap_endpoint_t *ep)
{
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_print(const coap_endpoint_t *ep)
{
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_snprint(char *str, size_t size, const coap_endpoint_t *ep)
{
  if(size > 0) {
    str[0] = '\0';
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_parse(const char *text, size_t size, coap_endpoint_t *ep)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_is_secure(const coap_endpoint_t *ep)
{
  return 0;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_is_connected(const coap_endpoint_t *ep)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
int
coap_endpoint_connect(coap_endpoint_t *ep)
{
  return 1;
}
/*---------------------------------------------------------------------------*/
void
coap_endpoint_disconnect(coap_endpoint_t *ep)
{
}
/*---------------------------------------------------------------------------*/
uint8_t *
coap_databuf(void)
{
  return databuf;
}
/*---------------------------------------------------------------------------*/
void
coap_transport_init(void)
{
}
/*---------------------------------------------------------------------------*/
int
coap_sendto(const coap_endpoint_t *ep, const uint8_t *data, uint16_t len)
{
  test_transport_sent_len = MIN(len, sizeof(test_transport_sent));
  memcpy(test_transport_sent, data, test_transport_sent_len);
  return len;
}
/*---------------------------------------------------------------------------*/
