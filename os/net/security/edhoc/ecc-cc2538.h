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
 *         ecc-ccc2538 headers
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */
#ifndef _ECC_CC2538_H_
#define _ECC_CC2538_H_
#if uECC
#include <stdint.h>
#include "lib/random.h"
#include <string.h>
#include <stdio.h>
#include "edhoc-config.h"
#include "edhoc-log.h"

#include "dev/ecc-algorithm.h"
#include "dev/ecc-curve.h"
#include "lib/random.h"
#include "sys/rtimer.h"
#include "sys/pt.h"

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
typedef struct  {
  /* Containers for the State */
  struct pt pt;
  struct process *process;

  /* Input Variables */
  ecc_curve_info_t *curve_info; /**< Curve defining the CyclicGroup */

  uint32_t rv;                  /**< Address of Next Result in PKA SRAM */
  uint32_t len;
  /* Output Variables */
  uint8_t result;            /**< Result Code */
  uint8_t public[64];
  uint8_t compressed[33];
} ecc_key_uncompress_t;

PT_THREAD(ecc_decompress_key(ecc_key_uncompress_t * state));

typedef struct  {
  /* Containers for the State */
  struct pt pt;
  struct process *process;

  ecc_curve_info_t *curve_info; /**< Curve defining the CyclicGroup */
  /* Output Variables */
  uint8_t x[32];           /**< Result Code */
  uint8_t y[32];
  uint8_t private[32];
} key_gen_t;

PT_THREAD(generate_key_hw(key_gen_t * key));

typedef struct ecc_curve_t {
  ecc_curve_info_t *curve;
}ecc_curve_t;

uint8_t cc2538_generate_IKM(uint8_t *gx, uint8_t *gy, uint8_t *private_key, uint8_t *ikm, ecc_curve_t curve);
void compress_key_hw(uint8_t *compressed, uint8_t *public, ecc_curve_info_t *curve);

#endif
#endif /* _ECDH_H_ */
