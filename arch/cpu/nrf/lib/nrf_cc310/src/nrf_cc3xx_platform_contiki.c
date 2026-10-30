/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * \file
 *   Contiki-NG glue for nrfxlib's cc3xx PSA crypto driver on the nRF52840.
 *
 *   The cc3xx driver's randomness (ECDSA nonces, key generation) is drawn
 *   through the mbedTLS PSA external-RNG hook, which this file implements on
 *   top of the CC310 TRNG exposed by nrf_cc3xx_platform. Using the external
 *   RNG (MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG) keeps the linked mbedTLS slice down
 *   to the PSA core + SHA-256 -- no entropy/CTR-DRBG/AES stack.
 */

#include "psa/crypto.h"
#include "nrf_cc3xx_platform.h"
#include "nrf_cc3xx_platform_entropy.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/*
 * ABI bridge to the nrfxlib cc3xx blob (built against an earlier mbedTLS than
 * Contiki's 3.6.2): supply the few mbedTLS internals the blob links against that
 * 3.6.2 inlined or renamed. (mbedtls_calloc / mbedtls_free live in
 * nrf_cc3xx_alloc_contiki.c.)
 */

/* 3.6.2 made the one-shot PSA hash driver-wrapper static-inline, so forward the
 * external symbol the blob needs to the public (stateless) PSA hash. */
psa_status_t
psa_driver_wrapper_hash_compute(psa_algorithm_t alg,
                                const uint8_t *input, size_t input_length,
                                uint8_t *hash, size_t hash_size,
                                size_t *hash_length)
{
  return psa_hash_compute(alg, input, input_length,
                          hash, hash_size, hash_length);
}

/*
 * Multi-part hash wrappers: only reached from the blob's RSA-PSS path, which
 * this EC backend never exercises. Stubbed (not forwarded) because the
 * psa_hash_operation_t layout differs from 3.6.2; they only satisfy the linker.
 */
psa_status_t
psa_driver_wrapper_hash_setup(psa_hash_operation_t *operation,
                              psa_algorithm_t alg)
{
  (void)operation; (void)alg;
  return PSA_ERROR_NOT_SUPPORTED;
}
psa_status_t
psa_driver_wrapper_hash_update(psa_hash_operation_t *operation,
                               const uint8_t *input, size_t input_length)
{
  (void)operation; (void)input; (void)input_length;
  return PSA_ERROR_NOT_SUPPORTED;
}
psa_status_t
psa_driver_wrapper_hash_finish(psa_hash_operation_t *operation,
                               uint8_t *hash, size_t hash_size,
                               size_t *hash_length)
{
  (void)operation; (void)hash; (void)hash_size; (void)hash_length;
  return PSA_ERROR_NOT_SUPPORTED;
}
psa_status_t
psa_driver_wrapper_hash_abort(psa_hash_operation_t *operation)
{
  (void)operation;
  return PSA_ERROR_NOT_SUPPORTED;
}
psa_status_t
psa_driver_wrapper_hash_clone(const psa_hash_operation_t *source,
                              psa_hash_operation_t *target)
{
  (void)source; (void)target;
  return PSA_ERROR_NOT_SUPPORTED;
}

#if defined(MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG)
psa_status_t
mbedtls_psa_external_get_random(mbedtls_psa_external_random_context_t *context,
                                uint8_t *output, size_t output_size,
                                size_t *output_length)
{
  (void)context;

  /* Required by MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG. Not reached on the cc3xx EC path
   * (the driver seeds its own DRBG from the TRNG), but the PSA core references
   * the symbol. Fill from the CC310 TRNG, looping as entropy_get may return
   * fewer bytes than requested. */
  size_t filled = 0;
  while(filled < output_size) {
    size_t olen = 0;
    int ret = nrf_cc3xx_platform_entropy_get(output + filled,
                                             output_size - filled, &olen);
    if(ret != NRF_CC3XX_PLATFORM_SUCCESS || olen == 0) {
      return PSA_ERROR_INSUFFICIENT_ENTROPY;
    }
    filled += olen;
  }

  *output_length = filled;
  return PSA_SUCCESS;
}
#endif /* MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG */
