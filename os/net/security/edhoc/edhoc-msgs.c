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
 *         EDHOC message serialization and parsing implementation using the CBOR library
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund, Marco Tiloca, Niclas Finne <niclas.finne@ri.se>, Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */
#include "contiki-lib.h"
#include "edhoc-msgs.h"
#include "lib/cbor.h"
#include <assert.h>
#include <inttypes.h>

#include "sys/log.h"
#define LOG_MODULE "EDHOC"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*---------------------------------------------------------------------------*/
void
print_msg_1(edhoc_msg_1_t *msg)
{
  LOG_DBG("Type: %d\n", msg->method);
  LOG_DBG("Suite I: ");
  LOG_DBG_BYTES(msg->suites_i, msg->suites_i_num);
  LOG_DBG_("\n");
  LOG_DBG("Gx: ");
  LOG_DBG_BYTES(msg->g_x, ECC_KEY_LEN);
  LOG_DBG_("\n");
  LOG_DBG("Ci: ");
  LOG_DBG_BYTES(msg->c_i, msg->c_i_sz);
  LOG_DBG_("\n");
  LOG_DBG("EAD (label: %" PRId32 "): ", msg->uad.ead_label);
  LOG_DBG_BYTES(msg->uad.ead_value, msg->uad.ead_value_sz);
  LOG_DBG_("\n");
}
/*---------------------------------------------------------------------------*/
void
print_msg_2(edhoc_msg_2_t *msg)
{
  LOG_DBG("gy_ciphertext_2: ");
  LOG_DBG_BYTES(msg->gy_ciphertext_2, msg->gy_ciphertext_2_sz);
  LOG_DBG_("\n");
}
/*---------------------------------------------------------------------------*/
void
print_msg_3(edhoc_msg_3_t *msg)
{
  LOG_DBG("CIPHERTEXT_3: ");
  LOG_DBG_BYTES(msg->ciphertext_3, msg->ciphertext_3_sz);
  LOG_DBG_("\n");
}
/*---------------------------------------------------------------------------*/
const uint8_t *
edhoc_read_byte_identifier(cbor_reader_state_t *reader, size_t *size)
{
  cbor_major_type_t next = cbor_peek_next(reader);
  if(next == CBOR_MAJOR_TYPE_BYTE_STRING) {
    return cbor_read_data(reader, size);
  }
  /* RFC 9528: Single-byte identifiers (0x00-0x17, 0x20-0x37) are encoded as
   * CBOR integers. These correspond to:
   * - 0x00-0x17: CBOR major type 0 (unsigned), values 0-23
   * - 0x20-0x37: CBOR major type 1 (signed), values -1 to -24
   * We use cbor_read_signed() because it handles both major types, returning
   * the raw byte position (not the decoded integer value). */
  const uint8_t *out = cbor_get_position(reader);
  int64_t value;
  if(cbor_read_signed(reader, &value) == CBOR_SIZE_1) {
    *size = 1;
    return out;
  }
  LOG_ERR("Unsupported CID: value outside ranges (0x00-0x17, 0x20-0x37)\n");
  return NULL;
}
/*---------------------------------------------------------------------------*/
void
edhoc_write_byte_identifier(cbor_writer_state_t *state, const uint8_t *bytes,
                            size_t byte_length)
{
  /* RFC 9528: The byte strings that coincide with a one-byte CBOR encoding
     of an integer be represented by the CBOR encoding of that integer.
     Other byte strings are simply encoded as CBOR byte strings.

     Check if the byte is in the range 0x00 to 0x17 (positive integers 0 to
     23) or in the range 0x20 to 0x37 (negative integers -1 to -24). */
  if(byte_length == 1 && ((*bytes <= 0x17) || (*bytes >= 0x20 && *bytes <= 0x37))) {
    /* RFC 9528: Single-byte identifiers that coincide with CBOR integer encoding
       should be represented as CBOR integers rather than byte strings */
    cbor_write_object(state, bytes, 1);
  } else {
    cbor_write_data(state, bytes, byte_length);
  }
}
/*---------------------------------------------------------------------------*/
static void
edhoc_write_suites(cbor_writer_state_t *state, const uint8_t *cipher_suites,
                   size_t suite_count)
{
  if(suite_count == 1) {
    return cbor_write_unsigned(state, cipher_suites[0]);
  }
  cbor_open_array(state);
  for(int idx = 0; idx < suite_count; idx++) {
    cbor_write_unsigned(state, cipher_suites[idx]);
  }
  cbor_close_array(state);
}
/*---------------------------------------------------------------------------*/
static bool
edhoc_deserialize_suites(cbor_reader_state_t *reader, uint8_t *suites_buffer,
                         size_t buffer_size, uint8_t *suite_count)
{
  if(!suites_buffer || !buffer_size) {
    return false;
  }
  size_t suite_array_size = cbor_read_array(reader);
  if(suite_array_size < SIZE_MAX) {
    if(suite_array_size > buffer_size) {
      /* Too many suites - Truncate */
      LOG_WARN("Too many suites (truncating): %zu/%zu\n", suite_array_size, buffer_size);
      suite_array_size = buffer_size;
    }
    *suite_count = suite_array_size;
    for(int i = 0; i < suite_array_size; i++) {
      uint64_t value;
      if(cbor_read_unsigned(reader, &value) != CBOR_SIZE_1) {
        /* Wrong size or type */
        return false;
      }
      suites_buffer[i] = (uint8_t)value;
    }
    return true;
  }

  uint64_t value;
  if(cbor_read_unsigned(reader, &value) == CBOR_SIZE_1) {
    suites_buffer[0] = (uint8_t)value;
    *suite_count = 1;
    return true;
  }

  return false;
}
/*---------------------------------------------------------------------------*/
size_t
edhoc_serialize_msg_1(edhoc_msg_1_t *msg, unsigned char *buffer,
                      size_t buffer_sz, bool suite_array)
{
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, buffer, buffer_sz);

  cbor_write_unsigned(&writer, msg->method);
  edhoc_write_suites(&writer, msg->suites_i, msg->suites_i_num);
  cbor_write_data(&writer, msg->g_x, ECC_KEY_LEN);
  /* byte identifier */
  edhoc_write_byte_identifier(&writer, msg->c_i, msg->c_i_sz);
  if(msg->uad.ead_value_sz > 0) {
    cbor_write_data(&writer, msg->uad.ead_value, msg->uad.ead_value_sz);
  }

  size_t cbor_sz = cbor_end_writer(&writer);
  if(!cbor_sz) {
    LOG_ERR("failed to serialize msg 1\n");
    return 0;
  }
  return cbor_sz;
}
/*---------------------------------------------------------------------------*/
size_t
edhoc_serialize_err(const edhoc_msg_error_t *msg, unsigned char *buffer,
                    size_t buffer_sz)
{
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, buffer, buffer_sz);
  cbor_write_unsigned(&writer, msg->err_code);
  switch(msg->err_code) {
  case EDHOC_MSG_ERR_CODE_UNSPECIFIED_ERROR:
    cbor_write_text(&writer, msg->info.tstr.err_info,
                    msg->info.tstr.err_info_sz);
    break;
  case EDHOC_MSG_ERR_CODE_WRONG_CIPHER_SUITE:
    edhoc_write_suites(&writer, msg->info.suites.suites,
                       msg->info.suites.suites_num);
    break;
  case EDHOC_MSG_ERR_CODE_UNKNOWN_CREDENTIAL_SELECTION:
    cbor_write_bool(&writer, true);
    break;
  default:
    LOG_ERR("edhoc_serialize_err: unhandled err code: %d\n", msg->err_code);
    return 0;
  }
  return cbor_end_writer(&writer);
}
/*---------------------------------------------------------------------------*/
bool
edhoc_deserialize_err(edhoc_msg_error_t *msg, const uint8_t *data,
                      size_t data_size)
{
  cbor_reader_state_t reader;
  cbor_init_reader(&reader, data, data_size);

  int64_t value;
  cbor_size_t sz = cbor_read_signed(&reader, &value);
  if(sz == CBOR_SIZE_NONE) {
    return false;
  }
  int32_t ret = (int32_t)value;

  memset(msg, 0, sizeof(*msg));
  msg->err_code = (uint8_t)ret;

  switch(msg->err_code) {
  case EDHOC_MSG_ERR_CODE_RESERVED_FOR_SUCCESS:
    /* RFC 9528: Error code 0 MUST NOT be used in EDHOC message exchange */
    return cbor_get_remaining(&reader) == 0;
  case EDHOC_MSG_ERR_CODE_UNSPECIFIED_ERROR:
    msg->info.tstr.err_info = cbor_read_text(&reader,
                                             &msg->info.tstr.err_info_sz);
    if(msg->info.tstr.err_info && msg->info.tstr.err_info_sz > 0
       && cbor_get_remaining(&reader) == 0) {
      /* Unspecified error message */
      return true;
    }
    return false;
  case EDHOC_MSG_ERR_CODE_WRONG_CIPHER_SUITE:
    if(edhoc_deserialize_suites(&reader, msg->info.suites.suites,
                                sizeof(msg->info.suites.suites),
                                &msg->info.suites.suites_num)
       && cbor_get_remaining(&reader) == 0) {
      /* Wrong cipher suite with proposed alternatives */
      return true;
    }
    return false;
  case EDHOC_MSG_ERR_CODE_UNKNOWN_CREDENTIAL_SELECTION: {
    /* RFC 9528: ERR_INFO should be true (boolean) for this error code */
    cbor_simple_value_t simple_value = cbor_read_simple(&reader);
    if(simple_value == CBOR_SIMPLE_VALUE_TRUE && cbor_get_remaining(&reader) == 0) {
      /* Unknown credential referenced by peer */
      return true;
    }
    return false;
  }
  case EDHOC_MSG_ERR_CODE_RESERVED:
    /* Reserved error code 23 */
    return cbor_get_remaining(&reader) == 0;
  default:
    /*
     * Error message must always start with a signed number followed by a CBOR object
     * (except for 0 (SUCCESS) and 23 (RESERVED)).
     */
    if(cbor_skip_next(&reader) && cbor_get_remaining(&reader) == 0) {
      /* RFC 9528: Valid error code ranges are -65536 to -25 and 1 to 65535 */
      if((ret < -65536) || (ret >= -24 && ret <= 0) || (ret > 65535)) {
        return false;
      }
      if(msg->err_code >= 4 && msg->err_code <= 22) {
        LOG_DBG("edhoc_deserialize_err: unassigned error code %d\n", msg->err_code);
      } else {
        LOG_DBG("edhoc_deserialize_err: unhandled error code %d\n", msg->err_code);
      }
      return true;
    }
    return false;
  }
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_deserialize_msg_1(edhoc_msg_1_t *msg, unsigned char *buffer, size_t buff_sz)
{
  cbor_reader_state_t reader;
  uint64_t value;
  cbor_init_reader(&reader, buffer, buff_sz);

  /* Get the METHOD */
  if(cbor_read_unsigned(&reader, &value) == CBOR_SIZE_1) {
    msg->method = (int8_t)value;
    LOG_INFO("msg1: method: %d\n", msg->method);
  }
  /* Get the suite */
  if(!edhoc_deserialize_suites(&reader, msg->suites_i, sizeof(msg->suites_i),
                               &msg->suites_i_num)) {
    LOG_ERR("Failed to deserialize cipher suites from MSG_1\n");
    return EDHOC_ERR_MSG_MALFORMED;
  }
  LOG_INFO("msg1: suites: %u\n", msg->suites_i_num);

  /* Get Gx */
  size_t data_size;
  const uint8_t *data = cbor_read_data(&reader, &data_size);
  if(!data || data_size != ECC_KEY_LEN) {
    LOG_ERR("Invalid G_X in MSG_1: expected %d bytes, got %zu\n", ECC_KEY_LEN, data_size);
    return EDHOC_ERR_MSG_MALFORMED;
  }
  msg->g_x = data;

  /* Get the session_id (Ci) */
  data = edhoc_read_byte_identifier(&reader, &data_size);
  if(!data || data_size > EDHOC_MAX_CID_LEN) {
    LOG_ERR("Invalid C_I in MSG_1: size %zu exceeds max %d\n", data_size, EDHOC_MAX_CID_LEN);
    return EDHOC_ERR_MSG_MALFORMED;
  }
  msg->c_i = data;
  msg->c_i_sz = data_size;

  /* Parse EAD (External Authorization Data) if present
   * RFC 9528: EAD is a CBOR sequence of (ead_label, ?ead_value) pairs */
  if(cbor_get_remaining(&reader) > 0) {
    /* Try to read EAD label as signed integer (for critical/negative labels) */
    int64_t signed_label;
    cbor_size_t sz = cbor_read_signed(&reader, &signed_label);
    if(sz != CBOR_SIZE_NONE) {
      /* Successfully read as signed integer */
      msg->uad.ead_label = (int32_t)signed_label;
    } else {
      LOG_DBG("MSG_1: No EAD label found, using legacy format\n");
      /* Fall back to legacy parsing - treat entire remaining data as EAD value */
      cbor_reader_state_t legacy_reader;
      cbor_init_reader(&legacy_reader, buffer, buff_sz);

      /* Skip to the EAD position by re-parsing */
      cbor_read_unsigned(&legacy_reader, &value); /* method */
      edhoc_deserialize_suites(&legacy_reader, msg->suites_i, sizeof(msg->suites_i), &msg->suites_i_num); /* suites */
      cbor_read_data(&legacy_reader, &data_size); /* g_x */
      edhoc_read_byte_identifier(&legacy_reader, &data_size); /* c_i */

      /* Read remaining as EAD value */
      data = cbor_read_data(&legacy_reader, &data_size);
      if(data && data_size) {
        msg->uad.ead_label = 0; /* Default label for legacy format */
        msg->uad.ead_value = data;
        msg->uad.ead_value_sz = data_size;
      }
      return EDHOC_SUCCESS;
    }

    /* Try to read optional EAD value */
    if(cbor_get_remaining(&reader) > 0) {
      const uint8_t *ead_value = cbor_read_data(&reader, &data_size);
      if(ead_value) {
        msg->uad.ead_value = ead_value;
        msg->uad.ead_value_sz = data_size;
      }
    } else {
      /* EAD item has label but no value */
      msg->uad.ead_value = NULL;
      msg->uad.ead_value_sz = 0;
    }

    LOG_DBG("MSG_1: Parsed EAD - label: %" PRId32 ", value_sz: %zu\n",
            msg->uad.ead_label, msg->uad.ead_value_sz);
  }

  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_deserialize_msg_2(edhoc_msg_2_t *msg, unsigned char *buffer, size_t buff_sz)
{
  if(!msg) {
    LOG_ERR("Invalid msg parameter for edhoc_deserialize_msg_2\n");
    return EDHOC_ERR_NULL_POINTER;
  }

  cbor_reader_state_t reader;
  cbor_init_reader(&reader, buffer, buff_sz);

  size_t data_size;
  const uint8_t *data = cbor_read_data(&reader, &data_size);
  if(!data) {
    LOG_ERR("Failed to read MSG_2 data\n");
    return EDHOC_ERR_MSG_MALFORMED;
  }

  /* Validate MSG_2 structure: must contain G_Y (32 bytes) + CIPHERTEXT_2 (at least 1 byte) */
  if(data_size < ECC_KEY_LEN + 1) {
    LOG_ERR("MSG_2 size (%zu) too small (minimum %d bytes)\n",
            data_size, ECC_KEY_LEN + 1);
    return EDHOC_ERR_MSG_MALFORMED;
  }

  /* Validate total size doesn't exceed maximum payload */
  if(data_size > EDHOC_MAX_PAYLOAD_LEN) {
    LOG_ERR("MSG_2 size (%zu) exceeds maximum payload length (%d)\n",
            data_size, EDHOC_MAX_PAYLOAD_LEN);
    return EDHOC_ERR_MSG_MALFORMED;
  }

  msg->gy_ciphertext_2 = (uint8_t *)data;
  msg->gy_ciphertext_2_sz = data_size;
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_deserialize_msg_3(edhoc_msg_3_t *msg, unsigned char *buffer, size_t buff_sz)
{
  if(!msg) {
    LOG_ERR("Invalid msg parameter for edhoc_deserialize_msg_3\n");
    return EDHOC_ERR_NULL_POINTER;
  }

  cbor_reader_state_t reader;
  cbor_init_reader(&reader, buffer, buff_sz);

  size_t data_size;
  const uint8_t *data = cbor_read_data(&reader, &data_size);
  if(!data) {
    LOG_ERR("Failed to read MSG_3 data\n");
    return EDHOC_ERR_MSG_MALFORMED;
  }

  /* Validate MSG_3 structure: CIPHERTEXT_3 must be at least MAC length */
  if(data_size < EDHOC_MAC_LEN_8) {
    LOG_ERR("MSG_3 size (%zu) too small (minimum %d bytes)\n",
            data_size, EDHOC_MAC_LEN_8);
    return EDHOC_ERR_MSG_MALFORMED;
  }

  /* Validate total size doesn't exceed maximum payload */
  if(data_size > EDHOC_MAX_PAYLOAD_LEN) {
    LOG_ERR("MSG_3 size (%zu) exceeds maximum payload length (%d)\n",
            data_size, EDHOC_MAX_PAYLOAD_LEN);
    return EDHOC_ERR_MSG_MALFORMED;
  }

  msg->ciphertext_3 = (uint8_t *)data;
  msg->ciphertext_3_sz = data_size;
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_get_auth_key_from_kid(uint8_t *kid, uint8_t kid_sz, cose_key_t **key)
{
  LOG_DBG("Looking for auth key with KID ");
  LOG_DBG_BYTES(kid, kid_sz);
  LOG_DBG_(" (sz=%d)\n", kid_sz);

  cose_key_t *auth_key;
  edhoc_error_t result = edhoc_check_key_list_kid(kid, kid_sz, &auth_key);
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Auth key ID not found. Searched for KID ");
    LOG_ERR_BYTES(kid, kid_sz);
    LOG_ERR_(" (sz=%d)\n", kid_sz);
    return result;
  }
  LOG_DBG("Auth key found with KID ");
  LOG_DBG_BYTES(kid, kid_sz);
  LOG_DBG_("\n");
  *key = auth_key;
  return EDHOC_SUCCESS;
}
static edhoc_error_t
parse_compact_encoding(cbor_reader_state_t *reader, cose_key_t *key)
{
  if(cbor_get_remaining(reader) < 1) {
    LOG_ERR("Insufficient data for compact encoding\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }

  const uint8_t *current_pos = cbor_get_position(reader);
  key->kid[0] = *current_pos;

  /* Compact encoding uses CBOR integers (both unsigned 0x00-0x17 and signed
   * 0x20-0x37). Use cbor_read_signed() which handles both major types. */
  int64_t value;
  if(cbor_read_signed(reader, &value) == CBOR_SIZE_NONE) {
    LOG_ERR("Failed to read compact encoding as integer\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }
  key->kid_sz = 1;

  cose_key_t *hkey = NULL;
  edhoc_error_t result = edhoc_get_auth_key_from_kid(key->kid, key->kid_sz, &hkey);
  if(result != EDHOC_SUCCESS) {
    return result;
  }

  memcpy(key, hkey, sizeof(cose_key_t));
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
static edhoc_error_t
parse_kid_map(cbor_reader_state_t *reader, cose_key_t *key)
{
  size_t data_size;
  const uint8_t *data = cbor_read_data(reader, &data_size);
  if(!data || data_size == 0) {
    LOG_ERR("Failed to read key ID data from CBOR map in ID_CRED_X\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }

  /* Validate key ID size - this is protocol-specific validation */
  if(data_size > sizeof(key->kid)) {
    LOG_ERR("Key ID size (%zu) exceeds maximum (%zu)\n", data_size, sizeof(key->kid));
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }

  cose_key_t *hkey = NULL;
  edhoc_error_t result = edhoc_get_auth_key_from_kid((uint8_t *)data, data_size, &hkey);
  if(result != EDHOC_SUCCESS) {
    return result;
  }

  memcpy(key, hkey, sizeof(cose_key_t));
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
static edhoc_error_t
parse_full_credential(cbor_reader_state_t *reader, cose_key_t *key)
{
  LOG_DBG("**** ID_CRED_R = CRED_R");

  /* Parse key type */
  uint64_t kty_val;
  if(cbor_read_unsigned(reader, &kty_val) == CBOR_SIZE_NONE) {
    LOG_ERR("Failed to read key type\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }
  key->kty = (uint8_t)kty_val;

  /* Parse curve parameter (-1) */
  int64_t neg_val;
  if(cbor_read_signed(reader, &neg_val) == CBOR_SIZE_NONE || neg_val != -1) {
    LOG_ERR("Expected curve parameter (-1)\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }

  uint64_t crv_val;
  if(cbor_read_unsigned(reader, &crv_val) == CBOR_SIZE_NONE) {
    LOG_ERR("Failed to read curve value\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }
  key->crv = (uint8_t)crv_val;

  /* Parse x coordinate (-2) */
  if(cbor_read_signed(reader, &neg_val) == CBOR_SIZE_NONE || neg_val != -2) {
    LOG_ERR("Expected x coordinate parameter (-2)\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }

  size_t x_size;
  const uint8_t *x_data = cbor_read_data(reader, &x_size);
  if(!x_data || x_size != ECC_KEY_LEN) {
    LOG_ERR("Invalid x coord: expected %d bytes, got %zu\n", ECC_KEY_LEN, x_size);
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }
  memcpy(key->ecc.pub.x, x_data, ECC_KEY_LEN);

  /* Parse y coordinate (-3) */
  if(cbor_read_signed(reader, &neg_val) == CBOR_SIZE_NONE || neg_val != -3) {
    LOG_ERR("Expected y coordinate parameter (-3)\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }

  size_t y_size;
  const uint8_t *y_data = cbor_read_data(reader, &y_size);
  if(!y_data || y_size != ECC_KEY_LEN) {
    LOG_ERR("Invalid y coord: expected %d bytes, got %zu\n", ECC_KEY_LEN, y_size);
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }
  memcpy(key->ecc.pub.y, y_data, ECC_KEY_LEN);

  /* Parse optional identity */
  size_t identity_sz;
  const char *identity_data = cbor_read_text(reader, &identity_sz);
  if(identity_data && identity_sz > 0) {
    /* Validate identity size */
    if(identity_sz > sizeof(key->identity)) {
      LOG_ERR("Identity size (%zu) exceeds maximum (%zu)\n", identity_sz, sizeof(key->identity));
      return EDHOC_ERR_ID_CRED_MALFORMED;
    }
    key->identity_sz = identity_sz;
    memcpy(key->identity, identity_data, identity_sz);
  }

  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
static uint8_t
determine_credential_label(cbor_reader_state_t *reader)
{
  cbor_major_type_t next = cbor_peek_next(reader);
  if(next == CBOR_MAJOR_TYPE_MAP) {
    size_t map_entries = cbor_read_map(reader);
    if(map_entries != SIZE_MAX && map_entries > 0) {
      uint64_t label_val;
      if(cbor_read_unsigned(reader, &label_val) != CBOR_SIZE_NONE) {
        return (uint8_t)label_val;
      }
    }
  }
  /* If not a map, it's compact encoding */
  return COSE_HEADER_LABEL_COMPACT_ENCODING;
}
/*---------------------------------------------------------------------------*/
static int8_t
copy_id_cred_x_to_output(const uint8_t *start, const uint8_t *end,
                        uint8_t *out_id_cred_x, size_t id_cred_x_buffer_size,
                        cose_key_t *key)
{
  uint16_t id_cred_x_sz = end - start;

  if(!out_id_cred_x) {
    return id_cred_x_sz;
  }

  /* Validate output buffer size */
  if(id_cred_x_buffer_size == 0) {
    LOG_ERR("Output buffer size is zero\n");
    return EDHOC_ERR_BUFFER_OVERFLOW;
  }

  if(id_cred_x_sz > id_cred_x_buffer_size) {
    LOG_ERR("ID_CRED_X size (%u) exceeds output buffer size (%zu)\n",
            id_cred_x_sz, id_cred_x_buffer_size);
    return EDHOC_ERR_BUFFER_OVERFLOW;
  }

  memcpy(out_id_cred_x, start, id_cred_x_sz);

  /* Rebuild from compact encoding if needed */
  if(id_cred_x_sz == 1) {
    cbor_writer_state_t writer;
    cbor_init_writer(&writer, out_id_cred_x, id_cred_x_buffer_size);
    cbor_open_map(&writer);
    cbor_write_unsigned(&writer, 4);
    cbor_write_data(&writer, key->kid, 1);
    cbor_close_map(&writer);
    id_cred_x_sz = cbor_end_writer(&writer);
    if(!id_cred_x_sz) {
      LOG_ERR("Failed to rebuild compact encoding\n");
      return EDHOC_ERR_ID_CRED_MALFORMED;
    }
  }

  return id_cred_x_sz;
}
/*---------------------------------------------------------------------------*/
int8_t
edhoc_get_key_id_cred_x(cbor_reader_state_t *reader, uint8_t *out_id_cred_x,
                       size_t id_cred_x_buffer_size, cose_key_t *key)
{
  if(!reader || !key) {
    LOG_ERR("Invalid parameters for edhoc_get_key_id_cred_x\n");
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }

  const uint8_t *start = cbor_get_position(reader);
  uint8_t label = determine_credential_label(reader);
  int8_t key_sz = 0;

  /* Parse credential based on encoding type */
  switch(label) {
  case COSE_HEADER_LABEL_COMPACT_ENCODING:
    key_sz = parse_compact_encoding(reader, key);
    break;
  case COSE_HEADER_LABEL_KID:
    key_sz = parse_kid_map(reader, key);
    break;
  case COSE_HEADER_LABEL_CRED_FULL:
    key_sz = parse_full_credential(reader, key);
    break;
  default:
    LOG_ERR("Unknown credential label %d\n", label);
    return EDHOC_ERR_ID_CRED_MALFORMED;
  }

  if(EDHOC_FAILED(key_sz)) {
    LOG_ERR("Failed to parse credential: %d\n", key_sz);
    return key_sz;  /* Return the error from parse function */
  }

  const uint8_t *end = cbor_get_position(reader);
  return copy_id_cred_x_to_output(start, end, out_id_cred_x,
                                 id_cred_x_buffer_size, key);
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_get_sign(cbor_reader_state_t *reader, uint8_t **sign)
{
  if(!reader || !sign) {
    LOG_ERR("Invalid parameters for edhoc_get_sign\n");
    return 0;
  }

  size_t sign_sz;
  const uint8_t *data = cbor_read_data(reader, &sign_sz);
  if(!data) {
    LOG_ERR("Failed to read signature data\n");
    return 0;
  }

  /* Validate signature size against known signature lengths */
  if(sign_sz > P384_SIGNATURE_LEN) {
    LOG_ERR("Signature size (%zu) exceeds maximum allowed (%d)\n", sign_sz, P384_SIGNATURE_LEN);
    return 0;
  }

  *sign = (uint8_t *)data;
  return sign_sz;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_get_ad(cbor_reader_state_t *reader, uint8_t *ad, size_t ad_buffer_size)
{
  if(!reader || !ad || ad_buffer_size == 0) {
    LOG_ERR("Invalid parameters for edhoc_get_ad\n");
    return 0;
  }

  size_t ad_sz;
  const uint8_t *data = cbor_read_data(reader, &ad_sz);
  if(!data) {
    LOG_ERR("Failed to read AD data\n");
    return 0;
  }

  /* Bounds checking to prevent buffer overflow */
  if(ad_sz > ad_buffer_size) {
    LOG_ERR("AD size (%zu) exceeds buffer size (%zu)\n", ad_sz, ad_buffer_size);
    return 0;
  }

  /* Validate AD size against maximum allowed */
  if(ad_sz > EDHOC_MAX_AD_SZ) {
    LOG_ERR("AD size (%zu) exceeds maximum allowed (%d)\n", ad_sz, EDHOC_MAX_AD_SZ);
    return 0;
  }

  memcpy(ad, data, ad_sz);
  return ad_sz;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_process_ead_item(const edhoc_ead_data_t *ead_data)
{
  if(!ead_data) {
    return EDHOC_ERR_NULL_POINTER;
  }

  /* RFC 9528: EAD label 0 is reserved for padding and can be ignored */
  if(ead_data->ead_label == 0) {
    LOG_DBG("EAD: Ignoring padding item (label 0)\n");
    return EDHOC_SUCCESS;
  }

  /* RFC 9528: Criticality is determined by the sign of ead_label:
   * - Negative ead_label = critical EAD item
   * - Non-negative ead_label = non-critical EAD item */
  bool is_critical = (ead_data->ead_label < 0);

  LOG_DBG("EAD: Processing %s item with label %" PRId32 ", value_sz=%zu\n",
          is_critical ? "critical" : "non-critical",
          ead_data->ead_label, ead_data->ead_value_sz);

  /*
   * Application-specific EAD processing would go here.
   * For now, this is a minimal implementation that follows RFC 9528:
   *
   * Applications should implement handlers for recognized EAD labels.
   * Unrecognized critical EAD items must trigger an error.
   * Unrecognized non-critical EAD items can be ignored.
   */

  /* TODO: Add application-specific EAD handlers here based on ead_label */
  /* Example:
   * switch(ead_data->ead_label) {
   *   case MY_APP_EAD_LABEL:
   *     return handle_my_app_ead(ead_data);
   *   case -MY_CRITICAL_EAD_LABEL:
   *     return handle_critical_ead(ead_data);
   *   default:
   *     // Fall through to general handling below
   * }
   */

  if(is_critical) {
    /* RFC 9528 Section 6: If an endpoint receives a critical EAD item it
     * does not recognize or cannot process, it MUST send an EDHOC error
     * message and MUST abort the EDHOC session. */
    LOG_ERR("EAD: Critical EAD item with label %" PRId32 " cannot be processed\n",
            ead_data->ead_label);
    return EDHOC_ERR_CRITICAL_EAD_UNSUPPORTED;
  } else {
    /* Non-critical EAD items can be safely ignored */
    LOG_DBG("EAD: Ignoring non-critical item (label %" PRId32 ")\n",
            ead_data->ead_label);
    return EDHOC_SUCCESS;
  }
}
/*---------------------------------------------------------------------------*/
