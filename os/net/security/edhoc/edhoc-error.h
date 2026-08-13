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
 *      Error handling module header for EDHOC.
 *
 *      This header defines a unified error handling system that provides
 *      consistent error reporting across all EDHOC operations.
 *
 * \author
 *      Niclas Finne <niclas.finne@ri.se>, Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#ifndef EDHOC_ERROR_H_
#define EDHOC_ERROR_H_

#include <stdint.h>
#include <stdbool.h>

/**
 * \brief Unified error type for EDHOC operations
 *
 * This enum defines all possible error conditions that can occur in EDHOC
 * operations. Error codes are categorized by type and use negative values
 * to distinguish them from success (0) and positive return values
 * (e.g., sizes, counts).
 *
 * Convention:
 * - EDHOC_SUCCESS (0): Operation completed successfully
 * - Negative values: Specific error conditions
 * - Positive values: Success with data (size, count, etc.)
 */
typedef enum {
  /* Success */
  EDHOC_SUCCESS = 0,

  /* Memory errors (-1 to -9) */
  EDHOC_ERR_MEMORY_ALLOCATION = -1,      /**< Memory allocation failed */
  EDHOC_ERR_BUFFER_TOO_SMALL = -2,       /**< Provided buffer is too small */
  EDHOC_ERR_BUFFER_OVERFLOW = -3,        /**< Buffer overflow detected */
  EDHOC_ERR_NULL_POINTER = -4,           /**< Null pointer provided */
  EDHOC_ERR_INVALID_LENGTH = -5,         /**< Invalid length parameter */

  /* Cryptographic errors (-10 to -19) */
  EDHOC_ERR_CRYPTO_KEYGEN = -10,         /**< Key generation failed */
  EDHOC_ERR_CRYPTO_SIGN = -11,           /**< Digital signature failed */
  EDHOC_ERR_CRYPTO_VERIFY = -12,         /**< Signature verification failed */
  EDHOC_ERR_CRYPTO_ENCRYPT = -13,        /**< Encryption operation failed */
  EDHOC_ERR_CRYPTO_DECRYPT = -14,        /**< Decryption operation failed */
  EDHOC_ERR_CRYPTO_HASH = -15,           /**< Hash operation failed */
  EDHOC_ERR_CRYPTO_KDF = -16,            /**< Key derivation failed */
  EDHOC_ERR_CRYPTO_AUTHENTICATION = -17,   /**< Authentication failed */
  EDHOC_ERR_CRYPTO_INVALID_KEY = -18,    /**< Invalid cryptographic key */

  /* Protocol errors (-20 to -29) */
  EDHOC_ERR_SUITE_NOT_SUPPORTED = -20,   /**< Cipher suite not supported */
  EDHOC_ERR_MSG_MALFORMED = -21,         /**< Malformed message received */
  EDHOC_ERR_METHOD_NOT_SUPPORTED = -22,   /**< EDHOC method not supported */
  EDHOC_ERR_CID_INVALID = -23,           /**< Invalid connection identifier */
  EDHOC_ERR_WRONG_CID = -24,             /**< Wrong connection identifier */
  EDHOC_ERR_CREDENTIAL_INVALID = -25,    /**< Invalid credential */
  EDHOC_ERR_ID_CRED_MALFORMED = -26,     /**< Malformed credential identifier */
  EDHOC_ERR_SEQUENCE_ERROR = -27,        /**< Message sequence error */
  EDHOC_ERR_CORRELATION = -28,           /**< Message correlation error */
  EDHOC_ERR_CRITICAL_EAD_UNSUPPORTED = -29, /**< Critical EAD item cannot be processed (RFC 9528 Section 6) */

  /* Storage errors (-30 to -39) */
  EDHOC_ERR_KEY_NOT_FOUND = -30,         /**< Key not found in storage */
  EDHOC_ERR_KEY_STORAGE_FULL = -31,      /**< Key storage is full */
  EDHOC_ERR_CREDENTIAL_NOT_FOUND = -32,   /**< Credential not found */
  EDHOC_ERR_STORAGE_INIT_FAILED = -33,   /**< Storage initialization failed */
  EDHOC_ERR_DUPLICATE_KEY = -34,         /**< Duplicate key in storage */

  /* Network errors (-40 to -49) */
  EDHOC_ERR_NETWORK_TIMEOUT = -40,       /**< Network operation timeout */
  EDHOC_ERR_NETWORK_CONNECTION = -41,    /**< Network connection error */
  EDHOC_ERR_COAP_ERROR = -42,            /**< CoAP protocol error */
  EDHOC_ERR_MESSAGE_TOO_LARGE = -43,     /**< Message too large for transport */

  /* CBOR errors (-50 to -59) */
  EDHOC_ERR_CBOR_ENCODING = -50,         /**< CBOR encoding failed */
  EDHOC_ERR_CBOR_DECODING = -51,         /**< CBOR decoding failed */
  EDHOC_ERR_CBOR_INVALID_TYPE = -52,     /**< Invalid CBOR data type */
  EDHOC_ERR_CBOR_MALFORMED = -53,        /**< Malformed CBOR data */

  /* State errors (-60 to -69) */
  EDHOC_ERR_INVALID_STATE = -60,         /**< Invalid context state */
  EDHOC_ERR_CONTEXT_NOT_INITIALIZED = -61,   /**< Context not initialized */
  EDHOC_ERR_ALREADY_INITIALIZED = -62,   /**< Already initialized */
  EDHOC_ERR_NOT_ALLOWED_IDENTITY = -63,   /**< Identity not allowed */

  /* General errors (-70 to -79) */
  EDHOC_ERR_INVALID_PARAMETER = -70,     /**< Invalid parameter provided */
  EDHOC_ERR_NOT_IMPLEMENTED = -71,       /**< Feature not implemented */
  EDHOC_ERR_INTERNAL_ERROR = -72,        /**< Internal implementation error */
  EDHOC_ERR_UNKNOWN = -73               /**< Unknown error condition */
} edhoc_error_t;

/**
 * \brief Check if an operation was successful
 * \param result The result to check
 * \return true if successful, false otherwise
 */
#define EDHOC_SUCCESS_CHECK(result) ((result) == EDHOC_SUCCESS)

/**
 * \brief Check if an operation failed
 * \param result The result to check
 * \return true if failed, false otherwise
 */
#define EDHOC_FAILED(result) ((result) < EDHOC_SUCCESS)

/**
 * \brief Get a human-readable error message for an error code
 * \param error The error code
 * \return String description of the error
 */
const char *edhoc_error_string(edhoc_error_t error);

/**
 * \brief Error context structure for debugging
 *
 * This structure provides detailed context information about where
 * an error occurred, useful for debugging and logging.
 */
typedef struct {
  edhoc_error_t code;        /**< Error code */
  const char *file;          /**< Source file where error occurred */
  int line;                 /**< Line number where error occurred */
  const char *function;     /**< Function where error occurred */
  const char *message;      /**< Additional error message */
} edhoc_error_context_t;

/**
 * \brief Set error context for debugging (internal use)
 * \param code Error code
 * \param file Source file name
 * \param line Line number
 * \param function Function name
 * \param message Additional message
 * \return The error code passed in
 */
edhoc_error_t edhoc_set_error_context(edhoc_error_t code, const char *file,
                                      int line, const char *function,
                                      const char *message);

/**
 * \brief Get the last error context (for debugging)
 * \return Pointer to last error context, or NULL if none
 */
const edhoc_error_context_t *edhoc_get_last_error_context(void);

/**
 * \brief Macro for setting error with context information
 *
 * Usage: return EDHOC_ERROR(EDHOC_ERR_MEMORY_ALLOCATION, "Failed to allocate key storage");
 */
#define EDHOC_ERROR(code, msg) \
        edhoc_set_error_context((code), __FILE__, __LINE__, __func__, (msg))

/**
 * \brief Macro for checking and propagating errors
 *
 * Usage: EDHOC_CHECK(some_function(params));
 */
#define EDHOC_CHECK(call) do { \
          edhoc_error_t _result = (call); \
          if(EDHOC_FAILED(_result)) { \
            return _result; \
          } \
} while(0)


#endif /* EDHOC_ERROR_H_ */
