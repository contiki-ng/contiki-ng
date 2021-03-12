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
 *         ecc-uecc headers
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */
#ifndef _ECC_UECC_H_
#define _ECC_UECC_H_

#include <stdint.h>
#include "edhoc-config.h"
#include "edhoc-log.h"

#include "lib/random.h"
#include "sys/rtimer.h"
#include "sys/pt.h"
#include <uECC.h>
#include <string.h>
#include <stdio.h>
#define uECC_PLATFORM uECC_arm

typedef struct point_affine {
  uint8_t x[ECC_KEY_BYTE_LENGHT];
  uint8_t y[ECC_KEY_BYTE_LENGHT];
} ecc_point_a;
typedef struct ecc_key {
  uint8_t kid[4];
  uint8_t kid_sz;
  uint8_t private_key[ECC_KEY_BYTE_LENGHT];
  ecc_point_a public;
  char *identity;
  uint8_t identity_size;
} ecc_key;

typedef struct ecc_curve_t {
  uECC_Curve curve;
}ecc_curve_t;

uint8_t uecc_generate_key(ecc_key *key, ecc_curve_t curve);
void uecc_uncompress(uint8_t *compressed, uint8_t *gx, uint8_t *gy, ecc_curve_t *curve);
uint8_t uecc_generate_IKM(uint8_t *gx, uint8_t *gy, uint8_t *private_key, uint8_t *ikm, ecc_curve_t curve);

#endif
