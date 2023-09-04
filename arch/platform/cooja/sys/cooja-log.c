/*
 * Copyright (c) 2006, Swedish Institute of Computer Science.
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

#define _GNU_SOURCE /* For vasprintf. */
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_LOG_LENGTH
#define MAX_LOG_LENGTH 8192
#endif /* MAX_LOG_LENGTH */

#ifndef COOJA_LOG_WITH_SLIP
#define COOJA_LOG_WITH_SLIP 0
#endif /* COOJA_LOG_WITH_SLIP */

/* Variables shared between COOJA and Contiki */
char simLoggedData[MAX_LOG_LENGTH];
int simLoggedLength;
char simLoggedFlag;

/*-----------------------------------------------------------------------------------*/
int
simlog_char(char c)
{
  if (simLoggedLength + 1 > MAX_LOG_LENGTH) {
    /* Dropping message due to buffer overflow */
    return EOF;
  }

  simLoggedData[simLoggedLength] = c;
  simLoggedLength += 1;
  simLoggedFlag = 1;
  return c;
}
/*-----------------------------------------------------------------------------------*/
void
simlog(const char *message)
{
  int message_len = strlen(message);
  if(simLoggedLength + message_len > MAX_LOG_LENGTH) {
    /* Dropping message due to buffer overflow */
    return;
  }

  memcpy(simLoggedData + simLoggedLength, message, message_len);
  simLoggedLength += message_len;
  simLoggedFlag = 1;
}
/*-----------------------------------------------------------------------------------*/
void
log_message(const char *part1, const char *part2)
{
  simlog(part1);
  simlog(part2);
}
/*-----------------------------------------------------------------------------------*/
static int log_putchar_with_slip = COOJA_LOG_WITH_SLIP != 0;
void
log_set_putchar_with_slip(int with)
{
  log_putchar_with_slip = with;
}
/*-----------------------------------------------------------------------------------*/
int
dbg_putchar(int c)
{
#define SLIP_END 0300
  static char debug_frame = 0;

  if(log_putchar_with_slip) {
    if(!debug_frame) {		/* Start of debug output */
      simlog_char(SLIP_END);
      simlog_char('\r');	/* Type debug line == '\r' */
      debug_frame = 1;
    }

    simlog_char((char)c);

    /*
     * Line buffered output, a newline marks the end of debug output and
     * implicitly flushes debug output.
     */
    if(c == '\n') {
      simlog_char(SLIP_END);
      debug_frame = 0;
    }
  } else {
    simlog_char(c);
  }
  return c;
}
/*-----------------------------------------------------------------------------------*/
#ifndef __APPLE__
extern int __wrap_putchar(int c) __attribute__((alias("putchar")));
extern int __wrap_puts(const char *str) __attribute__((nonnull, alias("puts")));
extern int __wrap_printf(const char *fmt, ...) __attribute__((nonnull, alias("printf")));
#endif
/*---------------------------------------------------------------------------*/
int
putchar(int c)
{
  return simlog_char(c);
}
/*---------------------------------------------------------------------------*/
int
puts(const char* s)
{
  simlog(s);
  return simlog_char('\n');
}
/*---------------------------------------------------------------------------*/
int
printf(const char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  char *buf;
  int res = vasprintf(&buf, fmt, ap);
  va_end(ap);
  if(res > 0) {
    simlog(buf);
    free(buf);
  }
  return res;
}
