/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
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
 * 3. Neither the name of the copyright holder nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
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
 *      COSE error implementation
 * \author
 *      Niclas Finne <niclas.finne@ri.se>, Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "cose-error.h"

/*---------------------------------------------------------------------------*/
const char *
cose_error_string(cose_error_t error)
{
  switch(error) {
  case COSE_SUCCESS:
    return "Success";

  /* Parameter and input errors */
  case COSE_ERR_INVALID_PARAMETER:
    return "Invalid parameter";
  case COSE_ERR_NULL_POINTER:
    return "Null pointer";
  case COSE_ERR_BUFFER_TOO_SMALL:
    return "Buffer too small";
  case COSE_ERR_INVALID_LENGTH:
    return "Invalid length";
  case COSE_ERR_UNSUPPORTED_ALGORITHM:
    return "Algorithm not supported";

  /* Memory and buffer errors */
  case COSE_ERR_MEMORY_ALLOCATION:
    return "Memory allocation failed";
  case COSE_ERR_BUFFER_OVERFLOW:
    return "Buffer overflow";

  /* Cryptographic operation errors */
  case COSE_ERR_CRYPTO_KEYGEN:
    return "Key generation failed";
  case COSE_ERR_CRYPTO_SIGN:
    return "Digital signature failed";
  case COSE_ERR_CRYPTO_VERIFY:
    return "Signature verification failed";
  case COSE_ERR_CRYPTO_ENCRYPT:
    return "Encryption failed";
  case COSE_ERR_CRYPTO_DECRYPT:
    return "Decryption failed";
  case COSE_ERR_CRYPTO_AUTHENTICATION:
    return "Authentication tag verification failed";
  case COSE_ERR_CRYPTO_INVALID_KEY:
    return "Invalid cryptographic key";

  /* CBOR structure errors */
  case COSE_ERR_CBOR_ENCODING:
    return "CBOR encoding failed";
  case COSE_ERR_CBOR_DECODING:
    return "CBOR decoding failed";
  case COSE_ERR_CBOR_INVALID_TYPE:
    return "Invalid CBOR type";
  case COSE_ERR_CBOR_MALFORMED:
    return "Malformed CBOR data";

  /* COSE structure errors */
  case COSE_ERR_INVALID_STRUCTURE:
    return "Invalid COSE structure";
  case COSE_ERR_MISSING_HEADER:
    return "Required header missing";
  case COSE_ERR_INVALID_HEADER:
    return "Invalid header content";
  case COSE_ERR_MISSING_PAYLOAD:
    return "Required payload missing";

  /* General errors */
  case COSE_ERR_NOT_IMPLEMENTED:
    return "Not implemented";
  case COSE_ERR_INTERNAL_ERROR:
    return "Internal error";
  case COSE_ERR_UNKNOWN:
    return "Unknown error";

  default:
    return "Unrecognized COSE error code";
  }
}
/*---------------------------------------------------------------------------*/