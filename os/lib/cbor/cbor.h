/*
 * Copyright (C) 2020 Yago Fontoura do Rosario <yago.rosario@hotmail.com.br>
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
/*---------------------------------------------------------------------------*/

/**
 * \file
 *      An implementation of the Concise Binary Object Representation (CBOR) RFC 7049
 * \author
 *      Yago Fontoura do Rosario <yago.rosario@hotmail.com.br
 */

#ifndef CBOR_H
#define CBOR_H

#include <stdint.h>

#define DEBUG 1

#if DEBUG
#include <stdio.h>
#define PRINTF(...) printf(__VA_ARGS__)
#else
#define PRINTF(...)
#endif

struct cbor_object {
  uint32_t position;
  uint32_t length;
  uint8_t *buffer;
};

#define CBOR_TOKEN_TYPE_UNSIGNED  0
#define CBOR_TOKEN_TYPE_NEGATIVE  1
#define CBOR_TOKEN_TYPE_BYTES     2
#define CBOR_TOKEN_TYPE_TEXT      3
#define CBOR_TOKEN_TYPE_ARRAY     4
#define CBOR_TOKEN_TYPE_MAP       5
#define CBOR_TOKEN_TYPE_TAG       6
#define CBOR_TOKEN_TYPE_PRIMITIVE 7

#define CBOR_TOKEN_MAJOR_TYPE_MASK  0x5
#define CBOR_TOKEN_STRUCTURE_MASK   0x1F

#define CBOR_TOKEN_PRIMITIVE_FALSE                      20
#define CBOR_TOKEN_PRIMITIVE_TRUE                       21
#define CBOR_TOKEN_PRIMITIVE_NULL                       22
#define CBOR_TOKEN_PRIMITIVE_UNDEFINED                  23
#define CBOR_TOKEN_PRIMITIVE_SIMPLE_VALUE               24
#define CBOR_TOKEN_PRIMITIVE_HALF_PRECISION_FLOAT       25
#define CBOR_TOKEN_PRIMITIVE_SINGLE_PRECISION_FLOAT     26
#define CBOR_TOKEN_PRIMITIVE_DOUBLE_PRECISION_FLOAT     27
#define CBOR_TOKEN_PRIMITIVE_BREAK                      31

struct cbor_token {
  union {
    struct {
      uint64_t value;
    } integer;
    struct {
      uint64_t length;
      uint8_t *buffer;
    } string;
    struct {
      uint64_t length;
    } collection;
    struct {
      uint64_t value;
    } tag;
    struct {
      uint8_t type;
      uint64_t value;
    } primitive;
  };
  uint8_t type;
  uint8_t additional_information;
};

void cbor_object_init(struct cbor_object *object, uint8_t *buffer, uint32_t length);
int cbor_object_insert(struct cbor_object *object, uint8_t value);
void cbor_object_dump_hex(struct cbor_object *object);
void cbor_token_init(struct cbor_token *token);

#endif /* CBOR_H */
/* EOF */
