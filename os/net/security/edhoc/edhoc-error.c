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
 *      Error handling module for EDHOC.
 * \author
 *      Niclas Finne <niclas.finne@ri.se>, Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "edhoc-error.h"
#include <string.h>

/* Static storage for error context */
static edhoc_error_context_t last_error_context = { EDHOC_SUCCESS, NULL, 0, NULL, NULL };
/*---------------------------------------------------------------------------*/
const char *
edhoc_error_string(edhoc_error_t error)
{
  switch(error) {
  case EDHOC_SUCCESS:
    return "Success";

  /* Memory errors */
  case EDHOC_ERR_MEMORY_ALLOCATION:
    return "Memory allocation failed";
  case EDHOC_ERR_BUFFER_TOO_SMALL:
    return "Buffer too small";
  case EDHOC_ERR_BUFFER_OVERFLOW:
    return "Buffer overflow";
  case EDHOC_ERR_NULL_POINTER:
    return "Null pointer";
  case EDHOC_ERR_INVALID_LENGTH:
    return "Invalid length";

  /* Cryptographic errors */
  case EDHOC_ERR_CRYPTO_KEYGEN:
    return "Cryptographic key generation failed";
  case EDHOC_ERR_CRYPTO_SIGN:
    return "Digital signature failed";
  case EDHOC_ERR_CRYPTO_VERIFY:
    return "Signature verification failed";
  case EDHOC_ERR_CRYPTO_ENCRYPT:
    return "Encryption failed";
  case EDHOC_ERR_CRYPTO_DECRYPT:
    return "Decryption failed";
  case EDHOC_ERR_CRYPTO_HASH:
    return "Hash operation failed";
  case EDHOC_ERR_CRYPTO_KDF:
    return "Key derivation failed";
  case EDHOC_ERR_CRYPTO_AUTHENTICATION:
    return "Authentication failed";
  case EDHOC_ERR_CRYPTO_INVALID_KEY:
    return "Invalid cryptographic key";

  /* Protocol errors */
  case EDHOC_ERR_SUITE_NOT_SUPPORTED:
    return "Cipher suite not supported";
  case EDHOC_ERR_MSG_MALFORMED:
    return "Malformed message";
  case EDHOC_ERR_METHOD_NOT_SUPPORTED:
    return "EDHOC method not supported";
  case EDHOC_ERR_CID_INVALID:
    return "Invalid connection identifier";
  case EDHOC_ERR_WRONG_CID:
    return "Wrong connection identifier";
  case EDHOC_ERR_CREDENTIAL_INVALID:
    return "Invalid credential";
  case EDHOC_ERR_ID_CRED_MALFORMED:
    return "Malformed credential identifier";
  case EDHOC_ERR_SEQUENCE_ERROR:
    return "Message sequence error";
  case EDHOC_ERR_CORRELATION:
    return "Message correlation error";
  case EDHOC_ERR_CRITICAL_EAD_UNSUPPORTED:
    return "Critical EAD item cannot be processed";

  /* Storage errors */
  case EDHOC_ERR_KEY_NOT_FOUND:
    return "Key not found";
  case EDHOC_ERR_KEY_STORAGE_FULL:
    return "Key storage full";
  case EDHOC_ERR_CREDENTIAL_NOT_FOUND:
    return "Credential not found";
  case EDHOC_ERR_STORAGE_INIT_FAILED:
    return "Storage initialization failed";
  case EDHOC_ERR_DUPLICATE_KEY:
    return "Duplicate key";

  /* Network errors */
  case EDHOC_ERR_NETWORK_TIMEOUT:
    return "Network timeout";
  case EDHOC_ERR_NETWORK_CONNECTION:
    return "Network connection error";
  case EDHOC_ERR_COAP_ERROR:
    return "CoAP error";
  case EDHOC_ERR_MESSAGE_TOO_LARGE:
    return "Message too large";

  /* CBOR errors */
  case EDHOC_ERR_CBOR_ENCODING:
    return "CBOR encoding failed";
  case EDHOC_ERR_CBOR_DECODING:
    return "CBOR decoding failed";
  case EDHOC_ERR_CBOR_INVALID_TYPE:
    return "Invalid CBOR type";
  case EDHOC_ERR_CBOR_MALFORMED:
    return "Malformed CBOR data";

  /* State errors */
  case EDHOC_ERR_INVALID_STATE:
    return "Invalid state";
  case EDHOC_ERR_CONTEXT_NOT_INITIALIZED:
    return "Context not initialized";
  case EDHOC_ERR_ALREADY_INITIALIZED:
    return "Already initialized";
  case EDHOC_ERR_NOT_ALLOWED_IDENTITY:
    return "Identity not allowed";

  /* General errors */
  case EDHOC_ERR_INVALID_PARAMETER:
    return "Invalid parameter";
  case EDHOC_ERR_NOT_IMPLEMENTED:
    return "Not implemented";
  case EDHOC_ERR_INTERNAL_ERROR:
    return "Internal error";
  case EDHOC_ERR_UNKNOWN:
    return "Unknown error";

  default:
    return "Unrecognized error code";
  }
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_set_error_context(edhoc_error_t code, const char *file,
                        int line, const char *function,
                        const char *message)
{
  last_error_context.code = code;
  last_error_context.file = file;
  last_error_context.line = line;
  last_error_context.function = function;
  last_error_context.message = message;

  return code;
}
/*---------------------------------------------------------------------------*/
const edhoc_error_context_t *
edhoc_get_last_error_context(void)
{
  if(last_error_context.code == EDHOC_SUCCESS) {
    return NULL;
  }
  return &last_error_context;
}
