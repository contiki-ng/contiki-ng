/*
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
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
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
 *         COSE, an implementation of COSE_Encrypt0 structure from: CBOR Object Signing and Encryption (COSE)(IETF RFC 8152)
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */

#include "cose.h"
#include "contiki-lib.h"
#include <os/lib/ccm-star.h>
#include <string.h>
#include "cose-log.h"

MEMB(encrypt0_storage, cose_encrypt0, 1);

static inline cose_encrypt0 *
encrypt0_storage_new()
{
  return (cose_encrypt0 *)memb_alloc(&encrypt0_storage);
}
static inline void
encrypt0_free(cose_encrypt0 *enc)
{
  memb_free(&encrypt0_storage, enc);
}
static void
encrypt0_storage_init(void)
{
  memb_init(&encrypt0_storage);
}
cose_encrypt0 *
cose_encrypt0_new()
{
  encrypt0_storage_init();
  cose_encrypt0 *enc;
  enc = encrypt0_storage_new();
  return enc;
}
void
cose_encrypt0_finalize(cose_encrypt0 *enc)
{
  encrypt0_free(enc);
}
static char enc_rec[] = RECIPIENT;

void
cose_print_key(cose_key *cose)
{
  LOG_DBG("kid:");
  cose_print_buff_8_dbg(cose->kid.buf, cose->kid.len);
  LOG_DBG("identity:");
  cose_print_char_8_dbg((uint8_t *)cose->identity.buf, cose->identity.len);
  LOG_DBG("kty: %d\n", cose->kty);
  LOG_DBG("crv: %d\n", cose->crv);
  LOG_DBG("x:");
  cose_print_buff_8_dbg(cose->x.buf, cose->x.len);
  LOG_DBG("y:");
  cose_print_buff_8_dbg(cose->y.buf, cose->y.len);
}
uint8_t
cose_encrypt0_set_key(cose_encrypt0 *enc, uint8_t alg, uint8_t *key, uint8_t key_sz, uint8_t *nonce, uint16_t nonce_sz)
{
  if(key_sz != KEY_LEN) {
    return 0;
  }
  if(nonce_sz != IV_LEN) {
    return 0;
  }
  enc->key_sz = key_sz;
  enc->nonce_sz = nonce_sz;
  memcpy(enc->key, key, key_sz);
  memcpy(enc->nonce, nonce, nonce_sz);
  return 1;
}
uint8_t
cose_encrypt0_set_content(cose_encrypt0 *enc, uint8_t *plain, uint16_t plain_sz, uint8_t *add, uint8_t add_sz)
{
  if(plain_sz > COSE_MAX_BUFFER) {
    return 0;
  }
  memcpy(enc->plaintext, plain, plain_sz);
  memcpy(enc->external_aad, add, add_sz);
  enc->plaintext_sz = plain_sz;
  enc->external_aad_sz = add_sz;
  return 1;
}
uint8_t
cose_encrypt0_set_ciphertext(cose_encrypt0 *enc, uint8_t *ciphertext, uint16_t ciphertext_sz)
{
  if(ciphertext_sz > MAX_CIPHER) {
    return 0;
  }
  memcpy(enc->ciphertext, ciphertext, ciphertext_sz);
  enc->ciphertext_sz = ciphertext_sz;
  return 1;
}
void
cose_encrypt0_set_header(cose_encrypt0 *enc, uint8_t *prot, uint16_t prot_sz, uint8_t *unp, uint16_t unp_sz)
{
  memcpy(enc->protected_header, prot, prot_sz);
  memcpy(enc->unprotected_header, unp, unp_sz);
  enc->protected_header_sz = prot_sz;
  enc->unprotected_header_sz = unp_sz;
}
static uint8_t
encode_enc_structure(enc_structure str, uint8_t *cbor)
{
  uint8_t size = cbor_put_array(&cbor, 3);
  size += cbor_put_text(&cbor, str.str_id.buf, strlen(str.str_id.buf));
  size += cbor_put_bytes(&cbor, str.protected.buf, str.protected.len);
  size += cbor_put_bytes(&cbor, str.external_aad.buf, str.external_aad.len);
  return size;
}
uint8_t
cose_decrypt(cose_encrypt0 *enc)
{
  LOG_DBG("Decrypt:\n");
  LOG_DBG("external aad size %d:", enc->external_aad_sz);
  cose_print_buff_8_dbg(enc->external_aad, enc->external_aad_sz);
  LOG_DBG("Plaintext:");
  cose_print_buff_8_dbg(enc->plaintext, enc->plaintext_sz);
  LOG_DBG("protected:");
  cose_print_buff_8_dbg(enc->protected_header, enc->protected_header_sz);
  enc_structure str = {
    .str_id = (sstr_cose){ enc_rec, sizeof(enc_rec) }, /*Encrypt0 */
    .protected = (bstr_cose){ enc->protected_header, enc->protected_header_sz }, /*empty */
    .external_aad = (bstr_cose){ enc->external_aad, enc->external_aad_sz }, /* OLD REF TH@ */
  }; /* the enc estructure have tha autetification data */

  uint8_t str_encode[2 * COSE_MAX_BUFFER];
  uint8_t str_sz = encode_enc_structure(str, str_encode);

  LOG_INFO("(CBOR-encoded AAD) (%d bytes):", str_sz);
  cose_print_buff_8_info(str_encode, str_sz);
  LOG_DBG("Key:");
  cose_print_buff_8_dbg(enc->key, KEY_LEN);
  LOG_DBG("nonce:");
  cose_print_buff_8_dbg(enc->nonce, IV_LEN);
  uint8_t tag[TAG_LEN];
  CCM_STAR.set_key(enc->key);
  enc->plaintext_sz = enc->ciphertext_sz - TAG_LEN;

  CCM_STAR.aead(enc->nonce, enc->ciphertext, enc->plaintext_sz, str_encode, str_sz, tag, TAG_LEN, 0);
  memcpy(enc->plaintext, enc->ciphertext, enc->plaintext_sz);

  LOG_DBG("DEC-CIPHERTEX:");
  cose_print_buff_8_dbg(enc->ciphertext, enc->ciphertext_sz);
  LOG_DBG("DEC-PLAINTEXT:");
  cose_print_buff_8_dbg(enc->plaintext, enc->plaintext_sz);
  LOG_DBG("TAG:");
  cose_print_buff_8_dbg(&enc->ciphertext[enc->plaintext_sz], TAG_LEN);
  LOG_DBG("TAG 2:");
  cose_print_buff_8_dbg(tag, TAG_LEN);
  LOG_DBG("str encode:");
  cose_print_buff_8_dbg(str_encode, str_sz);

  if(memcmp(tag, &(enc->ciphertext[enc->plaintext_sz]), TAG_LEN) != 0) {
    LOG_ERR("Decrypt msg error\n");
    return 0;     /* Decryption failure */
  }
  return 1;
}
uint8_t
cose_encrypt(cose_encrypt0 *enc)
{
  LOG_DBG("Encrypt:\n");
  LOG_DBG("Key:");
  cose_print_buff_8_dbg(enc->key, enc->key_sz);
  LOG_DBG("IV:");
  cose_print_buff_8_dbg(enc->nonce, enc->nonce_sz);
  LOG_DBG("external aad (%d):", enc->external_aad_sz);
  cose_print_buff_8_dbg(enc->external_aad, enc->external_aad_sz);
  LOG_DBG("Plaintext:");
  cose_print_buff_8_dbg(enc->plaintext, enc->plaintext_sz);
  LOG_DBG("protected:");
  cose_print_buff_8_dbg(enc->protected_header, enc->protected_header_sz);

  enc_structure str = {
    .str_id = (sstr_cose){ enc_rec, sizeof(enc_rec) }, /*Encrypt0 */
    .protected = (bstr_cose){ enc->protected_header, enc->protected_header_sz }, /*empty */
    .external_aad = (bstr_cose){ enc->external_aad, enc->external_aad_sz }, /* OLD REF TH@ */
  }; /* the enc estructure have tha autetification data */

  uint8_t str_encode[2 * COSE_MAX_BUFFER];
  uint8_t str_sz = encode_enc_structure(str, str_encode);
  LOG_INFO("(CBOR-encoded AAD) (%d bytes):", str_sz);
  cose_print_buff_8_info(str_encode, str_sz);
  LOG_DBG("Key:");
  cose_print_buff_8_dbg(enc->key, KEY_LEN);
  LOG_DBG("nonce:");
  cose_print_buff_8_dbg(enc->nonce, IV_LEN);

  /*TO DO: check the algorithm selected in enc */
  if(enc->key_sz != KEY_LEN || enc->nonce_sz != IV_LEN || enc->plaintext_sz > COSE_MAX_BUFFER || str_sz > (2 * COSE_MAX_BUFFER)) {
    LOG_ERR("The cose parameter are not corresponing with the selected algorithm or buffer sizes\n");
    return 0;
  }

  CCM_STAR.set_key(enc->key);
  memcpy(enc->ciphertext, enc->plaintext, enc->plaintext_sz);
  LOG_DBG("Set ciphhertext with plaintex:");
  cose_print_buff_8_dbg(enc->ciphertext, enc->plaintext_sz);

  CCM_STAR.aead(enc->nonce, enc->ciphertext, enc->plaintext_sz, str_encode, str_sz, &enc->ciphertext[enc->plaintext_sz], TAG_LEN, 1);
  enc->ciphertext_sz = enc->plaintext_sz + TAG_LEN;
  LOG_DBG("ciphertext&tag:");
  cose_print_buff_8_dbg(enc->ciphertext, enc->ciphertext_sz);
  LOG_DBG("ciphertext:");
  cose_print_buff_8_dbg(enc->ciphertext, enc->plaintext_sz);
  LOG_DBG("TAG: ");
  cose_print_buff_8_dbg(&enc->ciphertext[enc->plaintext_sz], TAG_LEN);
  return enc->ciphertext_sz;
}