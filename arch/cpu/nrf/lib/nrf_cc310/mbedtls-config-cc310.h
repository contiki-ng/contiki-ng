/*
 * Minimal mbedTLS configuration for using nrfxlib's cc3xx PSA transparent
 * driver as a standalone hardware ECC provider for lib/ecc.h on the nRF52840.
 *
 * The lib/ecc.h backend (arch/cpu/nrf/dev/nrf52840-cc310-ecc.c) calls the
 * cc3xx_* PSA driver entry points directly for ECDSA/ECDH/keygen, but those
 * call back into the mbedTLS PSA core (psa_generate_random and the PSA hash
 * driver-wrapper), so a slice of mbedTLS must be linked. This config enables
 * only that slice: the PSA core + SHA-256, with randomness taken from the
 * CC310 TRNG through an external-RNG hook (mbedtls_psa_external_get_random,
 * implemented in nrf_cc3xx_platform_contiki.c). No TLS, no X.509, and no
 * built-in asymmetric crypto (the CC310 does the EC math).
 */
#ifndef MBEDTLS_CONFIG_CC310_H
#define MBEDTLS_CONFIG_CC310_H

/* Platform allocator as function pointers (mbedtls_calloc/mbedtls_free): the
 * nrfxlib cc3xx blob was built with MBEDTLS_PLATFORM_MEMORY and references
 * these symbols, so platform.c must provide them (defaults to calloc/free). */
#define MBEDTLS_PLATFORM_C
#define MBEDTLS_PLATFORM_MEMORY

/* PSA crypto core (provides psa_generate_random + the hash driver-wrapper the
 * cc3xx driver calls into). */
#define MBEDTLS_PSA_CRYPTO_C

/* Randomness from the CC310 TRNG via an external RNG hook, instead of the
 * built-in entropy + CTR_DRBG + AES stack. */
#define MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG

/* SHA-256 for the PSA hash driver-wrapper. */
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA224_C

#endif /* MBEDTLS_CONFIG_CC310_H */
