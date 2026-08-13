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
 *         ECDH interface for the EDHOC implementation.
 *
 *         Thin synchronous wrapper around the Contiki-NG ECC driver
 *         (\c lib/ecc.h). Long-running operations from the underlying
 *         driver are busy-waited to completion so that the EDHOC
 *         protocol code can stay synchronous.
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>, Peter A Jonsson, Rikard Höglund, Marco Tiloca
 */
#ifndef _ECDH_H_
#define _ECDH_H_

#include <stdint.h>
#include <stdbool.h>
#include "edhoc-key-storage.h"

/**
 * \brief             Generates a fresh ECDH key pair.
 * \param curve_id    The EDHOC curve identifier (e.g. \c EDHOC_CURVE_P256).
 * \param pub_x       Output buffer for the x-coordinate (\c ECC_KEY_LEN bytes).
 * \param pub_y       Output buffer for the y-coordinate (\c ECC_KEY_LEN bytes).
 * \param priv        Output buffer for the private key (\c ECC_KEY_LEN bytes).
 * \return            true on success, false on error.
 */
bool ecdh_generate_keypair(uint8_t curve_id,
                           uint8_t *pub_x, uint8_t *pub_y, uint8_t *priv);

/**
 * \brief             Computes an ECDH shared secret.
 * \param curve_id    The EDHOC curve identifier (e.g. \c EDHOC_CURVE_P256).
 * \param peer_x      The peer's public-key x-coordinate (\c ECC_KEY_LEN bytes).
 *                    The y-coordinate is recovered from \p peer_x using
 *                    point decompression (odd-y branch), so EDHOC's
 *                    x-only on-wire encoding can be passed in directly.
 * \param private_key Our private key (\c ECC_KEY_LEN bytes).
 * \param ikm         Output buffer for the shared secret (\c ECC_KEY_LEN bytes).
 * \return            true on success, false on error.
 */
bool ecdh_generate_ikm(uint8_t curve_id,
                       const uint8_t *peer_x,
                       const uint8_t *private_key, uint8_t *ikm);

/**
 * \brief             Generates an ECDSA signature for a message hash.
 * \param curve_id    The EDHOC curve identifier (e.g. \c EDHOC_CURVE_P256).
 * \param hash        The message hash (\c ECC_KEY_LEN bytes).
 * \param private_key The signer's private key (\c ECC_KEY_LEN bytes).
 * \param signature   Output buffer for the signature (\c 2*ECC_KEY_LEN bytes).
 * \return            true on success, false on error.
 */
bool ecc_sign_hash(uint8_t curve_id,
                   const uint8_t *hash,
                   const uint8_t *private_key,
                   uint8_t *signature);

/**
 * \brief             Verifies an ECDSA signature of a message hash.
 * \param curve_id    The EDHOC curve identifier (e.g. \c EDHOC_CURVE_P256).
 * \param hash        The message hash (\c ECC_KEY_LEN bytes).
 * \param public_key  The signer's public key (\c 2*ECC_KEY_LEN bytes).
 * \param signature   The signature to verify (\c 2*ECC_KEY_LEN bytes).
 * \return            true if the signature is valid, false otherwise.
 */
bool ecc_verify_hash(uint8_t curve_id,
                     const uint8_t *hash,
                     const uint8_t *public_key,
                     const uint8_t *signature);

#endif /* _ECDH_H_ */
