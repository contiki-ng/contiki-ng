/*
 * Copyright (c) 2022, RISE Research Institutes of Sweden.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *    3. Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef PROJECT_CONF_DTLS_H
#define PROJECT_CONF_DTLS_H

#include "project-conf-common.h"

#define LOG_CONF_LEVEL_DTLS LOG_LEVEL_INFO

/*
 * ENDPOINT_NAME "Contiki-NG36065E1DD"
 * corresponds to CN in test-certificate with PK starting with
 * "MHcCAQEEIDhoMzFVUcmwhmlhSWZC4crijj48IvaUslsjWvHWiFNpoAoGCCqGSM49"
 */
#ifdef LWM2M_CONF_ENGINE_CLIENT_ENDPOINT_NAME
#define LWM2M_ENGINE_CLIENT_ENDPOINT_NAME LWM2M_CONF_ENGINE_CLIENT_ENDPOINT_NAME
#else
#define LWM2M_ENGINE_CLIENT_ENDPOINT_NAME "Contiki-NG36065E1DD"
#endif /* LWM2M_CONF_ENGINE_CLIENT_ENDPOINT_NAME */

#define LWM2M_SERVER_ADDRESS "coaps://[fd00::1]"

#define COAP_DTLS_CONF_MAX_PEERS 1
//#define COAP_MBEDTLS_CONF_MTU 200
//#define COAP_MBEDTLS_CONF_MAX_FRAG_LEN 1

/* Ensure debugs are compiled before increasing this value. */
#define COAP_MBEDTLS_LIB_CONF_DEBUG_LEVEL 0

#if defined(NRF52840_XXAA) || defined(NRF5340_XXAA_APPLICATION) || \
    defined(NRF5340_XXAA_NETWORK)
/* These platforms do not yet implement the CSPRNG --
   use an insecure PRNG for testing. */
#define COAP_DTLS_CONF_PRNG_INSECURE 1
#else
/* Use the cryptographically-secure pseudo random number generator. */
#define CSPRNG_CONF_ENABLED 1
#endif

#ifdef COAP_DTLS_CONF_WITH_CERT
/* Dynamic memory needed for Mbed TLS library operation with certificates. */
#define HEAPMEM_CONF_ARENA_SIZE (1024 * 16)
#define HEAPMEM_CONF_ALIGNMENT sizeof(uint64_t)

/* Use more queuebufs when using certificates. */
#ifndef QUEUEBUF_CONF_NUM
#define QUEUEBUF_CONF_NUM 8
#endif

/* Enable fragmentation to support larger datagrams. */
#ifndef SICSLOWPAN_CONF_FRAG
#define SICSLOWPAN_CONF_FRAG 1
#endif

/* Use Leshan test certificates. */
#define CHECKED_IN_COAP_DTLS_TEST_CA_CERT       \
"-----BEGIN CERTIFICATE-----\r\n" \
"MIICITCCAcYCCQDfbROysgtSGzAKBggqhkjOPQQDAjCBljELMAkGA1UEBhMCU0Ux\r\n" \
"EjAQBgNVBAgMCVN0b2NraG9sbTEOMAwGA1UEBwwFS2lzdGExEjAQBgNVBAoMCVJJ\r\n" \
"U0UtU0lDUzEfMB0GA1UECwwWQ29ubmVjdGVkLUludGVsbGlnZW5jZTENMAsGA1UE\r\n" \
"AwwEUm9vdDEfMB0GCSqGSIb3DQEJARYQZWpheWVuQGdtYWlsLmNvbTAgFw0yMjA3\r\n" \
"MjgxNDA2MjJaGA8yMTIyMDcwNDE0MDYyMlowgZYxCzAJBgNVBAYTAlNFMRIwEAYD\r\n" \
"VQQIDAlTdG9ja2hvbG0xDjAMBgNVBAcMBUtpc3RhMRIwEAYDVQQKDAlSSVNFLVNJ\r\n" \
"Q1MxHzAdBgNVBAsMFkNvbm5lY3RlZC1JbnRlbGxpZ2VuY2UxDTALBgNVBAMMBFJv\r\n" \
"b3QxHzAdBgkqhkiG9w0BCQEWEGVqYXllbkBnbWFpbC5jb20wWTATBgcqhkjOPQIB\r\n" \
"BggqhkjOPQMBBwNCAAQg8wdkfjxntwOkpK5HYpL4kO7/5LsVFobnU4D8jZlfX/Z8\r\n" \
"Ys+TPUqVmDVwwMXWy6ELs51nOBh8WLvlAC5KVeESMAoGCCqGSM49BAMCA0kAMEYC\r\n" \
"IQCAgRf942+typj1XFq8/4/+msOSE0HmfluR75wfa2PY6AIhAJW0ebNkiNgUKvmW\r\n" \
"fktB3aee55zvJc0piQblFe0OgJ3e\r\n" \
"-----END CERTIFICATE-----\r\n"

#define COAP_DTLS_TEST_CA_CERT  \
"-----BEGIN CERTIFICATE-----\r\n" \
"MIICBzCCAa+gAwIBAgIUcVVVy8vLLI2v3pxlOoZ810gPyNYwCgYIKoZIzj0EAwIw\r\n" \
"WTELMAkGA1UEBhMCQVUxEzARBgNVBAgMClNvbWUtU3RhdGUxITAfBgNVBAoMGElu\r\n" \
"dGVybmV0IFdpZGdpdHMgUHR5IEx0ZDESMBAGA1UEAwwJbG9jYWxob3N0MCAXDTIx\r\n" \
"MDcxNTEzMzUxOFoYDzIxMjEwNjIxMTMzNTE4WjBZMQswCQYDVQQGEwJBVTETMBEG\r\n" \
"A1UECAwKU29tZS1TdGF0ZTEhMB8GA1UECgwYSW50ZXJuZXQgV2lkZ2l0cyBQdHkg\r\n" \
"THRkMRIwEAYDVQQDDAlsb2NhbGhvc3QwWTATBgcqhkjOPQIBBggqhkjOPQMBBwNC\r\n" \
"AATSbarzAiS5luCVFoABZyGTa9wQkG+25w8KXqGtlvjbgVooIO/X89nKQ2Ea+KPh\r\n" \
"tGXPj93ZQcK5gVBAj7j1Vb+Oo1MwUTAdBgNVHQ4EFgQUl3UTD4jWyM376aj6yaLN\r\n" \
"jWCUm0gwHwYDVR0jBBgwFoAUl3UTD4jWyM376aj6yaLNjWCUm0gwDwYDVR0TAQH/\r\n" \
"BAUwAwEB/zAKBggqhkjOPQQDAgNGADBDAh8QrcYR4drlHAbiztd/32Ecw4bKmUR5\r\n" \
"76jwP1qYrqlUAiBD0EJ2HfIyRbQkKLXUx4Awt9rZO65VhdRWhcGtiAJF7w==\r\n" \
"-----END CERTIFICATE-----\r\n"

#ifdef COAP_DTLS_CONF_WITH_SERVER
#define COAP_DTLS_TEST_OWN_CERT \
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIBpDCCAUkCCQCLI2CsMlVzyTAKBggqhkjOPQQDAjCBljELMAkGA1UEBhMCU0Ux\r\n" \
"EjAQBgNVBAgMCVN0b2NraG9sbTEOMAwGA1UEBwwFS2lzdGExEjAQBgNVBAoMCVJJ\r\n" \
"U0UtU0lDUzEfMB0GA1UECwwWQ29ubmVjdGVkLUludGVsbGlnZW5jZTENMAsGA1UE\r\n" \
"AwwEUm9vdDEfMB0GCSqGSIb3DQEJARYQZWpheWVuQGdtYWlsLmNvbTAgFw0yMjA3\r\n" \
"MjgxNDI5MzNaGA8yMTIyMDcwNDE0MjkzM1owGjEYMBYGA1UEAwwPMC4wLjAuMC8w\r\n" \
"LjAuMC4wMFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAEbqosQlT76df51GPIi9/J\r\n" \
"JQtrPgukBnzH+qKtuZ2z+rZktFHRhm6h1hkl5I7NeymI5uWsKy4JsU2jTpdY05x+\r\n" \
"oTAKBggqhkjOPQQDAgNJADBGAiEAp6bOSfYw9ufBU/4kpxg+0d+harc949ItICXq\r\n" \
"kqaqhI0CIQDPfHeICJcKgYqgk6SxRJ0Gvq73S6XGmIo5t6vpO838Ag==\r\n" \
"-----END CERTIFICATE-----\r\n"
#define COAP_DTLS_TEST_PRIV_KEY \
"-----BEGIN EC PARAMETERS-----\r\n" \
"BggqhkjOPQMBBw==\r\n" \
"-----END EC PARAMETERS-----\r\n" \
"-----BEGIN EC PRIVATE KEY-----\r\n" \
"MHcCAQEEIMp44kGpPZdPsGtGswoNrMhGQx2ea2iD/+1Hh63zXT7poAoGCCqGSM49\r\n" \
"AwEHoUQDQgAEbqosQlT76df51GPIi9/JJQtrPgukBnzH+qKtuZ2z+rZktFHRhm6h\r\n" \
"1hkl5I7NeymI5uWsKy4JsU2jTpdY05x+oQ==\r\n" \
"-----END EC PRIVATE KEY-----\r\n"

#else /* COAP_DTLS_CONF_WITH_SERVER */

#define COAP_DTLS_TEST_OWN_CERT \
"-----BEGIN CERTIFICATE-----\r\n" \
"MIIBqDCCAU0CCQCLI2CsMlVzyDAKBggqhkjOPQQDAjCBljELMAkGA1UEBhMCU0Ux\r\n" \
"EjAQBgNVBAgMCVN0b2NraG9sbTEOMAwGA1UEBwwFS2lzdGExEjAQBgNVBAoMCVJJ\r\n" \
"U0UtU0lDUzEfMB0GA1UECwwWQ29ubmVjdGVkLUludGVsbGlnZW5jZTENMAsGA1UE\r\n" \
"AwwEUm9vdDEfMB0GCSqGSIb3DQEJARYQZWpheWVuQGdtYWlsLmNvbTAgFw0yMjA3\r\n" \
"MjgxNDI5MDhaGA8yMTIyMDcwNDE0MjkwOFowHjEcMBoGA1UEAwwTQ29udGlraS1O\r\n" \
"RzM2MDY1RTFERDBZMBMGByqGSM49AgEGCCqGSM49AwEHA0IABJhSKiXGjvzFwe6S\r\n" \
"XJR7Ai9+Ct/8PQVyJjNLAJLFrGBy0ELB8JqnpuaN4xkBoKopq48vOQHl8CvSsxpK\r\n" \
"UhcLrOEwCgYIKoZIzj0EAwIDSQAwRgIhAIOMOq0LvIHgPXUYx/eH7htnXDfevT7a\r\n" \
"c8iN6l55AQsRAiEA/AiLeDdc/YCnjfghBKs8us0kxZp5gXylVOLGtLaluMY=\r\n" \
"-----END CERTIFICATE-----\r\n"
#define COAP_DTLS_TEST_PRIV_KEY \
"-----BEGIN EC PARAMETERS-----\r\n" \
"BggqhkjOPQMBBw==\r\n" \
"-----END EC PARAMETERS-----\r\n" \
"-----BEGIN EC PRIVATE KEY-----\r\n" \
"MHcCAQEEIDhoMzFVUcmwhmlhSWZC4crijj48IvaUslsjWvHWiFNpoAoGCCqGSM49\r\n" \
"AwEHoUQDQgAEmFIqJcaO/MXB7pJclHsCL34K3/w9BXImM0sAksWsYHLQQsHwmqem\r\n" \
"5o3jGQGgqimrjy85AeXwK9KzGkpSFwus4Q==\r\n" \
"-----END EC PRIVATE KEY-----\r\n"

#endif /* COAP_DTLS_CONF_WITH_SERVER */
#else /* COAP_DTLS_CONF_WITH_CERT */
/* Dynamic memory needed for Mbed TLS library operation with PSK. */
#define HEAPMEM_CONF_ARENA_SIZE (1024 * 8)
#define HEAPMEM_CONF_ALIGNMENT sizeof(uint64_t)

/*
 * Private Shared Key (PSK) information.
 *
 * When using Leshan as the LwM2M server, you can add PSK security
 * information for the client through the web interface. The entered
 * PSK must be in hexadecimal format.
 *
 * The value for the default key "secretPSK" is "73656372657450534B" in hex
 *
 * To generate the hexadecimal string that corresponds to a PSK string, the
 * following shell commands can be used:
 *
 * $ echo -n "<PSK string>" | od -A n -t x1 | sed 's/ //g'
 */
#define COAP_DTLS_PSK_DEFAULT_IDENTITY "Client_identity"
#define COAP_DTLS_PSK_DEFAULT_KEY      "secretPSK"
#endif /* COAP_DTLS_CONF_WITH_CERT */

#endif /* PROJECT_CONF_DTLS_H */
