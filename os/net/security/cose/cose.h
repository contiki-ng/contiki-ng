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
 *         Public API declarations for COSE (RFC8152)
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 *         Christos Koulamas <cklm@isi.gr>
 */
/**
 * \defgroup COSE A COSE implementation (RFC8152)
 * @{
 * This is an implementation of CBOR Object Signing and Encryption (COSE) protocol (IETF RFC 8152)
 * when COSE_Encrypt0 structures are used. This specification describes how to create and process signatures,
 * message authentication codes, and encryption using CBOR for serialization. This specification additionally
 * describes how to represent cryptographic keys using CBOR. The specific file implements just the encryption of
 * COSE_Encrypt0 structures used by EDHOC protocol.
 **/

#ifndef _COSE_H_
#define _COSE_H_

#include <stdint.h>
#include <stddef.h>
#include "cbor.h"

/* COSE Algorthim parameters */
#define COSE_Algorithm_AES_CCM_16_64_128 10
#define COSE_algorithm_AES_CCM_16_64_128_KEY_LEN 16
#define COSE_algorithm_AES_CCM_16_64_128_IV_LEN  13
#define COSE_algorithm_AES_CCM_16_64_128_TAG_LEN  8

/*#define SIG "Signature"
 #define SIG1 "Signature1"
 #define CSIG "CounterSignature"
 #ifdef COSE_CONF_SIGN
 #define SIGN COSE_CONF_SIGN
 #else
 #define SIGN SIG1
 #endif*/

/**
 * \brief context of the  different COSE data structures
 */
/* #define ENC "Encrypt" */
#define ENC0 "Encrypt0"
/* #define ENC_REC "Enc_Recipient" */
/* #define MAC_REC "Mac_Reciepient" */
/* #define REC_REC "Rec_Recipient" */

/**
 * \brief Set the COSE data structure to be used
 */
#ifdef COSE_CONF_ENC
#define RECIPIENT COSE_CONF_ENC
#else
#define RECIPIENT ENC0
#endif

/**
 * \brief Set the AEAD encryption algorithm
 */
#ifdef COSE_CONF_ALGORITHM_ID
#define COSE_ALG_ID COSE_CONF_ALGORITHM_ID
#else
#define COSE_ALG_ID COSE_Algorithm_AES_CCM_16_64_128
#endif

#if COSE_ALG_ID == COSE_Algorithm_AES_CCM_16_64_128
/**
 * \brief Set Key lenght
 */
#define KEY_LEN COSE_algorithm_AES_CCM_16_64_128_KEY_LEN
/**
 * \brief Set nonce lenght
 */
#define IV_LEN COSE_algorithm_AES_CCM_16_64_128_IV_LEN
/**
 * \brief Set TAG lenght
 */
#define TAG_LEN COSE_algorithm_AES_CCM_16_64_128_TAG_LEN
/*#define KEy_SIGN 645 39 36 b1 e8 7c 37 */
#endif

/**
 * \brief Set the Maximum Buffer length
 */
#ifdef COSE_CONF_MAX_BUFF
#define COSE_MAX_BUFFER COSE_CONF_MAX_BUFF
#define MAX_CIPHER COSE_CONF_MAX_BUFF
#else
#define COSE_MAX_BUFFER 256
/**
 * \brief Set the Maximum ciphertext length
 */
#define MAX_CIPHER 256
#endif

/**
 * \brief Maximum Algorithm Identify length
 */
#define MAX_ALG_SZ 4

/**
 * \brief Buffer struct
 */
typedef struct bstr_cose {
  uint8_t *buf;
  size_t len;
} bstr_cose;

/**
 * \brief String Buffer struct
 */
typedef struct sstr_cose {
  char *buf;
  size_t len;
} sstr_cose;

/* IN CONSTRACTION */
/*typedef struct cose_sign1 { //sig_structure RFC8152
   uint8_t protected_header[COSE_MAX_BUFFER];
   uint8_t protected_header_sz;
   uint8_t unprotected_header[COSE_MAX_BUFFER];
   uint8_t unprotected_header_sz;
   uint8_t payload[COSE_MAX_BUFFER];
   uint8_t payload_sz;
   uint8_t signature[COSE_MAX_BUFFER];
   uint8_t signature_sz;
   uint8_t external_aad[COSE_MAX_BUFFER];
   uint8_t external_aad_sz;
   uint8_t alg[MAX_ALG_SZ];
   uint8_t alg_sz;
   uint8_t key[MAX_KEY_SIGN];
   uint8_t key_sz;
   } cose_sign1;
 */
/*typedef struct cose_sign1 { ///sig_structure RFC8152
    bstr_cose protected_header; // For EDHOC: ID_CRED_V
    bstr_cose unprotected_header; // For EDHOC: Non include at the message may content paramters such as algorithm...
    bstr_cose payload;  // For EDHOC: CRED_V
    bstr_cose external_aad; // For EDHOC: TH2
   } cose_sign1;*/

/*typedef struct sig_structure {
   sstr_cose str_id;
   bstr_cose protected;
   bstr_cose external_aad;
   bstr_cose payload;
   } sig_structure;
 */

/**
 * \brief COSE_encrypt0 struct
 */
typedef struct cose_encrypt0 {
  uint8_t protected_header[COSE_MAX_BUFFER];
  uint8_t protected_header_sz;
  uint8_t unprotected_header[COSE_MAX_BUFFER];
  uint8_t unprotected_header_sz;
  uint8_t plaintext[COSE_MAX_BUFFER];
  uint8_t plaintext_sz;
  uint8_t ciphertext[MAX_CIPHER];
  uint8_t ciphertext_sz;
  uint8_t alg[MAX_ALG_SZ];
  uint8_t alg_sz;
  uint8_t key[KEY_LEN];
  uint8_t key_sz;
  uint8_t nonce[IV_LEN];
  uint8_t nonce_sz;
  uint8_t external_aad[COSE_MAX_BUFFER];
  uint8_t external_aad_sz;
} cose_encrypt0;

/**
 * \brief enc_structure struct [RFC8152]
 */
typedef struct enc_structure {
  sstr_cose str_id;
  bstr_cose protected;
  bstr_cose external_aad;
}enc_structure;

/**
 * \brief COSE_key struct [RFC8152]
 */
typedef struct cose_key {
  bstr_cose kid;  /* Key Identifier*/
  uint8_t kty;
  uint8_t crv;
  bstr_cose x;
  bstr_cose y;
  sstr_cose identity;
  char name[8];  /* Identity */
} cose_key;

/**
 * \brief Create a new cose_encrypt0 context
 * \return cose_encrypt0 new cose_encrypt0 context struct
 *
 * Used to create a news ose_encrypt0 and allocate at the memory reserved dynamically
 */
cose_encrypt0 *cose_encrypt0_new();

/**
 * \brief Close the cose_encrypt0 context
 * \param enc cose_encrypt0 context struct
 *
 * Used to de-allocate the memory reserved for the cose_encrypt0 context
 */
void cose_encrypt0_finalize(cose_encrypt0 *enc);

/*IN CONSTRACTION*/
/*cose_sign1 *sign1_new();
   void sign1_finalize(cose_sign1 *sig);
   uint8_t cose_sign1_set_key(cose_sign1 *sig, uint8_t alg, uint8_t *key, uint8_t key_sz);
   uint8_t cose_sign1_set_content(cose_sign1 *sig, uint8_t *payload, uint16_t paylod_sz, uint8_t *add, uint8_t add_sz);
   uint8_t cose_sign1_set_header(cose_sign1 *sig, uint8_t *prot, uint16_t prot_sz, uint8_t *unp, uint16_t unp_sz);
   uint8_t cose_sign(cose_sign1 *sig, uint8_t sz);*/

/**
 * \brief Set the encryption key/nonce and the algorithm identifier on the cose_encrytp0 context
 * \param enc output cose_encrypt0 context
 * \param alg input algorithm identifier
 * \param key input point to the encryption/decryption key
 * \param key_sz input encryption/decryption key length
 * \param nonce input point to the nonce (Initialization Vector (IV) value)
 * \param nonce_sz input nonce length
 * \return 1 if both key and nonce have the correct length and 0 otherwise
 *
 *  Used before encryption/decryption operation to select:
 *  - the algorithm used for the security processing
 *  - the encryption Key
 *  - the nonce: It is the Initialization Vector (IV) value
 *
 */
uint8_t cose_encrypt0_set_key(cose_encrypt0 *enc, uint8_t alg, uint8_t *key, uint8_t key_sz, uint8_t *nonce, uint16_t nonce_sz);

/**
 * \brief Set the plaintext and aad (additional authentication data) of the message
 * \param enc output cose_encrypt0 context
 * \param plain input The plaintext contained by the message
 * \param plain_sz  input The plaintext length
 * \param add input The Aditional Authentication Data
 * \param add_sz input The Aditional Authentication Data length
 * \return 1 and the plain_sz is smaller than the maximum buffer size
 *
 *  Used before encryption operation to select:
 *  - The plaintext or ciphertext contained by the message to encrypt
 *  - Additional Authentication Data (AAD) contained by the message
 */
uint8_t cose_encrypt0_set_content(cose_encrypt0 *enc, uint8_t *plain, uint16_t plain_sz, uint8_t *add, uint8_t add_sz);

/**
 * \brief Set the ciphertext of the encrypted message
 * \param enc output cose_encrypt0 context
 * \param ciphertext input The ciphertext contained by the cipher message
 * \param ciphertext_sz  input The ciphertext length
 * \return 1 and the ciphertext_sz is smaller than the maximum buffer size
 *
 *  Used before decryption operation to select:
 *  - The plaintext or ciphertext contained by the message to decrypt
 */
uint8_t cose_encrypt0_set_ciphertext(cose_encrypt0 *enc, uint8_t *ciphertext, uint16_t ciphertext_sz);

/**
 * \brief Set the protected/unprotected bucket header information of the message
 * \param enc output cose_encrypt0 context
 * \param prot input protected bucket
 * \param prot_sz input protected bucket length
 * \param unp input unprotected bucket
 * \param unp_sz input unprotected bucket length
 *
 *  Used before encryption/decryption operation to select:
 *  - The protected bucket contains parameters about the current layer that are to be cryptographically protected
 *  - The unprotected bucket contains parameters about the current layer that are not cryptographically protected
 */
void cose_encrypt0_set_header(cose_encrypt0 *enc, uint8_t *prot, uint16_t prot_sz, uint8_t *unp, uint16_t unp_sz);

/**
 * \brief  encrypt the COSE_encrypt0 struct using AEAD algorithm
 * \param enc cose_encrypt0 context
 * \return ciphertext_sz if the input parameter selected is appropriate and the cypher success and 0 otherwise
 *
 * This function implements the encryption algorithm AEAD on the data structure contained by the COSE_encrypt0 struct.
 * Before this function be called must be selected every necessary parameter of the enc (cose_encrypt0 context)
 * The ciphertext is returned in the ciphertext element of the cose_encrypt0 struct tagged by CBOR tag 16 bytes.
 *
 */
uint8_t cose_encrypt(cose_encrypt0 *enc);

/**
 * \brief  decrypt the COSE_encrypt0 ciphertext using AEAD algorithm
 * \param enc cose_encrypt0 context
 * \return 1 if the TAG checking success and 0 otherwise
 *
 * This function implements the encryption algorithm AEAD on the data ciphertext element of the COSE_Encrypt0 tagged struct
 * to decrypted and check the TAG.
 * Before this function be called must be selected the ciphertext element of the enc (cose_encrypt0 context)
 * The plaintext is returned in the plaintext element of the cose_encrypt0 struct and the plaintext length in the plaintext_sz
 * element as well.
 *
 */
uint8_t cose_decrypt(cose_encrypt0 *enc);

/**
 * \brief Print a key in a format of cose_key struct for debugging
 * \param cose input cose_key struct
 *
 */
void cose_print_key(cose_key *cose);

#endif /* _COSE_H_ */
/** @} */