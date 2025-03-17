/*
 * Copyright (c) 2013, Hasso-Plattner-Institut.
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
 * \file
 *         Testing CCM*-MICs
 * \author
 *         Konrad Krentz <konrad.krentz@gmail.com>
 */

#include "contiki.h"
#include "net/packetbuf.h"
#include "net/netstack.h"
#include "net/mac/ccm-star-packetbuf.h"
#include "net/mac/anti-replay.h"
#include "lib/aes-128.h"
#include "lib/ccm-star.h"
#include "unit-test.h"
#include <stdio.h>
#include <string.h>

PROCESS(test_process, "test");
AUTOSTART_PROCESSES(&test_process);

/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_aes_128,
                   "Test vector C.1 from FIPS Pub 197");
UNIT_TEST(test_aes_128)
{
  const uint8_t key[16] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
  };
  uint8_t data[16] = {
    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
    0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
  };
  const uint8_t oracle[16] = {
    0x69, 0xC4, 0xE0, 0xD8, 0x6A, 0x7B, 0x04, 0x30,
    0xD8, 0xCD, 0xB7, 0x80, 0x70, 0xB4, 0xC5, 0x5A
  };

  UNIT_TEST_BEGIN();

  printf("Testing AES-128 ... ");

  AES_128.set_key(key);
  AES_128.encrypt(data);

  UNIT_TEST_ASSERT(!memcmp(data, oracle, 16));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_sec_lvl_2,
                   "Test vector C.2.2.2.1 from IEEE 802.15.4-2020");
UNIT_TEST(test_sec_lvl_2)
{
  const uint8_t key[16] = {
    0xC0, 0xC1, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xCA, 0xCB, 0xCC, 0xCD, 0xCE, 0xCF
  };
  const linkaddr_t source_address = {
    { 0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };
  const uint8_t data[26] = {
    0x08, 0xD0, 0x84, 0x21, 0x43,
    /* Source Address */
    0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC,
    /* Security Level*/
    0x02,
    /* Frame Counter */
    0x05, 0x00, 0x00, 0x00,
    /* Payload */
    0x55, 0xCF, 0x00, 0x00, 0x51, 0x52, 0x53, 0x54
  };
  const uint8_t oracle[] = {
    0x22, 0x3B, 0xC1, 0xEC,
    0x84, 0x1A, 0xB5, 0x53
  };
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  const uint8_t sec_lvl = 2;

  UNIT_TEST_BEGIN();

  printf("Testing verification ... ");

  linkaddr_copy(&linkaddr_node_addr, &source_address);
  packetbuf_clear();
  packetbuf_set_datalen(26);
  memcpy(packetbuf_hdrptr(), data, 26);
  anti_replay_parse_counter(data + 14);
  packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL, sec_lvl);
  packetbuf_hdrreduce(18);

  CCM_STAR.set_key(key);
  ccm_star_packetbuf_set_nonce(nonce, 1);
  CCM_STAR.aead(nonce,
                NULL,
                0,
                packetbuf_hdrptr(),
                packetbuf_totlen(),
                ((uint8_t *)packetbuf_dataptr()) + packetbuf_datalen(),
                LLSEC802154_MIC_LEN(sec_lvl),
                true);

  UNIT_TEST_ASSERT(!memcmp(
                       ((uint8_t *)packetbuf_dataptr()) + packetbuf_datalen(),
                       oracle,
                       LLSEC802154_MIC_LEN(sec_lvl)));

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(test_sec_lvl_6,
                   "Test vector C.2.3.2.1 from IEEE 802.15.4-2020");
UNIT_TEST(test_sec_lvl_6)
{
  const uint8_t key[16] = {
    0xC0, 0xC1, 0xC2, 0xC3,
    0xC4, 0xC5, 0xC6, 0xC7,
    0xC8, 0xC9, 0xCA, 0xCB,
    0xCC, 0xCD, 0xCE, 0xCF
  };
  const linkaddr_t source_address = {
    { 0xAC, 0xDE, 0x48, 0x00, 0x00, 0x00, 0x00, 0x01 }
  };
  const uint8_t data[30] = {
    0x2B, 0xDC, 0x84, 0x21, 0x43,
    /* Destination Address */
    0x02, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC,
    /* PAN-ID */
    0xFF, 0xFF,
    /* Source Address */
    0x01, 0x00, 0x00, 0x00, 0x00, 0x48, 0xDE, 0xAC,
    /* Security Level */
    0x06,
    /* Frame Counter */
    0x05, 0x00, 0x00, 0x00,
    0x01, 0xCE
  };
  const uint8_t oracle[] = {
    0x4F, 0xDE, 0x52, 0x90,
    0x61, 0xF9, 0xC6, 0xF1
  };
  uint8_t nonce[CCM_STAR_NONCE_LENGTH];
  const uint8_t sec_lvl = 6;

  UNIT_TEST_BEGIN();

  printf("Testing verification ... ");

  linkaddr_copy(&linkaddr_node_addr, &source_address);
  packetbuf_clear();
  packetbuf_set_datalen(30);
  memcpy(packetbuf_hdrptr(), data, 30);
  anti_replay_parse_counter(data + 24);
  packetbuf_set_attr(PACKETBUF_ATTR_SECURITY_LEVEL, sec_lvl);
  packetbuf_hdrreduce(29);

  CCM_STAR.set_key(key);
  ccm_star_packetbuf_set_nonce(nonce, 1);
  CCM_STAR.aead(nonce,
                packetbuf_dataptr(),
                packetbuf_datalen(),
                packetbuf_hdrptr(),
                packetbuf_hdrlen(),
                ((uint8_t *)packetbuf_hdrptr()) + 30,
                LLSEC802154_MIC_LEN(sec_lvl),
                true);

  UNIT_TEST_ASSERT(!memcmp(
                       ((uint8_t *)packetbuf_hdrptr()) + 30,
                       oracle,
                       LLSEC802154_MIC_LEN(sec_lvl)));

  printf("Testing encryption ... ");

  UNIT_TEST_ASSERT(((uint8_t *)packetbuf_hdrptr())[29] == 0xD8);

  printf("Testing decryption ... ");
  packetbuf_set_addr(PACKETBUF_ADDR_SENDER, &source_address);
  ccm_star_packetbuf_set_nonce(nonce, 0);
  CCM_STAR.aead(nonce,
                packetbuf_dataptr(),
                packetbuf_datalen(),
                packetbuf_hdrptr(),
                packetbuf_hdrlen(),
                ((uint8_t *)packetbuf_hdrptr()) + 30,
                LLSEC802154_MIC_LEN(sec_lvl),
                false);
  UNIT_TEST_ASSERT(((uint8_t *)packetbuf_hdrptr())[29] == 0xCE);

  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();

  printf("Run unit-test\n");
  printf("---\n");

  UNIT_TEST_RUN(test_aes_128);
  UNIT_TEST_RUN(test_sec_lvl_2);
  UNIT_TEST_RUN(test_sec_lvl_6);

  if(!UNIT_TEST_PASSED(test_aes_128)
     || !UNIT_TEST_PASSED(test_sec_lvl_2)
     || !UNIT_TEST_PASSED(test_sec_lvl_6)) {
    printf("=check-me= FAILED\n");
    printf("---\n");
  }

  printf("=check-me= DONE\n");
  printf("---\n");

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
