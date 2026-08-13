/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
 * Copyright (c) 2020, Industrial Systems Institute (ISI), Patras, Greece
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
 *
 */

/**
 * \file
 *         EDHOC message serialization and parsing API
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund, Marco Tiloca, Niclas Finne <niclas.finne@ri.se>, Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */
#ifndef _EDHOC_MSGS_H__
#define _EDHOC_MSGS_H__

#include <stdint.h>
#include <stddef.h>
#include "edhoc-key-storage.h"
#include "edhoc-config.h"
#include "edhoc-error.h"
#include "lib/cbor.h"


typedef struct edhoc_ead_data {
  int32_t ead_label;         /* RFC 9528: EAD label is integer, negative = critical */
  const uint8_t *ead_value;  /* Optional EAD value (byte string) */
  size_t ead_value_sz;       /* Size of EAD value */
} edhoc_ead_data_t;

typedef struct edhoc_msg_1 {
  uint8_t method;
  uint8_t suites_i[EDHOC_SUITES_MAX_COUNT];
  uint8_t suites_i_num;
  const uint8_t *g_x;
  const uint8_t *c_i;
  size_t c_i_sz;
  edhoc_ead_data_t uad;
} edhoc_msg_1_t;

typedef struct edhoc_msg_2 {
  uint8_t *gy_ciphertext_2;
  size_t gy_ciphertext_2_sz;
} edhoc_msg_2_t;

typedef struct edhoc_msg_3 {
  uint8_t *ciphertext_3;
  size_t ciphertext_3_sz;
} edhoc_msg_3_t;

#define EDHOC_MSG_ERR_CODE_RESERVED_FOR_SUCCESS 0
#define EDHOC_MSG_ERR_CODE_UNSPECIFIED_ERROR 1
#define EDHOC_MSG_ERR_CODE_WRONG_CIPHER_SUITE 2
#define EDHOC_MSG_ERR_CODE_UNKNOWN_CREDENTIAL_SELECTION 3
#define EDHOC_MSG_ERR_CODE_RESERVED 23

typedef struct edhoc_msg_error {
  uint8_t err_code;
  union {
    struct {
      const char *err_info;
      size_t err_info_sz;
    } tstr;
    struct {
      uint8_t suites[EDHOC_SUITES_MAX_COUNT];
      uint8_t suites_num;
    } suites;
  } info;
} edhoc_msg_error_t;

static inline uint8_t
edhoc_msg_error_get_code(const edhoc_msg_error_t *msg)
{
  return msg->err_code;
}

static inline const char *
edhoc_msg_error_get_info(const edhoc_msg_error_t *msg)
{
  return msg->info.tstr.err_info;
}

static inline size_t
edhoc_msg_error_get_info_sz(const edhoc_msg_error_t *msg)
{
  return msg->err_code == EDHOC_MSG_ERR_CODE_UNSPECIFIED_ERROR
    ? msg->info.tstr.err_info_sz
    : 0;
}

static inline const uint8_t *
edhoc_msg_error_get_suites(const edhoc_msg_error_t *msg)
{
  return msg->info.suites.suites;
}

static inline const uint8_t
edhoc_msg_error_get_suites_num(const edhoc_msg_error_t *msg)
{
  return msg->err_code == EDHOC_MSG_ERR_CODE_WRONG_CIPHER_SUITE
    ? msg->info.suites.suites_num
    : 0;
}

/**
 * \brief Print EDHOC message 1 contents for debugging
 * \param msg input message 1 structure to print
 */
void print_msg_1(edhoc_msg_1_t *msg);

/**
 * \brief Print EDHOC message 2 contents for debugging
 * \param msg input message 2 structure to print
 */
void print_msg_2(edhoc_msg_2_t *msg);

/**
 * \brief Print EDHOC message 3 contents for debugging
 * \param msg input message 3 structure to print
 */
void print_msg_3(edhoc_msg_3_t *msg);

/**
 * \brief Serialize EDHOC message 1 to CBOR format
 * \param msg input message 1 structure to serialize
 * \param buffer output buffer for serialized data
 * \param buffer_sz input size of output buffer
 * \param suite_array input whether to encode suites as array (unused)
 * \return size of serialized data on success, 0 on failure
 */
size_t edhoc_serialize_msg_1(edhoc_msg_1_t *msg, unsigned char *buffer, size_t buffer_sz, bool suite_array);

/**
 * \brief Serialize EDHOC error message to CBOR format
 * \param msg input error message structure to serialize
 * \param buffer output buffer for serialized data
 * \param buffer_sz input size of output buffer
 * \return size of serialized data on success, 0 on failure
 */
size_t edhoc_serialize_err(const edhoc_msg_error_t *msg, unsigned char *buffer, size_t buffer_sz);

/**
 * \brief Deserialize EDHOC message 1 from CBOR data
 * \param msg output message 1 structure
 * \param buffer input CBOR data to parse
 * \param buff_sz input size of CBOR data
 * \return EDHOC_SUCCESS on success, error code on failure
 */
edhoc_error_t edhoc_deserialize_msg_1(edhoc_msg_1_t *msg, unsigned char *buffer, size_t buff_sz);

/**
 * \brief Deserialize EDHOC message 2 from CBOR data
 * \param msg output message 2 structure
 * \param buffer input CBOR data to parse
 * \param buff_sz input size of CBOR data
 * \return EDHOC_SUCCESS on success, error code on failure
 */
edhoc_error_t edhoc_deserialize_msg_2(edhoc_msg_2_t *msg, unsigned char *buffer, size_t buff_sz);

/**
 * \brief Deserialize EDHOC message 3 from CBOR data
 * \param msg output message 3 structure
 * \param buffer input CBOR data to parse
 * \param buff_sz input size of CBOR data
 * \return EDHOC_SUCCESS on success, error code on failure
 */
edhoc_error_t edhoc_deserialize_msg_3(edhoc_msg_3_t *msg, unsigned char *buffer, size_t buff_sz);

/**
 * \brief Deserialize EDHOC error message from CBOR data
 * \param msg output error message structure
 * \param data input CBOR data to parse
 * \param data_size input size of CBOR data
 * \return \c true if the data could be parsed as an EDHOC error message,
 *         or \c false on error.
 */
bool edhoc_deserialize_err(edhoc_msg_error_t *msg, const uint8_t *data,
                           size_t data_size);

/**
 * \brief Get authentication key by Key ID (KID)
 * \param kid input key identifier bytes
 * \param kid_sz input key identifier length
 * \param key output pointer to retrieved authentication key
 * \return EDHOC_SUCCESS on success, error code on failure
 *
 * Searches the key repository for a key matching the provided KID.
 */
edhoc_error_t edhoc_get_auth_key_from_kid(uint8_t *kid, uint8_t kid_sz, cose_key_t **key);

/**
 * \brief Parse ID_CRED_X from CBOR data and extract authentication key
 * \param reader input CBOR reader state positioned at ID_CRED_X data
 * \param id_cred_x output buffer to store parsed ID_CRED_X (can be NULL)
 * \param id_cred_x_buffer_size input size of id_cred_x buffer
 * \param key output parsed authentication key information
 * \return size of ID_CRED_X data on success, negative error code on failure
 *
 * Parses ID_CRED_X in three supported formats:
 * - Compact encoding: bare KID value
 * - Map format: {4: KID}
 * - Full credential inclusion: complete COSE key structure
 * Populates the key structure with authentication information.
 */
int8_t edhoc_get_key_id_cred_x(cbor_reader_state_t *reader, uint8_t *id_cred_x, size_t id_cred_x_buffer_size, cose_key_t *key);

/**
 * \brief Parse signature data from CBOR reader
 * \param reader input CBOR reader state positioned at signature data
 * \param sign output pointer to signature data (points into reader buffer)
 * \return signature size in bytes on success, 0 on failure
 *
 * Reads and validates signature data from CBOR stream. The returned pointer
 * references data within the reader's buffer and remains valid until the
 * reader is reinitialized or goes out of scope.
 */
uint8_t edhoc_get_sign(cbor_reader_state_t *reader, uint8_t **sign);

/**
 * \brief Parse Additional Data (AD) from CBOR reader
 * \param reader input CBOR reader state positioned at AD data
 * \param ad output buffer to store parsed AD
 * \param ad_buffer_size input size of ad buffer
 * \return AD size in bytes on success, 0 on failure
 *
 * Reads Additional Data from CBOR stream and copies it to the provided buffer.
 * Validates that the AD size does not exceed buffer or protocol limits.
 */
uint8_t edhoc_get_ad(cbor_reader_state_t *reader, uint8_t *ad, size_t ad_buffer_size);

/**
 * \brief Process and validate EAD (External Authorization Data) items
 * \param ead_data input EAD data structure containing label and value
 * \return EDHOC_SUCCESS on success, EDHOC_ERR_CRITICAL_EAD_UNSUPPORTED if critical EAD cannot be processed, other error code on failure
 *
 * Processes EAD items according to RFC 9528 Section 6:
 * - Critical EAD items (negative ead_label) that cannot be processed trigger an error
 * - Non-critical EAD items (non-negative ead_label) can be ignored
 * Applications should implement custom handlers for recognized EAD labels.
 */
edhoc_error_t edhoc_process_ead_item(const edhoc_ead_data_t *ead_data);

/**
 * \brief Write EDHOC byte identifier according to RFC 9528 rules
 * \param state output CBOR writer state
 * \param bytes input identifier bytes to write
 * \param len input length of identifier
 *
 * Writes byte identifiers following RFC 9528 optimization: single-byte
 * identifiers that coincide with CBOR integer encoding (0x00-0x17, 0x20-0x37)
 * are written as integers, others as byte strings.
 */
void edhoc_write_byte_identifier(cbor_writer_state_t *state, const uint8_t *bytes, size_t len);

/**
 * \brief Read byte identifier from CBOR reader
 *
 * Reads byte identifiers following RFC 9528 optimization. Returns pointer to
 * the identifier bytes and sets size to the length.
 */
const uint8_t *edhoc_read_byte_identifier(cbor_reader_state_t *reader, size_t *size);


#endif
