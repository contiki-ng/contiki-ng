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
 *      COSE library error definitions
 *
 *      This header defines error codes for COSE (CBOR Object Signing and
 *      Encryption) operations as specified in RFC 8152.
 *
 * \author
 *      Niclas Finne <niclas.finne@ri.se>, Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef COSE_ERROR_H_
#define COSE_ERROR_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief COSE operation error codes
 *
 * Error codes for COSE operations use negative values to distinguish
 * them from success (0) and positive return values (e.g., sizes).
 */
typedef enum {
  /* Success */
  COSE_SUCCESS = 0,

  /* Parameter and input errors (-1 to -9) */
  COSE_ERR_INVALID_PARAMETER = -1,          /**< Invalid parameter provided */
  COSE_ERR_NULL_POINTER = -2,               /**< Null pointer provided */
  COSE_ERR_BUFFER_TOO_SMALL = -3,           /**< Buffer too small for operation */
  COSE_ERR_INVALID_LENGTH = -4,             /**< Invalid length parameter */
  COSE_ERR_UNSUPPORTED_ALGORITHM = -5,      /**< Algorithm not supported */

  /* Memory and buffer errors (-10 to -19) */
  COSE_ERR_MEMORY_ALLOCATION = -10,         /**< Memory allocation failed */
  COSE_ERR_BUFFER_OVERFLOW = -11,           /**< Buffer overflow detected */

  /* Cryptographic operation errors (-20 to -29) */
  COSE_ERR_CRYPTO_KEYGEN = -20,             /**< Key generation failed */
  COSE_ERR_CRYPTO_SIGN = -21,               /**< Digital signature failed */
  COSE_ERR_CRYPTO_VERIFY = -22,             /**< Signature verification failed */
  COSE_ERR_CRYPTO_ENCRYPT = -23,            /**< Encryption operation failed */
  COSE_ERR_CRYPTO_DECRYPT = -24,            /**< Decryption operation failed */
  COSE_ERR_CRYPTO_AUTHENTICATION = -25,     /**< Authentication tag verification failed */
  COSE_ERR_CRYPTO_INVALID_KEY = -26,        /**< Invalid cryptographic key */

  /* CBOR structure errors (-30 to -39) */
  COSE_ERR_CBOR_ENCODING = -30,             /**< CBOR encoding failed */
  COSE_ERR_CBOR_DECODING = -31,             /**< CBOR decoding failed */
  COSE_ERR_CBOR_INVALID_TYPE = -32,         /**< Invalid CBOR data type */
  COSE_ERR_CBOR_MALFORMED = -33,            /**< Malformed CBOR data */

  /* COSE structure errors (-40 to -49) */
  COSE_ERR_INVALID_STRUCTURE = -40,         /**< Invalid COSE structure */
  COSE_ERR_MISSING_HEADER = -41,            /**< Required header missing */
  COSE_ERR_INVALID_HEADER = -42,            /**< Invalid header content */
  COSE_ERR_MISSING_PAYLOAD = -43,           /**< Required payload missing */

  /* General errors (-50 to -59) */
  COSE_ERR_NOT_IMPLEMENTED = -50,           /**< Feature not implemented */
  COSE_ERR_INTERNAL_ERROR = -51,            /**< Internal implementation error */
  COSE_ERR_UNKNOWN = -52                   /**< Unknown error condition */
} cose_error_t;

/**
 * \brief Check if a COSE operation was successful
 * \param result The result to check
 * \return true if successful, false otherwise
 */
#define COSE_SUCCESS_CHECK(result) ((result) == COSE_SUCCESS)

/**
 * \brief Check if a COSE operation failed
 * \param result The result to check
 * \return true if failed, false otherwise
 */
#define COSE_FAILED(result) ((result) < COSE_SUCCESS)

/**
 * \brief Get a human-readable error message for a COSE error
 * \param error The COSE error code
 * \return String description of the error
 */
const char *cose_error_string(cose_error_t error);

/**
 * \brief Macro for checking and propagating COSE errors
 *
 * Usage: COSE_CHECK(some_cose_function(params));
 */
#define COSE_CHECK(call) do { \
          cose_error_t _result = (call); \
          if(COSE_FAILED(_result)) { \
            return _result; \
          } \
} while(0)

#endif /* COSE_ERROR_H_ */