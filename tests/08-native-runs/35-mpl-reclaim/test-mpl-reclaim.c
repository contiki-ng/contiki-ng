/*
 * Copyright (c) 2026, zeetee1235.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/**
 * \file
 *         Test MPL buffer reclamation through the multicast driver.
 */
#include "contiki.h"
#include "unit-test.h"
#include "net/ipv6/uip.h"
#include "net/ipv6/multicast/uip-mcast6.h"
#include "net/ipv6/multicast/mpl.h"

#include <stdio.h>
#include <string.h>

PROCESS(test_process, "MPL buffer reclamation tests");
AUTOSTART_PROCESSES(&test_process);

/* IPv6, an MPL Hop-by-Hop option, and a UDP datagram. */
static const uint8_t packet[] = {
  0x60, 0, 0, 0, 0, 20, 0, 64,
  0xfe, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  0xff, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0xfc,
  17, 0, HBHO_OPT_TYPE_MPL, MPL_OPT_LEN_S0, 0, 0, 0, 0,
  0x0b, 0xb9, 0x0b, 0xb9, 0, 12, 0x50, 0x5b, 'M', 'P', 'L', '7'
};
/*---------------------------------------------------------------------------*/
static uint8_t
receive(uint8_t seed, uint8_t seq)
{
  memcpy(uip_buf, packet, sizeof(packet));
  /* S=0 uses the IPv6 source address as the seed identifier. */
  uip_buf[23] = seed;
  uip_buf[45] = seq;
  uip_len = sizeof(packet);
  uip_ext_len = 8;
  /* MPL input bypasses UDP checksum validation. */
  return UIP_MCAST6.in();
}
/*---------------------------------------------------------------------------*/
UNIT_TEST_REGISTER(reclaim_full_buffer,
                   "a full message buffer reclaims from a valid seed");
UNIT_TEST(reclaim_full_buffer)
{
  uint8_t seq;

  UNIT_TEST_BEGIN();
  for(seq = 1; seq <= MPL_BUFFERED_MESSAGE_SET_SIZE; seq++) {
    UNIT_TEST_ASSERT(receive(1, seq) == UIP_MCAST6_ACCEPT);
  }
  UNIT_TEST_ASSERT(receive(1, MPL_BUFFERED_MESSAGE_SET_SIZE + 1)
                   == UIP_MCAST6_ACCEPT);
  UNIT_TEST_ASSERT(receive(1, MPL_BUFFERED_MESSAGE_SET_SIZE + 1)
                   == UIP_MCAST6_DROP);
  UNIT_TEST_ASSERT(receive(2, 1) == UIP_MCAST6_ACCEPT);
  UNIT_TEST_ASSERT(receive(2, 2) == UIP_MCAST6_ACCEPT);
  UNIT_TEST_ASSERT(receive(1, MPL_BUFFERED_MESSAGE_SET_SIZE + 2)
                   == UIP_MCAST6_ACCEPT);
  UNIT_TEST_END();
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(test_process, ev, data)
{
  PROCESS_BEGIN();
  printf("Run unit-test\n");
  UNIT_TEST_RUN(reclaim_full_buffer);
  if(!UNIT_TEST_PASSED(reclaim_full_buffer)) {
    printf("=check-me= FAILED\n");
  }
  printf("=check-me= DONE\n");
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
