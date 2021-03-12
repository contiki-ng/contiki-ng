/*
 * Copyright (c) 2020, Industrial Systems  Institute (ISI), Patras, Greece
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
 *         ecdh header
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */
#ifndef _ECDH_H_
#define _ECDH_H_

#include <stdint.h>
#include "lib/random.h"
#include <string.h>
#include <stdio.h>
#include "cose.h"
#include "edhoc-key-storage.h"
#include "hmac-sha.h"

/*Choose the ECC library to be used*/
#define CC2258_ECC 1
#define UECC_ECC 2
#define TEST_VECTOR 1

#ifdef EDHOC_TEST
#define TEST EDHOC_TEST
#else
#define TEST TEST_TOTAL
#endif

#define ERR_INFO_SIZE -1
#define ERR_OKM_SIZE -2

#ifdef EDHOC_CONF_ECC
#define ECC EDHOC_CONF_ECC
#else
#define ECC UECC_ECC
#endif

#ifdef EDHOC_CONF_MAX_PAYLOAD
#define MAX_PAYLOAD EDHOC_CONF_MAX_PAYLOAD
#else
#define MAX_PAYLOAD 256
#endif
#define MAX_KEY MAX_PAYLOAD

#if ECC == UECC_ECC
#include "ecc-uecc.h"
#endif

#if ECC == CC2538_ECC
#include "ecc-cc2538.h"
#endif

typedef struct session_key {
  uint8_t k2_e[MAX_KEY];
  uint8_t prk_2e[ECC_KEY_BYTE_LENGHT];
  uint8_t prk_3e2m[ECC_KEY_BYTE_LENGHT];
  uint8_t prk_4x3m[ECC_KEY_BYTE_LENGHT];
  uint8_t gx[ECC_KEY_BYTE_LENGHT];
  uint8_t gy[ECC_KEY_BYTE_LENGHT];
  uint8_t th[ECC_KEY_BYTE_LENGHT];
} session_key;

uint8_t generate_IKM(uint8_t *gx, uint8_t *gy, uint8_t *private_key, uint8_t *ikm, ecc_curve_t curve);
uint8_t compute_TH(uint8_t * in, uint8_t in_sz, uint8_t *hash, uint8_t hash_sz);
uint8_t hkdf_extrac(uint8_t *salt, uint8_t salt_sz, uint8_t *ikm, uint8_t ikm_sz, uint8_t *hmac);
int8_t hkdf_expand(uint8_t *prk, uint16_t prk_sz, uint8_t *info, uint16_t info_sz, uint8_t *okm, uint16_t okm_sz);
void generate_cose_key(ecc_key *key, cose_key *cose, char *identity, uint8_t id_sz);
void set_cose_key(ecc_key *key, cose_key *cose, cose_key_t *auth_key, ecc_curve_t curve);

#endif /* _ECDH_H_ */
