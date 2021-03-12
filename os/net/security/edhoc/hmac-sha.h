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
 *         hmac-sha header
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */
#ifndef _HMAC_SHA_H_
#define _HMAC_SHA_H_
#include <stdint.h>
#include <stddef.h>
#include "edhoc-log.h"
#include "edhoc-config.h"

#if SH256 == DECC_SH2
#define ECC_SH2 1
#endif
#if SH256 == DCC2538_SH2
#define CC2538_SH2 1
#endif

#define HMAC_BLOCKSIZE   64
#define HMAC_DIGEST_SIZE 32 /**< digest size (for SHA-256) */
#define HMAC_MAX    64  /**< max number of bytes in digest */
#define HASH_MAX 1 /*maximum number of hash functions that can be used in parallel */

#ifdef CC2538_SH2
#include "dev/rom-util.h"
#include "dev/sha256.h"
typedef struct {
  sha256_state_t data;              /**< context for hash function */
} sha_context_t;
#endif

#ifdef ECC_SH2
#include "sha.h"
typedef struct {
  SHA256Context data;             /**< context for hash function */
} sha_context_t;
#endif

typedef struct {
  unsigned char pad[HMAC_BLOCKSIZE]; /*< ipad and opad storage */
  sha_context_t sha;              /*< context for hash function */
} hmac_context_t;

void hmac_storage_init(void);
hmac_context_t *hmac_new(const unsigned char *key, size_t klen);
uint8_t hmac_init(hmac_context_t *ctx, const unsigned char *key, size_t klen);
uint8_t hmac_update(hmac_context_t *ctx, const unsigned char *input, size_t ilen);
int hmac_finalize(hmac_context_t *ctx, unsigned char *result);
void hmac_free(hmac_context_t *ctx);

uint8_t sha256(uint8_t *input, uint8_t input_sz, uint8_t *output);

#endif
