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
 *   Capture driver for the ip64 test: stands in for an Ethernet chip by
 *   recording the frames that ip64 transmits, so the test can inspect them.
 */

#ifndef IP64_TEST_DRIVER_H
#define IP64_TEST_DRIVER_H

#include <stdint.h>

/*
 * Note: ip64-driver.h is not self-contained; it uses uint8_t and uint16_t
 * without including <stdint.h>, so that header must come first.
 */
#include "ip64/ip64-driver.h"

#define IP64_TEST_DRIVER_BUFSIZE 1600

/* The most recently transmitted frame, and how many have been sent. */
extern uint8_t ip64_test_driver_buf[IP64_TEST_DRIVER_BUFSIZE];
extern uint16_t ip64_test_driver_len;
extern unsigned ip64_test_driver_count;

extern const struct ip64_driver ip64_test_driver;

void ip64_test_driver_reset(void);

#endif /* IP64_TEST_DRIVER_H */
