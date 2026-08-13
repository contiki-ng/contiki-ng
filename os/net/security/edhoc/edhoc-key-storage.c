/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB.
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
 *         edhoc-key-storage an implementation of a key storage to keep the ECC authentication keys to work with.
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Rikard Höglund, Marco Tiloca
 */

#include "edhoc-key-storage.h"
#include "edhoc-error.h"
#include "contiki.h"
#include "contiki-lib.h"
#include <string.h>

#include "sys/log.h"
#define LOG_MODULE "EDHOC"
#define LOG_LEVEL LOG_LEVEL_EDHOC

/*---------------------------------------------------------------------------*/
LIST(key_list);
MEMB(key_memb, cose_key_t, 2);
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_create_key_list(void)
{
  list_init(key_list);
  memb_init(&key_memb);
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_check_key_list_identity(char *identity, uint8_t identity_size,
                              cose_key_t **authentication_key)
{
  if(identity == NULL || authentication_key == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  if(identity_size == 0 || identity_size > EDHOC_MAX_IDENTITY_LEN) {
    return EDHOC_ERR_INVALID_LENGTH;
  }

  cose_key_t *key;
  for(key = list_head(key_list); key != NULL; key = list_item_next(key)) {
    if(key->identity_sz == identity_size &&
       memcmp(key->identity, identity, identity_size) == 0) {
      *authentication_key = key;
      return EDHOC_SUCCESS;
    }
  }
  return EDHOC_ERR_KEY_NOT_FOUND;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_check_key_list_kid(uint8_t *key_id, uint8_t key_id_size, cose_key_t **authentication_key)
{
  if(key_id == NULL || authentication_key == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  if(key_id_size == 0 || key_id_size > sizeof(((cose_key_t*)0)->kid)) {
    return EDHOC_ERR_INVALID_LENGTH;
  }

  cose_key_t *key;
  for(key = list_head(key_list); key != NULL; key = list_item_next(key)) {
    if(key->kid_sz == key_id_size &&
       memcmp(key->kid, key_id, key_id_size) == 0) {
      *authentication_key = key;
      return EDHOC_SUCCESS;
    }
  }
  return EDHOC_ERR_KEY_NOT_FOUND;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_add_key(cose_key_t *key)
{
  if(key == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  /* Check for duplicate keys */
  cose_key_t *existing_key = NULL;
  if(edhoc_check_key_list_kid(key->kid, key->kid_sz, &existing_key) == EDHOC_SUCCESS) {
    return EDHOC_ERR_DUPLICATE_KEY;
  }

  cose_key_t *k = memb_alloc(&key_memb);
  if(k == NULL) {
    LOG_ERR("Failed to allocate memory for key\n");
    return EDHOC_ERR_MEMORY_ALLOCATION;
  }

  memcpy(k, key, sizeof(cose_key_t));
  list_add(key_list, k);

  LOG_DBG("Added key KID 0x%02x (sz=%d), identity='%.*s'\n",
          k->kid[0], k->kid_sz, k->identity_sz, k->identity);

  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_copy_key(cose_key_t *k, cose_key_t *key)
{
  if(k == NULL || key == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  memcpy(k, key, sizeof(cose_key_t));
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_remove_key_kid(uint8_t *kid, uint8_t kid_sz)
{
  if(kid == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  cose_key_t *key = NULL;
  edhoc_error_t result = edhoc_check_key_list_kid(kid, kid_sz, &key);
  if(result == EDHOC_SUCCESS) {
    list_remove(key_list, key);
    memb_free(&key_memb, key);
    return EDHOC_SUCCESS;
  }
  return result; /* Propagate the error from check function */
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_remove_key_identity(char *identity, uint8_t identity_sz)
{
  if(identity == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  cose_key_t *key = NULL;
  edhoc_error_t result = edhoc_check_key_list_identity(identity, identity_sz, &key);
  if(result == EDHOC_SUCCESS) {
    list_remove(key_list, key);
    memb_free(&key_memb, key);
    return EDHOC_SUCCESS;
  }
  return result; /* Propagate the error from check function */
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_remove_key(cose_key_t *auth_key)
{
  if(auth_key == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  list_remove(key_list, auth_key);
  memb_free(&key_memb, auth_key);
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
void
cose_print_key(cose_key_t *cose)
{
  LOG_DBG("kid: ");
  LOG_DBG_BYTES(cose->kid, cose->kid_sz);
  LOG_DBG_("\n");
  LOG_DBG("identity: ");
  LOG_DBG_BYTES((uint8_t *)cose->identity, cose->identity_sz);
  LOG_DBG_("\n");
  LOG_DBG("kty: %d\n", cose->kty);
  LOG_DBG("crv: %d\n", cose->crv);
  LOG_DBG("x: ");
  LOG_DBG_BYTES(cose->ecc.pub.x, ECC_KEY_LEN);
  LOG_DBG_("\n");
  LOG_DBG("y: ");
  LOG_DBG_BYTES(cose->ecc.pub.y, ECC_KEY_LEN);
  LOG_DBG_("\n");
}
/*---------------------------------------------------------------------------*/
void
edhoc_print_key_info(const cose_key_t *key, const char *label)
{
  if(key == NULL || label == NULL) {
    LOG_ERR("Invalid parameters for key info print\n");
    return;
  }

  LOG_INFO("=== %s Key Information ===\n", label);
  LOG_INFO("KID: ");
  for(int i = 0; i < key->kid_sz; i++) {
    LOG_INFO_("%02x", key->kid[i]);
  }
  LOG_INFO_("\n");
  LOG_INFO("Identity: %.*s\n", key->identity_sz, key->identity);
  LOG_INFO("Key Type: %s\n", key->kty == 2 ? "EC2" : "Unknown");
  LOG_INFO("Curve: %s\n", key->crv == 1 ? "P-256" : "Unknown");
  LOG_INFO("Public Key X: ");
  for(int i = 0; i < 8; i++) {
    LOG_INFO_("%02x ", key->ecc.pub.x[i]);
  }
  LOG_INFO_("... (32 bytes total)\n");
  LOG_INFO("Public Key Y: ");
  for(int i = 0; i < 8; i++) {
    LOG_INFO_("%02x ", key->ecc.pub.y[i]);
  }
  LOG_INFO_("... (32 bytes total)\n");
  LOG_INFO("Private Key: %s\n",
           (key->ecc.priv[0] == 0 && key->ecc.priv[1] == 0) ? "Not present" : "Present");
  LOG_INFO("=============================\n");
}
/*---------------------------------------------------------------------------*/
void
edhoc_print_credential(const char *label, const uint8_t *cred, size_t cred_sz)
{
  if(label == NULL || cred == NULL || cred_sz == 0) {
    LOG_ERR("Invalid parameters for credential print\n");
    return;
  }

  LOG_INFO("%s (%zu bytes): ", label, cred_sz);
  for(size_t i = 0; i < cred_sz && i < 32; i++) {
    LOG_INFO_("%02x ", cred[i]);
  }
  if(cred_sz > 32) {
    LOG_INFO_("... (%zu more bytes)", cred_sz - 32);
  }
  LOG_INFO_("\n");

  LOG_INFO("%s structure: {identity: ..., cose_key: {kty, kid, crv, x, y}}\n", label);
}
/*---------------------------------------------------------------------------*/
uint8_t
edhoc_get_key_count(void)
{
  uint8_t count = 0;
  cose_key_t *key;
  for(key = list_head(key_list); key != NULL; key = list_item_next(key)) {
    count++;
  }
  return count;
}
/*---------------------------------------------------------------------------*/
void
edhoc_list_all_keys(void)
{
  cose_key_t *key;
  uint8_t count = 0;

  for(key = list_head(key_list); key != NULL; key = list_item_next(key)) {
    count++;
    LOG_INFO("  %d. KID: %02x, Identity: %.*s, Type: %s\n",
             count, key->kid[0], key->identity_sz, key->identity,
             key->kty == 2 ? "EC2" : "Unknown");
  }

  if(count == 0) {
    LOG_INFO("  No keys loaded\n");
  }
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_validate_key_setup(void)
{
  LOG_INFO("=== EDHOC Key Configuration Check ===\n");

  uint8_t key_count = edhoc_get_key_count();
  if(key_count == 0) {
    LOG_ERR("No keys loaded!\n");
    return EDHOC_ERR_KEY_NOT_FOUND;
  }

  LOG_INFO("Loaded keys (%d total):\n", key_count);
  edhoc_list_all_keys();

  cose_key_t *key;
  uint8_t keys_with_private = 0;
  uint8_t keys_public_only = 0;

  for(key = list_head(key_list); key != NULL; key = list_item_next(key)) {
    if(key->ecc.priv[0] != 0 || key->ecc.priv[1] != 0) {
      keys_with_private++;
    } else {
      keys_public_only++;
    }

    if(key->kty != 2) {
      LOG_WARN("Key %02x: Non-EC2 key type (%d)\n", key->kid[0], key->kty);
    }

    if(key->crv != 1) {
      LOG_WARN("Key %02x: Non-P-256 curve (%d)\n", key->kid[0], key->crv);
    }
  }

  LOG_INFO("Keys with private key: %d (for own identity)\n", keys_with_private);
  LOG_INFO("Public-only keys: %d (for peer verification)\n", keys_public_only);

  if(keys_with_private == 0) {
    LOG_WARN("No keys with private key found - cannot authenticate as any identity\n");
  }

  if(keys_public_only == 0) {
    LOG_WARN("No public-only keys found - cannot verify peer credentials\n");
  }

  LOG_INFO("=====================================\n");
  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
edhoc_error_t
edhoc_setup_key_pair(cose_key_t *own_key, cose_key_t *peer_key,
                     const char *own_label, const char *peer_label)
{
  if(own_key == NULL || peer_key == NULL || own_label == NULL || peer_label == NULL) {
    return EDHOC_ERR_NULL_POINTER;
  }

  memset(peer_key->ecc.priv, 0, sizeof(peer_key->ecc.priv));

  edhoc_error_t result = edhoc_add_key(own_key);
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to add %s key: %s\n", own_label, edhoc_error_string(result));
    return result;
  }

  result = edhoc_add_key(peer_key);
  if(result != EDHOC_SUCCESS) {
    LOG_ERR("Failed to add %s key: %s\n", peer_label, edhoc_error_string(result));
    return result;
  }

  LOG_INFO("Key pair configured: %s (KID: %02x) <-> %s (KID: %02x)\n",
           own_label, own_key->kid[0], peer_label, peer_key->kid[0]);

  return EDHOC_SUCCESS;
}
/*---------------------------------------------------------------------------*/
