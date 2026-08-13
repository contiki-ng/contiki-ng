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
 *         EDHOC, an implementation of Ephemeral Diffie-Hellman Over COSE (EDHOC) (IETF RFC9528)
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 *         Peter A Jonsson
 *         Rikard Höglund
 *         Marco Tiloca
 *         Niclas Finne <niclas.finne@ri.se>
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "lib/sha-256.h"
#include "lib/cbor.h"
#include "edhoc.h"
#include "edhoc-config.h"
#include "edhoc-msgs.h"
#include "edhoc-msg-generators.h"
#include "edhoc-error.h"
#include "cose.h"
#include <assert.h>

#include "sys/log.h"
#define LOG_MODULE "EDHOC"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*---------------------------------------------------------------------------*/
static edhoc_error_t
gen_mac(const edhoc_context_t *ctx, uint8_t mac_len, uint8_t *mac)
{
  if(ctx == NULL || mac == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  uint8_t mac_num;
  if(EDHOC_ROLE == EDHOC_INITIATOR) {
    mac_num = EDHOC_MAC_3;
  } else if(EDHOC_ROLE == EDHOC_RESPONDER) {
    mac_num = EDHOC_MAC_2;
  } else {
    return EDHOC_ERR_INVALID_STATE;
  }

  if(!edhoc_calc_mac(ctx, mac_num, mac_len, mac)) {
    LOG_ERR("Set MAC error\n");
    return EDHOC_ERR_CRYPTO_AUTHENTICATION;
  }

  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
static uint16_t
gen_plaintext(edhoc_context_t *ctx, const uint8_t *auth_data, size_t auth_data_size,
              bool msg2, const uint8_t *mac_or_signature,
              uint8_t mac_or_signature_size,
              uint8_t *plaintext_buffer, size_t plaintext_buffer_size)
{
  cbor_reader_state_t reader;
  cbor_writer_state_t writer;

  cbor_init_reader(&reader, ctx->buffers.id_cred_x, sizeof(ctx->buffers.id_cred_x));
  cbor_init_writer(&writer, plaintext_buffer, plaintext_buffer_size);
  if(msg2) {
    edhoc_write_byte_identifier(&writer, ctx->state.cid, ctx->state.cid_len);
  }

  size_t num = cbor_read_map(&reader);
  if(num == 1) {
    uint64_t value;
    cbor_read_unsigned(&reader, &value);
    const uint8_t *data = cbor_read_data(&reader, &num);
    if(!data || !num) {
      LOG_ERR("error to get bytes\n");
      return 0;
    }
    edhoc_write_byte_identifier(&writer, data, num);
  } else {
    cbor_write_object(&writer, ctx->buffers.id_cred_x, ctx->buffers.id_cred_x_sz);
  }

  cbor_write_data(&writer, mac_or_signature, mac_or_signature_size);
  if(auth_data_size != 0) {
    cbor_write_data(&writer, auth_data, auth_data_size);
  }

  size_t result = cbor_end_writer(&writer);
  if(result == 0) {
    LOG_ERR("CBOR encoding failed in plaintext_23 generation\n");
  }
  return result;
}
/*---------------------------------------------------------------------------*/
static uint16_t
gen_ciphertext_3(edhoc_context_t *ctx, const uint8_t *auth_data, uint16_t auth_data_size,
                 const uint8_t *mac_or_signature, uint16_t mac_or_signature_size,
                 uint8_t *ciphertext, size_t ciphertext_buffer_size)
{
  uint8_t alg = ctx->config.aead_alg;
  uint8_t key_len = cose_get_key_len(alg);
  uint8_t iv_len = cose_get_iv_len(alg);
  uint8_t tag_len = cose_get_tag_len(alg);
  if(key_len == 0 || iv_len == 0 || tag_len == 0) {
    return 0;
  }

  uint8_t aead_buf[EDHOC_MAX_BUFFER + COSE_MAX_TAG_LEN];
  uint16_t plaintext_sz = gen_plaintext(ctx, auth_data, auth_data_size, false,
                                        mac_or_signature, mac_or_signature_size,
                                        aead_buf, EDHOC_MAX_BUFFER);
  if(plaintext_sz == 0) {
    return 0;
  }
  LOG_DBG("PLAINTEXT_3 (%d bytes): ", (int)plaintext_sz);
  LOG_DBG_BYTES(aead_buf, plaintext_sz);
  LOG_DBG_("\n");

  /* Save plaintext_3 for TH_3. */
  memcpy(ctx->buffers.plaintext, aead_buf, plaintext_sz);
  ctx->buffers.plaintext_sz = plaintext_sz;

  /* Derive K_3. */
  uint8_t key[COSE_MAX_KEY_LEN];
  int16_t err = edhoc_kdf(ctx->state.prk_3e2m, K_3_LABEL, ctx->state.th,
                          HASH_LEN, key_len, key);
  if(err < 1) {
    LOG_ERR("Failed to derive K_3\n");
    return 0;
  }
  LOG_DBG("K_3 (%d bytes): ", (int)key_len);
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
  LOG_DBG("IV_3 (%d bytes): ", (int)iv_len);
  LOG_DBG_BYTES(nonce, iv_len);
  LOG_DBG_("\n");

  /* COSE_Encrypt0: AAD is TH_3, in-place encrypt. */
  size_t ciphertext_sz = cose_encrypt0_seal(alg, key, nonce,
                                            ctx->state.th, HASH_LEN,
                                            aead_buf, plaintext_sz,
                                            sizeof(aead_buf));
  if(ciphertext_sz == 0) {
    LOG_ERR("COSE_Encrypt0 encryption failed\n");
    return 0;
  }

  cbor_writer_state_t writer;
  cbor_init_writer(&writer, ciphertext, ciphertext_buffer_size);
  cbor_write_data(&writer, aead_buf, ciphertext_sz);

  size_t result = cbor_end_writer(&writer);
  if(result == 0) {
    LOG_ERR("CBOR encoding failed in ciphertext_3 generation\n");
  }
  return (uint16_t)result;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_generate_message_1(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz, bool suite_array)
{
  if(ctx == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  if(ad_sz > 0 && ad == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  if(ctx->config.suite_num == 0) {
    return EDHOC_ERR_SUITE_NOT_SUPPORTED;
  }

  /* Generate message 1 */
  edhoc_msg_1_t msg1 = {
    .method = ctx->config.method,
    .suites_i_num = MIN(EDHOC_SUITES_MAX_COUNT, ctx->config.suite_num),
    .g_x = (uint8_t *)&ctx->creds.ephemeral_key.pub.x,
    .c_i = ctx->state.cid,
    .c_i_sz = ctx->state.cid_len,
    .uad = { .ead_label = 0, .ead_value = ad, .ead_value_sz = ad_sz },
  };
  memcpy(msg1.suites_i, ctx->config.suite, msg1.suites_i_num * sizeof(msg1.suites_i[0]));

  /* CBOR encode message in the buffer */
  size_t size = edhoc_serialize_msg_1(&msg1, (ctx->buffers.msg_tx) + 1,
                                      EDHOC_MAX_PAYLOAD_LEN - 1, suite_array);
  if(!size) {
    /* Failed to generate message */
    return EDHOC_ERR_CBOR_ENCODING;
  }

  if(size >= EDHOC_MAX_PAYLOAD_LEN) {
    return EDHOC_ERR_BUFFER_TOO_SMALL;
  }

  ctx->buffers.tx_sz = size + 1;
  (ctx->buffers.msg_tx)[0] = 0xF5; /* Prepend CBOR true (0xF5) as per EDHOC spec */

  LOG_DBG("C_I chosen by Initiator (%d bytes): 0x", (int)msg1.c_i_sz);
  LOG_DBG_BYTES(msg1.c_i, msg1.c_i_sz);
  LOG_DBG_("\n");
  LOG_DBG("AD_1 (%d bytes): ", (int)ad_sz);
  LOG_DBG_STRING((char *)ad, ad_sz);
  LOG_DBG_("\n");
  for(int i = 0; i < msg1.suites_i_num; ++i) {
    LOG_DBG("SUITES_I[%d]: %d\n", i, (int)msg1.suites_i[i]);
  }

  LOG_DBG("message_1 (CBOR Sequence) (%d bytes): ", (int)ctx->buffers.tx_sz);
  LOG_DBG_BYTES(ctx->buffers.msg_tx, ctx->buffers.tx_sz);
  LOG_DBG_("\n");
  LOG_INFO("MSG1 sz: %d\n", (int)ctx->buffers.tx_sz);

  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_generate_message_2(edhoc_context_t *ctx, const uint8_t *auth_data, size_t auth_data_size)
{
  if(ctx == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  if(auth_data_size > 0 && auth_data == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  int8_t ret = edhoc_generate_transcript_hash_2(ctx, ctx->creds.ephemeral_key.pub.x,
                            ctx->buffers.msg_rx, ctx->buffers.rx_sz);
  if(ret < 0) {
    LOG_ERR("Failed to generate TH_2 (%d)\n", ret);
    return EDHOC_ERR_CRYPTO_HASH;
  }

  /* generate cred_x and id_cred_x */
  ctx->buffers.cred_x_sz =
    edhoc_generate_cred_x(ctx->creds.authen_key,
                          ctx->buffers.cred_x,
                          sizeof(ctx->buffers.cred_x));
  edhoc_print_credential("CRED_R", ctx->buffers.cred_x, ctx->buffers.cred_x_sz);

  ctx->buffers.id_cred_x_sz =
    edhoc_generate_id_cred_x(ctx->creds.authen_key,
                             ctx->buffers.id_cred_x,
                             sizeof(ctx->buffers.id_cred_x));
  LOG_DBG("ID_CRED_R (%d bytes): ", (int)ctx->buffers.id_cred_x_sz);
  LOG_DBG_BYTES(ctx->buffers.id_cred_x, ctx->buffers.id_cred_x_sz);
  LOG_DBG_("\n");

  edhoc_generate_prk_2e(ctx);

  uint8_t mac_or_signature_sz = -1;

  /* Generate MAC or Signature */

#if (EDHOC_METHOD == EDHOC_METHOD1) || (EDHOC_METHOD == EDHOC_METHOD3)
  /* generate prk_3e2m */
  edhoc_generate_prk_3e2m(ctx, &ctx->creds.authen_key->ecc, 1);

  uint8_t edhoc_mac_len = ctx->config.mac_len;
  uint8_t mac_or_sig[edhoc_mac_len];
  edhoc_error_t mac_result = gen_mac(ctx, edhoc_mac_len, mac_or_sig);
  if(mac_result != EDHOC_SUCCESS) {
    return mac_result;
  }
  LOG_DBG("MAC_2 (%d bytes): ", edhoc_mac_len);
  LOG_DBG_BYTES(mac_or_sig, edhoc_mac_len);
  LOG_DBG_("\n");
  mac_or_signature_sz = edhoc_mac_len;
#endif

#if (EDHOC_METHOD == EDHOC_METHOD0) || (EDHOC_METHOD == EDHOC_METHOD2)

  /* prk_3e2m is prk_2e */
  memcpy(ctx->state.prk_3e2m, ctx->state.prk_2e, HASH_LEN);

  /* Derive MAC with HASH_LEN size (buf fits later signature) */
  uint8_t mac_or_sig[EDHOC_MAC_OR_SIG_BUF_LEN];
  edhoc_error_t mac_result = gen_mac(ctx, HASH_LEN, mac_or_sig);
  if(mac_result != EDHOC_SUCCESS) {
    return mac_result;
  }
  LOG_DBG("MAC_2 (%d bytes): ", HASH_LEN);
  LOG_DBG_BYTES(mac_or_sig, HASH_LEN);
  LOG_DBG_("\n");

  /* Create signature from MAC and other data using COSE_Sign1. */

  /* External AAD (TH_2, CRED_R, ? EAD_2) */
  uint8_t external_aad[HASH_LEN + EDHOC_MAX_CRED_LEN];
  cbor_writer_state_t writer_aad;
  cbor_init_writer(&writer_aad, external_aad, sizeof(external_aad));
  cbor_write_data(&writer_aad, ctx->state.th, HASH_LEN);
  cbor_write_object(&writer_aad, ctx->buffers.cred_x, ctx->buffers.cred_x_sz);
  size_t external_aad_sz = cbor_end_writer(&writer_aad);
  if(external_aad_sz == 0) {
    LOG_ERR("Failed to encode external AAD for COSE_Sign1\n");
    return EDHOC_ERR_CRYPTO_SIGN;
  }

  size_t sig_sz = cose_sign1_sign(ctx->config.sign_alg,
                                  ctx->creds.authen_key->ecc.priv,
                                  ctx->buffers.id_cred_x,
                                  ctx->buffers.id_cred_x_sz,
                                  external_aad, external_aad_sz,
                                  mac_or_sig, HASH_LEN,
                                  mac_or_sig);
  if(sig_sz == 0) {
    LOG_ERR("COSE_Sign1 signing failed\n");
    return EDHOC_ERR_CRYPTO_SIGN;
  }

  LOG_DBG("Signature from COSE_Sign1 (%zu bytes): ", sig_sz);
  LOG_DBG_BYTES(mac_or_sig, sig_sz);
  LOG_DBG_("\n");

  mac_or_signature_sz = sig_sz;
#endif

  /* Generate and store the plaintext in the session */
  uint16_t plaint_sz = gen_plaintext(ctx, auth_data, auth_data_size, true, mac_or_sig,
                                     mac_or_signature_sz,
                                     ctx->buffers.plaintext,
                                     sizeof(ctx->buffers.plaintext));
  LOG_DBG("PLAINTEXT_2 (%d bytes): ", (int)plaint_sz);
  LOG_DBG_BYTES(ctx->buffers.plaintext, plaint_sz);
  LOG_DBG_("\n");
  if(plaint_sz == 0) {
    return EDHOC_ERR_CBOR_ENCODING;
  }
  ctx->buffers.plaintext_sz = plaint_sz;

  /* Derive KEYSTREAM_2 */
  uint8_t ks_2e[plaint_sz];
  edhoc_generate_keystream_2e(ctx, plaint_sz, ks_2e);

  /* Encrypt the plaintext */
  uint8_t ciphertext[plaint_sz];
  memcpy(ciphertext, ctx->buffers.plaintext, plaint_sz);
  edhoc_enc_dec_ciphertext_2(ctx, ks_2e, ciphertext, plaint_sz);
  LOG_DBG("CIPHERTEXT_2 (%d bytes): ", (int)plaint_sz);
  LOG_DBG_BYTES(ciphertext, plaint_sz);
  LOG_DBG_("\n");

  /* Set x and ciphertext in msg_tx */
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, ctx->buffers.msg_tx, sizeof(ctx->buffers.msg_tx));

  /* Write as a single CBOR byte string containing G_Y + CIPHERTEXT_2 */
  cbor_open_data(&writer);
  cbor_write_object(&writer, ctx->creds.ephemeral_key.pub.x, ECC_KEY_LEN);
  cbor_write_object(&writer, ciphertext, plaint_sz);
  cbor_close_data(&writer);

  ctx->buffers.tx_sz = cbor_end_writer(&writer);
  if(ctx->buffers.tx_sz == 0) {
    return EDHOC_ERR_CBOR_ENCODING;
  }

  LOG_INFO("MSG2 sz: %d\n", ctx->buffers.tx_sz);

  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_generate_message_3(edhoc_context_t *ctx, const uint8_t *auth_data, size_t auth_data_size)
{
  if(ctx == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  if(auth_data_size > 0 && auth_data == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }
  /* gen TH_3 */
  edhoc_generate_transcript_hash_3(ctx, ctx->buffers.cred_x, ctx->buffers.cred_x_sz,
                ctx->buffers.plaintext, ctx->buffers.plaintext_sz);

  cose_print_key(ctx->creds.authen_key);
  LOG_DBG("SK_I (Initiator's private auth key) (%d bytes): ",
          ECC_KEY_LEN);
  LOG_DBG_BYTES(ctx->creds.authen_key->ecc.priv, ECC_KEY_LEN);
  LOG_DBG_("\n");

  LOG_DBG("G_I (x)(Initiator's public auth key) (%d bytes): ",
          ECC_KEY_LEN);
  LOG_DBG_BYTES(ctx->creds.authen_key->ecc.pub.x, ECC_KEY_LEN);
  LOG_DBG_("\n");

  LOG_DBG("G_I (y)(Initiator's public auth key) (%d bytes): ",
          ECC_KEY_LEN);
  LOG_DBG_BYTES(ctx->creds.authen_key->ecc.pub.y, ECC_KEY_LEN);
  LOG_DBG_("\n");

  /* generate cred_x */
  ctx->buffers.cred_x_sz =
    edhoc_generate_cred_x(ctx->creds.authen_key,
                          ctx->buffers.cred_x,
                          sizeof(ctx->buffers.cred_x));
  LOG_DBG("CRED_I (%d bytes): ", (int)ctx->buffers.cred_x_sz);
  LOG_DBG_BYTES(ctx->buffers.cred_x, ctx->buffers.cred_x_sz);
  LOG_DBG_("\n");

  edhoc_print_credential("CRED_I", ctx->buffers.cred_x, ctx->buffers.cred_x_sz);

  /* generate id_cred_x */
  ctx->buffers.id_cred_x_sz =
    edhoc_generate_id_cred_x(ctx->creds.authen_key,
                             ctx->buffers.id_cred_x,
                             sizeof(ctx->buffers.id_cred_x));
  LOG_DBG("ID_CRED_I (%d bytes): ", (int)ctx->buffers.id_cred_x_sz);
  LOG_DBG_BYTES(ctx->buffers.id_cred_x, ctx->buffers.id_cred_x_sz);
  LOG_DBG_("\n");

  uint8_t mac_or_signature_sz = -1;

#if ((EDHOC_METHOD == EDHOC_METHOD2) || (EDHOC_METHOD == EDHOC_METHOD3))
  /* Generate prk_4e3m */
  edhoc_generate_prk_4e3m(ctx, &ctx->creds.authen_key->ecc, 0);

  uint8_t edhoc_mac_len = ctx->config.mac_len;
  uint8_t mac_or_sig[edhoc_mac_len];
  edhoc_error_t mac_result = gen_mac(ctx, edhoc_mac_len, mac_or_sig);
  if(mac_result != EDHOC_SUCCESS) {
    return mac_result;
  }
  LOG_DBG("MAC 3 (%d bytes): ", edhoc_mac_len);
  LOG_DBG_BYTES(mac_or_sig, edhoc_mac_len);
  LOG_DBG_("\n");
  mac_or_signature_sz = edhoc_mac_len;
#endif

#if (EDHOC_METHOD == EDHOC_METHOD0) || (EDHOC_METHOD == EDHOC_METHOD1)

  /* prk_4e3m is prk_3e2m */
  memcpy(ctx->state.prk_4e3m, ctx->state.prk_3e2m, HASH_LEN);

  /* Derive MAC with HASH_LEN size (buf fits later signature) */
  uint8_t mac_or_sig[EDHOC_MAC_OR_SIG_BUF_LEN];
  edhoc_error_t mac_result = gen_mac(ctx, HASH_LEN, mac_or_sig);
  if(mac_result != EDHOC_SUCCESS) {
    return mac_result;
  }
  LOG_DBG("MAC_3 (%d bytes): ", HASH_LEN);
  LOG_DBG_BYTES(mac_or_sig, HASH_LEN);
  LOG_DBG_("\n");

  /* Create signature from MAC and other data using COSE_Sign1. */

  /* External AAD (TH_3, CRED_I, ? EAD_3) */
  uint8_t external_aad[HASH_LEN + EDHOC_MAX_CRED_LEN];
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, external_aad, sizeof(external_aad));
  cbor_write_data(&writer, ctx->state.th, HASH_LEN);
  cbor_write_object(&writer, ctx->buffers.cred_x, ctx->buffers.cred_x_sz);
  size_t external_aad_sz = cbor_end_writer(&writer);
  if(external_aad_sz == 0) {
    LOG_ERR("Failed to encode external AAD for COSE_Sign1\n");
    return EDHOC_ERR_CRYPTO_SIGN;
  }

  size_t sig_sz = cose_sign1_sign(ctx->config.sign_alg,
                                  ctx->creds.authen_key->ecc.priv,
                                  ctx->buffers.id_cred_x,
                                  ctx->buffers.id_cred_x_sz,
                                  external_aad, external_aad_sz,
                                  mac_or_sig, HASH_LEN,
                                  mac_or_sig);
  if(sig_sz == 0) {
    LOG_ERR("COSE_Sign1 signing failed\n");
    return EDHOC_ERR_CRYPTO_SIGN;
  }

  LOG_DBG("Signature from COSE_Sign1 (%zu bytes): ", sig_sz);
  LOG_DBG_BYTES(mac_or_sig, sig_sz);
  LOG_DBG_("\n");

  mac_or_signature_sz = sig_sz;
#endif

  /* Gen ciphertext_3 */
  uint16_t ciphertext_sz = gen_ciphertext_3(ctx, auth_data, auth_data_size, mac_or_sig,
                                            mac_or_signature_sz,
                                            (ctx->buffers.msg_tx) + 1,
                                            sizeof(ctx->buffers.msg_tx) - 1);
  if(ciphertext_sz == 0) {
    return EDHOC_ERR_CRYPTO_ENCRYPT;
  }

  /* Prepend connection identifier C_R as per EDHOC MSG_3 format */
  if(ctx->state.cid_rx_len > sizeof(ctx->buffers.msg_tx) - ciphertext_sz) {
    return EDHOC_ERR_BUFFER_TOO_SMALL;
  }
  /* Move ciphertext to make room for CID */
  memmove(ctx->buffers.msg_tx + ctx->state.cid_rx_len,
          ctx->buffers.msg_tx + 1, ciphertext_sz);
  /* Copy CID to beginning */
  memcpy(ctx->buffers.msg_tx, ctx->state.cid_rx, ctx->state.cid_rx_len);
  ctx->buffers.tx_sz = ciphertext_sz + ctx->state.cid_rx_len;

  /* Compute TH_4 WIP */
  edhoc_generate_transcript_hash_4(ctx, ctx->buffers.cred_x, ctx->buffers.cred_x_sz,
                ctx->buffers.plaintext, ctx->buffers.plaintext_sz);

  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
