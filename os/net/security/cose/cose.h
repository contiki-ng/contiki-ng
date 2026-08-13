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
 *         Public API declarations for COSE (RFC 9052).
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Rikard Höglund, Marco Tiloca
 *         Christos Koulamas <cklm@isi.gr>, Niclas Finne <niclas.finne@ri.se>,
 *         Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

/**
 * \defgroup COSE A COSE implementation (RFC 9052)
 * @{
 *
 * Stateless helpers for the small subset of COSE used by EDHOC:
 * COSE_Encrypt0 with AES-CCM* and COSE_Sign1 with ES256.
 */

#ifndef _COSE_H_
#define _COSE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "edhoc-config.h"

/* COSE algorithm: AES-CCM-16-64-128. */
#define COSE_ALG_AES_CCM_16_64_128         10
#define COSE_ALG_AES_CCM_16_64_128_KEY_LEN 16
#define COSE_ALG_AES_CCM_16_64_128_IV_LEN  13
#define COSE_ALG_AES_CCM_16_64_128_TAG_LEN  8

/* COSE algorithm: AES-CCM-16-128-128. */
#define COSE_ALG_AES_CCM_16_128_128         30
#define COSE_ALG_AES_CCM_16_128_128_KEY_LEN 16
#define COSE_ALG_AES_CCM_16_128_128_IV_LEN  13
#define COSE_ALG_AES_CCM_16_128_128_TAG_LEN 16

/* Maxima across all supported COSE AEAD algorithms. */
#define COSE_MAX_KEY_LEN 16
#define COSE_MAX_IV_LEN  13
#define COSE_MAX_TAG_LEN 16

/*
 * Signature algorithm identifiers from the IANA "COSE Algorithms" registry
 * (RFC 9053). The values are CBOR integer labels, not magnitudes; signature
 * algorithms are registered with negative identifiers. Only ES256 is supported
 * here; EDDSA and ES384 are listed for reference.
 */
#define ES256 (-7)
#define EDDSA (-8)
#define ES384 (-35)

/**
 * \brief AEAD-encrypt a buffer in place as a COSE_Encrypt0.
 * \param alg          COSE AEAD algorithm identifier.
 * \param key          Symmetric key (length implied by \p alg).
 * \param nonce        Nonce/IV (length implied by \p alg).
 * \param external_aad CBOR-encoded external Additional Authenticated Data.
 * \param external_aad_len Length of \p external_aad in bytes.
 * \param buf          In/out buffer; on entry holds the plaintext, on
 *                     return holds plaintext || authentication tag.
 * \param plaintext_len Length of the plaintext in \p buf.
 * \param buf_capacity Total number of bytes available in \p buf. Must be at
 *                     least \p plaintext_len plus the tag length for \p alg,
 *                     or the call fails without writing the tag.
 * \return             Total ciphertext length on success, 0 on failure.
 *
 * The protected header is treated as empty, matching how EDHOC uses
 * COSE_Encrypt0. The \c Enc_structure built internally is
 * <tt>[ "Encrypt0", h'', external_aad ]</tt>.
 */
size_t cose_encrypt0_seal(uint8_t alg,
                          const uint8_t *key, const uint8_t *nonce,
                          const uint8_t *external_aad, size_t external_aad_len,
                          uint8_t *buf, size_t plaintext_len,
                          size_t buf_capacity);

/**
 * \brief AEAD-decrypt a buffer in place as a COSE_Encrypt0.
 * \param alg          COSE AEAD algorithm identifier.
 * \param key          Symmetric key (length implied by \p alg).
 * \param nonce        Nonce/IV (length implied by \p alg).
 * \param external_aad CBOR-encoded external Additional Authenticated Data.
 * \param external_aad_len Length of \p external_aad in bytes.
 * \param buf          In/out buffer; on entry holds plaintext || tag, on
 *                     return holds the recovered plaintext.
 * \param ciphertext_len Length of plaintext || tag in \p buf.
 * \return             Plaintext length on success, 0 on failure
 *                     (including authentication failure).
 *
 * The protected header is treated as empty, matching how EDHOC uses
 * COSE_Encrypt0.
 *
 * \warning Decryption happens in place before the tag is verified, so on
 *          failure (return 0) \p buf holds unauthenticated, attacker-influenced
 *          data. Callers must discard \p buf unless the return value is nonzero.
 */
size_t cose_encrypt0_open(uint8_t alg,
                          const uint8_t *key, const uint8_t *nonce,
                          const uint8_t *external_aad, size_t external_aad_len,
                          uint8_t *buf, size_t ciphertext_len);

/**
 * \brief Produce a COSE_Sign1 signature.
 * \param alg          Signing algorithm identifier (only \c ES256 is supported).
 * \param private_key  ECDSA private key.
 * \param protected_hdr CBOR-encoded protected header bytes.
 * \param protected_hdr_len Length of \p protected_hdr.
 * \param external_aad CBOR-encoded external Additional Authenticated Data.
 * \param external_aad_len Length of \p external_aad.
 * \param payload      Payload bytes to sign.
 * \param payload_len  Length of \p payload.
 * \param signature    Output buffer for the signature
 *                     (\c P256_SIGNATURE_LEN bytes for ES256).
 * \return             Signature length on success, 0 on failure.
 */
size_t cose_sign1_sign(int8_t alg, const uint8_t *private_key,
                       const uint8_t *protected_hdr, size_t protected_hdr_len,
                       const uint8_t *external_aad, size_t external_aad_len,
                       const uint8_t *payload, size_t payload_len,
                       uint8_t *signature);

/**
 * \brief Verify a COSE_Sign1 signature.
 * \param alg          Signing algorithm identifier (only \c ES256 is supported).
 * \param public_key   ECDSA public key (\c 2 * \c ECC_KEY_LEN bytes for ES256).
 * \param protected_hdr CBOR-encoded protected header bytes.
 * \param protected_hdr_len Length of \p protected_hdr.
 * \param external_aad CBOR-encoded external Additional Authenticated Data.
 * \param external_aad_len Length of \p external_aad.
 * \param payload      Payload bytes that were signed.
 * \param payload_len  Length of \p payload.
 * \param signature    Signature to verify.
 * \param signature_len Length of \p signature.
 * \return             true on a valid signature, false otherwise.
 */
bool cose_sign1_verify(int8_t alg, const uint8_t *public_key,
                       const uint8_t *protected_hdr, size_t protected_hdr_len,
                       const uint8_t *external_aad, size_t external_aad_len,
                       const uint8_t *payload, size_t payload_len,
                       const uint8_t *signature, size_t signature_len);

/**
 * \brief Get the symmetric key length for a COSE AEAD algorithm.
 * \param alg_id COSE algorithm identifier.
 * \return       Key length in bytes, 0 if the algorithm is unknown.
 */
uint8_t cose_get_key_len(uint8_t alg_id);

/**
 * \brief Get the nonce/IV length for a COSE AEAD algorithm.
 * \param alg_id COSE algorithm identifier.
 * \return       IV length in bytes, 0 if the algorithm is unknown.
 */
uint8_t cose_get_iv_len(uint8_t alg_id);

/**
 * \brief Get the authentication tag length for a COSE AEAD algorithm.
 * \param alg_id COSE algorithm identifier.
 * \return       Tag length in bytes, 0 if the algorithm is unknown.
 */
uint8_t cose_get_tag_len(uint8_t alg_id);

#endif /* _COSE_H_ */
/** @} */
