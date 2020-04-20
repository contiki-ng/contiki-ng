/*
 * Copyright (c) 2019, Yanzi Networks AB.
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
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "contiki.h"
#include "lib/random.h"
#include "unit-test.h"
#include "lib/ccm-star.h"
#include "lib/hexconv.h"
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

#define MICLEN 8

PROCESS(test_process, "test");
AUTOSTART_PROCESSES(&test_process);

static const char *key = "4044e65bf1ba4f8c4db767fa6c63e327";
static const char *nonce = "cb72d71df3c953aaec3ca4d75b";
/* A list of hdr, msg => enc. If first two are NULL, indicates wrong AUTH */
static const char *testcases[][3] = {
#include "test-vectors.c"
};

#define NUM_TESTSCASES (sizeof(testcases)/sizeof(testcases[0]))
#define MAXLEN 65536

/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(aesccm_encrypt, "AES-CCM encryption");
UNIT_TEST(aesccm_encrypt)
{
  int i;
  UNIT_TEST_BEGIN();

  printf("TEST: *** encryption\n");

  static uint8_t key_bytes[16];
  static uint8_t nonce_bytes[13];
  hexconv_unhexlify(key, strlen(key), key_bytes, sizeof(key_bytes));
  hexconv_unhexlify(nonce, strlen(nonce), nonce_bytes, sizeof(nonce_bytes));

  for(i = 0; i < NUM_TESTSCASES; i++) {
    bool success;
    const char *hdr_string = testcases[i][0];
    const char *cleartext_string = testcases[i][1];
    const char *ciphertext_string = testcases[i][2];

    if(hdr_string != NULL && cleartext_string != NULL) {
      uint8_t buffer[MAXLEN * 2 + MICLEN];
      uint8_t ciphertext_bytes[MAXLEN * 2 + MICLEN];
      size_t a_len = strlen(hdr_string) / 2;
      size_t m_len = strlen(cleartext_string) / 2;
      hexconv_unhexlify(hdr_string, strlen(hdr_string), buffer, sizeof(buffer));
      hexconv_unhexlify(cleartext_string, strlen(cleartext_string), buffer + a_len, sizeof(buffer) - a_len);
      hexconv_unhexlify(ciphertext_string, strlen(ciphertext_string), ciphertext_bytes, sizeof(ciphertext_bytes));

      printf("TEST: encrypt in: %u + %u bytes\n",
        (unsigned)a_len, (unsigned)m_len);

      CCM_STAR.set_key(key_bytes);
      CCM_STAR.aead(
          nonce_bytes,
          buffer + a_len,
          m_len,
          buffer,
          a_len,
          buffer + a_len + m_len,
          MICLEN,
          1
      );

      success = !memcmp(buffer, ciphertext_bytes, a_len + m_len + MICLEN);
      printf("TEST: encrypt out: %u bytes --- %s\n", (unsigned)(a_len + m_len + MICLEN), success ? "OK" : "FAIL");
      UNIT_TEST_ASSERT(success);
    }
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(aesccm_decrypt, "AES-CCM decryption");
UNIT_TEST(aesccm_decrypt)
{
  int i;
  UNIT_TEST_BEGIN();

  printf("TEST: *** decryption\n");

  static uint8_t key_bytes[16];
  static uint8_t nonce_bytes[13];
  hexconv_unhexlify(key, strlen(key), key_bytes, sizeof(key_bytes));
  hexconv_unhexlify(nonce, strlen(nonce), nonce_bytes, sizeof(nonce_bytes));

  for(i = 0; i < NUM_TESTSCASES; i++) {
    bool success;
    bool auth_check;
    const char *hdr_string = testcases[i][0];
    const char *cleartext_string = testcases[i][1];
    const char *ciphertext_string = testcases[i][2];
    uint8_t generated_mic[MICLEN];
    uint8_t buffer[MAXLEN * 2 + MICLEN];
    uint8_t hdr_bytes[MAXLEN];
    uint8_t cleartext_bytes[MAXLEN];

    size_t a_len = hdr_string ? strlen(hdr_string) / 2 : 0;
    size_t m_len = cleartext_string ? strlen(cleartext_string) / 2 : 0;
    hexconv_unhexlify(ciphertext_string, strlen(ciphertext_string), buffer, sizeof(buffer));
    hexconv_unhexlify(hdr_string ? hdr_string : "", hdr_string ? strlen(hdr_string) : 0, hdr_bytes, sizeof(hdr_bytes));
    hexconv_unhexlify(cleartext_string ? cleartext_string : "", cleartext_string ? strlen(cleartext_string) : 0, cleartext_bytes, sizeof(cleartext_bytes));

    printf("TEST: decrypt in: %u bytes\n", (unsigned)strlen(ciphertext_string));

    CCM_STAR.set_key(key_bytes);
    CCM_STAR.aead(
        nonce_bytes,
        buffer + a_len,
        m_len,
        buffer,
        a_len,
        generated_mic,
        MICLEN,
        0
    );

    auth_check = !memcmp(generated_mic, buffer + a_len + m_len, MICLEN);
    if(hdr_string != NULL && cleartext_string != NULL) {
      success = !memcmp(buffer, hdr_bytes, a_len)
                && !memcmp(buffer + a_len, cleartext_bytes, m_len)
                && auth_check;
    } else {
      /* No input means we expact a failed auth */
      success = !auth_check;
    }
    printf("TEST: encrypt out: %u + %u bytes --- %s\n", (unsigned)a_len, (unsigned)m_len, success ? "OK" : "FAIL");

    UNIT_TEST_ASSERT(success);
  }

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(aesccm_encrypt);
  UNIT_TEST_RUN(aesccm_decrypt);

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
