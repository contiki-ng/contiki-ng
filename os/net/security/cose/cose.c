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
 *         Stateless COSE helpers (RFC 9052) used by EDHOC.
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 *         Peter A Jonsson
 *         Rikard Höglund
 *         Marco Tiloca
 *         Niclas Finne <niclas.finne@ri.se>
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include "contiki.h"
#include "cose.h"
#include "ecdh.h"
#include "lib/cbor.h"
#include "lib/ccm-star.h"
#include "lib/sha-256.h"
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "COSE"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*
 * Maximum size of the serialized Enc_structure / Sig_structure used as
 * AAD or signed input. The structures contain three or four CBOR
 * elements: a short context string, an empty (or short) protected
 * header, the external AAD, and (for Sig_structure) the payload. The
 * largest contributor in EDHOC is the external AAD, which is bounded by
 * the EDHOC payload buffer.
 */
#define COSE_STRUCTURE_BUF_LEN (2 * EDHOC_MAX_PAYLOAD_LEN)

/*---------------------------------------------------------------------------*/
static char enc_header[] = "Encrypt0";
static char sig_header[] = "Signature1";
/*---------------------------------------------------------------------------*/
static size_t
encode_enc_structure(const uint8_t *external_aad, size_t external_aad_len,
                     uint8_t *out, size_t out_len)
{
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, out, out_len);
  cbor_open_array(&writer);
  cbor_write_text(&writer, enc_header, strlen(enc_header));
  /* Empty protected header. */
  cbor_write_data(&writer, NULL, 0);
  cbor_write_data(&writer, external_aad, external_aad_len);
  cbor_close_array(&writer);
  return cbor_end_writer(&writer);
}
/*---------------------------------------------------------------------------*/
static size_t
encode_sig_structure(const uint8_t *protected_hdr, size_t protected_hdr_len,
                     const uint8_t *external_aad, size_t external_aad_len,
                     const uint8_t *payload, size_t payload_len,
                     uint8_t *out, size_t out_len)
{
  cbor_writer_state_t writer;
  cbor_init_writer(&writer, out, out_len);
  cbor_open_array(&writer);
  cbor_write_text(&writer, sig_header, strlen(sig_header));
  cbor_write_data(&writer, protected_hdr, protected_hdr_len);
  cbor_write_data(&writer, external_aad, external_aad_len);
  cbor_write_data(&writer, payload, payload_len);
  cbor_close_array(&writer);
  return cbor_end_writer(&writer);
}
/*---------------------------------------------------------------------------*/
size_t
cose_encrypt0_seal(uint8_t alg,
                   const uint8_t *key, const uint8_t *nonce,
                   const uint8_t *external_aad, size_t external_aad_len,
                   uint8_t *buf, size_t plaintext_len,
                   size_t buf_capacity)
{
  uint8_t tag_len = cose_get_tag_len(alg);
  if(tag_len == 0 || cose_get_key_len(alg) == 0 || cose_get_iv_len(alg) == 0) {
    LOG_ERR("Unsupported COSE AEAD algorithm %u\n", alg);
    return 0;
  }

  /* The tag is appended in place, so buf must hold plaintext + tag. */
  if(plaintext_len > buf_capacity || buf_capacity - plaintext_len < tag_len) {
    LOG_ERR("COSE_Encrypt0 output buffer too small for plaintext + tag\n");
    return 0;
  }

  uint8_t enc_struct[COSE_STRUCTURE_BUF_LEN];
  size_t enc_struct_len = encode_enc_structure(external_aad, external_aad_len,
                                               enc_struct, sizeof(enc_struct));
  if(enc_struct_len == 0) {
    LOG_ERR("Failed to encode Enc_structure for COSE_Encrypt0\n");
    return 0;
  }

  /* The AEAD interface takes uint16_t lengths; reject anything that
     would be silently truncated. */
  if(plaintext_len > UINT16_MAX || enc_struct_len > UINT16_MAX) {
    LOG_ERR("COSE_Encrypt0 input too large for the AEAD interface\n");
    return 0;
  }

  /* CCM_STAR is a shared, stateful primitive: set_key() and the aead() that
     consumes it must run without an intervening yield or reentrant use. This
     holds under Contiki-NG cooperative scheduling and EDHOC's single handshake. */
  CCM_STAR.set_key(key);
  CCM_STAR.aead(nonce,
                buf, plaintext_len,
                enc_struct, enc_struct_len,
                buf + plaintext_len, tag_len, 1);

  return plaintext_len + tag_len;
}
/*---------------------------------------------------------------------------*/
size_t
cose_encrypt0_open(uint8_t alg,
                   const uint8_t *key, const uint8_t *nonce,
                   const uint8_t *external_aad, size_t external_aad_len,
                   uint8_t *buf, size_t ciphertext_len)
{
  uint8_t tag_len = cose_get_tag_len(alg);
  if(tag_len == 0 || cose_get_key_len(alg) == 0 || cose_get_iv_len(alg) == 0) {
    LOG_ERR("Unsupported COSE AEAD algorithm %u\n", alg);
    return 0;
  }
  if(ciphertext_len < tag_len) {
    LOG_ERR("Ciphertext (%zu) shorter than COSE tag length (%u)\n",
            ciphertext_len, tag_len);
    return 0;
  }

  uint8_t enc_struct[COSE_STRUCTURE_BUF_LEN];
  size_t enc_struct_len = encode_enc_structure(external_aad, external_aad_len,
                                               enc_struct, sizeof(enc_struct));
  if(enc_struct_len == 0) {
    LOG_ERR("Failed to encode Enc_structure for COSE_Encrypt0\n");
    return 0;
  }

  size_t plaintext_len = ciphertext_len - tag_len;

  /* The AEAD interface takes uint16_t lengths; reject anything that
     would be silently truncated. */
  if(plaintext_len > UINT16_MAX || enc_struct_len > UINT16_MAX) {
    LOG_ERR("COSE_Encrypt0 input too large for the AEAD interface\n");
    return 0;
  }

  uint8_t expected_tag[COSE_MAX_TAG_LEN];

  /*
   * CCM_STAR is a shared, stateful primitive (see cose_encrypt0_seal). The
   * buffer is decrypted in place here before the tag below is checked, so on
   * authentication failure buf holds unauthenticated data and must be discarded
   * by the caller.
   */
  CCM_STAR.set_key(key);
  CCM_STAR.aead(nonce,
                buf, plaintext_len,
                enc_struct, enc_struct_len,
                expected_tag, tag_len, 0);

  /* Constant-time comparison to prevent timing attacks. */
  uint8_t diff = 0;
  for(uint8_t i = 0; i < tag_len; i++) {
    diff |= expected_tag[i] ^ buf[plaintext_len + i];
  }
  if(diff != 0) {
    LOG_ERR("COSE_Encrypt0 authentication tag verification failed\n");
    return 0;
  }
  return plaintext_len;
}
/*---------------------------------------------------------------------------*/
size_t
cose_sign1_sign(int8_t alg, const uint8_t *private_key,
                const uint8_t *protected_hdr, size_t protected_hdr_len,
                const uint8_t *external_aad, size_t external_aad_len,
                const uint8_t *payload, size_t payload_len,
                uint8_t *signature)
{
  if(alg != ES256) {
    LOG_ERR("Unsupported COSE_Sign1 algorithm %d (only ES256=%d is supported)\n",
            alg, ES256);
    return 0;
  }

  uint8_t sig_struct[COSE_STRUCTURE_BUF_LEN];
  size_t sig_struct_len = encode_sig_structure(protected_hdr, protected_hdr_len,
                                               external_aad, external_aad_len,
                                               payload, payload_len,
                                               sig_struct, sizeof(sig_struct));
  if(sig_struct_len == 0) {
    LOG_ERR("Failed to encode Sig_structure for COSE_Sign1 signing\n");
    return 0;
  }

  uint8_t hash[HASH_LEN];
  sha_256_hash(sig_struct, sig_struct_len, hash);

  if(!ecc_sign_hash(EDHOC_CURVE_P256, hash, private_key, signature)) {
    LOG_ERR("COSE_Sign1 signature generation failed\n");
    return 0;
  }
  return P256_SIGNATURE_LEN;
}
/*---------------------------------------------------------------------------*/
bool
cose_sign1_verify(int8_t alg, const uint8_t *public_key,
                  const uint8_t *protected_hdr, size_t protected_hdr_len,
                  const uint8_t *external_aad, size_t external_aad_len,
                  const uint8_t *payload, size_t payload_len,
                  const uint8_t *signature, size_t signature_len)
{
  if(alg != ES256) {
    LOG_ERR("Unsupported COSE_Sign1 algorithm %d (only ES256=%d is supported)\n",
            alg, ES256);
    return false;
  }
  if(signature_len != P256_SIGNATURE_LEN) {
    LOG_ERR("COSE_Sign1: unexpected signature length %zu (expected %d)\n",
            signature_len, P256_SIGNATURE_LEN);
    return false;
  }

  uint8_t sig_struct[COSE_STRUCTURE_BUF_LEN];
  size_t sig_struct_len = encode_sig_structure(protected_hdr, protected_hdr_len,
                                               external_aad, external_aad_len,
                                               payload, payload_len,
                                               sig_struct, sizeof(sig_struct));
  if(sig_struct_len == 0) {
    LOG_ERR("Failed to encode Sig_structure for COSE_Sign1 verification\n");
    return false;
  }

  uint8_t hash[HASH_LEN];
  sha_256_hash(sig_struct, sig_struct_len, hash);

  if(!ecc_verify_hash(EDHOC_CURVE_P256, hash, public_key, signature)) {
    LOG_ERR("COSE_Sign1 signature verification failed\n");
    return false;
  }
  return true;
}
/*---------------------------------------------------------------------------*/
uint8_t
cose_get_key_len(uint8_t alg_id)
{
  switch(alg_id) {
  case COSE_ALG_AES_CCM_16_64_128:
    return COSE_ALG_AES_CCM_16_64_128_KEY_LEN;
  case COSE_ALG_AES_CCM_16_128_128:
    return COSE_ALG_AES_CCM_16_128_128_KEY_LEN;
  default:
    LOG_ERR("Invalid COSE algorithm %d specified\n", alg_id);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
cose_get_iv_len(uint8_t alg_id)
{
  switch(alg_id) {
  case COSE_ALG_AES_CCM_16_64_128:
    return COSE_ALG_AES_CCM_16_64_128_IV_LEN;
  case COSE_ALG_AES_CCM_16_128_128:
    return COSE_ALG_AES_CCM_16_128_128_IV_LEN;
  default:
    LOG_ERR("Invalid COSE algorithm %d specified\n", alg_id);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
uint8_t
cose_get_tag_len(uint8_t alg_id)
{
  switch(alg_id) {
  case COSE_ALG_AES_CCM_16_64_128:
    return COSE_ALG_AES_CCM_16_64_128_TAG_LEN;
  case COSE_ALG_AES_CCM_16_128_128:
    return COSE_ALG_AES_CCM_16_128_128_TAG_LEN;
  default:
    LOG_ERR("Invalid COSE algorithm %d specified\n", alg_id);
    return 0;
  }
}
/*---------------------------------------------------------------------------*/
