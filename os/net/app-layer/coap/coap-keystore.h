/*
 * Copyright (c) 2017, RISE SICS
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
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */

/**
 * \file
 *         API for CoAP keystore
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

/**
 * \addtogroup coap
 * @{
 *
 * \defgroup coap-keystore CoAP keystore API
 * @{
 *
 * The CoAP keystore API defines a common interface for retrieving
 * authorization information for CoAP/DTLS.
 */

#ifndef COAP_KEYSTORE_H_
#define COAP_KEYSTORE_H_

#include "coap-endpoint.h"

/**
 * The structure of a CoAP pre-shared key info.
 */
typedef struct {
  const uint8_t *identity_hint;
  uint16_t identity_hint_len;
  const uint8_t *identity;
  uint16_t identity_len;
  const uint8_t *key;
  uint16_t key_len;
} coap_keystore_psk_entry_t;

/**
 * The structure of a CoAP keystore.
 *
 * The keystore implementation provides a function callback for each type of
 * authorization supported. The API currently only specifies a function
 * callback for pre-shared keys.
 */
typedef struct {
  int (* coap_get_psk_info)(const coap_endpoint_t *address_info,
                            coap_keystore_psk_entry_t *info);
} coap_keystore_t;

/**
 * \brief           Set the CoAP keystore to use by CoAP.
 * \param keystore  A pointer to a CoAP keystore.
 */
void coap_set_keystore(const coap_keystore_t *keystore);

#endif /* COAP_KEYSTORE_H_ */
/** @} */
/** @} */
