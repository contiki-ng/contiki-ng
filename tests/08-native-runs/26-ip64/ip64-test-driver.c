/*
 * Copyright (c) 2026, RISE Research Institutes of Sweden AB
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
 */

/**
 * \file
 *   Capture driver for the ip64 test.
 */

#include "ip64-test-driver.h"

#include <string.h>

uint8_t ip64_test_driver_buf[IP64_TEST_DRIVER_BUFSIZE];
uint16_t ip64_test_driver_len;
unsigned ip64_test_driver_count;

/*---------------------------------------------------------------------------*/
void
ip64_test_driver_reset(void)
{
  ip64_test_driver_len = 0;
  ip64_test_driver_count = 0;
}
/*---------------------------------------------------------------------------*/
static void
init(void)
{
  ip64_test_driver_reset();
}
/*---------------------------------------------------------------------------*/
static int
output(uint8_t *packet, uint16_t packet_len)
{
  ip64_test_driver_count++;
  ip64_test_driver_len = packet_len > IP64_TEST_DRIVER_BUFSIZE ?
    IP64_TEST_DRIVER_BUFSIZE : packet_len;
  memcpy(ip64_test_driver_buf, packet, ip64_test_driver_len);
  return packet_len;
}
/*---------------------------------------------------------------------------*/
const struct ip64_driver ip64_test_driver = { init, output };
/*---------------------------------------------------------------------------*/
