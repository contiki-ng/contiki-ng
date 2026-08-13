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
#include "edhoc-trace.h"
#include "cose.h"
#include <assert.h>

#include "sys/log.h"
#define LOG_MODULE "EDHOC"
#define LOG_LEVEL LOG_LEVEL_EDHOC

#if HASH_LEN != SHA_256_DIGEST_LENGTH
#error Only SHA256 supported. Please update HASH_LEN.
#endif /* HASH_LEN != SHA_256_DIGEST_LENGTH */

edhoc_context_t *edhoc_ctx;

MEMB(edhoc_context_storage, edhoc_context_t, 1);
/*---------------------------------------------------------------------------*/
void
edhoc_storage_init(void)
{
  memb_init(&edhoc_context_storage);
}
/*---------------------------------------------------------------------------*/
edhoc_context_t *
edhoc_new(void)
{
  edhoc_context_t *ctx = memb_alloc(&edhoc_context_storage);
  if(ctx) {
    memset(ctx, 0, sizeof(edhoc_context_t));
  }
  return ctx;
}
/*---------------------------------------------------------------------------*/
void
edhoc_finalize(edhoc_context_t *ctx)
{
  memb_free(&edhoc_context_storage, ctx);
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_setup_suites(edhoc_context_t *ctx)
{
  /* Reverse order for the suite values */
  ctx->config.suite_num = 0;
  if(EDHOC_SUPPORTED_SUITE_4 > -1) {
    ctx->config.suite[ctx->config.suite_num] = EDHOC_SUPPORTED_SUITE_4;
    ctx->config.suite_num++;
  }
  if(EDHOC_SUPPORTED_SUITE_3 > -1) {
    ctx->config.suite[ctx->config.suite_num] = EDHOC_SUPPORTED_SUITE_3;
    ctx->config.suite_num++;
  }
  if(EDHOC_SUPPORTED_SUITE_2 > -1) {
    ctx->config.suite[ctx->config.suite_num] = EDHOC_SUPPORTED_SUITE_2;
    ctx->config.suite_num++;
  }
  if(EDHOC_SUPPORTED_SUITE_1 > -1) {
    ctx->config.suite[ctx->config.suite_num] = EDHOC_SUPPORTED_SUITE_1;
    ctx->state.suite_selected = EDHOC_SUPPORTED_SUITE_1;
    ctx->config.suite_num++;
  }

  if(ctx->config.suite_num == 0) {
    LOG_ERR("No cipher suites configured (error: %d)\n", EDHOC_ERR_SUITE_NOT_SUPPORTED);
    return 0;
  }
  return 1;
}
/*---------------------------------------------------------------------------*/
static int8_t
get_edhoc_mac_len(uint8_t ciphersuite_id)
{
  switch(ciphersuite_id) {
  case EDHOC_CIPHERSUITE_1:
  case EDHOC_CIPHERSUITE_3:
  case EDHOC_CIPHERSUITE_4:
  case EDHOC_CIPHERSUITE_5:
  case EDHOC_CIPHERSUITE_6:
  case EDHOC_CIPHERSUITE_24:
  case EDHOC_CIPHERSUITE_25:
    return EDHOC_MAC_LEN_16;
  case EDHOC_CIPHERSUITE_0:
  case EDHOC_CIPHERSUITE_2:
    return EDHOC_MAC_LEN_8;
  default:
    LOG_ERR("Invalid cipher suite for MAC length (%d)\n",
            EDHOC_ERR_SUITE_NOT_SUPPORTED);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int8_t
get_edhoc_aead_enc_alg(uint8_t ciphersuite_id)
{
  switch(ciphersuite_id) {
  case EDHOC_CIPHERSUITE_1:
  case EDHOC_CIPHERSUITE_3:
    return COSE_ALG_AES_CCM_16_128_128;
  case EDHOC_CIPHERSUITE_0:
  case EDHOC_CIPHERSUITE_2:
    return COSE_ALG_AES_CCM_16_64_128;
  default:
    LOG_ERR("Invalid cipher suite for encryption alg (%d)\n",
            EDHOC_ERR_SUITE_NOT_SUPPORTED);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int8_t
get_edhoc_curve(uint8_t ciphersuite_id)
{
  switch(ciphersuite_id) {
  case EDHOC_CIPHERSUITE_2:
  case EDHOC_CIPHERSUITE_3:
  case EDHOC_CIPHERSUITE_5:
    return EDHOC_CURVE_P256;
  default:
    LOG_ERR("Invalid cipher suite for curve (%d)\n", EDHOC_ERR_SUITE_NOT_SUPPORTED);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
static int8_t
get_edhoc_sign_alg(uint8_t ciphersuite_id)
{
  switch(ciphersuite_id) {
  case EDHOC_CIPHERSUITE_2:
  case EDHOC_CIPHERSUITE_3:
  case EDHOC_CIPHERSUITE_5:
  case EDHOC_CIPHERSUITE_6:
    return ES256;
  default:
    LOG_ERR("Invalid cipher suite for curve (%d)\n",
            EDHOC_ERR_SUITE_NOT_SUPPORTED);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
int8_t
edhoc_set_config_from_suite(edhoc_context_t *ctx, uint8_t suite)
{
  if((ctx->config.ecdh_curve = get_edhoc_curve(suite)) == 0) {
    return 0;
  }

  if((ctx->config.mac_len = get_edhoc_mac_len(ctx->state.suite_selected)) == 0) {
    return 0;
  }

  if((ctx->config.aead_alg = get_edhoc_aead_enc_alg(ctx->state.suite_selected)) == 0) {
    return 0;
  }

  if((ctx->config.sign_alg = get_edhoc_sign_alg(ctx->state.suite_selected)) == 0) {
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
size_t
edhoc_generate_cred_x(const cose_key_t *cose, uint8_t *cred, size_t cred_sz)
{
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, cred, cred_sz);
  cbor_open_map(&writer);
  cbor_write_unsigned(&writer, 2);
  cbor_write_text(&writer, cose->identity, cose->identity_sz);
  cbor_write_unsigned(&writer, 8);

  cbor_open_map(&writer);
  cbor_write_unsigned(&writer, 1);

  cbor_open_map(&writer);
  cbor_write_unsigned(&writer, 1);
  cbor_write_unsigned(&writer, cose->kty);
  cbor_write_unsigned(&writer, 2);
  cbor_write_data(&writer, cose->kid, cose->kid_sz);
  cbor_write_signed(&writer, -1);
  cbor_write_unsigned(&writer, cose->crv);
  cbor_write_signed(&writer, -2);
  cbor_write_data(&writer, cose->ecc.pub.x, ECC_KEY_LEN);
  if(cose->crv == 1) {
    cbor_write_signed(&writer, -3);
    cbor_write_data(&writer, cose->ecc.pub.y, ECC_KEY_LEN);
  }
  cbor_close_map(&writer);
  cbor_close_map(&writer);
  cbor_close_map(&writer);
  return cbor_end_writer(&writer);
}
/*---------------------------------------------------------------------------*/
size_t
edhoc_generate_id_cred_x(const cose_key_t *cose, uint8_t *cred, size_t cred_sz)
{
  LOG_DBG("kid (%i bytes): ", cose->kid_sz);
  LOG_DBG_BYTES(cose->kid, cose->kid_sz);
  LOG_DBG_("\n");

  /* Include KID */
  if(EDHOC_AUTHENT_TYPE == EDHOC_CRED_KID) {
    cbor_writer_state_t writer;
    cbor_init_writer(&writer, cred, cred_sz);
    cbor_open_map(&writer);
    cbor_write_unsigned(&writer, 4);
    cbor_write_data(&writer, cose->kid, cose->kid_sz);
    cbor_close_map(&writer);
    return cbor_end_writer(&writer);
  }

  /* Include directly the credential used for authentication ID_CRED_X = CRED_X */
  if(EDHOC_AUTHENT_TYPE == EDHOC_CRED_INCLUDE) {
    return edhoc_generate_cred_x(cose, cred, cred_sz);
  }
  return 0;
}
/*---------------------------------------------------------------------------*/
static size_t
generate_info(uint8_t info_label, const uint8_t *context, uint8_t context_sz,
              uint8_t length, uint8_t *info, size_t info_sz)
{
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, info, info_sz);
  cbor_write_object(&writer, &info_label, 1);
  cbor_write_data(&writer, context, context_sz);
  cbor_write_unsigned(&writer, length);
  return cbor_end_writer(&writer);
}
/*---------------------------------------------------------------------------*/
int8_t
edhoc_generate_transcript_hash_2(edhoc_context_t *ctx, const uint8_t *eph_pub,
              uint8_t *msg, uint16_t msg_sz)
{
  /* Create the input for TH_2 = H(G_Y, H(msg)), msg1 is in msg_rx */
  uint8_t h[CBOR_BYTE_STRING_SIZE(HASH_LEN) +  CBOR_BYTE_STRING_SIZE(ECC_KEY_LEN)];

  EDHOC_DBG_VALUE("Input MSG_1", msg, msg_sz);

  uint8_t msg_1_hash[HASH_LEN];
  sha_256_hash(msg + 1, msg_sz - 1, msg_1_hash); /*FIXME: Improve skipping of CBOR true for TH */

  EDHOC_TRACE_VALUE("H(MSG_1)", msg_1_hash, HASH_LEN);
  EDHOC_TRACE_VALUE("G_Y", eph_pub, ECC_KEY_LEN);

  cbor_writer_state_t writer;
  cbor_init_writer(&writer, h, sizeof(h));
  cbor_write_data(&writer, eph_pub, ECC_KEY_LEN);
  cbor_write_data(&writer, msg_1_hash, HASH_LEN);
  size_t h_buf_sz = cbor_end_writer(&writer);

  /* Compute TH_2 */
  sha_256_hash(h, h_buf_sz, ctx->state.th);
  edhoc_trace_transcript_hash("TH_2", ctx->state.th, h, h_buf_sz);
  return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_generate_transcript_hash_3(edhoc_context_t *ctx, const uint8_t *cred, uint16_t cred_sz,
              const uint8_t *plaintext, uint16_t plaintext_sz)
{
  /* Check for potential buffer overflow */
  uint16_t required_size = CBOR_BYTE_STRING_SIZE(HASH_LEN) + plaintext_sz + cred_sz;
  if(required_size > EDHOC_MAX_BUFFER || plaintext_sz > EDHOC_MAX_BUFFER || cred_sz > EDHOC_MAX_BUFFER) {
    LOG_ERR("Buffer sizes exceed maximum allowed\n");
    return 1;
  }

  /* TH_3 = H(TH_2, PLAINTEXT_2, CRED_R) */
  uint8_t h[EDHOC_MAX_BUFFER];
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, h, sizeof(h));
  cbor_write_data(&writer, ctx->state.th, HASH_LEN);
  EDHOC_DBG_VALUE("TH_2 for TH_3", ctx->state.th, HASH_LEN);
  cbor_write_object(&writer, plaintext, plaintext_sz);
  EDHOC_TRACE_VALUE("PLAINTEXT_2", plaintext, plaintext_sz);
  cbor_write_object(&writer, cred, cred_sz);
  EDHOC_TRACE_VALUE("CRED_R", cred, cred_sz);
  size_t h_sz = cbor_end_writer(&writer);

  /* Compute TH_3 */
  sha_256_hash(h, h_sz, ctx->state.th);
  edhoc_trace_transcript_hash("TH_3", ctx->state.th, h, h_sz);
  return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_generate_transcript_hash_4(edhoc_context_t *ctx, const uint8_t *cred, uint16_t cred_sz,
              const uint8_t *plaintext, uint16_t plaintext_sz)
{
  /* Check for potential buffer overflow */
  uint16_t required_size = CBOR_BYTE_STRING_SIZE(HASH_LEN) + plaintext_sz + cred_sz;
  if(required_size > EDHOC_MAX_BUFFER || plaintext_sz > EDHOC_MAX_BUFFER || cred_sz > EDHOC_MAX_BUFFER) {
    LOG_ERR("Buffer sizes exceed maximum allowed\n");
    return 1;
  }

  /* TH_4 = H(TH_3, PLAINTEXT_3, CRED_I) */
  uint8_t h[EDHOC_MAX_BUFFER];
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, h, sizeof(h));
  cbor_write_data(&writer, ctx->state.th, HASH_LEN);
  EDHOC_DBG_VALUE("TH_3 for TH_4", ctx->state.th, HASH_LEN);

  cbor_write_object(&writer, plaintext, plaintext_sz);
  EDHOC_TRACE_VALUE("PLAINTEXT_3", plaintext, plaintext_sz);

  cbor_write_object(&writer, cred, cred_sz);
  EDHOC_TRACE_VALUE("CRED_I", cred, cred_sz);
  size_t h_sz = cbor_end_writer(&writer);

  /* Compute TH_4 */
  sha_256_hash(h, h_sz, ctx->state.th);
  edhoc_trace_transcript_hash("TH_4", ctx->state.th, h, h_sz);
  return 0;
}
/*---------------------------------------------------------------------------*/
void
edhoc_print_session_info(const edhoc_context_t *ctx)
{
  /* Use the enhanced session summary instead */
  edhoc_trace_session_summary(ctx);
}
/*---------------------------------------------------------------------------*/
int16_t
edhoc_kdf(const uint8_t *prk, uint8_t info_label, const uint8_t *context,
          uint8_t context_sz, uint16_t length, uint8_t *result)
{
  LOG_DBG("edhoc_kdf label=%d, length=%d\n", info_label, length);
  LOG_DBG("PRK (%d bytes): ", ECC_KEY_LEN);
  LOG_DBG_BYTES(prk, ECC_KEY_LEN);
  LOG_DBG_("\n");
  if(context && context_sz > 0) {
    LOG_DBG("Context (%d bytes): ", context_sz);
    LOG_DBG_BYTES(context, context_sz);
    LOG_DBG_("\n");
  }

  uint8_t info_buf[CBOR_UNSIGNED_SIZE((unsigned)info_label)
    + CBOR_BYTE_STRING_SIZE((unsigned)context_sz)
    + CBOR_UNSIGNED_SIZE((unsigned)length)];
  uint16_t info_sz = generate_info(info_label, context, context_sz,
                                   length, info_buf, sizeof(info_buf));
  if(info_sz == 0) {
    LOG_ERR("Error generating INFO\n");
    return info_sz;
  }

  int16_t ret = edhoc_expand(prk, info_buf, info_sz, length, result);
  if(ret > 0) {
    LOG_DBG("KDF Result (%d bytes): ", length);
    LOG_DBG_BYTES(result, length);
    LOG_DBG_("\n");
  }
  return ret;
}
/*---------------------------------------------------------------------------*/
int16_t
edhoc_expand(const uint8_t *prk, const uint8_t *info, uint16_t info_sz,
             uint16_t length, uint8_t *result)
{
  LOG_DBG("INFO for HKDF_Expand (%d bytes): ", info_sz);
  LOG_DBG_BYTES(info, info_sz);
  LOG_DBG_("\n");
  sha_256_hkdf_expand(prk, ECC_KEY_LEN, info, info_sz, result, length);
  return length;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_calc_mac(const edhoc_context_t *ctx, uint8_t mac_num,
               uint8_t mac_len, uint8_t *mac)
{
  if(mac_num == EDHOC_MAC_2) {
    LOG_DBG("Calculating MAC_2\n");
    /* Build context_2 */
    uint8_t context_2[EDHOC_MAX_CID_LEN + ctx->buffers.id_cred_x_sz +
                      CBOR_BYTE_STRING_SIZE(HASH_LEN) + ctx->buffers.cred_x_sz];
    cbor_writer_state_t writer;
    cbor_init_writer(&writer, context_2, sizeof(context_2));

    /* Add C_R */
    if(EDHOC_ROLE == EDHOC_INITIATOR) {
      cbor_write_object(&writer, ctx->state.cid_rx, ctx->state.cid_rx_len);
      LOG_DBG("C_R (%d bytes): ", ctx->state.cid_rx_len);
      LOG_DBG_BYTES(ctx->state.cid_rx, ctx->state.cid_rx_len);
      LOG_DBG_("\n");
    } else {
      cbor_write_object(&writer, ctx->state.cid, ctx->state.cid_len);
      LOG_DBG("C_R (%d bytes): ", ctx->state.cid_len);
      LOG_DBG_BYTES(ctx->state.cid, ctx->state.cid_len);
      LOG_DBG_("\n");
    }

    cbor_write_object(&writer, ctx->buffers.id_cred_x, ctx->buffers.id_cred_x_sz);
    LOG_DBG("ID_CRED_X (%zu bytes): ", ctx->buffers.id_cred_x_sz);
    LOG_DBG_BYTES(ctx->buffers.id_cred_x, ctx->buffers.id_cred_x_sz);
    LOG_DBG_("\n");

    cbor_write_data(&writer, ctx->state.th, HASH_LEN);
    LOG_DBG("TH_2 for MAC_2 (%d bytes): ", HASH_LEN);
    LOG_DBG_BYTES(ctx->state.th, HASH_LEN);
    LOG_DBG_("\n");

    cbor_write_object(&writer, ctx->buffers.cred_x, ctx->buffers.cred_x_sz);
    LOG_DBG("CRED_X (%zu bytes): ", ctx->buffers.cred_x_sz);
    LOG_DBG_BYTES(ctx->buffers.cred_x, ctx->buffers.cred_x_sz);
    LOG_DBG_("\n");

    size_t context_2_buffer_size = cbor_end_writer(&writer);
    LOG_DBG("CONTEXT_2 (%zu bytes): ", context_2_buffer_size);
    LOG_DBG_BYTES(context_2, context_2_buffer_size);
    LOG_DBG_("\n");

    /* Use edhoc_kdf to generate MAC_2 */
    int16_t err = edhoc_kdf(ctx->state.prk_3e2m, MAC_2_LABEL,
                           context_2, context_2_buffer_size, mac_len, mac);
    if(err < 0) {
      LOG_ERR("Failed to expand MAC_2\n");
      return 0;
    }
  } else if(mac_num == EDHOC_MAC_3) {
    /* Build context_3 */
    uint8_t context_3[ctx->buffers.id_cred_x_sz +
                      CBOR_BYTE_STRING_SIZE(HASH_LEN) + ctx->buffers.cred_x_sz];
    cbor_writer_state_t writer;
    cbor_init_writer(&writer, context_3, sizeof(context_3));

    cbor_write_object(&writer, ctx->buffers.id_cred_x, ctx->buffers.id_cred_x_sz);
    cbor_write_data(&writer, ctx->state.th, HASH_LEN);
    cbor_write_object(&writer, ctx->buffers.cred_x, ctx->buffers.cred_x_sz);
    size_t context_3_buffer_size = cbor_end_writer(&writer);
    LOG_DBG("CONTEXT_3 (%zu bytes): ", context_3_buffer_size);
    LOG_DBG_BYTES(context_3, context_3_buffer_size);
    LOG_DBG_("\n");

    /* Use edhoc_kdf to generate MAC_3 */
    int16_t err = edhoc_kdf(ctx->state.prk_4e3m, MAC_3_LABEL,
                           context_3, context_3_buffer_size, mac_len, mac);
    if(err < 0) {
      LOG_ERR("Failed to expand MAC_3\n");
      return 0;
    }
  } else {
    LOG_ERR("Wrong MAC value\n");
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
#if (EDHOC_METHOD == EDHOC_METHOD3) || INITIATOR_METHOD1 || RESPONDER_METHOD2
static uint16_t
check_mac(const edhoc_context_t *ctx, const uint8_t *received_mac,
          uint16_t received_mac_sz)
{
  uint8_t mac_num;
  if(EDHOC_ROLE == EDHOC_INITIATOR) {
    mac_num = EDHOC_MAC_2;
  } else if(EDHOC_ROLE == EDHOC_RESPONDER) {
    mac_num = EDHOC_MAC_3;
  }

  uint8_t edhoc_mac_len = ctx->config.mac_len;
  uint8_t mac[edhoc_mac_len];
  if(!edhoc_calc_mac(ctx, mac_num, edhoc_mac_len, mac)) {
    LOG_ERR("Set MAC error\n");
    return 0;
  }

  LOG_DBG("Received MAC (%d): ", (int)received_mac_sz);
  LOG_DBG_BYTES(received_mac, received_mac_sz);
  LOG_DBG_("\n");

  LOG_DBG("Recalculated MAC (%d): ", (int)edhoc_mac_len);
  LOG_DBG_BYTES(mac, edhoc_mac_len);
  LOG_DBG_("\n");

  /* Verify the MAC value */
  uint16_t mac_sz = edhoc_mac_len;
  uint8_t diff = 0;
  for(int i = 0; i < edhoc_mac_len; i++) {
    diff |= (mac[i] ^ received_mac[i]);
  }

  if(diff != 0) {
    LOG_ERR("error code in check mac (%d)\n", EDHOC_ERR_CRYPTO_AUTHENTICATION);
    return 0;
  }

  return mac_sz;
}
#endif /* (EDHOC_METHOD == EDHOC_METHOD3) || INITIATOR_METHOD1 || RESPONDER_METHOD2 */
/*---------------------------------------------------------------------------*/
bool
edhoc_generate_prk_2e(edhoc_context_t *ctx)
{
  uint8_t ikm[ECC_KEY_LEN];

  if(!ecdh_generate_ikm(ctx->config.ecdh_curve, ctx->state.gx,
                         ctx->creds.ephemeral_key.priv, ikm)) {
    LOG_ERR("Failed to generate shared secret for PRK_2e\n");
    return false;
  }
  EDHOC_DBG_VALUE("GXY", ikm, ECC_KEY_LEN);

  sha_256_hkdf_extract(ctx->state.th, HASH_LEN, ikm, ECC_KEY_LEN,
                       ctx->state.prk_2e);
  edhoc_trace_prk_derivation("PRK_2e", ctx->state.prk_2e, ctx->state.th, ikm);
  return true;
}
/*---------------------------------------------------------------------------*/
/* Derive KEYSTREAM_2 */
int16_t
edhoc_generate_keystream_2e(edhoc_context_t *ctx, uint16_t length, uint8_t *ks_2e)
{
  int err = edhoc_kdf(ctx->state.prk_2e, KEYSTREAM_2_LABEL,
                     ctx->state.th, HASH_LEN, length, ks_2e);
  if(err < 0) {
    return err;
  }
  EDHOC_DBG_VALUE("KEYSTREAM_2", ks_2e, length);
  return 1;
}
/*---------------------------------------------------------------------------*/
#if (EDHOC_METHOD == EDHOC_METHOD3) || INITIATOR_METHOD1 || RESPONDER_METHOD2
bool
edhoc_generate_prk_3e2m(edhoc_context_t *ctx, const ecc_key_t *auth_key, uint8_t gen)
{
  uint8_t grx[ECC_KEY_LEN];
  bool success;

  EDHOC_TRACE_COMPUTE(gen ? "PRK_3e2m (ephemeral DH)" : "PRK_3e2m (static DH)");

  if(gen) {
    success = ecdh_generate_ikm(ctx->config.ecdh_curve,
                                ctx->state.gx,
                                auth_key->priv,
                                grx);
  } else {
    EDHOC_DBG_VALUE("Static auth key X", auth_key->pub.x, ECC_KEY_LEN);
    EDHOC_DBG_VALUE("Static auth key Y", auth_key->pub.y, ECC_KEY_LEN);
    success = ecdh_generate_ikm(ctx->config.ecdh_curve,
                                auth_key->pub.x,
                                ctx->creds.ephemeral_key.priv,
                                grx);
  }
  if(!success) {
    LOG_ERR("Failed to generate shared secret for PRK_3e2m\n");
    return false;
  }

  EDHOC_DBG_VALUE("G_RX (DH secret)", grx, ECC_KEY_LEN);

  /* Use edhoc_kdf to generate SALT_3e2m */
  uint8_t salt[HASH_LEN];
  int16_t err = edhoc_kdf(ctx->state.prk_2e, SALT_3E2M_LABEL, ctx->state.th,
                         HASH_LEN, HASH_LEN, salt);
  if(err < 1) {
    LOG_ERR("Error calculating SALT_3e2m (%d)\n", err);
    return false;
  }

  /* Extract PRK_3e2m */
  sha_256_hkdf_extract(salt, HASH_LEN, grx, ECC_KEY_LEN, ctx->state.prk_3e2m);
  edhoc_trace_prk_derivation("PRK_3e2m", ctx->state.prk_3e2m, salt, grx);
  return true;
}
#endif /* (EDHOC_METHOD == EDHOC_METHOD3) || INITIATOR_METHOD1 || RESPONDER_METHOD2 */
/*---------------------------------------------------------------------------*/
#if (EDHOC_METHOD == EDHOC_METHOD2) || (EDHOC_METHOD == EDHOC_METHOD3) || INITIATOR_METHOD1 || RESPONDER_METHOD2
bool
edhoc_generate_prk_4e3m(edhoc_context_t *ctx, const ecc_key_t *auth_key, uint8_t gen)
{
  uint8_t giy[ECC_KEY_LEN];
  bool success;

  if(gen) {
    success = ecdh_generate_ikm(ctx->config.ecdh_curve,
                                auth_key->pub.x,
                                ctx->creds.ephemeral_key.priv,
                                giy);
  } else {
    success = ecdh_generate_ikm(ctx->config.ecdh_curve,
                                ctx->state.gx,
                                auth_key->priv,
                                giy);
  }
  if(!success) {
    LOG_ERR("Failed to generate shared secret for PRK_4e3m\n");
    return false;
  }
  LOG_DBG("G_IY (ECDH shared secret) (%d bytes): ", ECC_KEY_LEN);
  LOG_DBG_BYTES(giy, ECC_KEY_LEN);
  LOG_DBG_("\n");

  /* Use edhoc_kdf to generate SALT_4e3m */
  uint8_t salt[HASH_LEN];
  int16_t err = edhoc_kdf(ctx->state.prk_3e2m, SALT_4E3M_LABEL, ctx->state.th,
                         HASH_LEN, HASH_LEN, salt);
  if(err < 1) {
    LOG_ERR("Error calculating SALT_4e3m (%d)\n", err);
    return false;
  }
  LOG_DBG("SALT_4e3m (%d bytes): ", HASH_LEN);
  LOG_DBG_BYTES(salt, HASH_LEN);
  LOG_DBG_("\n");

  sha_256_hkdf_extract(salt, HASH_LEN, giy, ECC_KEY_LEN, ctx->state.prk_4e3m);
  LOG_DBG("PRK_4e3m (%d bytes): ", HASH_LEN);
  LOG_DBG_BYTES(ctx->state.prk_4e3m, HASH_LEN);
  LOG_DBG_("\n");
  return true;
}
#endif /* (EDHOC_METHOD == EDHOC_METHOD2) || (EDHOC_METHOD == EDHOC_METHOD3) ||
INITIATOR_METHOD1 || RESPONDER_METHOD2 */
/*---------------------------------------------------------------------------*/
int16_t
edhoc_enc_dec_ciphertext_2(const edhoc_context_t *ctx, const uint8_t *ks_2e,
                           uint8_t *plaintext, uint16_t plaintext_sz)
{
  LOG_DBG("Cipher/Plaintext (%d bytes): ", plaintext_sz);
  LOG_DBG_BYTES(plaintext, plaintext_sz);
  LOG_DBG_("\n");

  LOG_DBG("**** ks_2e in enc func (%d bytes): ", plaintext_sz);
  LOG_DBG_BYTES(ks_2e, plaintext_sz);
  LOG_DBG_("\n");

  for(int i = 0; i < plaintext_sz; i++) {
    plaintext[i] = plaintext[i] ^ ks_2e[i];
  }

  LOG_DBG("Plain/Ciphertext (%d bytes): ", plaintext_sz);
  LOG_DBG_BYTES(plaintext, plaintext_sz);
  LOG_DBG_("\n");

  return plaintext_sz;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_initialize_context(edhoc_context_t *ctx)
{
  /* Retrieve a pointer to own auth key */
  cose_key_t *key = NULL;
  if(!edhoc_get_own_auth_key(ctx, &key)) {
    return 0;
  }

  /* Set pointer to found key */
  ctx->creds.authen_key = key;

  /* Set up the cipher suites selection logic */
  if(!edhoc_setup_suites(ctx)) {
    return 0;
  }

  /* Set CID */
#ifdef EDHOC_CID_BYTES
  /* Variable-length CID for testing */
  {
    const uint8_t cid_bytes[] = EDHOC_CID_BYTES;
    ctx->state.cid_len = sizeof(cid_bytes);
    if(ctx->state.cid_len > EDHOC_MAX_CID_LEN) {
      LOG_ERR("CID length (%d) exceeds maximum (%d)\n", ctx->state.cid_len, EDHOC_MAX_CID_LEN);
      return 0;
    }
    memcpy(ctx->state.cid, cid_bytes, ctx->state.cid_len);
    LOG_INFO("Variable CID initialized: length %d bytes\n", ctx->state.cid_len);
    LOG_DBG("Variable CID bytes: ");
    for(int i = 0; i < ctx->state.cid_len; i++) {
      LOG_DBG_("%02x ", ctx->state.cid[i]);
    }
    LOG_DBG_("\n");
  }
#else
  /* Default single-byte CID */
  ctx->state.cid[0] = EDHOC_CID;
  ctx->state.cid_len = EDHOC_DEFAULT_CID_LEN;
  LOG_INFO("Default CID: 0x%02x (len %d)\n", ctx->state.cid[0], ctx->state.cid_len);
#endif

  /* Set role and method */
  ctx->config.role = EDHOC_ROLE;
  ctx->config.method = EDHOC_METHOD;

  /* Initiator sets config to use based on selected suite */
  int8_t err = edhoc_set_config_from_suite(ctx, ctx->state.suite_selected);
  if(err != 1) {
    return 0;
  }

  return 1;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_get_own_auth_key(edhoc_context_t *ctx, cose_key_t **key)
{
#ifdef EDHOC_AUTH_SUBJECT_NAME
  if(edhoc_check_key_list_identity(EDHOC_AUTH_SUBJECT_NAME,
                                   strlen(EDHOC_AUTH_SUBJECT_NAME), key) == EDHOC_SUCCESS) {
    /* Key found using identity */
    return 1;
  } else {
    LOG_ERR("Auth key identity not found\n");
  }
#endif

#ifdef EDHOC_AUTH_KID
  if(*key == NULL) {
    uint8_t key_id[sizeof(int)];
    int kid = EDHOC_AUTH_KID;
    int quotient = (EDHOC_AUTH_KID) / 256;
    uint8_t key_id_sz = 1;
    while(quotient != 0) {
      key_id_sz++;
      quotient /= 256;
    }
    memcpy(key_id, (uint8_t *)&kid, key_id_sz);

    LOG_DBG("Looking for auth key KID 0x%02x (sz=%d)\n", key_id[0], key_id_sz);

    if(edhoc_check_key_list_kid(key_id, key_id_sz, key) == EDHOC_SUCCESS) {
      /* Key found using KID */
      LOG_DBG("Auth key found with KID 0x%02x\n", key_id[0]);
      return 1;
    } else {
      LOG_ERR("Does not contain a key for the key ID 0x%02x\n", key_id[0]);
    }
  }
#endif

  LOG_ERR("No matching key found in the storage\n");
  return 0;
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_generate_error_message(uint8_t *msg_er, size_t msg_er_sz,
                    const edhoc_context_t *ctx, int8_t err)
{
  edhoc_msg_error_t msg;
  memset(&msg, 0, sizeof(edhoc_msg_error_t));
  msg.err_code = EDHOC_MSG_ERR_CODE_UNSPECIFIED_ERROR;
  switch(err) {
  default:
    msg.info.tstr.err_info = "Unknown error";
    break;
  case EDHOC_ERR_SUITE_NOT_SUPPORTED:
    msg.err_code = EDHOC_MSG_ERR_CODE_WRONG_CIPHER_SUITE;
    msg.info.suites.suites_num = MIN(sizeof(msg.info.suites.suites), ctx->config.suite_num);
    memcpy(msg.info.suites.suites, ctx->config.suite, msg.info.suites.suites_num);
    break;
  case EDHOC_ERR_MSG_MALFORMED:
    msg.info.tstr.err_info = "Malformed message";
    break;
  case EDHOC_ERR_METHOD_NOT_SUPPORTED:
    msg.info.tstr.err_info = "Method not supported";
    break;
  case EDHOC_ERR_CID_INVALID:
    msg.info.tstr.err_info = "Invalid connection ID";
    break;
  case EDHOC_ERR_WRONG_CID:
    msg.info.tstr.err_info = "Wrong connection ID";
    break;
  case EDHOC_ERR_ID_CRED_MALFORMED:
    msg.info.tstr.err_info = "Malformed credential ID";
    break;
  case EDHOC_ERR_CRYPTO_AUTHENTICATION:
    msg.info.tstr.err_info = "Authentication failed";
    break;
  case EDHOC_ERR_CRYPTO_DECRYPT:
    msg.info.tstr.err_info = "Decryption failed";
    break;
  case EDHOC_ERR_INTERNAL_ERROR:
    msg.info.tstr.err_info = "Internal error";
    break;
  case EDHOC_ERR_NOT_ALLOWED_IDENTITY:
    msg.info.tstr.err_info = "Identity not allowed";
    break;
  case EDHOC_ERR_CREDENTIAL_NOT_FOUND: /* Unknown credential referenced */
  case EDHOC_ERR_KEY_NOT_FOUND: /* Key not found - maps to unknown credential */
    msg.err_code = EDHOC_MSG_ERR_CODE_UNKNOWN_CREDENTIAL_SELECTION;
    /* RFC 9528: ERR_INFO for error code 3 should be boolean true */
    /* This will be handled in serialization as cbor_write_bool(&writer, true) */
    break;
  case EDHOC_ERR_NETWORK_TIMEOUT:
    msg.info.tstr.err_info = "Network timeout";
    break;
  case EDHOC_ERR_CORRELATION:
    msg.info.tstr.err_info = "Message correlation error";
    break;
  case EDHOC_ERR_SEQUENCE_ERROR:
    msg.info.tstr.err_info = "Message sequence error";
    break;
  case EDHOC_ERR_BUFFER_OVERFLOW:
    msg.info.tstr.err_info = "Buffer overflow";
    break;
  case EDHOC_ERR_CRITICAL_EAD_UNSUPPORTED:
    msg.info.tstr.err_info = "Critical EAD item cannot be processed";
    break;
  }
  LOG_ERR("ERR MSG (%d): ", msg.err_code);
  if(msg.err_code == EDHOC_MSG_ERR_CODE_UNSPECIFIED_ERROR) {
    msg.info.tstr.err_info_sz = strlen(msg.info.tstr.err_info);
    LOG_ERR_STRING(msg.info.tstr.err_info, msg.info.tstr.err_info_sz);
  } else if(msg.err_code == EDHOC_MSG_ERR_CODE_WRONG_CIPHER_SUITE) {
    LOG_ERR("Wrong cipher suite, proposing %d suites", msg.info.suites.suites_num);
  } else if(msg.err_code == EDHOC_MSG_ERR_CODE_UNKNOWN_CREDENTIAL_SELECTION) {
    LOG_ERR("Unknown credential referenced");
  }
  LOG_ERR_("\n");

  size_t err_sz = edhoc_serialize_err(&msg, msg_er, msg_er_sz);
  LOG_DBG("ERR MSG CBOR: ");
  LOG_DBG_BYTES(msg_er, err_sz);
  LOG_DBG_("\n");
  return err_sz;
}
/*---------------------------------------------------------------------------*/
int
edhoc_authenticate_msg(edhoc_context_t *ctx, uint8_t *ad, bool msg2)
{
  /* Point to decrypted plaintext for key retrieval */
  uint8_t *plaintext_ptr = NULL;
  size_t parse_size = ctx->buffers.plaintext_sz;
  if(msg2) {
    /* Skip C_R at beginning of plaintext_2 */
    cbor_reader_state_t skip_reader;
    cbor_init_reader(&skip_reader, ctx->buffers.plaintext, ctx->buffers.plaintext_sz);
    size_t cid_size;
    edhoc_read_byte_identifier(&skip_reader, &cid_size);
    plaintext_ptr = (uint8_t *)cbor_get_position(&skip_reader);
    parse_size = ctx->buffers.plaintext_sz - (plaintext_ptr - ctx->buffers.plaintext);
  } else {
    plaintext_ptr = ctx->buffers.plaintext;
  }

  /* Retrieve the peer credential from msg info and cred storage */
  cose_key_t peer_key;
  cose_key_t *key = &peer_key;

  /* Initialize shared CBOR reader for sequential parsing */
  cbor_reader_state_t reader;
  cbor_init_reader(&reader, plaintext_ptr, parse_size);

  /* Parse ID_CRED_X using shared reader state */
  int8_t key_result = edhoc_get_key_id_cred_x(&reader, NULL, 0, key);
  if(key_result < 0) {
    LOG_ERR("Failed to parse ID_CRED_X (error: %d)\n", key_result);
    return key_result;
  }

  /* Get MAC from the decrypted message using shared reader state */
  uint8_t *received_mac = NULL;
  uint16_t received_mac_sz = edhoc_get_sign(&reader, &received_mac);
  if(received_mac_sz == 0) {
    LOG_ERR("Failed to parse signature/MAC: invalid CBOR\n");
    return EDHOC_ERR_CRYPTO_AUTHENTICATION;
  }

  /* Get the additional data from the decrypted message if present */
  uint16_t ad_sz = 0;
  if(cbor_get_remaining(&reader) > 0 && ad) {
    ad_sz = edhoc_get_ad(&reader, ad, EDHOC_MAX_AD_SZ);
    if(ad_sz == 0) {
      LOG_WARN("Failed to parse additional data\n");
      ad = NULL;
    }
  } else {
    ad = NULL;
  }

  /* generate cred_x and id_cred_x */
  ctx->buffers.cred_x_sz = edhoc_generate_cred_x(key, ctx->buffers.cred_x,
      sizeof(ctx->buffers.cred_x));
  LOG_DBG("CRED_X auth (%zu): ", ctx->buffers.cred_x_sz);
  LOG_DBG_BYTES(ctx->buffers.cred_x, ctx->buffers.cred_x_sz);
  LOG_DBG_("\n");

  ctx->buffers.id_cred_x_sz =
    edhoc_generate_id_cred_x(key, ctx->buffers.id_cred_x,
                             sizeof(ctx->buffers.id_cred_x));
  LOG_DBG("ID_CRED_X auth (%zu): ", ctx->buffers.id_cred_x_sz);
  LOG_DBG_BYTES(ctx->buffers.id_cred_x, ctx->buffers.id_cred_x_sz);
  LOG_DBG_("\n");

#if (EDHOC_METHOD == EDHOC_METHOD3) || INITIATOR_METHOD1 || RESPONDER_METHOD2
  /* Generate prk_3e2m or prk_4e3m */
  if(msg2 == true) {
    edhoc_generate_prk_3e2m(ctx, &key->ecc, 0);
  } else { /* msg3 */
    edhoc_generate_prk_4e3m(ctx, &key->ecc, 1);
  }

  if(check_mac(ctx, received_mac, received_mac_sz) == 0) {
    LOG_ERR("error code in handler (%d)\n", EDHOC_ERR_CRYPTO_AUTHENTICATION);
    return EDHOC_ERR_CRYPTO_AUTHENTICATION;
  }
#endif

#if (EDHOC_METHOD == EDHOC_METHOD0) || INITIATOR_METHOD2 || RESPONDER_METHOD1
  if(msg2 == true) {
    /* prk_3e2m is prk_2e */
    memcpy(ctx->state.prk_3e2m, ctx->state.prk_2e, HASH_LEN);
  } else { /* msg3 */
    /* prk_4e3m is prk_3e2m */
    memcpy(ctx->state.prk_4e3m, ctx->state.prk_3e2m, HASH_LEN);
  }

  /* Verify peer signature using COSE_Sign1. */

  /* External AAD (TH_2/3, CRED_X, ? EAD_2/3) */
  uint8_t external_aad[HASH_LEN + EDHOC_MAX_CRED_LEN];
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, external_aad, sizeof(external_aad));
  cbor_write_data(&writer, ctx->state.th, HASH_LEN);
  cbor_write_object(&writer, ctx->buffers.cred_x, ctx->buffers.cred_x_sz);
  size_t external_aad_sz = cbor_end_writer(&writer);
  if(external_aad_sz == 0) {
    LOG_ERR("Failed to encode external AAD for COSE_Sign1 verification\n");
    return EDHOC_ERR_INTERNAL_ERROR;
  }

  /* Recompute the local MAC that the peer is supposed to have signed. */
  uint8_t mac_num = msg2 ? EDHOC_MAC_2 : EDHOC_MAC_3;
  uint8_t mac[HASH_LEN];
  edhoc_calc_mac(ctx, mac_num, HASH_LEN, mac);
  LOG_DBG("MAC_%d (%d bytes): ", msg2 ? 2 : 3, HASH_LEN);
  LOG_DBG_BYTES(mac, HASH_LEN);
  LOG_DBG_("\n");

  uint8_t other_public_key[ECC_KEY_LEN * 2];
  memcpy(other_public_key, key->ecc.pub.x, ECC_KEY_LEN);
  memcpy(other_public_key + ECC_KEY_LEN, key->ecc.pub.y, ECC_KEY_LEN);

  if(!cose_sign1_verify(ctx->config.sign_alg, other_public_key,
                        ctx->buffers.id_cred_x, ctx->buffers.id_cred_x_sz,
                        external_aad, external_aad_sz,
                        mac, HASH_LEN,
                        received_mac, received_mac_sz)) {
    LOG_ERR("COSE_Sign1 signature verification failed\n");
    return EDHOC_ERR_CRYPTO_AUTHENTICATION;
  }
#endif

  /* Compute TH_4 WIP (after verifying MAC_3) */
  if(msg2 == false) { /* msg 3 */
    /* Calculate TH_4 */
    edhoc_generate_transcript_hash_4(ctx, ctx->buffers.cred_x, ctx->buffers.cred_x_sz,
                  ctx->buffers.plaintext, ctx->buffers.plaintext_sz);
  }

  return ad_sz;
}
/*---------------------------------------------------------------------------*/
void
edhoc_print_config_summary(const edhoc_context_t *ctx)
{
  if(ctx == NULL) {
    LOG_ERR("Cannot print config summary: NULL context\n");
    return;
  }

  LOG_INFO("\n=== EDHOC Session Configuration ===\n");
  LOG_INFO("Role: %s\n", ctx->config.role == EDHOC_INITIATOR ? "Initiator" : "Responder");
  LOG_INFO("Method: %d\n", ctx->config.method);
  LOG_INFO("Suite Selected: %d\n", ctx->state.suite_selected);
  LOG_INFO("ECDH Curve: %s\n", ctx->config.ecdh_curve == 1 ? "P-256" : "Unknown");
  LOG_INFO("AEAD Algorithm: %d\n", ctx->config.aead_alg);
  LOG_INFO("MAC Length: %d\n", ctx->config.mac_len);

  if(ctx->creds.authen_key != NULL) {
    LOG_INFO("Own Authentication Key: KID=%02x, Identity=%.*s\n",
             ctx->creds.authen_key->kid[0],
             ctx->creds.authen_key->identity_sz,
             ctx->creds.authen_key->identity);
  } else {
    LOG_INFO("Own Authentication Key: Not set\n");
  }

  LOG_INFO("Own Connection ID: ");
  if(ctx->state.cid_len > 0) {
    for(int i = 0; i < ctx->state.cid_len; i++) {
      LOG_INFO_("%02x", ctx->state.cid[i]);
    }
    LOG_INFO_("\n");
  } else {
    LOG_INFO_("Not set\n");
  }

  LOG_INFO("Peer Connection ID: ");
  if(ctx->state.cid_rx_len > 0) {
    for(int i = 0; i < ctx->state.cid_rx_len; i++) {
      LOG_INFO_("%02x", ctx->state.cid_rx[i]);
    }
    LOG_INFO_("\n");
  } else {
    LOG_INFO_("Not set\n");
  }

  LOG_INFO("===================================\n");
}
/*---------------------------------------------------------------------------*/
