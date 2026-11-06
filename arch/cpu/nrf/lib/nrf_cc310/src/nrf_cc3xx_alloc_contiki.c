/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden.
 * All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause
 */

/**
 * \file
 *   mbedtls_calloc / mbedtls_free for the nrfxlib cc3xx blob and linked mbedTLS.
 *
 *   These must be real Thumb functions, not function-pointer variables: the blob
 *   and mbedTLS call them directly, and a branch to a data symbol links as an
 *   ARM-interworking veneer that faults (UNDEFINSTR) on the Thumb-only
 *   Cortex-M4. This TU includes no mbedTLS headers, so the MBEDTLS_PLATFORM_MEMORY
 *   pointer declaration / "#define mbedtls_calloc calloc" macro cannot interfere.
 */

#include <stdlib.h>

void *
mbedtls_calloc(size_t n, size_t size)
{
  return calloc(n, size);
}

void
mbedtls_free(void *ptr)
{
  free(ptr);
}
