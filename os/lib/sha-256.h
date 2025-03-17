/*
 * Copyright (c) 2021, Uppsala universitet.
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
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \addtogroup crypto
 * @{
 * \file
 *         Platform-independent SHA-256 API.
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#ifndef SHA_256_H_
#define SHA_256_H_

#include "contiki.h"
#include <stddef.h>
#include <stdint.h>

#define SHA_256_DIGEST_LENGTH 32
#define SHA_256_BLOCK_SIZE 64

#ifdef SHA_256_CONF
#define SHA_256 SHA_256_CONF
#else /* SHA_256_CONF */
#define SHA_256 sha_256_driver
#endif /* SHA_256_CONF */

typedef struct {
  uint64_t bit_count;
  uint32_t state[SHA_256_DIGEST_LENGTH / sizeof(uint32_t)];
  uint8_t buf[SHA_256_BLOCK_SIZE];
  size_t buf_len;
  uint8_t opad[SHA_256_BLOCK_SIZE]; /* HMAC's outer padding */
} sha_256_checkpoint_t;

/**
 * Structure of SHA-256 drivers.
 */
struct sha_256_driver {

  /**
   * \brief Starts a hash session.
   */
  void (* init)(void);

  /**
   * \brief Processes a chunk of data.
   * \param data pointer to the data to hash
   * \param len  length of the data to hash in bytes
   */
  void (* update)(const uint8_t *data, size_t len);

  /**
   * \brief Terminates the hash session and produces the digest.
   * \param digest pointer to the hash value
   */
  void (* finalize)(uint8_t digest[static SHA_256_DIGEST_LENGTH]);

  /**
   * \brief Saves the hash session, e.g., before pausing a protothread.
   */
  void (* create_checkpoint)(sha_256_checkpoint_t *checkpoint);

  /**
   * \brief Restores a hash session, e.g., after resuming a protothread.
   */
  void (* restore_checkpoint)(const sha_256_checkpoint_t *checkpoint);

  /**
   * \brief Does init, update, and finalize at once.
   * \param data   pointer to the data to hash
   * \param len    length of the data to hash in bytes
   * \param digest pointer to the hash value
   */
  void (* hash)(const uint8_t *data, size_t len,
                uint8_t digest[static SHA_256_DIGEST_LENGTH]);
};

extern const struct sha_256_driver SHA_256;

/**
 * \brief Generic implementation of sha_256_driver#hash.
 */
void sha_256_hash(const uint8_t *data, size_t len,
                  uint8_t digest[static SHA_256_DIGEST_LENGTH]);

/**
 * \brief Initiates a stepwise HMAC-SHA-256 computation.
 * \param key     the key to authenticate with
 * \param key_len length of key in bytes
 */
void sha_256_hmac_init(const uint8_t *key, size_t key_len);

/**
 * \brief Proceeds with the computation of an HMAC-SHA-256.
 * \param data     further data to authenticate
 * \param data_len length of data in bytes
 */
void sha_256_hmac_update(const uint8_t *data, size_t data_len);

/**
 * \brief Finishes the computation of an HMAC-SHA-256.
 * \param hmac pointer to where the resulting HMAC shall be stored
 */
void sha_256_hmac_finish(uint8_t hmac[static SHA_256_DIGEST_LENGTH]);

/**
 * \brief Computes HMAC-SHA-256 as per RFC 2104.
 * \param key      the key to authenticate with
 * \param key_len  length of key in bytes
 * \param data     the data to authenticate
 * \param data_len length of data in bytes
 * \param hmac     pointer to where the resulting HMAC shall be stored
 */
void sha_256_hmac(const uint8_t *key, size_t key_len,
                  const uint8_t *data, size_t data_len,
                  uint8_t hmac[static SHA_256_DIGEST_LENGTH]);

/**
 * \brief Extracts a key as per RFC 5869.
 * \param salt     optional salt value
 * \param salt_len length of salt in bytes
 * \param ikm      input keying material
 * \param ikm_len  length of ikm in bytes
 * \param prk      pointer to where the extracted key shall be stored
 */
void sha_256_hkdf_extract(const uint8_t *salt, size_t salt_len,
                          const uint8_t *ikm, size_t ikm_len,
                          uint8_t prk[static SHA_256_DIGEST_LENGTH]);

/**
 * \brief Expands a key as per RFC 5869.
 * \param prk      a pseudorandom key of at least SHA_256_DIGEST_LENGTH bytes
 * \param prk_len  length of prk in bytes
 * \param info     optional context and application specific information
 * \param info_len length of info in bytes
 * \param okm      output keying material
 * \param okm_len  length of okm in bytes (<= 255 * SHA_256_DIGEST_LENGTH)
 */
void sha_256_hkdf_expand(const uint8_t *prk, size_t prk_len,
                         const uint8_t *info, size_t info_len,
                         uint8_t *okm, uint_fast16_t okm_len);

/**
 * \brief Performs both extraction and expansion as per RFC 5869.
 * \param salt     optional salt value
 * \param salt_len length of salt in bytes
 * \param ikm      input keying material
 * \param ikm_len  length of ikm in bytes
 * \param info     optional context and application specific information
 * \param info_len length of info in bytes
 * \param okm      output keying material
 * \param okm_len  length of okm in bytes (<= 255 * SHA_256_DIGEST_LENGTH)
 */
void sha_256_hkdf(const uint8_t *salt, size_t salt_len,
                  const uint8_t *ikm, size_t ikm_len,
                  const uint8_t *info, size_t info_len,
                  uint8_t *okm, uint_fast16_t okm_len);

#endif /* SHA_256_H_ */

/** @} */
