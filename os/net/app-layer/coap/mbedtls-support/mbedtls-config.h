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

/**
 * \file
 *		Mbed TLS library configuration for CoAP
 *
 * \author
 *		Jayendra Ellamathy <ejayen@gmail.com>
 */

#include "dtls-support-config.h"
#include "lib/heapmem.h"

/* Basic settings */
#define MBEDTLS_SSL_TLS_C
#define MBEDTLS_VERSION_C

/* RNG Support */
#ifndef CONTIKI_TARGET_NATIVE
#define MBEDTLS_NO_DEFAULT_ENTROPY_SOURCES
#define MBEDTLS_NO_PLATFORM_ENTROPY
#endif /* CONTIKI_TARGET_NATIVE */
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C

/* Timing */
#define MBEDTLS_TIMING_C
#define MBEDTLS_TIMING_ALT

#ifdef COAP_DTLS_CONF_WITH_CERT
#define MBEDTLS_HMAC_DRBG_C
#endif /* COAP_DTLS_CONF_WITH_CERT */

/* RFC 7925 profile */
#define MBEDTLS_SSL_PROTO_DTLS
#define MBEDTLS_SSL_PROTO_TLS1_2

#ifdef COAP_DTLS_CONF_WITH_PSK
#define MBEDTLS_KEY_EXCHANGE_PSK_ENABLED
#endif /* COAP_DTLS_CONF_WITH_PSK */

#ifdef COAP_DTLS_CONF_WITH_CERT
#define MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#define MBEDTLS_ECP_DP_SECP256R1_ENABLED
#define MBEDTLS_ECDH_C
#define MBEDTLS_CAN_ECDH
#define MBEDTLS_ECDSA_C
#define MBEDTLS_ECP_C
#define MBEDTLS_ECP_LIGHT
#define MBEDTLS_BIGNUM_C
#define MBEDTLS_ASN1_PARSE_C
#define MBEDTLS_OID_C
#define MBEDTLS_PK_PARSE_C
#define MBEDTLS_PK_C
#define MBEDTLS_PK_CAN_ECDSA_SIGN
#define MBEDTLS_PK_HAVE_ECC_KEYS
#define MBEDTLS_BASE64_C
#define MBEDTLS_PEM_PARSE_C
#define MBEDTLS_ASN1_WRITE_C
#define MBEDTLS_X509_USE_C
#define MBEDTLS_X509_CRT_PARSE_C
#define MBEDTLS_ECP_NIST_OPTIM
#define MBEDTLS_ECDSA_DETERMINISTIC
#endif /* COAP_DTLS_CONF_WITH_CERT */

#define MBEDTLS_AES_C
#define MBEDTLS_CCM_GCM_CAN_AES
#define MBEDTLS_CCM_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_MD_CAN_SHA256
#define MBEDTLS_MD_C
#define MBEDTLS_CIPHER_C

/* DTLS */
#define MBEDTLS_SSL_DTLS_ANTI_REPLAY
#define MBEDTLS_SSL_DTLS_HELLO_VERIFY

/* I/O message buffer sizes */
#define MBEDTLS_SSL_IN_CONTENT_LEN COAP_MBEDTLS_MTU
#define MBEDTLS_SSL_OUT_CONTENT_LEN COAP_MBEDTLS_MTU
#define MBEDTLS_SSL_DTLS_MAX_BUFFERING (2 * COAP_MBEDTLS_MTU)

/* Client Role */
#ifdef COAP_DTLS_CONF_WITH_CLIENT
#define MBEDTLS_SSL_CLI_C
/*#define MBEDTLS_SSL_SERVER_NAME_INDICATION */
#endif /* COAP_DTLS_CONF_WITH_CLIENT */

/* Server Role */
#ifdef COAP_DTLS_CONF_WITH_SERVER
#define MBEDTLS_SSL_SRV_C
#define MBEDTLS_SSL_COOKIE_C
#define MBEDTLS_SSL_CACHE_C
#endif /* COAP_DTLS_CONF_WITH_SERVER */

#ifdef COAP_DTLS_CONF_DEBUG
/* Debugging */
#define MBEDTLS_DEBUG_C
#define MBEDTLS_ERROR_C
#define MBEDTLS_SSL_ALL_ALERT_MESSAGES
#define MBEDTLS_SSL_DEBUG_ALL
#endif /* COAP_DTLS_CONF_DEBUG */

/* IoT features */
#define MBEDTLS_SSL_MAX_FRAGMENT_LENGTH

/* HW Acceleration. This is disabled because the current nRF SDK
   submodule lacks the necessary modules. */
#ifdef COAP_DTLS_CONF_HW_ACCEL
#ifdef COAP_DTLS_CONF_WITH_CERT
#ifdef NRF52840_XXAA /* Curr. only for nRF52840 */
#define NRF_HW_ACCEL_FOR_MBEDTLS
#define MBEDTLS_ECDSA_VERIFY_ALT
#define MBEDTLS_ECDH_COMPUTE_SHARED_ALT
#else
#error "COAP_DTLS_CONF_HW_ACCEL enabled, but is not implemented for the Contiki-NG target."
#endif /* NRF52840_XXAA */
#endif /* COAP_DTLS_CONF_WITH_CERT */
#endif /* COAP_DTLS_CONF_HW_ACCEL */

/* Use the Contiki-NG HeapMem module for Mbed TLS dynamic memory. */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_CALLOC_MACRO heapmem_calloc
#define MBEDTLS_PLATFORM_FREE_MACRO heapmem_free

#include "mbedtls/build_info.h"
