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
#ifndef STRFORMAT_H_
#define STRFORMAT_H_
/*---------------------------------------------------------------------------*/
#include "contiki.h"

#include <stdarg.h>
/*---------------------------------------------------------------------------*/
#define STRFORMAT_OK 0
#define STRFORMAT_FAILED 1
/*---------------------------------------------------------------------------*/
typedef unsigned int strformat_result;
/*---------------------------------------------------------------------------*/
/* The data argument may only be considered valid during the function call */
typedef strformat_result (*strformat_write)(void *user_data,
                                            const char *data,
                                            unsigned int len);

typedef struct strformat_context_s {
  strformat_write write_str;
  void *user_data;
} strformat_context_t;
/*---------------------------------------------------------------------------*/
int format_str(const strformat_context_t *ctxt, const char *format, ...)
     __attribute__ ((__format__ (__printf__, 2,3)));

int
format_str_v(const strformat_context_t *ctxt, const char *format, va_list ap)
     __attribute__ ((__format__ (__printf__, 2, 0)));
/*---------------------------------------------------------------------------*/
#endif /* STRFORMAT_H_ */
/*---------------------------------------------------------------------------*/
