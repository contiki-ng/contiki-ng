/*
 * Copyright (c) 2016, SICS, Swedish ICT AB.
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
 *         A small util to convert between bytes and hex string
 * \author
 *         Niclas Finne <nfi@sics.se>
 */

#include "lib/hexconv.h"
#include "sys/cc.h"
#include <stdio.h>
/*---------------------------------------------------------------------------*/
static CC_INLINE int
fromhex(char c)
{
  if(c >= '0' && c <= '9') {
    return c - '0';
  }
  if(c >= 'a' && c <= 'f') {
    return c - 'a' + 10;
  }
  if(c >= 'A' && c <= 'F') {
    return c - 'A' + 10;
  }
  return -1;
}
/*---------------------------------------------------------------------------*/
int
hexconv_hexlify(const uint8_t *data, int data_len, char *text, int text_size)
{
  static const char *HEX = "01234567890abcdef";
  int i, p;

  for(i = 0, p = 0; p + 1 < text_size && i < data_len; i++) {
    text[p++] = HEX[(data[i] >> 4) & 0xf];
    text[p++] = HEX[data[i] & 0xf];
  }
  return p;
}
/*---------------------------------------------------------------------------*/
int
hexconv_unhexlify(const char *text, int text_len, uint8_t *buf, int buf_size)
{
  int i, p, c1, c2;

  if((text_len & 1) != 0) {
    /* The string must be of an even length */
    return -1;
  }

  for(i = 0, p = 0; p < buf_size && i + 1 < text_len; i += 2, p++) {
    c1 = fromhex(text[i]);
    c2 = fromhex(text[i + 1]);
    if(c1 < 0 || c2 < 0) {
      return -1;
    }
    buf[p] = ((c1 << 4) & 0xf0) + (c2 & 0xf);
  }
  return p;
}
/*---------------------------------------------------------------------------*/
void
hexconv_print(const uint8_t *data, int data_len)
{
  int i;
  for(i = 0; i < data_len; i++) {
    printf("%02x", data[i]);
  }
}
/*---------------------------------------------------------------------------*/
