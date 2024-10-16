/*
 * Copyright (c) 2022, RISE Research Institutes of Sweden AB
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
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

#ifndef DTLS_SUPPORT_CONFIG_H
#define DTLS_SUPPORT_CONFIG_H

#include "uip.h"

/*
 * Macro to control debug level of Mbed TLS lib. Two pre-requisites are needed:
 * Note -- 1. Debug prints of Mbed TLS are printed as DTLS logs at level
 *            of LOG_LEVEL_DBG. Hence, LOG_CONF_LEVEL_DTLS must be set to
 *            LOG_LEVEL_DBG.
 *         2. Mbed TLS debugs prints are compiled out to save memory and
 *            should be enabled in mbedtls-config.h. */
#ifdef COAP_MBEDTLS_LIB_CONF_DEBUG_LEVEL
#define COAP_MBEDTLS_LIB_DEBUG_LEVEL COAP_MBEDTLS_LIB_CONF_DEBUG_LEVEL
#else
#define COAP_MBEDTLS_LIB_DEBUG_LEVEL 0 /* Value between 0 to 5 */
#endif

/* Determines whether an insecure PRNG should be used for testing on
   platforms that have not yet implemented a proper CSPRNG. */
#ifdef COAP_DTLS_CONF_PRNG_INSECURE
#define COAP_DTLS_PRNG_INSECURE COAP_DTLS_CONF_PRNG_INSECURE
#else
#define COAP_DTLS_PRNG_INSECURE 0
#endif

/* Macro to control number of DTLS sessions. Default is limited to 1
   to save memory. */
#ifdef COAP_DTLS_CONF_MAX_SESSIONS
#define COAP_DTLS_MAX_SESSIONS COAP_DTLS_CONF_MAX_SESSIONS
#else
#define COAP_DTLS_MAX_SESSIONS 1
#endif /* COAP_DTLS_CONF_MAX_SESSIONS */

/* Macro to control the min and max re-transmission timeout values. */
#ifdef COAP_MBEDTLS_CONF_HANDSHAKE_MIN_TIMEOUT_MS
#define COAP_MBEDTLS_HANDSHAKE_MIN_TIMEOUT_MS COAP_MBEDTLS_CONF_HANDSHAKE_MIN_TIMEOUT_MS
#else
/* Set according to RFC 7925. */
#define COAP_MBEDTLS_HANDSHAKE_MIN_TIMEOUT_MS 9000
#endif /* COAP_MBEDTLS_CONF_HANDSHAKE_MIN_TIMEOUT_MS */

#ifdef COAP_MBEDTLS_CONF_HANDSHAKE_MAX_TIMEOUT_MS
#define COAP_MBEDTLS_HANDSHAKE_MAX_TIMEOUT_MS COAP_MBEDTLS_CONF_HANDSHAKE_MAX_TIMEOUT_MS
#else
#define COAP_MBEDTLS_HANDSHAKE_MAX_TIMEOUT_MS 60000
#endif /* COAP_MBEDTLS_CONF_HANDSHAKE_MAX_TIMEOUT_MS */

/*
 * Macro to enable the MFL extension (RFC 6066).
 *
 * Fragmentation length can be None (0), 512 (1), 1024 (2), 2048 (3)
 * Check mbedtls_ssl_conf_max_frag_len() for more info. */
#ifdef COAP_MBEDTLS_CONF_MAX_FRAG_LEN
#define COAP_MBEDTLS_MAX_FRAG_LEN COAP_MBEDTLS_CONF_MAX_FRAG_LEN
#else
#define COAP_MBEDTLS_MAX_FRAG_LEN 0
#endif

/*
 * Macro to control the interval in-between sending of consecutive messages.
 *
 * Mbed TLS may produce messages at a much faster rate than the
 * underlying network stack or the DTLS peer can handle. In such a
 * case, it is useful to provide an interval of time to wait
 * in-between of sending consecutive messages.
 *
 * This is the case when DTLS fragmentation is enabled and long HS
 * messages (~1000 bytes) are fragmented. We may want to wait until a
 * fragment is processed before pushing the next one onto the queue
 * buffer.
 *
 * A default value of 2s is set after experimenting with the nRF52840.
 */
#ifdef COAP_MBEDTLS_CONF_FRAGMENT_TIMER
#define COAP_MBEDTLS_FRAGMENT_TIMER COAP_MBEDTLS_CONF_FRAGMENT_TIMER
#else
#define COAP_MBEDTLS_FRAGMENT_TIMER 2000 /* Time in ms */
#endif /* COAP_MBEDTLS_CONF_FRAGMENT_TIMER */

/*
 * Macro to control the MTU size of Mbed DTLS. Mbed TLS will fragment
 * its messages accordingly. This is different from MFL. MFL can be
 * communicated in the Client Hello message to the peer to limit its
 * outgoing message size.
 *
 * UIP_CONF_BUFFER_SIZE must be > than App. Data + DTLS header + UDP
 * header (8) + IPv6 Header (40) + IPv6 Fragment header (8). The DTLS
 * header size can be queried with mbedtls_ssl_get_record_expansion().
 */
#ifdef COAP_MBEDTLS_CONF_MTU
#define COAP_MBEDTLS_MTU COAP_MBEDTLS_CONF_MTU
#else
#define COAP_MBEDTLS_MTU (UIP_CONF_BUFFER_SIZE - UIP_IPUDPH_LEN - UIP_FRAGH_LEN)
#endif /* COAP_MBEDTLS_CONF_FRAGMENT_TIMER */

/*
 * Macro to enable use of all supported ciphersuites. When enabled,
 * the Client Hello message will contain a list of all possible
 * ciphersuites and the strongest one will be chosen.
 *
 * Depending on the security mode, either
 * MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8 or
 * MBEDTLS_TLS_PSK_WITH_AES_128_CCM_8 will be used by default when
 * disabled.
 */
/* #define COAP_MBEDTLS_CONF_USE_ALL_CIPHERSUITES */

#endif /* DTLS_SUPPORT_CONFIG_H */
