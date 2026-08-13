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
 *         An implementation of Ephemeral Diffie-Hellman Over COSE (EDHOC)
 *         (RFC9528)
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund, Marco Tiloca
 *         Christos Koulamas <cklm@isi.gr>, Niclas Finne <niclas.finne@ri.se>,
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

/**
 * \defgroup EDHOC An EDHOC implementation (RFC9528)
 * @{
 *
 * This is an implementation of Ephemeral Diffie-Hellman Over COSE (EDHOC),
 * a very compact and lightweight authenticated Diffie-Hellman key exchange with
 * ephemeral keys that provides mutual authentication, perfect forward secrecy,
 * and identity protection as described in RFC9528.
 *
 **/

#ifndef EDHOC_H_
#define EDHOC_H_

#include "edhoc-config.h"
#include "ecdh.h"
#include "edhoc-msgs.h"
#include <stdint.h>
#include <string.h>

/* EDHOC_KDF label values */
#define KEYSTREAM_2_LABEL    0
#define SALT_3E2M_LABEL      1
#define MAC_2_LABEL          2
#define K_3_LABEL            3
#define IV_3_LABEL           4
#define SALT_4E3M_LABEL      5
#define MAC_3_LABEL          6
#define PRK_OUT_LABEL        7
#define K_4_LABEL            8
#define IV_4_LABEL           9
#define PRK_EXPORTER_LABEL   10

#define EDHOC_MAC_2 2
#define EDHOC_MAC_3 3

typedef struct edhoc_config {
  uint8_t role;
  uint8_t method;
  uint8_t suite[EDHOC_SUITES_MAX_COUNT];
  uint8_t suite_num;
  uint8_t aead_alg;
  uint8_t mac_len;
  uint8_t ecdh_curve;
  uint8_t sign_alg;
} edhoc_config_t;

typedef struct edhoc_state {
  uint8_t suite_selected;
  uint8_t cid[EDHOC_MAX_CID_LEN];     /* Own connection identifier */
  uint8_t cid_len;                    /* Length of own CID */
  uint8_t cid_rx[EDHOC_MAX_CID_LEN];  /* Peer's connection identifier */
  uint8_t cid_rx_len;                 /* Length of peer's CID */
  uint8_t th[HASH_LEN];
  uint8_t prk_2e[HASH_LEN];
  uint8_t prk_3e2m[HASH_LEN];
  uint8_t prk_4e3m[HASH_LEN];
  uint8_t gx[ECC_KEY_LEN];
} edhoc_state_t;

typedef struct edhoc_buffers {
  uint8_t msg_rx[EDHOC_MAX_PAYLOAD_LEN];     /* Receive buffer */
  uint8_t msg_tx[EDHOC_MAX_PAYLOAD_LEN];     /* Transmit buffer */
  uint16_t rx_sz;                            /* Size of received message */
  uint16_t tx_sz;                            /* Size of message to transmit */
  uint8_t plaintext[EDHOC_MAX_BUFFER];       /* Plaintext buffer (needed across message boundaries) */
  size_t plaintext_sz;
  uint8_t cred_x[EDHOC_MAX_CRED_LEN];        /* Credential storage */
  size_t cred_x_sz;
  uint8_t id_cred_x[EDHOC_MAX_ID_CRED_LEN];  /* Credential ID storage */
  size_t id_cred_x_sz;
} edhoc_buffers_t;

typedef struct edhoc_creds {
  cose_key_t *authen_key; /* Points to key in cred storage */
  ecc_key_t ephemeral_key;
} edhoc_creds_t;

typedef struct edhoc_context {
  edhoc_config_t config;
  edhoc_state_t state;
  edhoc_creds_t creds;
  edhoc_buffers_t buffers;
} edhoc_context_t;

/**
 * \brief EDHOC context struct used in the EDHOC protocol
 */
extern edhoc_context_t *edhoc_ctx;

/**
 * \brief Reserve memory for the EDHOC context struct
 *
 * Used by both Initiator and Responder EDHOC roles to reserve memory
 */
void edhoc_storage_init(void);

/**
 * \brief Create a new EDHOC context
 * \relates edhoc_storage_init
 * \return edhoc_context_t* Pointer to EDHOC context struct, or NULL on failure
 *
 * Used by both Initiator and Responder EDHOC roles to create a new EDHOC context
 * and allocate at the memory reserved before with the edhoc_storage_init function
 */
edhoc_context_t *edhoc_new(void);

/**
 * \brief Initialize the EDHOC Context with the defined EDHOC parameters
 * \param ctx An EDHOC context struct to fill in
 * \return 1 on success, 0 if no supported cipher suites are configured
 *
 * Used in the edhoc_new to set the default protocol definitions and in the Responder to
 * reset the initial values to prepare for a new EDHOC connection. Sets up the cipher
 * suites selection logic and validates that at least one cipher suite is supported.
 */

uint8_t edhoc_setup_suites(edhoc_context_t *ctx);

/**
 * \brief Close the EDHOC context
 * \param ctx EDHOC context struct
 *
 * Used by both Initiator and Responder EDHOC roles to de-allocate the memory reserved
 * for the EDHOC context when EDHOC protocol finalize.
 */
void edhoc_finalize(edhoc_context_t *ctx);

/* See edhoc-msg-generators.h for detailed documentation */
edhoc_error_t edhoc_generate_message_1(edhoc_context_t *ctx, uint8_t *ad, size_t ad_sz, bool suite_array);

/* See edhoc-msg-generators.h for detailed documentation */
edhoc_error_t edhoc_generate_message_2(edhoc_context_t *ctx, const uint8_t *auth_data, size_t auth_data_size);

/* See edhoc-msg-generators.h for detailed documentation */
edhoc_error_t edhoc_generate_message_3(edhoc_context_t *ctx, const uint8_t *auth_data, size_t auth_data_size);

/**
 * \brief Generate the EDHOC ERROR Message
 * \param msg_er A pointer to a buffer to copy the generated CBOR message error
 * \param msg_er_sz The size of the destination buffer
 * \param ctx EDHOC Context struct
 * \param err EDHOC error number
 * \return err_sz CBOR Message Error size
 *
 * An EDHOC error message can be sent by both parties as a reply to any non-error
 * EDHOC message. If any verification step fails on the EDHOC protocol the Initiator
 * or Responder must send an EDHOC error message back that contains a brief human-readable
 * diagnostic message.
 * - msg_er = (?C_x_identifier, ERR_MSG:tstr)
 */
uint8_t edhoc_generate_error_message(uint8_t *msg_er, size_t msg_er_sz, const edhoc_context_t *ctx, int8_t err);

/**
 * \brief Authenticate the rx message
 * \param ctx EDHOC Context struct
 * \param ad A pointer to a buffer to copy the Application Data of the rx message
 * \param msg2 Determines whether the message is a Message 2
 * \retval negative error code when an EDHOC ERROR is detected
 * \retval ad_sz The length of the Application Data received in Message 2, when EDHOC success
 *
 * Used by Initiator and Responder EDHOC role to Authenticate the other party
 * - Verify that the EDHOC Responder role identity is among the allowed if it is necessary
 * - Verify MAC
 * - Pass Application data AD
 *
 * If any verification step fails to return an EDHOC ERROR code and, if all the steps success
 * the length of the Application Data receive on the Message is returned.
 */
int edhoc_authenticate_msg(edhoc_context_t *ctx, uint8_t *ad, bool msg2);

/**
 * \brief EDHOC Key Derivation Function (KDF) based on HMAC-based Expand (RFC 5869)
 * \param result OKM (Output Keying Material) - the buffer where the derived key will be stored.
 * \param prk PRK (Pseudorandom Key) - a pseudorandom key used as input to the key derivation, should be at least `HASH_LEN` bytes.
 * \param info_label Label used to generate the CBOR-based info input for key derivation.
 * \param context Context data used to generate the info input for key derivation.
 * \param context_sz The size of the Context data.
 * \param length Desired length of the output key material (OKM) in bytes.
 * \return The number of output key bytes generated (equal to \p length) on success, or 0 on failure.
 *
 * This function combines the PRK, info_label, and context to generate an input info
 * parameter that is used for HKDF-Expand as defined in RFC 5869. It is used by both
 * the Initiator and Responder in the EDHOC protocol to generate keying material.
 * Internally, this function calls `edhoc_expand` to compute the final OKM.
 *
 * The function performs the following steps:
 *  - Calls `generate_info` to prepare the `info` input.
 *  - Passes the PRK, generated `info`, and length to `edhoc_expand`.
 *
 * Example usage:
 *  - OKM = EDHOC_Expand(PRK, info, length)
 */
int16_t edhoc_kdf(const uint8_t *prk, uint8_t info_label, const uint8_t *context, uint8_t context_sz, uint16_t length, uint8_t *result);

/**
 * \brief HMAC-based Key Expansion Function for EDHOC context using HKDF (RFC 5869)
 * \param prk PRK (Pseudorandom Key) - a pseudorandom key used as input for the HMAC-based key derivation.
 * \param info Additional context information used in the key derivation, which is generated from the info_label and context.
 * \param info_sz The size of the info parameter in bytes.
 * \param length The desired length of the output key material (OKM) in bytes.
 * \param result OKM (Output Keying Material) - the buffer where the derived key will be stored.
 * \return The number of output key bytes generated (equal to \p length).
 *
 * This function implements the HKDF-Expand function as described in RFC 5869.
 * It takes the PRK, context info, and the desired length to produce the final key material.
 * It calls hkdf_expand internally to perform the HMAC-based key expansion using SHA-256.
 *
 * The steps include:
 *  - Verifying the size of the info and output key material (OKM).
 *  - Using HMAC-Expand to expand the PRK and info into OKM.
 *  - Returning the length of the derived key or an error code in case of failure.
 *
 * Example usage:
 *  - OKM = HKDF-Expand(PRK, info, length)
 */
int16_t edhoc_expand(const uint8_t *prk, const uint8_t *info, uint16_t info_sz, uint16_t length, uint8_t *result);

/**
 * Internal API functions.
 */

/**
 * \brief Initialize the EDHOC context with default parameters
 * \param ctx EDHOC context to initialize
 * \return 1 on success, 0 on failure
 *
 * Sets up cipher suites, authentication keys, connection ID, role, and method
 */
uint8_t edhoc_initialize_context(edhoc_context_t *ctx);

/**
 * \brief Retrieve own authentication key from key storage
 * \param ctx EDHOC context
 * \param key Pointer to store the found authentication key
 * \return 1 if key found, 0 if not found
 *
 * Searches for authentication key using subject name or key ID
 */
uint8_t edhoc_get_own_auth_key(edhoc_context_t *ctx, cose_key_t **key);

/**
 * \brief Set EDHOC configuration parameters based on cipher suite
 * \param ctx EDHOC context
 * \param suite Selected cipher suite
 * \return 1 on success, 0 on failure
 *
 * Configures ECDH curve, MAC length, AEAD algorithm, and signature algorithm
 */
int8_t edhoc_set_config_from_suite(edhoc_context_t *ctx, uint8_t suite);

/**
 * \brief Print current EDHOC session information for debugging
 * \param ctx EDHOC context
 *
 * Logs session details including role, method, cipher suite, and connection IDs
 */
void edhoc_print_session_info(const edhoc_context_t *ctx);

/**
 * \brief Generate transcript hash TH_2
 * \param ctx EDHOC context
 * \param eph_pub Ephemeral public key
 * \param msg Message 1 buffer
 * \param msg_sz Message 1 size
 * \return 0 on success, negative on error
 *
 * Computes TH_2 = H(G_Y, H(message_1)) for EDHOC protocol
 */
int8_t edhoc_generate_transcript_hash_2(edhoc_context_t *ctx, const uint8_t *eph_pub,
                     uint8_t *msg, uint16_t msg_sz);

/**
 * \brief Generate transcript hash TH_3
 * \param ctx EDHOC context
 * \param cred Credential data
 * \param cred_sz Credential size
 * \param plaintext Plaintext data
 * \param plaintext_sz Plaintext size
 * \return 0 on success, 1 on buffer overflow
 *
 * Computes TH_3 = H(TH_2, PLAINTEXT_2, CRED_R) for EDHOC protocol
 */
uint8_t edhoc_generate_transcript_hash_3(edhoc_context_t *ctx,
                      const uint8_t *cred, uint16_t cred_sz,
                      const uint8_t *plaintext, uint16_t plaintext_sz);

/**
 * \brief Generate transcript hash TH_4
 * \param ctx EDHOC context
 * \param cred Credential data
 * \param cred_sz Credential size
 * \param plaintext Plaintext data
 * \param plaintext_sz Plaintext size
 * \return 0 on success, 1 on buffer overflow
 *
 * Computes TH_4 = H(TH_3, PLAINTEXT_3, CRED_I) for EDHOC protocol
 */
uint8_t edhoc_generate_transcript_hash_4(edhoc_context_t *ctx,
		      const uint8_t *cred, uint16_t cred_sz,
		      const uint8_t *plaintext, uint16_t plaintext_sz);

/**
 * \brief Generate pseudorandom key PRK_2e
 * \param ctx EDHOC context
 * \return true on success, false on failure
 *
 * Generates PRK_2e using ECDH shared secret and TH_2
 */
bool edhoc_generate_prk_2e(edhoc_context_t *ctx);

/**
 * \brief Generate pseudorandom key PRK_3e2m
 * \param ctx EDHOC context
 * \param auth_key Authentication key
 * \param gen Key generation mode
 * \return true on success, false on failure
 *
 * Generates PRK_3e2m using authentication key and ephemeral key
 */
bool edhoc_generate_prk_3e2m(edhoc_context_t *ctx, const ecc_key_t *auth_key,
			uint8_t gen);

/**
 * \brief Generate pseudorandom key PRK_4e3m
 * \param ctx EDHOC context
 * \param auth_key Authentication key
 * \param gen Key generation mode
 * \return true on success, false on failure
 *
 * Generates PRK_4e3m using authentication key and ephemeral key
 */
bool edhoc_generate_prk_4e3m(edhoc_context_t *ctx, const ecc_key_t *auth_key,
			uint8_t gen);

/**
 * \brief Generate keystream KS_2e for encryption
 * \param ctx EDHOC context
 * \param length Keystream length
 * \param ks_2e Output keystream buffer
 * \return 1 on success, negative on error
 *
 * Derives keystream for encrypting/decrypting EDHOC message 2
 */
int16_t edhoc_generate_keystream_2e(edhoc_context_t *ctx, uint16_t length, uint8_t *ks_2e);

/**
 * \brief Encrypt/decrypt ciphertext 2 using XOR with keystream
 * \param ctx EDHOC context
 * \param ks_2e Keystream for XOR operation
 * \param plaintext Input/output buffer for plaintext/ciphertext
 * \param plaintext_sz Buffer size
 * \return Size of processed data
 *
 * Performs XOR encryption/decryption of EDHOC message 2 content
 */
int16_t edhoc_enc_dec_ciphertext_2(const edhoc_context_t *ctx,
                                   const uint8_t *ks_2e,
                                   uint8_t *plaintext, uint16_t plaintext_sz);

/**
 * \brief Generate CBOR-encoded credential CRED_X
 * \param cose COSE key containing credential information
 * \param cred Output buffer for credential
 * \param cred_sz Output buffer size
 * \return Size of generated credential, 0 on error
 *
 * Generates CBOR credential structure from COSE key
 */
size_t edhoc_generate_cred_x(const cose_key_t *cose, uint8_t *cred, size_t cred_sz);

/**
 * \brief Generate credential identifier ID_CRED_X
 * \param cose COSE key containing credential information
 * \param cred Output buffer for credential ID
 * \param cred_sz Output buffer size
 * \return Size of generated credential ID, 0 on error
 *
 * Generates credential identifier (KID or full credential)
 */
size_t edhoc_generate_id_cred_x(const cose_key_t *cose, uint8_t *cred, size_t cred_sz);

/**
 * \brief Calculate message authentication code (MAC)
 * \param ctx EDHOC context
 * \param mac_num MAC number (2 or 3)
 * \param mac_len MAC length
 * \param mac Output buffer for MAC
 * \return 1 on success, 0 on failure
 *
 * Computes MAC_2 or MAC_3 for EDHOC message authentication
 */
uint8_t edhoc_calc_mac(const edhoc_context_t *ctx, uint8_t mac_num,
		       uint8_t mac_len, uint8_t *mac);

/**
 * \brief Print EDHOC session configuration summary
 * \param ctx EDHOC context
 *
 * Prints a comprehensive summary of the current EDHOC session configuration
 * including role, method, cipher suite, and key information.
 */
void edhoc_print_config_summary(const edhoc_context_t *ctx);

#endif /* EDHOC_H_ */
/** @} */
