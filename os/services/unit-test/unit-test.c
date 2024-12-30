/*
 * Copyright (c) 2010, Swedish Institute of Computer Science
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
 *	Default print function for unit tests.
 * \author
 * 	Nicolas Tsiftes <nicolas.tsiftes@ri.se>
 */

#include <inttypes.h>
#include <stdio.h>

#include "unit-test.h"

struct pt unit_test_pt;

/*---------------------------------------------------------------------------*/
/**
 * Print the results of a unit test.
 *
 * \param utp The unit test descriptor.
 */
void
unit_test_print_report(const unit_test_t *utp)
{
  printf("\nUnit test: %s\n", utp->descr);
  printf("Result: %s\n", utp->passed ? "success" : "failure");
  printf("Exit point: %s:%u\n", utp->test_file, utp->exit_line);
  printf("Assertions executed: %"PRIu32"\n", utp->assertions);
  printf("Start: %"PRIu64"\n", (uint64_t)utp->start);
  printf("End: %"PRIu64"\n", (uint64_t)utp->end);
  printf("Duration: %"PRId64" ticks\n", (int64_t)(utp->end - utp->start));
  printf("Ticks per second: %u\n", (unsigned)CLOCK_SECOND);
}
/*---------------------------------------------------------------------------*/
