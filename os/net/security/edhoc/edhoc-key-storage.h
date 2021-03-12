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
 *         edhoc-msgs header an implementation of key storage to keep the
 *         DH-static authentication pair keys to work with. Can be used to keep also
 *         the DH-static authentication public key of the other EDHOC parts.
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 *         Christos Koulamas <cklm@isi.gr>
 */
#ifndef _EDHOC_KEY_STORAGE_H_
#define _EDHOC_KEY_STORAGE_H_
#include "contiki.h"
#include "contiki-lib.h"
#include <stdio.h>

/**
 * \brief KEY lenght in bytes
 *
 */
#ifndef ECC_KEY_BYTE_LENGHT
#define ECC_KEY_BYTE_LENGHT 32
#endif

/* COSE KEY struct */
typedef struct cose_key_t {
  struct  key *next;
  uint8_t kid[4];
  uint8_t kid_sz;
  char identity[16];
  uint8_t identity_sz;
  uint8_t kty;
  uint8_t crv;
  uint8_t x[ECC_KEY_BYTE_LENGHT];
  uint8_t y[ECC_KEY_BYTE_LENGHT];
  uint8_t private[ECC_KEY_BYTE_LENGHT];
}cose_key_t;

/**
 * \brief Create the keys repository
 *
 * Create a repository of keys in the form of a list
 */
void edhoc_create_key_list();

/**
 * \brief Add a DH key to the repository
 * \param key input key to add in cose_key_t format
 *
 * Add new keys to the repository in a form of cose_key_t struct
 */
void edhoc_add_key(cose_key_t *key);

/**
 * \brief Check in the keys repository for the key with the specific kid
 * \param kid input Key Identification
 * \param kid_sz input Key Identification lenght
 * \param auth_key output cose_key_t key that corresponds to the kid
 * \return 1 if a key with the specific kid exists in the repository 0 otherwise
 *
 *  This function checks the repository and return a DH cose_key_t key
 *  that is associated with the requested kid if exist.
 */
uint8_t edhoc_check_key_list_kid(uint8_t *kid, uint8_t kid_sz, cose_key_t **auth_key);

/**
 * \brief Check in the keys repository for the key with the specific identity
 * \param identity input key identity
 * \param identity_sz input key identity lenght
 * \param auth_key output cose_key_t key that corresponds to the identity
 * \return 1 if a key with the specific identity exists in the repository 0 otherwise
 *
 *  This function checks the repository and return a DH cose_key_t key
 *  that is associated with the requested kid if exist.
 */
uint8_t edhoc_check_key_list_identity(char *identity, uint8_t identity_sz, cose_key_t **auth_key);

/**
 * \brief Remove from the keys repository the key with the specific kid
 * \param kid input Key Identification
 * \param kid_sz input Key Identification lenght
 * \return 1 if a key with the specific kid existed in the repository 0 otherwise
 *
 *  This function delete from the repository the DH cose_key_t key
 *  that is associated with the kid if exist.
 */
uint8_t edhoc_remove_key_kid(uint8_t *kid, uint8_t kid_sz);

/**
 * \brief Remove from the keys repository the key with the specific identity
 * \param identity input key identity
 * \param identity_sz input key identity lenght
 * \return 1 if a key with the specific identity existed in the repository 0 otherwise
 *
 *  This function delete from the repository the DH cose_key_t key
 *  that is associated with the kid if exist.
 */
uint8_t edhoc_remove_key_identity(char *identity, uint8_t identity_sz);

/**
 * \brief Remove from the keys repository the specific DH cose_key_t key
 * \param auth_key input key to remove from the repository
 *
 *  This function deletes from the repository the DH cose_key_t key pointed by the auth_key parameter
 */
void edhoc_remove_key(cose_key_t *auth_key);
#endif