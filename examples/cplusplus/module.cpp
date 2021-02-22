/*
 * Copyright (c) 2020, Institute of Electronics and Computer Science.
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
 *         A very simple C++ file to demonstrate how to include C++ code in Contiki-NG projects.
 * \author
 *         Atis Elsts <atis.elsts@edi.lv>
 */

/* prefix all C header files with `extern "C"` */
extern "C" {
#include "contiki.h"
}

/*---------------------------------------------------------------------------*/
class cplusplus_class {
public:
  uint8_t i;

  cplusplus_class() : i(13) {}
};
/*---------------------------------------------------------------------------*/
uint8_t
cplusplus_function(void)
{
  cplusplus_class object;
  return object.i;
}
/*---------------------------------------------------------------------------*/
/* Define the function as `extern "C"` since its called from C code */
extern "C" uint8_t
wrapper_function(void)
{
  return cplusplus_function();
}
/*---------------------------------------------------------------------------*/
