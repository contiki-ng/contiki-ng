/*
 * Copyright (c) 2024, RISE Research Institutes of Sweden AB
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
 */

/**
 * \file
 *         RFC 9529 Test Credentials for EDHOC
 *
 *         This header contains the test credentials from RFC 9529 for
 *         EDHOC protocol testing. These are publicly known credentials
 *         and MUST NOT be used in production systems.
 *
 *         The credentials support Method 3 (Static Diffie-Hellman) authentication.
 */

#ifndef EDHOC_CRED_RFC9529_H_
#define EDHOC_CRED_RFC9529_H_

#include "edhoc-key-storage.h"
#include <string.h>

/*---------------------------------------------------------------------------*/
/* RFC 9529 Test Credentials - Method 3 (Static Diffie-Hellman) */
/*---------------------------------------------------------------------------*/

/**
 * RFC 9529 Static Diffie-Hellman Client Authentication Key
 */
static const cose_key_t auth_rfc9529_static_dh_client = {
  /* Pointer to next key in linked list */
  .next = NULL,
  /* Key Identifier array */
  .kid = { 0x2b },
  /* Key Identifier size */
  .kid_sz = 1,
  /* Subject identity string */
  .identity = { "42-50-31-FF-EF-37-32-39" },
  /* Subject identity size */
  .identity_sz = strlen("42-50-31-FF-EF-37-32-39"),
  /* Key type (2 = EC2) */
  .kty = 2,
  /* Curve identifier (1 = P-256) */
  .crv = 1,
  /* ECC key data */
  .ecc = {
    /* Private key bytes */
    .priv = { 0xfb, 0x13, 0xad, 0xeb, 0x65, 0x18, 0xce, 0xe5, 0xf8, 0x84, 0x17, 0x66, 0x08, 0x41, 0x14, 0x2e,
              0x83, 0x0a, 0x81, 0xfe, 0x33, 0x43, 0x80, 0xa9, 0x53, 0x40, 0x6a, 0x13, 0x05, 0xe8, 0x70, 0x6b },
    /* Public key point */
    .pub = {
      /* Public key X coordinate */
      .x = { 0xac, 0x75, 0xe9, 0xec, 0xe3, 0xe5, 0x0b, 0xfc, 0x8e, 0xd6, 0x03, 0x99, 0x88, 0x95, 0x22, 0x40,
             0x5c, 0x47, 0xbf, 0x16, 0xdf, 0x96, 0x66, 0x0a, 0x41, 0x29, 0x8c, 0xb4, 0x30, 0x7f, 0x7e, 0xb6 },
      /* Public key Y coordinate */
      .y = { 0x6e, 0x5d, 0xe6, 0x11, 0x38, 0x8a, 0x4b, 0x8a, 0x82, 0x11, 0x33, 0x4a, 0xc7, 0xd3, 0x7e, 0xcb,
             0x52, 0xa3, 0x87, 0xd2, 0x57, 0xe6, 0xdb, 0x3c, 0x2a, 0x93, 0xdf, 0x21, 0xff, 0x3a, 0xff, 0xc8 }
    }
  }
};

/**
 * RFC 9529 Static Diffie-Hellman Server Authentication Key
 */
static const cose_key_t auth_rfc9529_static_dh_server = {
  /* Pointer to next key in linked list */
  .next = NULL,
  /* Key Identifier array */
  .kid = { 0x32 },
  /* Key Identifier size */
  .kid_sz = 1,
  /* Subject identity string */
  .identity = { "example.edu" },
  /* Subject identity size */
  .identity_sz = strlen("example.edu"),
  /* Key type (2 = EC2) */
  .kty = 2,
  /* Curve identifier (1 = P-256) */
  .crv = 1,
  /* ECC key data */
  .ecc = {
    /* Private key bytes */
    .priv = { 0x72, 0xcc, 0x47, 0x61, 0xdb, 0xd4, 0xc7, 0x8f, 0x75, 0x89, 0x31, 0xaa, 0x58, 0x9d, 0x34, 0x8d,
              0x1e, 0xf8, 0x74, 0xa7, 0xe3, 0x03, 0xed, 0xe2, 0xf1, 0x40, 0xdc, 0xf3, 0xe6, 0xaa, 0x4a, 0xac },
    /* Public key point */
    .pub = {
      /* Public key X coordinate */
      .x = { 0xbb, 0xc3, 0x49, 0x60, 0x52, 0x6e, 0xa4, 0xd3, 0x2e, 0x94, 0x0c, 0xad, 0x2a, 0x23, 0x41, 0x48,
             0xdd, 0xc2, 0x17, 0x91, 0xa1, 0x2a, 0xfb, 0xcb, 0xac, 0x93, 0x62, 0x20, 0x46, 0xdd, 0x44, 0xf0 },
      /* Public key Y coordinate */
      .y = { 0x45, 0x19, 0xe2, 0x57, 0x23, 0x6b, 0x2a, 0x0c, 0xe2, 0x02, 0x3f, 0x09, 0x31, 0xf1, 0xf3, 0x86,
             0xca, 0x7a, 0xfd, 0xa6, 0x4f, 0xcd, 0xe0, 0x10, 0x8c, 0x22, 0x4c, 0x51, 0xea, 0xbf, 0x60, 0x72 }
    }
  }
};

#endif /* EDHOC_CRED_RFC9529_H_ */
