/*
 * Copyright (c) 2017, Yasuyuki Tanaka
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

#include <stdio.h>

#include "net/mac/tsch/sixtop/sixtop.h"

#include "unit-test/unit-test.h"
#include "common.h"

#include "lib/simEnvChange.h"
#include "sys/cooja_mt.h"

#define TEST_MAC_MAX_PAYLOAD_LEN 100

static uint8_t mac_send_is_called;
static mac_callback_t mac_sent_callback;
static void *mac_sent_callback_arg;

void
test_print_report(const unit_test_t *utp)
{
  printf("=check-me= ");
  if(utp->result == unit_test_failure) {
    printf("FAILED   - %s: exit at L%u\n", utp->descr, utp->exit_line);
  } else {
    printf("SUCCEEDED - %s\n", utp->descr);
  }

  /* give up the CPU so that the mote can output messages in the serial buffer */
  simProcessRunValue = 1;
  cooja_mt_yield();
}

uint8_t
test_mac_send_function_is_called(void)
{
  return mac_send_is_called;
}

void
test_mac_invoke_sent_callback(int status, int num_tx)
{
  if(mac_sent_callback != NULL) {
    mac_sent_callback(mac_sent_callback_arg, status, num_tx);
  }
}

static void
init(void)
{
  mac_send_is_called = 0;
  mac_sent_callback = NULL;
  mac_sent_callback_arg = NULL;
}

static void
send(mac_callback_t sent_callback, void *ptr)
{
  mac_send_is_called = 1;
  mac_sent_callback = sent_callback;
  mac_sent_callback_arg = ptr;
}

static void
input(void)
{
  /* do nothing */
}

static int
on(void)
{
  return 1; /* always on */
}

static int
off(void)
{
  return 0; /* never be turned off */
}

static int
max_payload(void)
{
  return TEST_MAC_MAX_PAYLOAD_LEN;
}

const struct mac_driver test_mac_driver = {
  "Test MAC",
  init,
  send,
  input,
  on,
  off,
  max_payload,
};
