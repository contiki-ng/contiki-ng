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
 *         EDHOC message handling functions.
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 *         Peter A Jonsson
 *         Rikard Höglund
 *         Marco Tiloca
 *         Niclas Finne <niclas.finne@ri.se>
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include <assert.h>

#include "edhoc-msg-handlers.h"
#include "cose.h"

#include "sys/log.h"
#define LOG_MODULE "EDHOC"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*---------------------------------------------------------------------------*/
static uint16_t
decrypt_ciphertext_3(edhoc_context_t *ctx, const uint8_t *ciphertext,
                     uint16_t ciphertext_size, uint8_t *plaintext)
{
  uint8_t alg = ctx->config.aead_alg;
  uint8_t key_len = cose_get_key_len(alg);
  uint8_t iv_len = cose_get_iv_len(alg);
  uint8_t tag_len = cose_get_tag_len(alg);
  if(key_len == 0 || iv_len == 0 || tag_len == 0) {
    return 0;
  }

  if(ciphertext_size > EDHOC_MAX_BUFFER) {
    LOG_ERR("CIPHERTEXT_3 size (%u) exceeds max buffer (%d)\n",
            (unsigned)ciphertext_size, EDHOC_MAX_BUFFER);
    return 0;
  }

  /* Derive K_3. */
  uint8_t key[COSE_MAX_KEY_LEN];
  int16_t err = edhoc_kdf(ctx->state.prk_3e2m, K_3_LABEL, ctx->state.th,
                          HASH_LEN, key_len, key);
  if(err < 1) {
    LOG_ERR("Failed to derive K_3\n");
    return 0;
  }
  LOG_DBG("K_3 (%d bytes): ", key_len);
  LOG_DBG_BYTES(key, key_len);
  LOG_DBG_("\n");

  /* Derive IV_3. */
  uint8_t nonce[COSE_MAX_IV_LEN];
  err = edhoc_kdf(ctx->state.prk_3e2m, IV_3_LABEL, ctx->state.th, HASH_LEN,
                  iv_len, nonce);
  if(err < 1) {
    LOG_ERR("Failed to derive IV_3\n");
    return 0;
  }
  LOG_DBG("IV_3 (%d bytes): ", iv_len);
  LOG_DBG_BYTES(nonce, iv_len);
  LOG_DBG_("\n");

  /* COSE_Encrypt0: AAD is TH_3, in-place decrypt. */
  uint8_t aead_buf[EDHOC_MAX_BUFFER];
  memcpy(aead_buf, ciphertext, ciphertext_size);
  size_t plaintext_sz = cose_encrypt0_open(alg, key, nonce,
                                           ctx->state.th, HASH_LEN,
                                           aead_buf, ciphertext_size);
  if(plaintext_sz == 0) {
    LOG_ERR("CIPHERTEXT_3 decryption failed\n");
    return 0;
  }

  memcpy(plaintext, aead_buf, plaintext_sz);
  return (uint16_t)plaintext_sz;
}
/*---------------------------------------------------------------------------*/
static edhoc_error_t
set_rx_cid(edhoc_context_t *ctx, const uint8_t *received_cid, size_t received_cid_size)
{
  /* set connection id from rx */
  if(received_cid_size == 0 || received_cid_size > EDHOC_MAX_CID_LEN) {
    LOG_ERR("Invalid CID length: %zu (max %d)\n", received_cid_size, EDHOC_MAX_CID_LEN);
    return EDHOC_ERR_CID_INVALID;
  }

  memcpy(ctx->state.cid_rx, received_cid, received_cid_size);
  ctx->state.cid_rx_len = (uint8_t)received_cid_size;

  /* Check if received CID is same as own CID */
  if(ctx->state.cid_rx_len == ctx->state.cid_len &&
     memcmp(ctx->state.cid_rx, ctx->state.cid, ctx->state.cid_len) == 0) {
    LOG_ERR("Received CID matches own CID (error: %d)\n", EDHOC_ERR_CID_INVALID);
    return EDHOC_ERR_CID_INVALID;
  }
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
static int8_t
check_rx_suite_i(edhoc_context_t *ctx,
                 const uint8_t *received_suites, uint8_t suite_count)
{
  if(suite_count == 0) {
    return EDHOC_ERR_SUITE_NOT_SUPPORTED;
  }
  /* Get the selected cipher suite (last element) */
  uint8_t peer_selected_suite = received_suites[suite_count - 1];

  /* Check if the selected suite is supported */
  for(uint8_t i = 0; i < ctx->config.suite_num; i++) {
    if(ctx->config.suite[i] == peer_selected_suite) {
      ctx->state.suite_selected = peer_selected_suite;
      LOG_DBG("Selected cipher suite: %d\n", ctx->state.suite_selected);

      /* Responder sets config to use based on selected suite */
      int8_t err = edhoc_set_config_from_suite(ctx, ctx->state.suite_selected);
      if(err != 1) {
        LOG_WARN("Cipher suite not supported\n");
        return EDHOC_ERR_SUITE_NOT_SUPPORTED;
      }
      return 0;
    }
  }

  LOG_WARN("Cipher suite not supported\n");
  return EDHOC_ERR_SUITE_NOT_SUPPORTED;
}
/*---------------------------------------------------------------------------*/
static void
set_rx_gx(edhoc_context_t *ctx, const uint8_t *gx)
{
  memcpy(ctx->state.gx, gx, ECC_KEY_LEN);
}
/*---------------------------------------------------------------------------*/
static int8_t
set_rx_method(edhoc_context_t *ctx, uint8_t method)
{
  if(method != EDHOC_METHOD) {
    LOG_ERR("error code (%d)\n", EDHOC_ERR_METHOD_NOT_SUPPORTED);
    return EDHOC_ERR_METHOD_NOT_SUPPORTED;
  }
  ctx->config.method = method;
  return 0;
}
/*---------------------------------------------------------------------------*/
static int8_t
set_rx_msg(edhoc_context_t *ctx, const uint8_t *message, size_t message_size)
{
  if(message_size > EDHOC_MAX_PAYLOAD_LEN) {
    LOG_ERR("Message size (%zu) exceeds max payload (%d)\n", message_size, EDHOC_MAX_PAYLOAD_LEN);
    return EDHOC_ERR_MSG_MALFORMED;
  }
  memcpy(ctx->buffers.msg_rx, message, message_size);
  ctx->buffers.rx_sz = message_size;
  return 0;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_check_err_rx_msg(uint8_t message, const uint8_t *data, size_t data_size)
{
  /* Check if the rx msg is an msg_err */
  edhoc_msg_error_t err;

  /* If deserialize_err succeeds, this IS an error message */
  if(edhoc_deserialize_err(&err, data, data_size)) {
    switch(edhoc_msg_error_get_code(&err)) {
    case EDHOC_MSG_ERR_CODE_UNSPECIFIED_ERROR:
      LOG_ERR("RX MSG %u ERR %u: ", message, edhoc_msg_error_get_code(&err));
      size_t info_sz = edhoc_msg_error_get_info_sz(&err);
      if(info_sz) {
        LOG_ERR_(": ");
        LOG_ERR_STRING(edhoc_msg_error_get_info(&err), info_sz);
      }
      LOG_ERR_("\n");
      return EDHOC_ERR_MSG_MALFORMED;  /* Return negative to indicate error message detected */
    case EDHOC_MSG_ERR_CODE_WRONG_CIPHER_SUITE:
      LOG_ERR("RX MSG %u ERROR WITH SUITE PROPOSE: ", message);
      LOG_ERR_BYTES(edhoc_msg_error_get_suites(&err),
                    edhoc_msg_error_get_suites_num(&err));
      LOG_ERR_("\n");
      return EDHOC_ERR_SUITE_NOT_SUPPORTED;  /* Return negative to indicate error message detected */
    case EDHOC_MSG_ERR_CODE_UNKNOWN_CREDENTIAL_SELECTION:
      LOG_ERR("RX MSG %u ERROR: Unknown credential referenced\n", message);
      return EDHOC_ERR_CREDENTIAL_NOT_FOUND;  /* Return negative to indicate error message detected */
    default:
      return EDHOC_ERR_MSG_MALFORMED;
    }
  }

  /* If deserialize_err fails, this is NOT an error message - continue normal parsing */
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
int
edhoc_handler_msg_1(edhoc_context_t *ctx, uint8_t *payload,
                    size_t payload_size, uint8_t *auth_data)
{
  edhoc_msg_1_t msg1 = { 0 };
  int err = 0;

  /* Decode MSG1 */
  err = set_rx_msg(ctx, payload, payload_size);
  if(err < 0) {
    return err;
  }

  /* Check if the rx msg is an msg_err */
  edhoc_error_t error_status = edhoc_check_err_rx_msg(1, payload, payload_size);
  if(error_status != EDHOC_SUCCESS) {
    return error_status;
  }

  LOG_DBG("MSG1 (%d bytes): ", (int)ctx->buffers.rx_sz - 1);
  LOG_DBG_BYTES((ctx->buffers.msg_rx) + 1, ctx->buffers.rx_sz - 1);
  LOG_DBG_("\n");
  err = edhoc_deserialize_msg_1(&msg1, (ctx->buffers.msg_rx) + 1,
                               ctx->buffers.rx_sz - 1);
  if(err < 0) {
    LOG_ERR("MSG1 malformed\n");
    return err;
  }
  print_msg_1(&msg1);

#if EDHOC_EAD_PROCESSING
  /* Process EAD (External Authorization Data) items if present
   * RFC 9528 Section 6: Critical EAD items that cannot be processed must trigger an error */
  if(msg1.uad.ead_value_sz > 0 || msg1.uad.ead_label != 0) {
    edhoc_error_t ead_result = edhoc_process_ead_item(&msg1.uad);
    if(ead_result != EDHOC_SUCCESS) {
      LOG_ERR("EAD processing failed for MSG_1: %d\n", ead_result);
      return ead_result;
    }
  }
#endif /* EDHOC_EAD_PROCESSING */

  /* check rx suite and set connection identifier of the other peer */
  err = check_rx_suite_i(ctx, msg1.suites_i, msg1.suites_i_num);
  if(err < 0) {
    LOG_ERR("Rx Suite not supported\n");
    return err;
  }

  /* Check to not have the same cid */
  err = set_rx_cid(ctx, msg1.c_i, msg1.c_i_sz);
  if(err < 0) {
    LOG_ERR("Not support cid rx\n");
    return err;
  }

  /* Set EDHOC method */
  err = set_rx_method(ctx, msg1.method);
  if(err < 0) {
    LOG_ERR("Rx Method not supported\n");
    return err;
  }

  /* Set GX */
  set_rx_gx(ctx, msg1.g_x);
  edhoc_print_session_info(ctx);

  LOG_DBG("MSG EAD (%d)", (int)msg1.uad.ead_value_sz);
  LOG_DBG_STRING((char *)msg1.uad.ead_value, msg1.uad.ead_value_sz);
  LOG_DBG_("\n");

  if(msg1.uad.ead_value_sz != 0) {
    if(msg1.uad.ead_value_sz > EDHOC_MAX_AD_SZ) {
      LOG_ERR("EAD value size (%zu) exceeds max AD (%d)\n",
              msg1.uad.ead_value_sz, EDHOC_MAX_AD_SZ);
      return EDHOC_ERR_MSG_MALFORMED;
    }
    memcpy(auth_data, msg1.uad.ead_value, msg1.uad.ead_value_sz);
  }

  return msg1.uad.ead_value_sz;
}
/*---------------------------------------------------------------------------*/
int
edhoc_handler_msg_2(edhoc_msg_2_t *msg2, edhoc_context_t *ctx,
                    uint8_t *payload, size_t payload_size)
{
  int err = 0;
  err = set_rx_msg(ctx, payload, payload_size);
  if(err < 0) {
    return err;
  }
  edhoc_error_t error_status = edhoc_check_err_rx_msg(2, payload, payload_size);
  if(error_status != EDHOC_SUCCESS) {
    return error_status;
  }
  err = edhoc_deserialize_msg_2(msg2, ctx->buffers.msg_rx, ctx->buffers.rx_sz);
  if(err < 0) {
    LOG_ERR("MSG2 malformed\n");
    return err;
  }
  print_msg_2(msg2);

  set_rx_gx(ctx, msg2->gy_ciphertext_2);
  edhoc_generate_transcript_hash_2(ctx, msg2->gy_ciphertext_2, ctx->buffers.msg_tx,
                ctx->buffers.tx_sz);
  edhoc_generate_prk_2e(ctx);

  /* Gen KS_2e */
  if(msg2->gy_ciphertext_2_sz <= ECC_KEY_LEN) {
    LOG_ERR("Invalid ciphertext size: %zu <= %d\n", msg2->gy_ciphertext_2_sz, ECC_KEY_LEN);
    return 0;
  }
  int ciphertext2_sz = msg2->gy_ciphertext_2_sz - ECC_KEY_LEN;
  if(ciphertext2_sz > EDHOC_MAX_BUFFER) {
    LOG_ERR("Ciphertext size exceeds maximum buffer: %d > %d\n", ciphertext2_sz, EDHOC_MAX_BUFFER);
    return 0;
  }
  uint8_t ks_2e[EDHOC_MAX_BUFFER];
  edhoc_generate_keystream_2e(ctx, ciphertext2_sz, ks_2e);

  /* Prepare ciphertext for decryption */
  memcpy(ctx->buffers.plaintext, msg2->gy_ciphertext_2 + ECC_KEY_LEN,
         ciphertext2_sz);
  LOG_DBG("CIPHERTEXT_2 (%d bytes): ", ciphertext2_sz);
  LOG_DBG_BYTES(ctx->buffers.plaintext, ciphertext2_sz);
  LOG_DBG_("\n");

  /* Actually decrypt the ciphertext */
  size_t plaint_sz = edhoc_enc_dec_ciphertext_2(ctx, ks_2e,
                                                ctx->buffers.plaintext,
                                                ciphertext2_sz);
  ctx->buffers.plaintext_sz = plaint_sz;
  LOG_DBG("PLAINTEXT_2 (%zu bytes): ", plaint_sz);
  LOG_DBG_BYTES(ctx->buffers.plaintext, plaint_sz);
  LOG_DBG_("\n");

  /* Parse C_R from beginning of plaintext_2 using CBOR */
  cbor_reader_state_t cid_reader;
  cbor_init_reader(&cid_reader, ctx->buffers.plaintext, ctx->buffers.plaintext_sz);
  size_t cid_data_size;
  const uint8_t *cid_data = edhoc_read_byte_identifier(&cid_reader, &cid_data_size);
  if(!cid_data || cid_data_size == 0 || cid_data_size > EDHOC_MAX_CID_LEN) {
    LOG_ERR("Invalid C_R in plaintext_2: size %zu\n", cid_data_size);
    return EDHOC_ERR_MSG_MALFORMED;
  }
  err = set_rx_cid(ctx, cid_data, cid_data_size);
  if(err < 0) {
    return err;
  }
  LOG_DBG("cid (%d bytes): ", ctx->state.cid_rx_len);
  LOG_DBG_BYTES(ctx->state.cid_rx, ctx->state.cid_rx_len);
  LOG_DBG_("\n");

  return 1;
}
/*---------------------------------------------------------------------------*/
int
edhoc_handler_msg_3(edhoc_msg_3_t *msg3, edhoc_context_t *ctx,
                    uint8_t *payload, size_t payload_size)
{
  /* Decode MSG3 */
  int8_t err = set_rx_msg(ctx, payload, payload_size);
  if(err < 0) {
    return err;
  }

  /* Check if the rx msg is an msg_err */
  edhoc_error_t error_status = edhoc_check_err_rx_msg(3, payload, payload_size);
  if(error_status != EDHOC_SUCCESS) {
    return error_status;
  }

  /* Skip the first byte (C_R connection identifier) to parse CIPHERTEXT_3 */
  err = edhoc_deserialize_msg_3(msg3, (ctx->buffers.msg_rx) + 1,
                              ctx->buffers.rx_sz - 1);
  if(err < 0) {
    LOG_ERR("MSG3 malformed\n");
    return err;
  }
  print_msg_3(msg3);

  LOG_DBG("CIPHERTEXT_3 (%d bytes): ", (int)msg3->ciphertext_3_sz);
  LOG_DBG_BYTES(msg3->ciphertext_3, msg3->ciphertext_3_sz);
  LOG_DBG_("\n");

  /* generate TH_3 */
  edhoc_generate_transcript_hash_3(ctx, ctx->buffers.cred_x, ctx->buffers.cred_x_sz,
                ctx->buffers.plaintext, ctx->buffers.plaintext_sz);

  /* decrypt msg3 and check the TAG for verify the outer */
  uint16_t plaintext_sz = decrypt_ciphertext_3(ctx, msg3->ciphertext_3,
                                               msg3->ciphertext_3_sz,
                                               ctx->buffers.plaintext);
  ctx->buffers.plaintext_sz = plaintext_sz;
  if(plaintext_sz == 0) {
    LOG_ERR("Error in decrypt ciphertext 3\n");
    return EDHOC_ERR_CRYPTO_DECRYPT;
  }
  LOG_DBG("PLAINTEXT_3 (%d): ", (int)plaintext_sz);
  LOG_DBG_BYTES(ctx->buffers.plaintext, plaintext_sz);
  LOG_DBG_("\n");

  return 1;
}
/*---------------------------------------------------------------------------*/
