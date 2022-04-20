/*
 * Copyright (c) 2009, Simon Berg
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
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
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdio.h>
#include <strformat.h>
#include <string.h>
#undef snprintf
#undef vsnprintf
/*---------------------------------------------------------------------------*/
struct fmt_buffer {
  char *pos;
  size_t left;
};
/*---------------------------------------------------------------------------*/
static strformat_result
buffer_str(void *user_data, const char *data, unsigned int len)
{
  struct fmt_buffer *buffer = (struct fmt_buffer *)user_data;
  if(len >= buffer->left) {
    len = buffer->left;
    len--;
  }

  memcpy(buffer->pos, data, len);
  buffer->pos += len;
  buffer->left -= len;
  return STRFORMAT_OK;
}
/*---------------------------------------------------------------------------*/
int
snprintf(char *str, size_t size, const char *format, ...)
{
  int res;
  va_list ap;
  va_start(ap, format);
  res = vsnprintf(str, size, format, ap);
  va_end(ap);
  return res;
}
/*---------------------------------------------------------------------------*/
int
vsnprintf(char *str, size_t size, const char *format, va_list ap)
{
  struct fmt_buffer buffer;
  strformat_context_t ctxt;
  int res;
  ctxt.write_str = buffer_str;
  ctxt.user_data = &buffer;
  buffer.pos = str;
  buffer.left = size;
  res = format_str_v(&ctxt, format, ap);
  *buffer.pos = '\0';
  return res;
}
/*---------------------------------------------------------------------------*/
