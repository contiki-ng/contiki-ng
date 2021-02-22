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
 *         ecc-cc2538 driver extension of ecc-algorithm. Interface the necessary functionalities
 *         of cc2538 HW module
 *
 * \author
 *         Lidia Pocero <pocero@isi.gr>
 */

#include "ecc-cc2538.h"
#if uECC
#include "contiki-lib.h"
#include <dev/watchdog.h>
#include "sys/rtimer.h"
#include "sys/process.h"

#include "dev/pka.h"
#define CHECK_RESULT(...) \
  state->result = __VA_ARGS__; \
  if(state->result) { \
    printf("Line: %u Error: %u\n", __LINE__, (unsigned int)state->result); \
    PT_EXIT(&state->pt); \
  }

static uint32_t _3[1] = { 3 };
uint32_t exp[8];
static uint32_t d4[8] = { 0x40000000, 0x00000000, 0x00000000, 0x00000000,
                          0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static uint32_t p1[8] = { 0x00000001, 0x00000000, 0x00000000, 0x00000000,
                          0x00000000, 0x00000000, 0x00000000, 0x00000000 };
static uint32_t p10[8] = { 0x10000000, 0x00000000, 0x00000000, 0x00000000,
                           0x00000000, 0x00000000, 0x00000000, 0x00000000 };

void
eccnativeToBytes(uint8_t *bytes,
                 int num_bytes,
                 const uint32_t *native)
{
  int8_t i;
  for(i = 0; i < num_bytes; ++i) {
    unsigned b = num_bytes - 1 - i;
    bytes[i] = native[b / 4] >> (8 * (b % 4));
  }
}
void
eccbytesToNative(uint32_t *native,
                 const uint8_t *bytes,
                 int num_bytes)
{
  int8_t i;
  memset(native, 0, sizeof(uint32_t) * 8);
  for(i = 0; i < num_bytes; ++i) {
    unsigned b = num_bytes - 1 - i;
    native[b / 4] |=
      (uint32_t)bytes[i] << (8 * (b % 4));
  }
}
static void
ecc_set_random_key(uint32_t *secret)
{
  int i;
  for(i = 0; i < 8; ++i) {
    secret[i] = (uint32_t)random_rand() | (uint32_t)random_rand() << 16;
  }
}
void
compress_key_hw(uint8_t *compressed, uint8_t *public, ecc_curve_info_t *curve)
{
  int8_t i;
  for(i = 0; i < curve->size * 4; ++i) {
    compressed[i + 1] = public[i];
  }
#if uECC_VLI_NATIVE_LITTLE_ENDIAN
  compressed[0] = 2 + (curve_info->size * 4] & 0x01);
#else
  compressed[0] = 2 + (public[curve->size * 4 * 2 - 1] & 0x01);
#endif
}
PT_THREAD(generate_key_hw(key_gen_t * key)) {
  static ecc_compare_state_t state = {
    .size = 8,
  };
  state.process = key->process;
  pka_init();
  PT_BEGIN(&key->pt);
  memcpy(state.b, key->curve_info->n, sizeof(uint32_t) * 8);
  static uint32_t secret_a[8];
  do {
    ecc_set_random_key(secret_a);
    memcpy(state.a, secret_a, sizeof(uint32_t) * 8);
    PT_SPAWN(&key->pt, &(state.pt), ecc_compare(&state));
    LOG_DBG("finished compare\n");
  } while(state.result != PKA_STATUS_A_LT_B);

  static ecc_multiply_state_t side_a;
  side_a.process = key->process;
  side_a.curve_info = key->curve_info;

  memcpy(side_a.point_in.x, side_a.curve_info->x, sizeof(uint32_t) * 8);
  memcpy(side_a.point_in.y, side_a.curve_info->y, sizeof(uint32_t) * 8);
  memcpy(side_a.secret, secret_a, sizeof(secret_a));

  ecc_mul_start(side_a.secret, &side_a.point_in, side_a.curve_info, &side_a.rv, side_a.process);
  while(!pka_check_status()) {
  }
  ecc_mul_get_result(&side_a.point_out, side_a.rv);

  uint8_t public[ECC_KEY_BYTE_LENGHT * 2];
  eccnativeToBytes(public, ECC_KEY_BYTE_LENGHT, side_a.point_out.x);
  eccnativeToBytes(public + ECC_KEY_BYTE_LENGHT, ECC_KEY_BYTE_LENGHT, side_a.point_out.y);
  eccnativeToBytes(key->y, ECC_KEY_BYTE_LENGHT, side_a.point_out.y);
  eccnativeToBytes(key->private, ECC_KEY_BYTE_LENGHT, secret_a);
  eccnativeToBytes(key->x, ECC_KEY_BYTE_LENGHT, side_a.point_out.x);

  LOG_DBG("Public:");
  print_buff_8_dbg(public, 64);

  LOG_DBG("x:");
  print_buff_8_dbg(key->x, ECC_KEY_BYTE_LENGHT);
  LOG_DBG("y:");
  print_buff_8_dbg(key->y, ECC_KEY_BYTE_LENGHT);
  LOG_DBG("secret:");
  print_buff_8_dbg(key->private, ECC_KEY_BYTE_LENGHT);
  pka_disable();
  PT_END(&key->pt);
}

PT_THREAD(ecc_decompress_key(ecc_key_uncompress_t * state)){

  uint32_t point[state->curve_info->size * 2];

  uint32_t *y = point + state->curve_info->size;

  memset(point, 0, sizeof(uint32_t) * 16);
  eccbytesToNative(point, state->compressed + 1, 32);

  int8_t num_words = state->curve_info->size;

  uint32_t l_result[16];
  uint8_t result[64];
  PT_BEGIN(&state->pt);
  memset(l_result, 1, sizeof(uint32_t) * 16);
  watchdog_periodic();
  CHECK_RESULT(bignum_mul_start(point, state->curve_info->size, point, state->curve_info->size, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  state->len = state->curve_info->size * 2;
  memset(l_result, 0, sizeof(uint32_t) * 16);
  CHECK_RESULT(bignum_mul_get_result(l_result, &state->len, state->rv));
  CHECK_RESULT(bignum_mod_start(l_result, num_words * 2, state->curve_info->prime, num_words, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  memset(l_result, 0, sizeof(uint32_t) * 16);
  CHECK_RESULT(bignum_mod_get_result(l_result, num_words, state->rv));
  /*printf("x2 mod p:"); */
  /*print_buff(l_result,16); */
  /*eccnativeToBytes(result,32,l_result); */
  /*print_buff_8(result,32); */
  watchdog_periodic();
  CHECK_RESULT(bignum_subtract_start(l_result, state->curve_info->size, _3, 1, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  memset(l_result, 0, sizeof(uint32_t) * 16);
  CHECK_RESULT(bignum_subtract_get_result(l_result, &state->len, state->rv));
  /*printf("(x2 -3):"); */
  /*print_buff(y,16); */
  CHECK_RESULT(bignum_mod_start(l_result, num_words, state->curve_info->prime, num_words, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  memset(l_result, 0, sizeof(uint32_t) * 16);
  CHECK_RESULT(bignum_mod_get_result(l_result, num_words, state->rv));
  /*printf("(x2 -3) mod p:"); */
  /* print_buff(l_result,16); */
  /* eccnativeToBytes(result,32,l_result); */
  /* print_buff_8(result,32); */
  watchdog_periodic();
  CHECK_RESULT(bignum_mul_start(l_result, state->curve_info->size, point, state->curve_info->size, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  state->len = state->curve_info->size * 2;
  memset(l_result, 0, sizeof(uint32_t) * 16);
  CHECK_RESULT(bignum_mul_get_result(l_result, &state->len, state->rv));
  /*printf("(x2 -3)*x:"); */
  /*print_buff(l_result,16); */
  CHECK_RESULT(bignum_mod_start(l_result, num_words * 2, state->curve_info->prime, num_words, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  memset(l_result, 0, sizeof(uint32_t) * 16);
  CHECK_RESULT(bignum_mod_get_result(l_result, num_words, state->rv));
  /*printf("(x2 -3)*x mod p:"); */
  /*print_buff(l_result,16); */
  /* eccnativeToBytes(result,32,l_result); */
  /*print_buff_8(result,32); */
  watchdog_periodic();
  CHECK_RESULT(bignum_add_start(l_result, state->curve_info->size, state->curve_info->b, state->curve_info->size, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  memset(y, 0, sizeof(uint32_t) * 16);
  CHECK_RESULT(bignum_subtract_get_result(l_result, &state->len, state->rv));
  /*printf("(x2 -3)*x + b:"); */
  /*print_buff(l_result,16); */
  CHECK_RESULT(bignum_mod_start(l_result, num_words * 2, state->curve_info->prime, num_words, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  memset(l_result, 0, sizeof(uint32_t) * 16);
  CHECK_RESULT(bignum_mod_get_result(l_result, num_words, state->rv));
  /*printf("(x2 -3)*x + b mod p:"); */
  /*print_buff(l_result,16); */
  /*eccnativeToBytes(result,32,l_result); */
  /*print_buff_8(result,32); */
  watchdog_periodic();
  uint32_t exp[8];

  memset(exp, 0, sizeof(uint32_t) * 8);

  /*/memset(prime,0,32); */
  /*eccnativeToBytes(prime,32,state->curve_info->prime); */
  /*printf("P1:"); */
  /*print_buff(p1,8); */
  /*printf("d4:"); */
  /*print_buff(d4,8); */
  /*printf("prime:"); */
  /*print_buff((uint32_t*)state->curve_info->prime,8); */
  /*print_buff_8(prime,32); */

  CHECK_RESULT(bignum_add_start(state->curve_info->prime, state->curve_info->size, p1, 1, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  state->len = state->curve_info->size + 1;
  memset(exp, 0, sizeof(uint32_t) * 8);
  CHECK_RESULT(bignum_add_get_result(exp, &state->len, state->rv));
  /*printf("prime+1:"); */
  /*print_buff(exp,state->len); */

  CHECK_RESULT(bignum_divide_start(exp, 8, d4, 8, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  state->len = state->curve_info->size;
  memset(exp, 0, sizeof(uint32_t) * 8);
  CHECK_RESULT(bignum_divide_get_result(exp, &state->len, state->rv));
  /*printf("prime+1/4:"); */
  /*print_buff(exp,state->len); */

  CHECK_RESULT(bignum_mul_start(exp, 8, p10, 8, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  state->len = state->curve_info->size;
  memset(exp, 0, sizeof(uint32_t) * 8);
  CHECK_RESULT(bignum_divide_get_result(exp, &state->len, state->rv));
  /* printf("prime+1/4:"); */
  /* print_buff(exp,state->len); */

  /*print_buff(exp,state->len); */

  /* printf("exp:"); */
  /* print_buff(exp,8); */
  /* eccnativeToBytes(result,32,l_result); */
  /* print_buff_8(result,32); */

  /* printf("prime:"); */
  /* print_buff((uint32_t*)state->curve_info->prime,num_words); */

  /* printf("lresult:"); */
/*  print_buff(l_result,16); */
/* eccnativeToBytes(result,32,l_result); */
/* print_buff_8(result,32); */
  CHECK_RESULT(bignum_exp_mod_start(exp, 8, state->curve_info->prime, num_words, l_result, num_words, &state->rv, state->process));
  PT_WAIT_UNTIL(&state->pt, pka_check_status());
  memset(y, 0, sizeof(uint32_t) * 8);
  CHECK_RESULT(bignum_exp_mod_get_result(y, num_words, state->rv));

  /*printf("y:"); */
  /*print_buff(y,8); */
  memset(result, 0, 32);
  eccnativeToBytes(result, 32, y);
  /*print_buff_8(result,32); */
  /*printf("%d\n",result[state->curve_info->size * 4 - 1]); */
  /* printf("%d\n",state->compressed[0] & 0x01); */

  if((result[state->curve_info->size * 4 - 1] & 0x01) != (state->compressed[0] & 0x01)) {
    /* printf("is not this\n"); */
    CHECK_RESULT(bignum_subtract_start(state->curve_info->prime, state->curve_info->size, y, state->curve_info->size, &state->rv, state->process));
    PT_WAIT_UNTIL(&state->pt, pka_check_status());
    memset(y, 0, sizeof(uint32_t) * 8);
    state->len = state->curve_info->size;
    CHECK_RESULT(bignum_subtract_get_result(y, &state->len, state->rv));
    /* printf("y:"); */
    /* print_buff(y,8); */
    memset(result, 0, 32);
    eccnativeToBytes(result, 32, y);
    /* print_buff_8(result,32); */
  }
  memcpy(state->public, state->compressed + 1, sizeof(uint32_t) * state->curve_info->size);
  memcpy(state->public + (state->curve_info->size * 4), result, sizeof(uint32_t) * state->curve_info->size);
  PT_END(&state->pt);
}

uint8_t
cc2538_generate_IKM(uint8_t *gx, uint8_t *gy, uint8_t *private_key, uint8_t *ikm, ecc_curve_t curve)
{
  int er = 0;
  static ecc_multiply_state_t shared;
  shared.curve_info = &nist_p_256;
  pka_init();
  /*If just one coordinate is have it we put first byte non zero */
  if(gy[0] == 0) {
    gy[0] = 0x01;
  }
  eccbytesToNative(shared.point_in.x, gx, ECC_KEY_BYTE_LENGHT);
  eccbytesToNative(shared.point_in.y, gy, ECC_KEY_BYTE_LENGHT);
  eccbytesToNative(shared.secret, private_key, ECC_KEY_BYTE_LENGHT);
  watchdog_periodic();
  ecc_mul_start(shared.secret, &shared.point_in, shared.curve_info, &shared.rv, shared.process);
  watchdog_periodic();
  while(!pka_check_status()) {
  }
  watchdog_periodic();
  ecc_mul_get_result(&shared.point_out, shared.rv);
  watchdog_periodic();

  eccnativeToBytes(ikm, ECC_KEY_BYTE_LENGHT, shared.point_out.x);
  LOG_DBG("IKM:");
  print_buff_8_dbg(ikm, ECC_KEY_BYTE_LENGHT);
  pka_disable();
  er = 1;

  return er;
}
#endif