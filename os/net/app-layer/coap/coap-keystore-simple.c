/*
 * Copyright (c) 2017, RISE SICS AB.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
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
 *         A simple keystore with fixed credentials.
 * \author
 *         Niclas Finne <nfi@sics.se>
 *         Joakim Eriksson <joakime@sics.se>
 */

/**
 * \addtogroup coap-keystore
 * @{
 */

#include "coap-endpoint.h"
#include "coap-keystore.h"
#include <string.h>

#ifdef WITH_DTLS
#ifdef COAP_DTLS_PSK_DEFAULT_IDENTITY
#ifdef COAP_DTLS_PSK_DEFAULT_KEY
/*---------------------------------------------------------------------------*/
static int
get_default_psk_info(const coap_endpoint_t *address_info,
                     coap_keystore_psk_entry_t *info)
{
  if(info == NULL) {
    return 0;
  }

  /* Return the default identify if no identity is provided. */
  if(info->identity == NULL || info->identity_len == 0) {
    /* Identity requested */
    info->identity = (uint8_t *)COAP_DTLS_PSK_DEFAULT_IDENTITY;
    info->identity_len = strlen(COAP_DTLS_PSK_DEFAULT_IDENTITY);
    return 1;
  }

  /* We support only the default identity when querying for a key. */
  if(info->identity_len != strlen(COAP_DTLS_PSK_DEFAULT_IDENTITY) ||
     memcmp(info->identity, COAP_DTLS_PSK_DEFAULT_IDENTITY,
            info->identity_len) != 0) {
    /* Identity not matching */
    return 0;
  }

  /* The identity matches the default identity -- fill in the key. */
  info->key = (uint8_t *)COAP_DTLS_PSK_DEFAULT_KEY;
  info->key_len = strlen(COAP_DTLS_PSK_DEFAULT_KEY);
  return 1;
}
/*---------------------------------------------------------------------------*/
static const coap_keystore_t simple_key_store = {
  .coap_get_psk_info = get_default_psk_info
};
/*---------------------------------------------------------------------------*/
#endif /* COAP_DTLS_PSK_DEFAULT_KEY */
#endif /* COAP_DTLS_PSK_DEFAULT_IDENTITY */

#ifdef COAP_DTLS_TEST_CA_CERT
#ifdef COAP_DTLS_TEST_OWN_CERT
#ifdef COAP_DTLS_TEST_PRIV_KEY
/*---------------------------------------------------------------------------*/
static int
get_default_cert_info(const coap_endpoint_t *address_info,
                     coap_keystore_cert_entry_t *info)
{
  if(info == NULL) {
    return 0;
  }

  info->ca_cert = (uint8_t *)COAP_DTLS_TEST_CA_CERT;
  info->ca_cert_len = sizeof(COAP_DTLS_TEST_CA_CERT);

  info->own_cert = (uint8_t *)COAP_DTLS_TEST_OWN_CERT;
  info->own_cert_len = sizeof(COAP_DTLS_TEST_OWN_CERT);

  info->priv_key = (uint8_t *)COAP_DTLS_TEST_PRIV_KEY;
  info->priv_key_len = sizeof(COAP_DTLS_TEST_PRIV_KEY);
  return 1;
}
/*---------------------------------------------------------------------------*/
static const coap_keystore_t simple_key_store = {
  .coap_get_cert_info = get_default_cert_info
};
/*---------------------------------------------------------------------------*/
#endif /* COAP_DTLS_TEST_CA_CERT */
#endif /* COAP_DTLS_TEST_OWN_CERT */
#endif /* COAP_DTLS_TEST_PRIV_KEY */
#endif /* WITH_DTLS */
/*---------------------------------------------------------------------------*/
void
coap_keystore_simple_init(void)
{
#ifdef WITH_DTLS
#if (defined(COAP_DTLS_PSK_DEFAULT_IDENTITY) \
    && defined(COAP_DTLS_PSK_DEFAULT_KEY)) \
  || (defined(COAP_DTLS_TEST_CA_CERT) \
  && defined(COAP_DTLS_TEST_OWN_CERT) \
  && defined(COAP_DTLS_TEST_PRIV_KEY))

  coap_set_keystore(&simple_key_store);
#endif /* (defined(COAP_DTLS_PSK_DEFAULT_IDENTITY) \
    && defined(COAP_DTLS_PSK_DEFAULT_KEY)) \
  || (defined(COAP_DTLS_TEST_CA_CERT) \
  && defined(COAP_DTLS_TEST_OWN_CERT) \
  && defined(COAP_DTLS_TEST_PRIV_KEY)) */
#endif /* WITH_DTLS */
}
/*---------------------------------------------------------------------------*/
/** @} */
