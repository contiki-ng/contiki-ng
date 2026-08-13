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
 *         EDHOC key storage - Implementation of key storage for managing
 *         DH-static authentication key pairs. Can also be used to store
 *         the DH-static authentication public keys of other EDHOC peers.
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund, Marco Tiloca
 *         Christos Koulamas <cklm@isi.gr>
 */
#ifndef _EDHOC_KEY_STORAGE_H_
#define _EDHOC_KEY_STORAGE_H_
#include "contiki.h"
#include "contiki-lib.h"
#include "ecc-common.h"
#include "edhoc-error.h"
#include "edhoc-config.h"
#include <stdio.h>

/**
 * \brief KEY length in bytes
 *
 */
#ifndef ECC_KEY_LEN
#define ECC_KEY_LEN 32
#endif

/* COSE KEY struct */
typedef struct cose_key {
  struct  cose_key_t *next;
  uint8_t kid[EDHOC_MAX_KID_LEN];
  uint8_t kid_sz;
  char identity[EDHOC_MAX_IDENTITY_LEN];
  uint8_t identity_sz;
  uint8_t kty;
  uint8_t crv;
  ecc_key_t ecc;
} cose_key_t;

/**
 * \brief Create the keys repository
 *
 * Create a repository of keys in the form of a list
 * \return EDHOC_SUCCESS on success, error code on failure
 */
edhoc_error_t edhoc_create_key_list(void);

/**
 * \brief Add a DH key to the repository
 * \param key Input key to add in cose_key_t format
 *
 * Adds a new key to the repository in the form of a cose_key_t struct.
 * \return EDHOC_SUCCESS on success, error code on failure
 */
edhoc_error_t edhoc_add_key(cose_key_t *key);

/**
 * \brief Check in the keys repository for the key with the specific KID
 * \param kid Input Key Identification
 * \param kid_sz Input Key Identification length
 * \param auth_key Output cose_key_t key that corresponds to the KID
 * \return EDHOC_SUCCESS if found, EDHOC_ERR_KEY_NOT_FOUND if not found, other error code on failure
 *
 * This function checks the repository and returns a DH cose_key_t key
 * that is associated with the requested KID if it exists.
 */
edhoc_error_t edhoc_check_key_list_kid(uint8_t *kid, uint8_t kid_sz, cose_key_t **auth_key);

/**
 * \brief Check in the keys repository for the key with the specific identity
 * \param identity Input key identity
 * \param identity_sz Input key identity length
 * \param auth_key Output cose_key_t key that corresponds to the identity
 * \return EDHOC_SUCCESS if found, EDHOC_ERR_KEY_NOT_FOUND if not found, other error code on failure
 *
 * This function checks the repository and returns a DH cose_key_t key
 * that is associated with the requested identity if it exists.
 */
edhoc_error_t edhoc_check_key_list_identity(char *identity, uint8_t identity_sz, cose_key_t **auth_key);

/**
 * \brief Remove from the keys repository the key with the specific KID
 * \param kid Input Key Identification
 * \param kid_sz Input Key Identification length
 * \return EDHOC_SUCCESS if removed, EDHOC_ERR_KEY_NOT_FOUND if not found, other error code on failure
 *
 * This function deletes from the repository the DH cose_key_t key
 * that is associated with the KID if it exists.
 */
edhoc_error_t edhoc_remove_key_kid(uint8_t *kid, uint8_t kid_sz);

/**
 * \brief Remove from the keys repository the key with the specific identity
 * \param identity Input key identity
 * \param identity_sz Input key identity length
 * \return EDHOC_SUCCESS if removed, EDHOC_ERR_KEY_NOT_FOUND if not found, other error code on failure
 *
 * This function deletes from the repository the DH cose_key_t key
 * that is associated with the identity if it exists.
 */
edhoc_error_t edhoc_remove_key_identity(char *identity, uint8_t identity_sz);

/**
 * \brief Remove from the keys repository the specific DH cose_key_t key
 * \param auth_key Input key to remove from the repository
 *
 * This function deletes from the repository the DH cose_key_t key pointed to by the auth_key parameter.
 * \return EDHOC_SUCCESS on success, error code on failure
 */
edhoc_error_t edhoc_remove_key(cose_key_t *auth_key);

/**
 * \brief Copy a COSE key from one structure to another
 * \param k The destination where the COSE key will be copied
 * \param key The source COSE key to be copied
 *
 * This function copies the contents of the COSE key structure from the source (key)
 * to the destination (k). The entire structure is copied using memcpy.
 * \return EDHOC_SUCCESS on success, error code on failure
 */
edhoc_error_t edhoc_copy_key(cose_key_t *k, cose_key_t *key);

/**
 * \brief Print a key in cose_key_t struct format for debugging
 * \param cose Input cose_key_t struct
 */
void cose_print_key(cose_key_t *cose);

/**
 * \brief Print detailed key information at INFO level for user visibility
 * \param key The COSE key to print
 * \param label A descriptive label for the key (e.g., "Client", "Server")
 */
void edhoc_print_key_info(const cose_key_t *key, const char *label);

/**
 * \brief Print credential (CRED_I/CRED_R) values in readable format
 * \param label A descriptive label for the credential (e.g., "CRED_I", "CRED_R")
 * \param cred The credential bytes to print
 * \param cred_sz The size of the credential
 */
void edhoc_print_credential(const char *label, const uint8_t *cred, size_t cred_sz);

/**
 * \brief Get the total number of keys loaded in the key storage
 * \return The number of keys currently stored
 */
uint8_t edhoc_get_key_count(void);

/**
 * \brief List all keys in the key storage with basic information
 */
void edhoc_list_all_keys(void);

/**
 * \brief Validate the current key setup and print diagnostic information
 * \return EDHOC_SUCCESS if validation passes, error code otherwise
 */
edhoc_error_t edhoc_validate_key_setup(void);

/**
 * \brief Helper function to set up a key pair (own + peer) with validation
 * \param own_key The key for own identity (with private key)
 * \param peer_key The key for peer identity (public only)
 * \param own_label Descriptive label for own key
 * \param peer_label Descriptive label for peer key
 * \return EDHOC_SUCCESS if setup successful, error code otherwise
 */
edhoc_error_t edhoc_setup_key_pair(cose_key_t *own_key, cose_key_t *peer_key,
                                   const char *own_label, const char *peer_label);

#endif
